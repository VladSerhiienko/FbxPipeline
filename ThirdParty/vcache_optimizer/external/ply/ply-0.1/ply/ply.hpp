#ifndef PLY_PLY_HPP_INCLUDED
#define PLY_PLY_HPP_INCLUDED

#include <stdint.h>

namespace ply {

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef float float32;
typedef double float64;

template <typename ScalarType>
struct type_traits;

#ifdef PLY_TYPE_TRAITS
#  error
#endif

#define PLY_TYPE_TRAITS(TYPE, NAME, OLD_NAME)\
template <>\
struct type_traits<TYPE>\
{\
  typedef TYPE type;\
  static const char* name() { return NAME; }\
  static const char* old_name() { return OLD_NAME; }\
};

PLY_TYPE_TRAITS(int8, "int8", "char")
PLY_TYPE_TRAITS(int16, "int16", "short")
PLY_TYPE_TRAITS(int32, "int32", "int")
PLY_TYPE_TRAITS(uint8, "uint8", "uchar")
PLY_TYPE_TRAITS(uint16, "uint16", "ushort")
PLY_TYPE_TRAITS(uint32, "uint32", "uint")
PLY_TYPE_TRAITS(float32, "float32", "float")
PLY_TYPE_TRAITS(float64, "float64", "double")

#undef PLY_TYPE_TRAITS

typedef int format_type;
enum format { ascii_format, binary_little_endian_format, binary_big_endian_format };

} // namespace ply

#endif // PLY_PLY_HPP_INCLUDED
