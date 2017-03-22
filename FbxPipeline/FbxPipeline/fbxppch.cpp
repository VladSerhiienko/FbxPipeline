#include <fbxppch.h>

namespace mathfu
{
    template <>
    bool InRange< vec2 >( vec2 val, vec2 range_start, vec2 range_end ) {
        return InRange( val.x( ), range_start.x( ), range_end.x( ) ) &&
               InRange( val.y( ), range_start.y( ), range_end.y( ) );
    }

    template <>
    bool InRange< vec3 >( vec3 val, vec3 range_start, vec3 range_end ) {
        return InRange( val.x( ), range_start.x( ), range_end.x( ) ) &&
               InRange( val.y( ), range_start.y( ), range_end.y( ) ) &&
               InRange( val.z( ), range_start.z( ), range_end.z( ) );
    }
}