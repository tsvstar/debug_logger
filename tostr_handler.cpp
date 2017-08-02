/*********************************************************************
  Purpose: Universal tuneable conversion to string
            [ special cases implementations here ]
  Author: Taranenko Sergey (tsvstar@gmail.com)
  Date: 02-Aug-2017
  License: BSD. See License.txt
**********************************************************************/
#include <sstream>
#include "tostr_handler.h"

using namespace std;

namespace tsv {
namespace util{
namespace tostr{

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

}
}
}
