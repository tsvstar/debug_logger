#ifndef DEBUGWATCH_H_
#define DEBUGWATCH_H_

/*********************************************************************
  Purpose:  Watching access to built-in typed class members
  Author: Taranenko Sergey (tsvstar@gmail.com)
  Date: 06-Aug-2017
  License: BSD. See License.txt
**********************************************************************/

#include "properties_ext.h"
#include "debuglog.h"


// Declare hidden member "RealName" and public "PropertyName". Where Public have watched accessors
// 	OwnerClass - class which own that members
// 	MemberType - type of member
// 	ProperyName - name of public member (which actually call setter/getter which are watchers)
//      RealName    - name of underlying member (which actually store value and is tracked)
//      BacktraceDepth - if <0 then each access logged with full callstack of its context;
//                   if >0 - then limited to this num of frames callstack
//                   if =0 - then no callstack logged
//      ShowValues  - if <0 then logged fact of access
//                   otherwise that is print mode (which is one of ::tsv::util::tostr::ENUM_TOSTR_* values)

#define def_prop_watch( OwnerClass, MemberType, PropertyName, RealName, BacktraceDepth, ShowValues )                    \
  MemberType RealName;                                                                                                  \
  static size_t prop_offset_ ## PropertyName () { return offsetof (OwnerClass, PropertyName); }                         \
  static MemberType const &prop_get_ ## PropertyName (OwnerClass const &self, const char* comment )                     \
      { return ::tsv::debug::Watch_Getter( self, #PropertyName, self.RealName, BacktraceDepth, ShowValues, comment ); } \
  static void prop_set_ ## PropertyName (OwnerClass &self, MemberType const &value, const char* comment )               \
      { ::tsv::debug::Watch_Setter( self, #PropertyName, self.RealName, value, BacktraceDepth, ShowValues, comment );   \
        self.RealName = value; }                                                                                        \
  ::properties_extension::prop<OwnerClass, MemberType, prop_get_ ## PropertyName, prop_set_ ## PropertyName, prop_offset_ ## PropertyName> PropertyName


namespace tsv {
namespace debug {

// Logging write access
//  val            - current value of refered member
//  membername     - its name
//  backTraceDepth - if !=0, then print calltrace to accessor
//  showValues     - if <0 (default), then no value will be displayed
//                   otherwise it have be one of ::tsv::util::tostr::ENUM_TOSTR_* values
//  comment        - context of getter ( nullptr - do not track )
template<typename OwnerClass, typename T>
const T& Watch_Getter( const OwnerClass& self, const char* membername, const T& val, int backTraceDepth = 0, int showValues = -1, const char* comment = "" )
{
    // comment == nullptr,  means do not track
    if ( !comment )
        return val;

    std::string type_name = ::tsv::debug::demangle( typeid(self).name() ).c_str();
    const char* suffix_comment = "";
    if ( comment[0] )
      suffix_comment = ": ";
    else
      comment = "";
    if ( showValues >= 0 )
        SentryLogger::print_event( "%s%sGET %s{0x%p}.%s ( %s )", comment, suffix_comment, type_name.c_str(), &self, membername,
                                        ::tsv::util::tostr::toStr( val, showValues ).c_str() );
    else
        SentryLogger::print_event( "%s%sGET %s{0x%p}.%s", comment, suffix_comment, type_name.c_str(), &self, membername );
    if ( backTraceDepth )
        // print calltrace with excluding three extra frames: this, prop_set, and prop::operator
        SentryLogger::printBackTrace( backTraceDepth + 3, 1 );
    return val;
}

// Logging write access (and change value)
//  existed_val    - reference to real object member with not changed yet value
//  new_val        - new value
//  membername     - its name
//  backTraceDepth - if !=0, then print calltrace to accessor
//  showValues     - if <0 (default), then no value will be displayed
//                   otherwise it have be one of ::tsv::util::tostr::ENUM_TOSTR_* values
//  comment        - context of setter
template<typename OwnerClass, typename T>
T& Watch_Setter( const OwnerClass& self, const char* membername, T& existed_val, const T& new_val, int backTraceDepth = 0, int showValues = -1, const char* comment = "" )
{
    // comment == nullptr,  means do not track
    if ( !comment )
        return existed_val;

    std::string type_name = ::tsv::debug::demangle( typeid(self).name() );
    const char* suffix_comment = "";
    if ( comment[0] )
      suffix_comment = ": ";
    else
      comment = "";
    if ( showValues >= 0 )
        SentryLogger::print_event( "%s%sSET %s{0x%p}.%s ( %s%s%s )", comment, suffix_comment, type_name.c_str(), &self, membername,
                                            ::tsv::util::tostr::toStr( existed_val, showValues ).c_str(),
                                            ((&existed_val==&new_val)? "" : " ==> " ),
                                            ((&existed_val==&new_val)? "" : (::tsv::util::tostr::toStr( new_val, showValues ).c_str()))
                                  );
    else
        SentryLogger::print_event( "%s%sSET %s{0x%p}.%s", comment, suffix_comment, type_name.c_str(), &self, membername );
    if ( backTraceDepth )
        // print calltrace with excluding three extra frames: this, prop_set, and prop::operator
        SentryLogger::printBackTrace( backTraceDepth + 3, 1 );
    return existed_val;
}

}   // end of namespace debug
}   // end of namespace tsv
#endif
