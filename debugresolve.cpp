/*********************************************************************
  Purpose:  Resolving symbols and calltrace
  Author: Taranenko Sergey (tsvstar@gmail.com)
  Date: 02-Aug-2017
  License: BSD. See License.txt
**********************************************************************/

// @tsv TEMPORARY
//#undef __GNUG__

#define BACKTRACE_AVAILABLE 0
#define ADDR2LINE_AVAILABLE 0

#include "debugresolve.h"
#include "debuglog.h"
#include "tostr.h"
#include <string>
#include <cstdlib>
#include <cstring>      //strlen
#include <memory>
#include <unordered_map>
#if BACKTRACE_AVAILABLE
#include <execinfo.h>
#endif

#if ADDR2LINE_AVAILABLE
#include <signal.h>     // kill()
#endif

#ifdef __GNUG__
#include <cxxabi.h>
#endif

namespace tsv {
namespace debug {

/******************** SETTINGS **************************************/
namespace settings
{
    // Backtrace specific stopwords. If function match to this word, break backtrace
    std::string backtraceStopWords[] = { "main" };

    // BackTrace enabled
    bool btEnabled = false;                 // If false, then printBackTrace() do nothing.
    bool btEnableAfterInit = true;          // Value of btEnabled which should be assigned after finishing of system initialization.
                                            // (do that manually)
    // Backtrace tunings
    static const bool btIncludeLine = true;     // if true, include to stacktrace "at file:line"
    static const bool btIncludeAddr = true;     // if true, include to stacktrace addr
    static const bool btShortList = true;       // if true, remember already printed stacktraces and just say it's number on repeat
    static const bool btShortListOnly = false;  // if true, do not print full stack - only short line
    static const int  btNumLeadFuncs = 4;       // how many lead functions include into collapsed stacktrace
}


/************************** Demangle symbol **********************************/

#ifdef __GNUG__

std::string demangle(const char* name) {

    int status = -4; // some arbitrary value to eliminate the compiler warning

    // enable c++11 by passing the flag -std=c++11 to g++
    std::unique_ptr<char, void(*)(void*)> res {
        abi::__cxa_demangle(name, NULL, NULL, &status),
        std::free
    };

    return (status==0) ? res.get() : name ;
}

#else

// does nothing if not g++
std::string demangle(const char* name) {
    return name;
}

#endif

/************************** tsv::debug::settings::isStopWord() **********************************/

namespace settings {

    // Return true if funcname match to anything in stopwordlist
    bool isStopWord( const std::string& funcname )
    {
        for ( auto& stopword : backtraceStopWords )
        {
            if ( funcname == stopword )
                return true;
        }
        return false;
    }

}

/***************** BEGIN OF local aux functions *******************************/


namespace {
namespace symbol_resolve {

/***************************************************************************
     Symbol resolving ( call external utility addr2line and parse output )
***************************************************************************/

#if ADDR2LINE_AVAILABLE

// PURPOSE: split string "s" to vector "elems" by delimiter "delim"
int ssplit( const std::string &s, char delim, std::vector<std::string> &elems )
{
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems.size();
};

// PURPOSE: squeeze ".." from path
// Auxilary function to make path shorter
std::string squeezePath( const std::string& path )
{
    std::vector<std::string> v;
    ssplit( path, '/', v );
    for ( int i = 2; i < static_cast<int>(v.size()); )
    {
        if ( i <= 1 )
          { i++; continue; }
        if ( !v[i].length() )
         {  v.erase(v.begin()+i); continue; }
        if ( v[i]!=".." )
          { i++; continue; }

        v.erase( v.begin()+i-1, v.begin()+i+1 );
        i--;
    }

    std::string result;
    for ( int i = 1; i < static_cast<int>(v.size()); i++ )
        result += std::string("/")+v[i];
    return result;
}

/***************************************************************************
        Auxilary class which actually run child "addr2line" and
            communicate with it to resolve address
***************************************************************************/
class Addr2LineResolver
{
   public:
        Addr2LineResolver()
        {
           child_pid_ = 0;
           // cleanup other session
           if ( system("killall -9 addr2line 2>/dev/null") < 0 )
                perror("Fail to killall:");
        }

        ~Addr2LineResolver()
        {
            if ( child_pid_ > 0 )
                kill( child_pid_, 9 );
            else if ( child_pid_ < 0)
                kill( -child_pid_, 9);
            if ( child_pid_ != 0 )
            {
                close( pipefd_[0] );
                close( pipefd_[1] );
            }
        }


        struct CacheEntry
        {
            std::string funcName_;
            std::string pathName_;
            std::string getFuncAndLine() { return funcName_ + pathName_; }
            std::string getSymbol( bool includeLine ) { return includeLine ? (funcName_ + pathName_) : funcName_; }
        };

        CacheEntry request( void* addr );

        static bool isStopWord( const std::string& funcname )
            { return ::tsv::debug::settings::isStopWord( funcname ); }

   private:
        std::unordered_map< void*, CacheEntry > addrCache_;
        char  buf_[512];
        pid_t child_pid_;       // 0=do not exists yet, <0=failed
        int   pipefd_[2];       // [0]=to say child, [1]=listen child

   private:
        static pid_t popen2( const char *command, int *infp, int *outfp );
        void pipe_say();
        void pipe_getline();
        static bool checkstopwords();

};

// Main method: ask child and parse answer about name/line
Addr2LineResolver::CacheEntry Addr2LineResolver::request( void* addr )
{
    // Protection check
    if ( !addr )
        return { "nullptr", "" };

    auto it = addrCache_.find( addr );
    if ( it != addrCache_.end() )
         return it->second;

    if ( child_pid_ == 0 )
    {
        sprintf( buf_, "/usr/bin/addr2line -C -e `readlink /proc/%d/exe` -f", getpid() );
        child_pid_ = popen2( buf_, &pipefd_[0], &pipefd_[1] );
            if ( child_pid_ <= 0)
        {
            perror("popen2 fail:");
            SAY_DBG( "Unable to exec: rv=%d\n", child_pid_ );
            child_pid_=-1;
        }
    }
    if ( child_pid_ <= 0 )
         return { "{no info}", "" };

    sprintf( buf_, "%p\n", addr );
    pipe_say();

    pipe_getline();
    std::string funcName( buf_ );

    pipe_getline();

    std::string path;
    if ( path.find( "??:" ) != 0 )
    {
        path =  buf_ ;

        if ( path.find( "/.." ) != std::string::npos )
            path = squeezePath( path );

        static const std::string at_str(" at ");
        path =  at_str + path;
    } 

    CacheEntry entry { funcName, path };

    addrCache_[ addr ] = entry;
    return entry;
}

// Run command and bind with pipes to descriptors *infp/*outfp
pid_t Addr2LineResolver::popen2( const char *command, int *infp, int *outfp )
{
    int p_stdin[2], p_stdout[2];
    pid_t pid;

    enum {PIPEREAD=0, PIPEWRITE=1};

    if (pipe(p_stdin) != 0 || pipe(p_stdout) != 0)
        return -1;

    pid = fork();

    if (pid < 0)
        return pid;
    else if (pid == 0)
    {
        close(p_stdin[PIPEWRITE]);
        dup2(p_stdin[PIPEREAD], PIPEREAD);
        close(p_stdout[PIPEREAD]);
        dup2(p_stdout[PIPEWRITE], PIPEWRITE);

        execl("/bin/sh", "sh", "-c", command, NULL);
        perror("execl");
        exit(1);
    }

    if (infp == NULL)
        close(p_stdin[PIPEWRITE]);
    else
        *infp = p_stdin[PIPEWRITE];

    if (outfp == NULL)
        close(p_stdout[PIPEREAD]);
    else
        *outfp = p_stdout[PIPEREAD];

    return pid;
}

// send string buf_ to child
void Addr2LineResolver::pipe_say()
{
        if ( child_pid_ <= 0 )
                return;
        int ln = write( pipefd_[0], buf_, strlen( buf_ ) ); // write message to the process
        if ( ln < 1 )
        {
            perror("Fail to pipe write:");
            child_pid_ = -child_pid_;
        }

}

// get string from child to buf_ (and cut off terminal \n )
void Addr2LineResolver::pipe_getline()
{
        buf_[0] = 0;
        if ( child_pid_ <= 0 )
                return;

        int idx = 0;
        char ch;
        int len;
        for(;;)
        {
            len = read( pipefd_[1], &ch, 1 );
            if ( len < 1 )
            {
                child_pid_ = -child_pid_;
                buf_[idx] = 0;
                perror("fail read pipe");
                return;
            }

            if ( ch == '\n' )
            {
                buf_[idx] = 0;
                return;
            }
            if ( idx > ( sizeof(buf_) - 5 ) )
                continue;
            buf_[idx++] = ch;
        }
}

static Addr2LineResolver a2l_resolver;
#endif // ADDR2LINE_AVAILABLE

#if BACKTRACE_AVAILABLE
/***************************************************************************
        Create stacktraces

 NOTE: For performance and simplicity reason Key is just 64bit-hash
       so collision are unlike but possible
***************************************************************************/

// AUX: Simple quick hash
uint64_t FNV1aHash ( const unsigned char *buf, uint64_t len )
{
  uint64_t hval = 0x811c9dc5;

    for ( ; len; len--)
    {
      hval ^= (uint64_t)*buf++;
      hval *= 1099511628211;
    }

  return hval;
}

// AUX: Convert array of pointers to hash (to catch repeating)
uint64_t makeKey( void* ar[], int bufsize )
{
    return FNV1aHash( reinterpret_cast<const unsigned char*>(ar), bufsize*sizeof(void*) );
}

// PURPOSE:   Create short notation of stacktrace ( just few func names from begin and end )
// ARGUMENTS: vector of function names
std::string collapseNames( const std::vector<std::string>& func_names )
{
    const int lastIdx = func_names.size()-1;
    std::string res;
    int found;

    for ( int i=0; i<=lastIdx; i++)
    {
        if ( i == ::tsv::debug::settings::btNumLeadFuncs && lastIdx > ::tsv::debug::settings::btNumLeadFuncs )
        res += " - ...";
        if ( i >= ::tsv::debug::settings::btNumLeadFuncs && i < lastIdx )
            continue;
        std::string fname( func_names[i] );
        found = fname.find_first_of( " (" );
        if ( found == std::string::npos )
        res += " - " + fname + "()";
        else
            res += " - " + fname.substr( 0, found ) + "()";
    }
    return res.substr(3);
}

#endif
}   // namespace symbol_resolve
}   // anonymous namespace

/***************** END OF local aux functions *******************************/


std::string resolveAddr2Name( void* addr, bool addLineNum /*=false*/, bool includeHexAddr /*= false */ )
{
#if ADDR2LINE_AVAILABLE
    auto symbolEntry = symbol_resolve::a2l_resolver.request( addr );
    if ( !includeHexAddr )
        return symbolEntry.getSymbol( addLineNum );
    return ::tsv::util::tostr::hex_addr( addr ) + " " +symbolEntry.getSymbol( addLineNum );
#else
    return ::tsv::util::tostr::hex_addr( addr );
#endif
}


// Get stack backtrace
// ARGUMENTS:
//      depth   = how many levels print
//      skip    = how many first levels skipped
//      enforce = if true, then print stacktrace anyway(ignore btEnabled value)
// RETURN VALUE:
//      vector of values to display
//===================================================
std::vector<std::string> getBackTrace( int depth /*=-1*/, int skip /*=0*/, bool enforce /*=false*/ )
{
    std::vector<std::string> return_value;

    // Return if disabled
    if ( !::tsv::debug::settings::btEnabled && !enforce )
        return return_value;
#if !BACKTRACE_AVAILABLE
    return_value.push_back( "Backtrace feature is not available" );
#else

    // Prepare values
    if ( depth < 0 )
        depth = 100;
    skip++;         // skip this function
    depth += skip;
    if ( depth > 100 )
        depth = 100;

    // Get backtrace using GNU C std library (execinfo.h)
    void* array[102];
    int size = backtrace( array, depth );

    // Remember printed backtraces and later use its id only
    if ( ::tsv::debug::settings::btShortList || ::tsv::debug::settings::btShortListOnly )
    {
        // cachedStackTrace[ calltrace_hash ] = { short_notation_str, callstack_id_int }
        static std::unordered_map< uint64_t, std::pair< std::string, int > > cachedStackTrace;

        uint64_t key = symbol_resolve::makeKey( array, size );
        auto it = cachedStackTrace.find( key );
        if ( it != cachedStackTrace.end() )
        {
            // This stacktrace was already mentioned -- USE SHORT NOTATION ONLY (to make shorter output)
            auto& value = it->second;
            return_value.push_back( ::tsv::util::tostr::strfmt( "StackTrace#%d - repeated: %s", value.second, value.first.c_str() ) );
            return return_value;
        }
        else
        {
            // This stacktrace wasn't mentioned before. Remember it

            // (a) create function name list
            std::vector<std::string> tracedNames;
            for ( int i = skip; i < depth; i++ )
            {
                auto symbolEntry = symbol_resolve::a2l_resolver.request( array[i] );
                if ( !symbolEntry.funcName_.length() )
                   break;
                tracedNames.push_back( symbolEntry.funcName_ );
                if ( symbol_resolve::a2l_resolver.isStopWord( symbolEntry.funcName_ ) )
                   break;
            }

            // (b) Create short notation and remember it
            std::string shortName( symbol_resolve::collapseNames( tracedNames ) );
            int stackTraceId = cachedStackTrace.size()+1;
            cachedStackTrace[ key ] = std::make_pair( shortName, stackTraceId );

            return_value.push_back( ::tsv::util::tostr::strfmt( " .. StackTrace#%d : %s", stackTraceId, shortName.c_str() ) );
        }
    }

    // If only short notation is requested, return it
    if ( ::tsv::debug::settings::btShortListOnly )
        return return_value;

    // Fill lines-by-line stacktrace
    for ( int i = skip; i < depth; i++ )
    {
        auto symbolEntry = symbol_resolve::a2l_resolver.request( array[i] );
        if ( !symbolEntry.funcName_.length() )
           break;

        if ( ::tsv::debug::settings::btIncludeAddr )
            return_value.push_back( ::tsv::util::tostr::strfmt( " .. #%02d[%p] %s", i, array[i], symbolEntry.getSymbol( ::tsv::debug::settings::btIncludeLine ).c_str() ) );
        else
            return_value.push_back( ::tsv::util::tostr::strfmt( " .. #%02d %s", i, symbolEntry.getSymbol( ::tsv::debug::settings::btIncludeLine ).c_str() ) );
        if ( symbol_resolve::a2l_resolver.isStopWord( symbolEntry.funcName_ ) )
            break;
    }
    return return_value;
#endif
}


}   // namespace debug
}   // namespace tsv

