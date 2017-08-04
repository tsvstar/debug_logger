#ifndef DEBUGLOG_H_
#define DEBUGLOG_H_


/*********************************************************************
  Purpose: Scope/function enter/exit and regular logging
  Author:  Taranenko Sergey (tsvstar@gmail.com)
  Date:    02-Aug-2017
  License: BSD. See License.txt
**********************************************************************/

#include <sstream>
#include <string>
#include "debugresolve.h"
#include "tostr.h"

/****************************************************************************
                        MACROS TO USE

    SENTRY_FUNC(...)            - sentry on entering to function
                                (show enter/exit, function arguments are printf-like )
    SENTRY_FUNC_W_ARGS(arg1,arg2..)   - like SENTRY_FUNC, but just mention all interested variables to print them
    SENTRY_ALT_FUNC(FLAG1|FLAG2)(...) - like SENTRY_FUNC, but include extra tuning sentry with flags
    SENTRY_CONTEXT( context_name, ... ) - create context sentry
    SENTRY_SILENT()                     - create function sentry with no enter/exit track

    SENTRY_FNSTREAM() << your << vars;          - create function sentry and print in iostream-style
    SENTRY_ALT_FNSTREAM(FLAG1|FLAG2)() << your; - like SENTRY_FNSTREAM, but include extra tuning sentry with flags


    SAY_STACKTRACE( [ depth=-1[, skip=0[, enforce=false]]] ) - print stacktrace to current
    SAY_DBG( std::string | printf-like ) - print to log in scope of current sentry
    SAY_ARGS( var1, var2,.. )            - print variables names and values
	SAY_EXPR( ... )				         - similar to above but useful to expressions
    EXECUTE_IF_DEBUGLOG( line of code ) -- if logging is enabled, instantiate code inside, otherwise skip it
                                           Actually quick one-line version of #if DEBUG_LOGGING\nline of code\n#endif

NOTES:
    1. These macro do jobs only if DEBUG_LOGGING is true. Otherwise no code produced at all.
        To enforced enable/disable, include #define before including this file
    2. Designed to work with cooperation with TOSTR_* family macros. Check them
    3. Sentry class is designed to be used as RAII!!! Do not entagle yourself.
       You can use SAY_* even without declaration SENTRY_ inside of context/function.
    4. SentryLogger class could be used without macros to make specific scenario
    5. Giving nullptr as first value of SENTRY_* macro turn off its output
        ( make easy runtime conditional output )

****************************************************************************/

// Are SentryLogger macros generate output (SENTRY_*, PRINT_*, SAY_DBG)
// Could be replaced for local file (define before #include "debuglog.h")
#ifndef DEBUG_LOGGING
#define DEBUG_LOGGING 1
#endif

#if DEBUG_LOGGING

#define SENTRY_SILENT        using namespace ::tsv::debug::SentryLoggerFlags; ::tsv::debug::SentryLogger sentry( __func__, "", LOG_EVENTS );               SentryLoggerEmpty::empty_func
#define SENTRY_FUNC          using namespace ::tsv::debug::SentryLoggerFlags; ::tsv::debug::SentryLogger sentry( __func__, "", LOG_ALL|LOG_NO_AUTOENTER);  sentry.print_enter
#define SENTRY_FUNC_W_ARGS(...)  SENTRY_FUNC( TOSTR_ARGS( __VA_ARGS__ ) )
#define SENTRY_ALT_FUNC(val)     using namespace ::tsv::debug::SentryLoggerFlags; ::tsv::debug::SentryLogger sentry( __func__, "", val | LOG_NO_AUTOENTER );   sentry.print_enter
#define SENTRY_FNSTREAM()        using namespace ::tsv::debug::SentryLoggerFlags; ::tsv::debug::SentryLogger sentry( __func__, "", LOG_ALL|LOG_NO_AUTOENTER ); sentry<<LoggerEvent(LOG_ENTER)<<"Enter "
#define SENTRY_ALT_FNSTREAM(val) using namespace ::tsv::debug::SentryLoggerFlags; ::tsv::debug::SentryLogger sentry( __func__, "", val|LOG_NO_AUTOENTER );     sentry<<LoggerEvent(LOG_ENTER)<<"Enter "
#define SENTRY_CONTEXT( contextname,...)  using namespace ::tsv::debug::SentryLoggerFlags; ::tsv::debug::SentryLogger sentry( contextname, "", LOG_ALL|LOG_NO_AUTOENTER ); sentry.print_enter( __VA_ARGS__ )

#define SAY_STACKTRACE  ::tsv::debug::SentryLogger::printBackTrace
#define SAY_DBG         ::tsv::debug::SentryLogger::vwrite
#define SAY_ARGS(...)   ::tsv::debug::SentryLogger::vwrite( TOSTR_ARGS(__VA_ARGS__) )
#define SAY_EXPR(...)   ::tsv::debug::SentryLogger::vwrite( TOSTR_EXPR(__VA_ARGS__) )

#define EXECUTE_IF_DEBUGLOG(...) __VA_ARGS__

#else

//#define SENTRY_SILENT(...)        using namespace SentryLoggerFlags;

#define SENTRY_SILENT(...)        using namespace ::tsv::debug::SentryLoggerFlags; ::tsv::debug::SentryLoggerEmpty()
#define SENTRY_FUNC(...)          using namespace ::tsv::debug::SentryLoggerFlags; ::tsv::debug::SentryLoggerEmpty()
#define SENTRY_ALT_FUNC(val)      SENTRY_SILENT
#define SENTRY_ALT_FNSTREAM(val)  SENTRY_SILENT

#define SAY_DBG(...)             ;
#define SAY_STACKTRACE(...)      ;
#define SAY_ARGS(...)            ;
#define SAY_EXPR(...)            ;
#define EXECUTE_IF_DEBUGLOG(...) ;

#endif

/*****************************************************************************
                            Classes
*****************************************************************************/

namespace tsv {
namespace debug {

// PURPOSE: Output adaptor
// Assign to handler_s function which will handle output of loggers
// Also could add static member, which needed to control its behavior
//=========================
struct LoggerHandler
{
    // print handler works in printf-like style
    // and get: "fmt"=format string, "args" = va_list*
    typedef void (*handle_t)( const char* fmt, void* args );

    static handle_t handler_s;              // output handler
};


// PURPOSE: Control values for SentryLogger output
//=========================================================
namespace SentryLoggerFlags
{
    enum FlagsEnum { LOG_OFF=0, LOG_ENTER=1, LOG_LEAVE=2, LOG_EVENTS=4, LOG_ALL=7,
                     LOG_STDOUT=8 , LOG_ENFORCE=16, LOG_TIMING=32, LOG_NO_AUTOENTER = 64 };
}


/************************************************************
//      Use this class to log enter/leave func or scope
//
//  To conditional logging use nullptr or empty string:
//      - nullptr mean no enter/leave logging + no vwrite log
//  example:
//      SentryLogger sentry( cond ? name : nullptr )
//      sentry.vwrite("This only if cond (%d)", cond );
************************************************************/
class SentryLogger;

class SentryLogger
{
    public:

        SentryLogger( const char* name="", const char* args="", int logFlags = SentryLoggerFlags::LOG_ALL );
        ~SentryLogger();

        static void setLogStdoutSystemFlag( bool flag ) { logStdoutFlag_s = flag; }
        static void setNestedLevelMode( bool flag ) { isNestedLevelMode_s = flag; }

        int  getLoggingFlags() { return loggingFlags_; }
        void setLoggingFlags( int flags )
        {
             loggingFlags_ = ( loggingFlags_ & ~SentryLoggerFlags::LOG_ALL ) | ( flags & SentryLoggerFlags::LOG_ALL );
        }

        // turn on or turn off timing
        void startTiming( bool value = true );

        // printf-style debug log inside of sentried scope
        // to make conditional output use ( cond ? "FMT" : "" )
        static void vwrite( const char* format, ... );
        static void vwrite( const std::string& content ) { vwrite( "%s", content.c_str() ); }

        // Stream interface
        template<class T>
        SentryLogger& operator<< ( const T& val )
        {
                stream<<val;
                processStream( false );
                return *this;
        }

        // print "depth" entries of stack backtrace from "skip"
        static void printBackTrace( int depth = -1, int skip = 0, bool enforce = false );

    public:
        // logging of "enter scope".
        // If format=nullptr, logging of this sentry is turned off completely
        void print_enter( const char* format = "", ... );

        // Safe versions of functions (ignore '%' chars if that is just a string )
        void print_enter( const std::string& content )        { print_enter( "%s", content.c_str() ); }

        // logging of enforced "events"
        // to make conditional output use ( cond ? "FMT" : "" )
        static void print_event( const char* format, ... );
        static void print_event( const std::string& content ) { print_event( "%s", content.c_str() ); }

    protected:
        std::string name_;              // name of sentry
        int loggingFlags_;              // set of LOG_* flags
        int state_;                     // current state of sentry
        int log_state_;                 // current state of sentry (as thought by processStream)
        std::ostringstream stream;      // content of <<
        double starttime_;              // timestamp of start sentry timer


        static int  curLevel_s;             // current level (depth)

        static SentryLogger* last_s;     // head of stack (nullptr means no sentry was allocated)
        SentryLogger* prev_sentry_;      // uni-direction backward linked list of sentries

    protected:
        // System settings
        static bool logStdoutFlag_s;        // true to enforce LOG_STDOUT
        static bool isNestedLevelMode_s;    // true to log in format  >>>>> value

    protected:
        SentryLogger( const SentryLogger& );
        SentryLogger& operator=( const SentryLogger& );

        void processStream( bool enforce );

        // real string processor ("args" actually is va_list* )
        static void vwriteImplVA( int level, const std::string& fn_name, bool isNested, const char* prefix, const char* format, void* args );
};

struct LoggerEvent;

template<>
SentryLogger& SentryLogger::operator<< ( const LoggerEvent& val );

// Modifier for SentryLogger stream
//=====================================
struct LoggerEvent
{
    LoggerEvent( int type ) : type_( type ) {}
    int type_;
};

// Auxilary class which is the stub to debug macros
//==================================================
struct SentryLoggerEmpty
{
        static void empty_func( ... ) {}
        template<class T> SentryLoggerEmpty& operator<< ( const T& val ) { return *this; }
        static void printBackTrace( int depth = -1, int skip = 0, bool enforce = false );
        void startTiming( bool flag = true ) {}
};

}
}

#endif
