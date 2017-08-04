#include <iostream>

/************** TEST **********/

using namespace std;

bool test( bool& isOkTotal, const char* prefix, std::string val, const char* compare = nullptr, bool equal = true )
{
    std::cout << prefix << val << "\n";
    if ( compare == nullptr )
        return true;

    bool isOk = ( val == compare );
    if ( !equal )
        isOk = !isOk;
    isOkTotal = isOkTotal && isOk;
    if ( !isOk )
    {
        std::cout << "TEST FAIL! Should be " << compare << "\n";
    }
    return isOk;
}

// Declaration from another test_*.cpp
bool test_tostr();
bool test_sentry();
bool test_objlog();
bool test_watcher();

/**************** MAIN() ***************/
int main()
{
    std::cout<< "\n *** TOSTR module ***\n";
    test_tostr();

    std::cout<< "\n *** DEBUGLOG module ***\n";
    test_sentry();

    std::cout<< "\n *** OBJLOG module ***\n";
    test_objlog();

    std::cout<< "\n *** DEBUGWATCH module ***\n";
    test_watcher();

    return 0;
}

