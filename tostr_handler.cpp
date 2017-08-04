/*********************************************************************
  Purpose: Universal tuneable conversion to string
            [ special cases implementations here ]
  Author: Taranenko Sergey (tsvstar@gmail.com)
  Date: 02-Aug-2017
  License: BSD. See License.txt
**********************************************************************/
#include <sstream>
#include <cstdarg>      // va_list
#include <cstdio>      // vsprintf
#include <memory>       // unique_ptr
#include "tostr_handler.h"

using namespace std;

namespace tsv {
namespace util{
namespace tostr{


/*********** Special cases of toStr() *************/

// Special case: const char*
std::string toStr( const char* v, int mode )
{
    if ( !v )
        return "nullptr";
    if ( mode == ENUM_TOSTR_DEFAULT )
        return std::string(v);
    return "\"" + std::string(v) + "\"";
}

// Special case: void*
std::string toStr( void* v, int )
{
    return hex_addr( v );
}

// Special case: nullptr
std::string toStr( std::nullptr_t value, int )
{
    return "nullptr";
}

/*********** BEGIN OF namespace impl *************/
namespace impl
{

// Handler of std::string
ToStringRV __toString( const std::string& value, int mode )
{
    if ( mode == ENUM_TOSTR_DEFAULT )
        return { value, true };
    else
        return { "\"" + value + "\"", true };
}

}
/*********** END OF namespace impl *************/



//Get pointer hex representation
std::string hex_addr( const void* ptr )
{
    if ( !ptr )
        return "nullptr";
    else
    {
        std::stringstream ss;
        ss << ptr;
        return ss.str();
    }
}

/*******************************************
        Safe sprintf replacement
*******************************************/

// vsprintf-like
std::string strfmtVA( const std::string& fmt_str, va_list* args )
{
    static unsigned int bufsize=10000;
    std::unique_ptr<char[]> buf( new char[10000] );
    //printf("VA %s *%p=%p\n", fmt_str.c_str(), args, *args);

    if ( bufsize <= fmt_str.size() )
    {
        bufsize = fmt_str.size()*2+1 + 100;
        buf.reset(new char[bufsize]);
        //printf("presize/%d %p\n",bufsize,buf.get());
    }

    while( true )
    {
        int final_n = vsnprintf( buf.get(), bufsize-5, fmt_str.c_str(), *args );
        if ( final_n < 0 )                  // error happens
            return fmt_str;
        else if ( final_n < static_cast<int>(bufsize) )     // ok
            return std::string( buf.get() );

        // need more then current buffer - resize it with extra and try again
        bufsize = final_n+100;
        buf.reset(new char[bufsize]);
            printf("resize/%d %p\n",bufsize,buf.get());
    }
}

// Safe sprintf-like
std::string strfmt( const std::string& fmt_str, ... )
{
    va_list ap;
    va_start( ap, fmt_str );
    const std::string& s = strfmtVA( fmt_str, &ap );
    va_end( ap );
    return s;
}


}   // end of namespace tostr
}   // end of namespace util
}   // end of namespace tsv
