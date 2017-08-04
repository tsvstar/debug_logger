#ifndef DEBUGWATCH_H_
#define DEBUGWATCH_H_

#include "properties_ext.h"
#include "debuglog.h"

namespace tsv {
namespace debug {

// Logging write access
//  val            - current value of refered member
//  membername     - its name
//  backTraceDepth - if !=0, then print calltrace to accessor
//  showValues     - if <0 (default), then no value will be displayed
//                   otherwise it have be one of ::tsv::util::tostr::ENUM_TOSTR_* values
template<typename OwnerClass, typename T>
const T& Watch_Getter( const OwnerClass& self, const char* membername, const T& val, int backTraceDepth = 0, int showValues = -1 )
{
    std::string type_name = ::tsv::debug::demangle( typeid(self).name() ).c_str();
    if ( showValues >= 0 )
        SentryLogger::print_event( "Get %s{0x%p}.%s ( = %s )", type_name.c_str(), &self, membername,
                                        ::tsv::util::tostr::toStr( val, showValues ).c_str() );
    else
        SentryLogger::print_event( "Get %s{0x%p}.%s", type_name.c_str(), &self, membername );
    if ( backTraceDepth )
        SentryLogger::printBackTrace( backTraceDepth, 1 );
    return val;
}

// Logging write access (and change value)
//  existed_val    - reference to real object member with not changed yet value
//  new_val        - new value
//  membername     - its name
//  backTraceDepth - if !=0, then print calltrace to accessor
//  showValues     - if <0 (default), then no value will be displayed
//                   otherwise it have be one of ::tsv::util::tostr::ENUM_TOSTR_* values
template<typename OwnerClass, typename T>
T& Watch_Setter( const OwnerClass& self, const char* membername, T& existed_val, const T& new_val, int backTraceDepth = 0, int showValues = -1 )
{
    std::string type_name = ::tsv::debug::demangle( typeid(self).name() );
    if ( showValues >= 0 )
        SentryLogger::print_event( "Set %s{0x%p}.%s ( %s ==> %s )", type_name.c_str(), &self, membername,
                                            ::tsv::util::tostr::toStr( existed_val, showValues ).c_str(),
                                            ::tsv::util::tostr::toStr( new_val, showValues ).c_str()
                                   );
    else
        SentryLogger::print_event( "Set %s{0x%p}.%s", type_name.c_str(), &self, membername );
    if ( backTraceDepth )
        SentryLogger::printBackTrace( backTraceDepth, 1 );
    return existed_val;
}

}   // end of namespace debug
}   // end of namespace tsv
#endif
