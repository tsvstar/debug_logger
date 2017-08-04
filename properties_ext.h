#ifndef PROPERTIES_EXT_H
#define PROPERTIES_EXT_H 1

/* Zero-overhead CPP properties (hidden getter/setter) by pippijn
   ( get only "prop", not "prop_value" class )
   http://xinutec.org/~pippijn/home/programming/cpp/properties
*/

namespace properties_extension
{

template<
  typename Class,                  // Class which contain property [Owner]
  typename T,                      // type of property             [int]
  T const& (get) (Class const&),   // static function getter
                                        // * Static member to avoid fragile member-pointers (and optimization of it)
  void (set) (Class&, T const&),   // static function setter
                                        // * Static member to avoid fragile member-pointers (and optimization of it)
  size_t (offset) ()               // static function which returns offset of property in Owner
                                        // * We need to know offset of property to do not calculate this of Owner
                                        //   and in same time we don't know it before instantiation which is impossible
                                        //   with incomplete declaration of this template. So we use outer symbol
                                        // Usually it will be inlined.
>
struct prop
{
    prop() {}

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
    prop& operator   = (T const& rhs) { set( self(), rhs); return *this; }
    prop& operator  += (T const& rhs) { set( self(), get( self() )  + rhs ); return *this; }
    prop& operator  -= (T const& rhs) { set( self(), get( self() )  - rhs ); return *this; }
    prop& operator  *= (T const& rhs) { set( self(), get( self() )  * rhs ); return *this; }
    prop& operator  /= (T const& rhs) { set( self(), get( self() )  / rhs ); return *this; }
    prop& operator  %= (T const& rhs) { set( self(), get( self() )  % rhs ); return *this; }
    prop& operator  ^= (T const& rhs) { set( self(), get( self() )  ^ rhs ); return *this; }
    prop& operator  |= (T const& rhs) { set( self(), get( self() )  | rhs ); return *this; }
    prop& operator  &= (T const& rhs) { set( self(), get( self() )  & rhs ); return *this; }
    prop& operator  <<= (T const& rhs) { set( self(), get( self() ) << rhs ); return *this; }
    prop& operator  >>= (T const& rhs) { set( self(), get( self() ) >> rhs ); return *this; }

    // Accessors
    // operation of cast to T. That is enough for all kind of binary/unary operators
    operator T const& () const    { return get( self() ); }

    T* operator ->()              {  return &const_cast<T&> ( get( self() ) );  }
    T const* operator ->() const  {   return &get( self() ); }
};

#define def_prop(MemberType, OwnerClass, name, get, set)                                  \
  static size_t prop_offset_ ## name () { return offsetof (OwnerClass, name); }           \
  static MemberType const &prop_get_ ## name (OwnerClass const &self) get                 \
  static void prop_set_ ## name (OwnerClass &self, MemberType const &value) set           \
  ::properties_extension::prop<OwnerClass, MemberType, prop_get_ ## name, prop_set_ ## name, prop_offset_ ## name> name

#define def_property( MemberType, OwnerClass, name, get, set)                                      \
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