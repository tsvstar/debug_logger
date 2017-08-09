#ifndef DEBUGOBJLOG_H
#define DEBUGOBJLOG_H 1

/*********************************************************************
  Purpose: Tracking objects lifecycle

  Author: Taranenko Sergey (tsvstar@gmail.com)
  Date: 02-Aug-2017
  License: BSD. See License.txt
**********************************************************************/

#include <string>

// Reset #define ObjLogger if it existed (to make correct declaration below)
#undef ObjLogger

namespace tsv {
namespace debug{

/******************************************************************************
  Use this class to log construct/desctruct objects

  HOWTO USE:
   1. Add private member to class
           ObjLogger debug_sentry_;
   2. Add to initalization list of ctor
             ,debug_sentry_( this, "ClassName", backtracedepth, "CommentIfNeeded")
   3. Add to initalization list of copy ctor
             ,debug_sentry_( obj.debug_sentry_ )
   To disable logging of some object class - just replace "this" to nullptr
******************************************************************************/

class ObjLogger
{
    public:
        ObjLogger( void* ptr, const char* className, int depth=0, const char* comment="" );
        ObjLogger( void* ptr, void* copied_from, const char* className, int depth, const char* comment );
        ObjLogger( const ObjLogger& obj );
        ObjLogger( const ObjLogger&& obj );
        ObjLogger& operator=( const ObjLogger& obj );
        //ObjLogger& ObjLogger::operator=( const ObjLogger&& obj );    // not implemented yet

        ~ObjLogger();

        // say that all these objects are equal ( to not influe onto trivial classes )
        bool operator==( const ObjLogger& ) { return true; }
        bool operator<( const ObjLogger& )  { return false; }

        static void printTrackedPtr( const char* className = nullptr );

    public:
        // Settings
        //static bool useNested_s;  // true if you would like to align with SentryLogger output
        static bool includeContextName_s;   // if true, then logging will include current context name

    public:
        int depth_;                 // how many stacktrace records print on

    private:
        const int offs_;            // offset of ObjLogger from begin of owner
        std::string className_;

};

// Stub class which replace real one if OBJ logging is disabled
class ObjLogEmptyClass
{
    public:
        ObjLogEmptyClass ( void* ptr, const char* className, int depth=0, const char* comment="" ) {}
        ObjLogEmptyClass ( void* ptr, void* copied_from, const char* className, int depth, const char* comment ) {}
        static void printTrackedPtr( const char* className = nullptr ) {}
    private:
};

}
}
#endif

/**************************************************************
    YES, that is the end of header guard.
    Section below make possible turn on/off obj logging
    inside of header
**************************************************************/

// Is ObjLogger object real
#ifndef DEBUG_OBJLOG
#define DEBUG_OBJLOG 1
#endif

#undef ObjLogger
#if DEBUG_OBJLOG
#else
// ObjLogging is forbidden
#define ObjLogger ObjLogEmptyClass
#endif
