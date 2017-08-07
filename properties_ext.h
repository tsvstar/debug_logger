#ifndef PROPERTIES_EXT_H
#define PROPERTIES_EXT_H 1

/* Zero-overhead CPP properties (hidden getter/setter) by pippijn
   ( get only "prop" class and modify it )
   http://xinutec.org/~pippijn/home/programming/cpp/properties
*/

namespace properties_extension
{

template<
  typename Class,                  // Class which contain property [Owner]
  typename T,                      // type of property             [int]
  T const& (get)( Class const&, const char* comment ),   // static function getter
                                        // * Static member to avoid fragile member-pointers (and optimization of it)
  void (set)( Class&, T const&, const char* comment ),   // static function setter
                                        // * Static member to avoid fragile member-pointers (and optimization of it)
  size_t (offset) ()               // static function which returns offset of property in Owner
                                        // * We need to know offset of property to do not calculate this of Owner
                                        //   and in same time we don't know it before instantiation which is impossible
                                        //   with incomplete declaration of this template. So we use outer symbol
                                        // Usually it will be inlined.
>
struct prop
{
    prop() { set( self(), get( self(), nullptr ), "default ctor" ); }
    prop( const T& rhs ) { set( self(), rhs, "value ctor" );}

    // Calculate "this" of Owner
    Class& self()
    {
      return *reinterpret_cast<Class *> (reinterpret_cast<char *> (this)
                                         - offset ());
    }

    // Calculate "this" of Owner
    Class const& self() const
    {
      return *reinterpret_cast<Class const *> (reinterpret_cast<char const *> (this)
                                               - offset ());
    }

    // Mutators
    // All kind of assignment operators ()
    prop& operator   = (T const& rhs) { set( self(), rhs, "op=" ); return *this; }
    prop& operator  += (T const& rhs) { set( self(), get( self(), "op+=" )  + rhs, "op+=" ); return *this; }
    prop& operator  -= (T const& rhs) { set( self(), get( self(), "op-=" )  - rhs, "op-=" ); return *this; }
    prop& operator  *= (T const& rhs) { set( self(), get( self(), "op*=" )  * rhs, "op*=" ); return *this; }
    prop& operator  /= (T const& rhs) { set( self(), get( self(), "op/=" )  / rhs, "op/=" ); return *this; }
    prop& operator  %= (T const& rhs) { set( self(), get( self(), "op%=" )  % rhs, "op%=" ); return *this; }
    prop& operator  ^= (T const& rhs) { set( self(), get( self(), "op^=" )  ^ rhs, "op^=" ); return *this; }
    prop& operator  |= (T const& rhs) { set( self(), get( self(), "op|=" )  | rhs, "op|=" ); return *this; }
    prop& operator  &= (T const& rhs) { set( self(), get( self(), "op&=" )  & rhs, "op&=" ); return *this; }
    prop& operator  <<= (T const& rhs) { set( self(), get( self(), "op<<=" ) << rhs, "op<<=" ); return *this; }
    prop& operator  >>= (T const& rhs) { set( self(), get( self(), "op>>=" ) >> rhs, "op>>=" ); return *this; }

    // Accessors
    // operation of cast to T. That is enough for all kind of binary/unary operators
    operator T const& () const    { return get( self(), "cast" ); }

    T* operator ->()              {  return &const_cast<T&> ( get( self(), "op->" ) );  }
    T const* operator ->() const  {   return &get( self(), "op->" ); }
};

#define def_prop( OwnerClass, MemberType, name, get, set)                                                   \
  static size_t prop_offset_ ## name () { return offsetof (OwnerClass, name); }                             \
  static MemberType const &prop_get_ ## name (OwnerClass const &self, const char* comment ) get             \
  static void prop_set_ ## name (OwnerClass &self, MemberType const &value, const char* comment ) set       \
  ::properties_extension::prop<OwnerClass, MemberType, prop_get_ ## name, prop_set_ ## name, prop_offset_ ## name> name

#define def_property( OwnerClass, MemberType, name, get, set)                             \
  static size_t prop_offset_ ## name () { return offsetof (OwnerClass, name); }           \
  ::properties_extension::prop<OwnerClass, MemberType, get, set, prop_offset_ ## name> name


/*
  //USAGE EXAMPLE
struct point
{

  int y_;
  def_prop (int, point, y,
    {
      return self.y_;
    },
    {
      self.y_ = value;
    }
  );
};
*/

}

#endif
