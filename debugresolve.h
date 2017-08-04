#ifndef DEBUG_H_
#define DEBUG_H_ 1

/*********************************************************************
  Purpose:  Resolving symbols and calltrace

  Author: Taranenko Sergey (tsvstar@gmail.com)
  Date: 02-Aug-2017
  License: BSD. See License.txt
**********************************************************************/

#include <vector>
#include <string>

#ifdef __GNUG__
// Macro to get find out address of virtual function which will be actually called
#define WHICH_FUNC_WILL_BE_CALLED ( baseclass, obj_ptr, method ) ( obj_ptr ? (void*)(obj_ptr->*(&baseclass::method)) : nullptr )
#endif

namespace tsv{
namespace debug {

    // Transform type and function names to pretty form
    std::string demangle(const char* name);

    // Resolve pointer "addr" to function name ( if addLineNum, then include "at file:lineno" )
    std::string resolveAddr2Name( void* addr, bool addLineNum = false );

    // Get backtrace
    std::vector<std::string> getBackTrace( int depth = -1, int skip = 0, bool enforce = false );

    // List of system-wide area of visibility settings
    // ( rest of settings are in debug.c )
    namespace settings
    {
        // BackTrace enabled
        extern bool btEnabled;                          // if false, then suppress getBackTrace() output

        bool isStopWord( const std::string& funcname ); // return true if function name match to something in stopword list
    }

}
}

#endif // DEBUG_H_
