/*********************************************************************
  Purpose: Tracking objects lifecycle

  Author: Taranenko Sergey (tsvstar@gmail.com)
  Date: 02-Aug-2017
  License: BSD. See License.txt
**********************************************************************/

// Enforce flag to correct declaration and definition
#define DEBUG_OBJLOG 1

#include "objlog.h"
#include "debuglog.h"
#include <unordered_map>
#include <map>
#include <cstdio>   //sprintf

using namespace ::tsv::debug;

//**************************************************************************
//              ObjLogger
//**************************************************************************

namespace
{
    typedef std::unordered_map<const void*, int> UnordMapPtrInt_t;
    // Auxiliary containers, which store info about object allocations
    std::map<std::string, UnordMapPtrInt_t > allObjectsMap;    // ["className"][object_ptr] = counter
    std::map<std::string, int> objectsCounter;                 // ["className"] = counter_s
}
//bool ObjLogger::useNested_s = true;

namespace tsv {
namespace debug {

// Setting
bool ObjLogger::includeContextName_s = true;

namespace
{
    typedef void (*handler_t)( const char* format, ... );
    handler_t getFunc()
    {
        handler_t handler = SentryLogger::print_event;
        if ( ObjLogger::includeContextName_s )
            handler =  SentryLogger::vwrite;
        return handler;
    }
}

ObjLogger::ObjLogger( void* ptr, const char* className, int depth, const char* comment /*=""*/)
    : depth_( depth ), offs_( ptr ? ( reinterpret_cast<char*>(this) - reinterpret_cast<char*>(ptr) ) : -1 ), className_( className )
{
    if ( ptr != nullptr )
    {
        allObjectsMap[className_][ptr]++;
        objectsCounter[className_]++;
        (*getFunc())( "[obj:%s:%d] create %x %s", className, objectsCounter[className], ptr, comment?comment:"");
        if ( depth_ )
            SentryLogger::printBackTrace( depth_, 1 );
    }
}

ObjLogger::ObjLogger( void* ptr, void* copied_from, const char* className, int depth, const char* comment )
    : depth_( depth ), offs_( ptr ? ( reinterpret_cast<char*>(this) - reinterpret_cast<char*>(ptr) ) : -1 ), className_( className )
{
    if ( ptr != nullptr )
    {
        allObjectsMap[className_][ptr]++;
        objectsCounter[className_]++;
        (*getFunc())( "[obj:%s:%d] %x copy_ctor(%x) %s", className, objectsCounter[className_], ptr, copied_from, comment?comment:"");
        if ( depth_ )
            SentryLogger::printBackTrace( depth_, 1 );
    }
}

ObjLogger::ObjLogger( const ObjLogger& obj ) :
        depth_( obj.depth_ ), offs_( obj.offs_ ), className_( obj.className_ )
{
    if ( offs_ > 0 )
    {
        const void* ptr = reinterpret_cast<const char*>(this) - offs_;
        const void* copied_from = reinterpret_cast<const char*>(&obj) - offs_;

        allObjectsMap[className_][ptr]++;
        objectsCounter[className_]++;
        (*getFunc())( "[obj:%s:%d] %x copy_ctor_dflt(%x)", className_.c_str(), objectsCounter[className_], ptr, copied_from );
        if ( depth_ )
            SentryLogger::printBackTrace( depth_, 1 );
    }
}

ObjLogger::ObjLogger( const ObjLogger&& obj ) :
        depth_( obj.depth_ ), offs_( obj.offs_ ), className_( obj.className_ )
{
    if ( offs_ > 0 )
    {
        const void* ptr = reinterpret_cast<const char*>(this) - offs_;
        const void* copied_from = reinterpret_cast<const char*>(&obj) - offs_;

        allObjectsMap[className_][ptr]++;
        objectsCounter[className_]++;
        auto pprint = getFunc();
        (*pprint)( "[obj:%s:%d] %x copy_ctor_move(%x)", className_.c_str(), objectsCounter[className_], ptr, copied_from );
        (*pprint)( "[obj:%s:%d] %x become unitialized", className_.c_str(), objectsCounter[className_], copied_from );
        if ( depth_ )
            SentryLogger::printBackTrace( depth_, 1 );
    }
}


ObjLogger& ObjLogger::operator=( const ObjLogger& obj )
{
    if ( const_cast<const ObjLogger*>(this) == &obj )
        return *this;

    const void* ptr = ( offs_ > 0 ) ? reinterpret_cast<const char*>(this) - offs_ : nullptr;
    const void* copied_from = ( offs_ > 0 ) ? reinterpret_cast<const char*>(&obj) - obj.offs_ : nullptr;
    if ( offs_!= obj.offs_ || className_ != obj.className_ )
    {
      (*getFunc())( "[obj] INCONSISTENCE operator=() -> className=%s|%s offs=%d|%d", className_.c_str(), obj.className_.c_str(), offs_, obj.offs_ );
    }
    else
    {
      (*getFunc())( "[obj:%s:%d] %x operator=(%x)", className_.data(), objectsCounter[className_], ptr, copied_from );
      // depth could be different
      depth_ = obj.depth_;
    }

    if ( depth_ )
        SentryLogger::printBackTrace( depth_, 1 );

    // Do nothing real work because both sides are exists (and so registered)

    return *this;
}



ObjLogger::~ObjLogger()
{
    if ( offs_ > 0 )
    {
        void* objPtr = reinterpret_cast<char*>( this ) - offs_ ;
        UnordMapPtrInt_t& obj = allObjectsMap[ className_ ];
        UnordMapPtrInt_t::iterator objCntr = obj.find( objPtr );
        int& classCntr = objectsCounter[className_];

        if ( objCntr == obj.end() )
            (*getFunc())( "[obj:%s:%d] destroy %x. ERROR: not registered pointer", className_.c_str(), classCntr, objPtr );
        else
        {
            classCntr--;
            (*getFunc())( "[obj:%s:%d] destroy %x", className_.c_str(), classCntr, objPtr );
            if ( objCntr->second > 1 )
                objCntr->second--;
            else
                obj.erase( objCntr );
        }

        if ( depth_ )
            SentryLogger::printBackTrace( depth_, 1 );
    }
}

// Print all or exact class pointers
//      className    = if nullptr print all tracked pointers
//                     otherwise only tracked pointers of given class
void ObjLogger::printTrackedPtr( const char* className /*= nullptr */ )
{
    if ( !className )
    {
        (*getFunc())( " [obj] Print all active pointers" );
        for ( auto& tmp : allObjectsMap )
            printTrackedPtr( tmp.first.c_str() );
        return;
    }

    auto it = allObjectsMap.find( className );
    if ( it == allObjectsMap.end() )
    {
        (*getFunc())( " [obj] Print %s: not found such class", className );
        return;
    }

    std::string output;
    int cntr = 0;
    auto& ptrMap = it->second;
    for ( const auto& ptr : ptrMap )
    {
        static char buf[40];
        if ( !ptr.second )
          continue;
        if ( ptr.second==1 )
            sprintf( buf, "%s%p", cntr?",":"", ptr.first );
        else
            sprintf( buf, "%s%p(%d)", cntr?",":"", ptr.first, ptr.second );
        output += buf;
        cntr++;
    }

    (*getFunc())( " [obj] Print %s (%d objects): %s", className, cntr, output.c_str() );
}


}   // namespace debug
}   // namespace tsv
