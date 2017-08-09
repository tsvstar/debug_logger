/*********************************************************************
  Purpose: Scope/function enter/exit and regular logging
  Purpose: Debug logging system
  Author: Taranenko Sergey (tsvstar@gmail.com)
  Date: 02-Aug-2017
  License: BSD. See License.txt
**********************************************************************/

// Always replace trigger with 1 here because we need to full base declarations
#define DEBUG_LOGGING 1

#include <iostream>
#include <sys/time.h>
#include <memory>       // unique_ptr
#include <cstring>      // strlen
#include <cstdio>       // sprintf


#include "debuglog.h"
#include "tostr.h"

namespace tsv {
namespace debug {

//**************************************************************************
//      Define essential defaults for logging library
//**************************************************************************

// Output adaptor
namespace
{
  void defaultLoggerHandler( const char* fmt, void* args )
  {
    std::cout<< ::tsv::util::tostr::strfmtVA( fmt, static_cast<va_list*>(args) ) << "\n";
  }
}

LoggerHandler::handle_t LoggerHandler::handler_s = defaultLoggerHandler;  //

/************** SentryLogger defaults and settings **********************/
bool SentryLogger::logStdoutFlag_s       = false;        // if true, when duplicate log output to stdout
bool SentryLogger::isNestedLevelMode_s   = true;         // if true, then show graphically hierarchy
static bool SentryLogger_contextname_vwrite = true;      // include {contextname} as event prefix in vwrite()

//**************************************************************************
//              SentryLogger
//**************************************************************************

using namespace SentryLoggerFlags;
int  SentryLogger::curLevel_s          = 0 ;
SentryLogger* SentryLogger::last_s  = nullptr ;

/**********************************************************************************
   PURPOSE:   Ctor
   ARGUMENTS: name = function name (mentioned in prefix of all events for this level)
	          args = text of suffix "Enter/Leave" events (if empty or null ptr = "scope" )
	          logFlags = set of flags
************************************************************************************/
SentryLogger::SentryLogger( const char* name /*=""*/, const char* args /*=""*/, int logFlags /*=LOG_ALL*/ ) :
        name_( name ? name : "" ), loggingFlags_ ( logFlags )
{
    if ( logStdoutFlag_s )
        loggingFlags_ |= LOG_STDOUT;

    // Conditional logging: turn off sentry if name==nullptr
    if ( !name )
        setLoggingFlags( LOG_OFF );

    const char* comment = "scope";
    if ( args && args[0] )
        comment = args;

    // Track timing
    starttime_ = 0.0;
    if ( loggingFlags_ & LOG_TIMING )
        startTiming( true );

    // Add to the list
    prev_sentry_ = last_s;
    last_s = this;

    // Increase nested level
    curLevel_s++;

    // Print automatic enter message
    log_state_  = LOG_ENTER;
    if ( !( loggingFlags_ & LOG_NO_AUTOENTER ) )
        vwrite( "%sEnter %s", (isNestedLevelMode_s?"":">> "), comment );
    // ..and go to main state
    log_state_ = state_ = LOG_EVENTS;
}

/**********************************************************************************
   PURPOSE:   Dtor
**********************************************************************************/
SentryLogger::~SentryLogger()
{
    // Finalize << operations
    if ( stream.str().length() )
        processStream(true);

    // Print leave message
    log_state_  = LOG_LEAVE;
    if ( loggingFlags_ & LOG_TIMING )
    {
        struct timeval t;
        gettimeofday( &t, nullptr );
        double fintime = t.tv_sec + (double)t.tv_usec/1000.0;

        vwrite( "%sLeave scope. exectime=%.2lf s", (isNestedLevelMode_s?">> ":""), fintime-starttime_ );
    }
    else
        vwrite( "%sLeave scope", (isNestedLevelMode_s?">> ":"") );

    // Exclude from chain
    if ( last_s == this )
    {
        last_s = prev_sentry_;
    }
    else
    {
        // something strange happens. deallocate not in order of allocation
        SentryLogger* cur = last_s;
        for( ; !cur ; cur = cur->prev_sentry_ )
        {
            if ( cur->prev_sentry_ == this )
            {
                cur->prev_sentry_ = cur->prev_sentry_->prev_sentry_;
                break;
            }
        }
    }
    // Decrease nested level
    curLevel_s--;
}

// Turn on/off timing
//=====================
void SentryLogger::startTiming( bool start /*=true*/ )
{
    if ( !start )
        loggingFlags_ &= ~LOG_TIMING;
    else
    {
        struct timeval t;
        gettimeofday( &t, nullptr );
        starttime_ = t.tv_sec + (double)t.tv_usec/1000.0;
        loggingFlags_ |= LOG_TIMING;
    }
}


// Auxilary function
// Check for EOL symbol in the stream and flush it using vwriteImpl
//=================================================================
void SentryLogger::processStream( bool enforceFlush )
{
    std::string s = stream.str();

    std::size_t found;
    bool foundFlag = false;
    while ( (found = s.find("\n") ) != std::string::npos )
    {
        foundFlag = true;
        if ( found )
        {
            std::string pre = s.substr( 0, found );
            vwrite( "%s", pre.c_str() );
        }

        s = s.substr( found+1 );
    }

    if ( enforceFlush )
    {
       if ( s.length() )
       {
         vwrite( "%s", s.c_str() );
         stream.str("");
       }
    }
    else if ( foundFlag )
    {
        stream.str( s );
    }
}

// Print to log (if pass log_state_ condition)
//=================================================================
void SentryLogger::vwrite( const char* format, ... )
{
    va_list args;
    SentryLogger* self = last_s;

    // If no any sentry was allocated,
    // treat as printing event with nestedLevel=0
    if ( !self )
    {
        va_start( args, format );
        int level = LOG_EVENTS | ( logStdoutFlag_s ? LOG_STDOUT : 0 );
        vwriteImplVA( level, "", isNestedLevelMode_s, "", format, &args );
        va_end( args );
        return;
    }

    // If not enforce and not enabled by flags, skip it
    if ( !(self->loggingFlags_ & LOG_ENFORCE) )
    {
        if ( !( self->loggingFlags_ & self->log_state_ ) )
            return;
    }

    static std::string empty_str;
    va_start( args, format );
    int level = ( self->log_state_ & LOG_ALL ) | ( self->loggingFlags_ & LOG_STDOUT );
    vwriteImplVA( level,
                 SentryLogger_contextname_vwrite ? self->name_ : empty_str,
                 isNestedLevelMode_s,
                 "",
                 format,
                 &args );
    va_end( args );

    self->log_state_ = self->state_;
}

// Internal function to print log
// (actually prepare string and call handlers)
//=================================================================
void SentryLogger::vwriteImplVA( int level, const std::string& fn_name, bool isNested, const char* prefix, const char* format, void* args )
{
    static size_t sizeStr = 0;
    static std::unique_ptr<char[]> formatNew;

    // Check arguments
    if ( !prefix )
        prefix = "";
    if ( !format || !format[0] )
    {
        if ( prefix[0] )
            format = "";
        else
            return;
    }

    // Check that handler_s exists or stdout is active
    if ( !LoggerHandler::handler_s &&
         !(level & LOG_STDOUT) )
        return;

    // Resize format buffer if needed
    size_t sizeNew = strlen( format ) +
                     fn_name.length() +
                     curLevel_s +
                     15;        // "[DBG]%02d{f} \0"
    if ( sizeStr < sizeNew )
    {
        sizeStr = sizeNew + 150;        // extra to minimize reallocation
        formatNew.reset( new char[ sizeStr ] );
    }

    std::string nested;
    static char buflevel[10];
    if ( !isNested )
    {
        buflevel[0] = 0;
    }
    else
    {
        sprintf( buflevel, "%02d", curLevel_s );
        switch ( level & LOG_ALL )
        {
        case LOG_ENTER: nested = std::string( curLevel_s, '>' ); break;
        case LOG_EVENTS:nested = std::string( curLevel_s, ' ' ); break;
        case LOG_LEAVE: nested = std::string( curLevel_s, '<' ); break;
        }
    }

    if ( fn_name.length() )
        sprintf( &formatNew[0], "[DBG]%s%s{%s} %s%s", buflevel, nested.c_str(), fn_name.c_str(), prefix, format );
    else
        sprintf( &formatNew[0], "[DBG]%s%s %s%s", buflevel, nested.c_str(), prefix, format );

    if ( LoggerHandler::handler_s )
            LoggerHandler::handler_s( &formatNew[0], args );
    if ( level & LOG_STDOUT )
    {
        vprintf( formatNew.get()+5, *reinterpret_cast<va_list*>(args) );
        printf("\n");
    }
}

// Enforced print "Enter scope" message
void SentryLogger::print_enter( const char* format /*=""*/ , ... )
{
    // format==nullptr means no logging by sentry
    if ( !format )
    {
        setLoggingFlags( LOG_OFF );
        return;
    }

    // If not enforce and not enabled by flags, skip it
    if ( !(loggingFlags_ & LOG_ENFORCE) )
    {
        if ( !( loggingFlags_ & LOG_ENTER ) )
            return;
    }

    // Do print
    va_list args;
    va_start( args, format );

    int level = LOG_ENTER | ( loggingFlags_ & LOG_STDOUT );
    if ( !format[0] )
        format = "scope";
    if ( isNestedLevelMode_s )
        vwriteImplVA( level, name_, true, ">> Enter ", format, &args );
    else
        vwriteImplVA( level, name_, false, "Enter ", format, &args );
    va_end( args );

    log_state_ = state_;
}

// Enforced print as event without context
//=============================
void SentryLogger::print_event( const char* format, ... )
{
    va_list args;
    va_start( args, format );

    int level = LOG_EVENTS | ( logStdoutFlag_s ? LOG_STDOUT : 0 );
    vwriteImplVA( level, "", isNestedLevelMode_s, "", format, &args );
    va_end( args );
}

// Specialization of stream operator
//=============================
template<>
SentryLogger& SentryLogger::operator<< ( const LoggerEvent& val )
{
    processStream( true );
    log_state_ = (val.type_ & LOG_ALL);
    return *this;
}

// Print stack backtrace
//      depth   = how many levels print
//      skip    = how many first levels skipped
//      enforce = if true, then print stacktrace anyway(ignore btEnabled value)
//===================================================
void SentryLogger::printBackTrace( int depth /*=-1*/, int skip /*=0*/, bool enforce /*=false*/ )
{
    // ask for backtrace (and ignore this function)
    auto array = ::tsv::debug::getBackTrace( depth, skip+1, enforce );
    for ( auto& s : array )
    {
        vwrite( s );
    }
}


}
}
