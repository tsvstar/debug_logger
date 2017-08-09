#include <iostream>
#include <string>
#include <cstdio>

#include <vector>
#include <fstream>

#include "../tostr_handler.h"
#include "../debuglog.h"
#include "../objlog.h"

// Declaration from main.cpp
bool test( bool& isOkTotal, const char* prefix, std::string val, const char* compare = nullptr, bool equal = true );

static bool isOkTotal;

namespace {

  std::string last_value;
  void testLoggerHandlerObj( const char* fmt, void* args )
  {
      std::string output;
      output = ::tsv::util::tostr::strfmtVA( fmt, static_cast<va_list*>(args) ) + "\n";
      last_value += output;
      std::cout<< output;
  }
}

/************* TEST OBJ1 **********************/

// Sample trivial item
class TrackedItem
{
public:
    TrackedItem();
    TrackedItem( int x );

    int x_;
    virtual void test() { std::cout<<"TrackedItem.test()\n"; }
private:
    ::tsv::debug::ObjLogger debug_entry_;
};

TrackedItem::TrackedItem( ) :
    x_(1),
    debug_entry_(                   // need to add ctor to every non-default ctor and process non-default operator= too
                 this,
                 "TrackedItem",
                 3,
                 "default ctor"
                 )
{
}

TrackedItem::TrackedItem( int x ) :
    x_( x ),
    debug_entry_(
                 (x == 3 ? nullptr : this ),
                 "TrackedItem",
                 3,
                 TOSTR_JOIN( "ctor x=",x ).c_str()
                 )
{
}

/************* TEST OBJ2 **********************/
class DerivedTrackedItem : public TrackedItem
{
public:
    int y = 10;
    virtual void test() { std::cout<<"DerivedTrackedItem.test()\n"; }
};

/************* Example toStr() extending **********************/

namespace tsv { namespace util{ namespace tostr{ namespace impl {

    // Handler definition
    ToStringRV __toString( const ::TrackedItem& value, int mode )
    {
        using namespace ::tsv::util;

        if ( mode == ENUM_TOSTR_DEFAULT )
            return { tostr::toStr(value.x_), true };
        else if ( mode == ENUM_TOSTR_REPR )
            return { "{x="+tostr::toStr(value.x_)+"}", true };
        // Extended info
        return { "{ addr=" + tostr::hex_addr(&value) +", x="+tostr::toStr(value.x_)+"}", true };
    }

} } } }

/************* TESTS OBJLOGGING **********************/

TrackedItem func2_val( TrackedItem arg )
{
    SENTRY_FUNC_W_ARGS( arg );
    arg.x_ += 100;
    return arg;
}

TrackedItem func2_val( TrackedItem&& arg, int val = 0 )
{
    SENTRY_FUNC( "moved func2_val version" );
    return arg;
}

TrackedItem func2_ref( TrackedItem& arg )
{
    SENTRY_FUNC_W_ARGS( arg );
    arg.x_ += 100;
    return arg;
}

void func1( TrackedItem tp )
{
    SENTRY_FUNC_W_ARGS( tp );
    TrackedItem t3 = func2_val( tp );
    SAY_ARGS( "after func2_val:", tp, t3 );
    func2_ref( tp );
    SAY_ARGS( "after func2_ref:", tp );
}

void test_objlog_body()
{
    SENTRY_FUNC();
    TrackedItem t1;
    TrackedItem t2(11);
    TrackedItem t3(33);
    SAY_ARGS( t1, t2, t3 );
    t1 = t2;
    SAY_ARGS( t1, t2 );
    func1( std::move(t1) );

    {
        SENTRY_CONTEXT("");
        DerivedTrackedItem dt1;
        DerivedTrackedItem* dt2 = new DerivedTrackedItem();
        SAY_ARGS( dt1, dt2 );

	// Example how to determine which virtual function will be called
        TrackedItem* dt3 = &dt1;
        SAY_DBG( "CALL %s", ::tsv::debug::resolveAddr2Name( WHICH_FUNC_WILL_BE_CALLED( TrackedItem, dt3, test ), true, true ).c_str() );
        dt3->test();
    }
    // Leak dt2 here

    t3 = func2_val( TrackedItem(44), 1 );
    SAY_DBG( TOSTR_ARGS_EXTENDED( t3 ) );

   ::tsv::debug::ObjLogger::printTrackedPtr();
}

bool test_objlog()
{
    // Prepare sequence
    isOkTotal = true;
    // Replace test handler
    ::tsv::debug::LoggerHandler::handler_s = testLoggerHandlerObj;
    test_objlog_body();
    // See leaked object
    ::tsv::debug::ObjLogger::printTrackedPtr();

    /*
    std::ofstream myfile;
    myfile.open ("C:\\MY\\cpp_logger\\debug_logger\\objlog.txt");
    myfile << last_value;
    myfile.close();
    */

    return isOkTotal;
}
