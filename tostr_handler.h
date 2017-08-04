#ifndef TOSTR_HANDLER_H_
#define TOSTR_HANDLER_H_ 1

/*********************************************************************
  Purpose: Universal tuneable conversion to string
           [ used for implicit conversion inside of TOSTR_* macro
             and standalone ]
  Author: Taranenko Sergey (tsvstar@gmail.com)
  Date: 02-Aug-2017
  License: BSD. See License.txt
**********************************************************************/

#define CPP11_FEATURES (__cplusplus >= 201103L)    // Explicit define are C++11 feature available
// @tsv TEMPORARY
#define CPP11_FEATURES 0

#include <string>
#include <type_traits>
#include <typeinfo>         // typeid
#include <cstdarg>          // va_list
#if CPP11_FEATURES
#include <memory>           // for unique_ptr,shared_ptr, weak_ptr handlers
#else
#include <sstream>          // for old-style implementation of arithmetical print
#endif

// Forward declaration
namespace tsv{ namespace debug {
    std::string demangle(const char* name);
}}

// Forward declarations
class TrackedItem;
/** ... add your own classes here ***/

/********* MAIN PART **********/

namespace tsv {
namespace util{
namespace tostr{


// Auxiliary function to decode pointer to hex string
std::string hex_addr( const void* ptr );

/************************* SPRINTF-like functions  *******************************/

// Like sprintf, but returns std::string instead of using charbuf
std::string strfmt( const std::string& fmt_str, ... );

// Similar to vsprintf, but return std::string
std::string strfmtVA( const std::string& fmt_str, va_list* args );


/******************************************************
  toStr() internal processors implementations for types

  NOTE: processors are separated from main toStr() to
        make automatic dereferencing
*******************************************************/

// String conversion modes
enum ToStrEnum
{
    ENUM_TOSTR_DEFAULT,  // as string: value
    ENUM_TOSTR_REPR,     // as representation: "value"
    ENUM_TOSTR_EXTENDED  // extended info
};

namespace impl
{

// Return value of __toString
struct ToStringRV
{
    std::string value_;
    bool valid_;
};

// Default handler
template<typename T,
    typename std::enable_if< !std::is_arithmetic<T>::value, int >::type* = nullptr >
ToStringRV __toString( const T& value, int mode )
{
    // default case - generic kind of type
    return { "", false };   // no value
}

// Integral + floating_point handler
template<typename T,
    typename std::enable_if< std::is_arithmetic<T>::value, int >::type* = nullptr >
ToStringRV __toString( const T& value, int mode )
{
#if CPP11_FEATURES
    return { std::to_string( value ), true };
#else
    std::ostringstream ss;
    ss << value;
    std::string s( ss.str() );
    return { s, true };
#endif
}
// std::string handler
ToStringRV __toString( const std::string& value, int mode );


#if CPP11
template<typename T>
std::string __toString( const unique_ptr<T>& value, int mode  )
    { return  hex_addr( value.get() );
}
template<typename T>
ToStringRV __toString( const shared_ptr<T>& value, int mode  )
    { return  hex_addr( value.get() );

template<typename T>
ToStringRV __toString( const weak_ptr<T>& value, int mode  )
    { return  hex_addr( value.get() );
#endif

/**
       ... Your own classes special handlers  ...

To correct processing (and auto-dereferencing) your own classes forward declaration of handler have to be placed here
  and forward declaration of that class have to be placed in beginning of this file.
NOTE: Do not use specific to your class version of toStr(), otherwise pointers to it will not be auto-dereferenced

Example:

in beginning of this file:
    class YourClass;
here:
    ToStringRV toStr( const YourClass& value, int mode );

in .c:
   #include "tostr_handler.h"
   namespace tsv { namespace util { namespace tostr { namespace impl {
        ToStringRV __toString( const ::YourClass& value, int mode )
        {
            // Return: { VALUE_STD::STRING, true }
            return { "value", true };
        }
   } } } }
**/

    // That is sample class from test_objlog
    ToStringRV __toString( const ::TrackedItem& value, int mode );

}  // namespace impl


/**********************************************
    Main toStr() interface.

Purpose: Print val to std::string
Usage:
    std::string val = toStr( var );
    std::string val = toStr( var, ENUM_TOSTR_EXTENDED );
************************************************/
// Main implementation for values
template<typename T,
    typename std::enable_if< !std::is_pointer<T>::value, int >::type* = nullptr >
std::string toStr( const T& val, int mode = ENUM_TOSTR_DEFAULT  )
{
    auto decoded = impl::__toString( val, mode );
    if ( !decoded.valid_ )
    {
        // If no known converter found, return typename
        return ::tsv::debug::demangle( typeid(T).name() );
    }

    return decoded.value_;
}

// Main implementation for pointers
template<typename T>
//    typename std::enable_if< std::is_pointer<T*>::value, int >::type* = nullptr >
std::string toStr( const T* val, int mode = ENUM_TOSTR_DEFAULT  )
{
    if ( !val )
        return "nullptr";

    // If val is not pointer to pointer, then show its content
    if ( !std::is_pointer<T>() )
        return hex_addr(val) + " (" + toStr( *val, mode ) + ")";

    return hex_addr( val );
}

/**      Some basic specializations of handlers     **/
std::string toStr( void* v, int mode = ENUM_TOSTR_DEFAULT );
std::string toStr( std::nullptr_t value, int mode = ENUM_TOSTR_DEFAULT );
std::string toStr( const char* v, int mode = ENUM_TOSTR_DEFAULT );

} // namespace tostr
} // namespace util
} // namespace tsv

#endif
