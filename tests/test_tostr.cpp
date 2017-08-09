#include <iostream>
#include "../tostr.h"

// Declaration from main.cpp
bool test( bool& isOkTotal, const char* prefix, std::string val, const char* compare = nullptr, bool equal = true );

/************** TEST **********/

using namespace tsv::util::tostr;

// TEST
struct TempClass
{
    int x;
};

int add( int a, int b )
{
    return a + b;
}

bool test_tostr()
{
    int x = 10;
    TempClass c;
    const char* vv = "str";
    std::string ss( "std_string");

    bool isOk = true;

    std::cout << " ==== Test toStr() and TOSTR_* =======\n\n";
    test( isOk, "", toStr(x), "10" );
    test( isOk, "", toStr(15), "15" );
    test( isOk, "", toStr("literal"), "literal");
    test( isOk, "", toStr(vv), "str");
    test( isOk, "", toStr(ss), "std_string");
    test( isOk, "", toStr(ss, ENUM_TOSTR_REPR), "\"std_string\"");

    std::cout << "\nPointers:\n";
    std::string vv_addr( hex_addr(vv) );
    test( isOk, "", hex_addr(vv) );   // address could differ, so just print not test
    test( isOk, "", toStr( &vv ) );   // address could differ, so just print not test
    test( isOk, "", toStr( (void*)vv ), vv_addr.c_str() );
    test( isOk, "", toStr( nullptr ), "nullptr");
    test( isOk, "", toStr( &ss ) );   // address could differ, so just print not test

    std::cout << "\nObjects:\n";
    test( isOk, "", toStr( c ) );    // unknown object - say type. It differ depends on compiler
                                        // so just print not test
    test( isOk, "", toStr( &c ) );   // address could differ, so just print not test

    std::cout << "\n\nMacro:\n";
    test( isOk, "", TOSTR_ARGS( x, "15", vv, add(x,13) ),
 		"x = 10, 15 vv = \"str\", add(x,13) = 23" );
    test( isOk, "", TOSTR_JOIN( x, "15", vv, add(x,13) ), "1015str23" );

    int res = 3 + add(x,13);
    test( isOk, "", TOSTR_EXPR( res, "=", 3, "+", add(x,13) ),
 			"res{26} = 3 + add(x,13){23} " );

    std::cout << "\n";

    return isOk;
}

