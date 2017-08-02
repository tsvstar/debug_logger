#include <iostream>
#include "../tostr.h"

/************** TEST **********/

using namespace std;
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

int main()
{
    int x = 10;
    TempClass c;
    const char* vv = "str";
    std::string ss( "std::string");

    std::cout << "\n" << toStr(x);
    std::cout << "\n" << toStr(15);
    std::cout << "\n" << toStr("literal");
    std::cout << "\n" << toStr(vv);
    std::cout << "\n" << toStr(ss);
    std::cout << "\n\nPointers:";
    std::cout << "\n" << hex_addr(vv);
    std::cout << "\n" << toStr( &vv );
    std::cout << "\n" << toStr( (void*)vv );
    std::cout << "\n" << toStr( nullptr );
    std::cout << "\n" << toStr( &ss );

    std::cout << "\n\nObjects:";
    std::cout << "\n" << toStr( c );
    std::cout << "\n" << toStr( &c );

    std::cout << "\n\nMacro:";
    std::cout << "\n" << TOSTR_ARGS( x, "15", vv, c, add(x,13) );
    std::cout << "\n" << TOSTR_JOIN( x, "15", vv, c, add(x,13) );

    int res = 3 + add(x,13);
    std::cout << "\n" << TOSTR_EXPR( res, "=", 3, "+", add(x,13) );

    std::cout << "\n";

    return 0;
}

