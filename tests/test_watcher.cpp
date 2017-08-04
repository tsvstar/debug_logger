#include <iostream>
#include <string>
#include <cstdio>

#include <vector>
#include <fstream>

#include "../debuglog.h"
#include "../debugwatch.h"

// Declaration from main.cpp
bool test( bool& isOkTotal, const char* prefix, std::string val, const char* compare = nullptr, bool equal = true );

static bool isOkTotal;

namespace {

  std::string last_value;
  void testLoggerHandlerWatch( const char* fmt, void* args )
  {
      std::string output;
      output = ::tsv::util::tostr::strfmtVA( fmt, static_cast<va_list*>(args) ) + "\n";
      last_value += output;
      std::cout<< output;
  }
}

/************* TEST OBJ1 **********************/


struct Base
{
    Base() : s__("1") {}
    ~Base() {}
    Base& operator=( const Base& b ) {return *this; }
    std::string s__;

    def_prop (std::string, Base, s_,
        {
          return ::tsv::debug::Watch_Getter( self, "s_", self.s__, 0, ::tsv::util::tostr::ENUM_TOSTR_REPR );
        },
        {
          ::tsv::debug::Watch_Setter( self, "s_", self.s__, value, 0, ::tsv::util::tostr::ENUM_TOSTR_REPR );
        }
      );

};

struct Deriv : public Base
{

};


int test_watcher()
{
    // Prepare sequence
    isOkTotal = true;
    // Replace test handler
    ::tsv::debug::LoggerHandler::handler_s = testLoggerHandlerWatch;

    SENTRY_FUNC();

    Base b, c;
    b.s_ += "x";
    std::string vv = std::string(b.s_) + "y" ;



    // Construction doesn't trigger
    Deriv* p = static_cast<Deriv*>(new Base());
    Deriv v;
    //TODO: Why here operation= wasn't triggered ??
    v = *p;
    delete p;
}
