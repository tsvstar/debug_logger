#include <iostream>
#include <string>
#include "../debuglog.h"

// Declaration from main.cpp
bool test( bool& isOkTotal, const char* prefix, std::string val, const char* compare = nullptr, bool equal = true );

using namespace ::tsv::debug;

static bool isOkTotal;

namespace {

  std::string last_value;
  void testLoggerHandlerSentry( const char* fmt, void* args )
  {
      std::string output;
      output = ::tsv::util::tostr::strfmtVA( fmt, static_cast<va_list*>(args) ) + "\n";
      last_value += output;
      std::cout<< output;
  }
}


void func1()
{
    // Example of simple function sentry
    SENTRY_FUNC();
    // Example if we need catch stacktrace to this point
    SAY_STACKTRACE();

    for ( int x = 0; x < 4; x ++ )
    {
        // Example of internal scope sentry
        //  with conditional output ( suppress output on cycle with x==1)
        SENTRY_CONTEXT( (x==1?nullptr:"LOOP"), TOSTR_ARGS( x ) );
        int res = 100 - x;
        SAY_ARGS( res );
    }
}

void func2( int arg1, const char* arg2 )
{
    // Example func sentry with arguments
    SENTRY_FUNC_W_ARGS( arg1, arg2 );
    // Start timing for this pointer
    // (note: SENTRY_* macro define local var "sentry")
    EXECUTE_IF_DEBUGLOG( sentry.startTiming() );

    // Example to simple output
    SAY_DBG( "Going to the cycle");

    // Example of complex SAY_DBG ()
    for ( int x = 0; x < 3; x ++ )
        SAY_DBG( "loop iteration %d", x );

    // Example to quick logging of expressions
    int x = 8;
    int res = arg1 + x;
    SAY_EXPR( "Example expression:", res, "=", arg1, "+", x );  // could be any mix of arguments

    // To see how it is visble inside of
    void func3( int arg );
    void func4( int arg );
    void func5();
    func3(33);
    func4(44);
    func5();
}

void func3( int arg )
{
    // Using SAY_* before sentry declare include it into upper sentry
    SAY_DBG( "Check for func3.." );

    // Example of create sentry with non-standard flagging
    // (enforce output to stdout + display only leave message)
    SENTRY_ALT_FUNC( LOG_STDOUT | LOG_LEAVE )( "This shouldn't be visible" );

    // Check that absence LOG_EVENTS cause suppressing of events output
    SAY_DBG( "and this one too..." );

    // Example output, using iostream syntax
    // Using any declaration of SENTRY_ adds "sentry" local variable;
    sentry << "Sample " << arg <<"\n"
            << "and this is second line\n";
}

void func4( int arg )
{
    // Using SAY_* before sentry declare include it into upper sentry
    SAY_DBG( "Check for func4.." );

    // iostream syntax could be used with SENTRY_FNSTREAM() macro too
    // Have to be finished with \n !!
    SENTRY_FNSTREAM() << "Initial message\n";

    // All syntax (printf/iostream/ARGS/EXPR) are available at same time
    SAY_STACKTRACE();
    SAY_DBG( "Test" );
    SAY_ARGS( arg );

    // And look how it process output
    // NOTE: Only EOL ("\n") actually flush such string to log
    sentry << "2:Sample " << arg <<"\n"
            << "2:and this is second line\n";
}

void func7()
{
    // Example how to use raw SentryLogger
    using namespace ::tsv::debug;
    using namespace ::tsv::debug::SentryLoggerFlags;
    SentryLogger s( "my context", "this is args", LOG_ALL | LOG_TIMING );
    // regular output
    s.vwrite( "vwrite(%d)", 1 );
    // enforced output without context
    s.print_event( "print_event()" );
    // output using iostream
    s<<"iostream interface\n";
}

void func6(int x)
{
    // To display how displayed deep stack
    SENTRY_FUNC_W_ARGS( x );
    SAY_DBG("Inside of func6");
    func7();
}
void func5()
{
    // Use blank non-empty value to print blank line
    SAY_DBG(" ");
    // Logged outside of func5 sentry, because before its definition
    SAY_DBG("Checking func5");

    // Actually show
    SENTRY_FUNC();

    // Go deeper
    func6(66);

    SAY_DBG("After func6");
}

bool test_sentry()
{
    // Prepare sequence
    isOkTotal = true;
    // Replace test handler
    LoggerHandler::handler_s = testLoggerHandlerSentry;

    int intvalue = 10;

    // Example outer printing
    SAY_DBG("outside value:\n");     // print to log text remark
    SAY_ARGS( intvalue );            // print to log one or several variables
    SAY_ARGS( "Example", intvalue ); // example that printing vars could include remark

    func1();
    func2( intvalue, "str_" );

    return isOkTotal;
}
