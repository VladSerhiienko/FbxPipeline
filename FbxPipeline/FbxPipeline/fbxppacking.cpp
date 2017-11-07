#include <fbxppch.h>
#include <fbxpstate.h>
#include <fbxpnorm.h>

using namespace apemode;
using namespace apemodefb;

template < typename X, typename Y >
inline X Cast( const Y v ) {
    static_assert( sizeof( X ) == sizeof( Y ), "Cannot cast." );
    return X( reinterpret_cast< const float* >( &v ) );
}

template <>
inline apemodefb::vec4 Cast( const mathfu::vec4 v ) {
    return apemodefb::vec4( v.x, v.y, v.z, v.w );
}

template <>
inline apemodefb::vec3 Cast( const mathfu::vec3 v ) {
    return apemodefb::vec3( v.x, v.y, v.z );
}

template <>
inline apemodefb::vec2 Cast( const mathfu::vec2 v ) {
    return apemodefb::vec2( v.x, v.y );
}

template <>
inline mathfu::vec4 Cast( const apemodefb::vec4 v ) {
    return mathfu::vec4( v.x( ), v.y( ), v.z( ), v.w( ) );
}

template <>
inline mathfu::vec3 Cast( const apemodefb::vec3 v ) {
    return mathfu::vec3( v.x( ), v.y( ), v.z( ) );
}

template <>
inline mathfu::vec2 Cast( const apemodefb::vec2 v ) {
    return mathfu::vec2( v.x( ), v.y( ) );
}

template < typename TMathFu >
inline TMathFu Map( const TMathFu input,
                    const TMathFu input_start,
                    const TMathFu input_end,
                    const TMathFu output_start,
                    const TMathFu output_end ) {
    const TMathFu input_range  = input_end - input_start;
    const TMathFu output_range = output_end - output_start;
    return ( input - input_start ) * output_range / input_range + output_start;
}

inline bool IsNearlyZero( float a, float tolerance ) {
    return abs( a ) <= tolerance;
}

inline bool IsNearlyEqual( float a, float b, float tolerance ) {
    return IsNearlyZero( a - b, tolerance );
}

template < typename TMathFu >
inline void AssertInRange( const TMathFu v, float vmin = 0.0f, float vmax = 1.0f, float tolerance = 0.0001f ) {
    /*float values[ sizeof( TMathFu ) / sizeof( float ) ];
    static_assert( sizeof( values ) == sizeof( v ), "Cannot copy." );
    memcpy( values, &v, sizeof( values ) );

    for ( auto value : values ) {
        assert( value >= ( vmin - tolerance ) && value <= ( vmax + tolerance ) );
    }*/
}

struct PackedVertex {
    uint32_t position;
    uint32_t normal;
    uint32_t tangent;
    uint32_t texcoords;
};

union UIntPack_16_16 {
    uint32_t u;
    struct {
        uint32_t x : 16;
        uint32_t y : 16;
    } q;
};

union UIntPack_10_10_10_2 {
    uint32_t u;
    struct {
        uint32_t x : 10;
        uint32_t y : 10;
        uint32_t z : 10;
        uint32_t w : 2;
    } q;
};

union UIntPack_8_8_8_8 {
    uint32_t u;
    struct {
        uint32_t x : 8;
        uint32_t y : 8;
        uint32_t z : 8;
        uint32_t w : 8;
    } q;
};

static_assert( sizeof( Half< true > ) == sizeof( uint16_t ), "Must match" );
static_assert( sizeof( Half< false > ) == sizeof( uint16_t ), "Must match" );
static_assert( sizeof( UIntPack_8_8_8_8 ) == sizeof( uint32_t ), "Must match" );
static_assert( sizeof( UIntPack_10_10_10_2 ) == sizeof( uint32_t ), "Must match" );
static_assert( sizeof( UIntPack_16_16 ) == sizeof( uint32_t ), "Must match" );

uint32_t PackPosition_10_10_10_2( const mathfu::vec3 position,
                                  const mathfu::vec3 positionMin,
                                  const mathfu::vec3 positionMax ) {
    const mathfu::vec3 positionSize  = positionMax - positionMin;
    const mathfu::vec3 positionScale = ( position - positionMin ) / positionSize;
    AssertInRange( positionScale );

    UIntPack_10_10_10_2 packed;
    packed.q.w = 0;

    /*

    Matches GL_UNSIGNED_INT_2_10_10_10_REV format + GL_RGBA token (OpenGL):
    10-bit R component in bits   0..9
    10-bit G component in bits 10..19
    10-bit B component in bits 20..29
    2-bit  A component in bits 30..31

    https://stackoverflow.com/questions/17726676/what-does-rev-suffix-mean-in-opengl

    */

    packed.q.x = Unorm< 10 >( positionScale.x ).Bits( );
    packed.q.y = Unorm< 10 >( positionScale.y ).Bits( );
    packed.q.z = Unorm< 10 >( positionScale.z ).Bits( );

    /*

    Matches VK_FORMAT_A2R10G10B10_UNORM_PACK32 format (Vulkan):
    10-bit B component in bits   0..9
    10-bit G component in bits 10..19
    10-bit R component in bits 20..29
    2-bit  A component in bits 30..31

    https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkFormat.html

    packed.q.z = Unorm< 10 >( positionScale.x ).Bits( );
    packed.q.y = Unorm< 10 >( positionScale.y ).Bits( );
    packed.q.x = Unorm< 10 >( positionScale.z ).Bits( );

    */

    return packed.u;
}

uint32_t PackTexcoord_16_16_fixed( const mathfu::vec2 texcoord,
                                   const mathfu::vec2 texcoordMin,
                                   const mathfu::vec2 texcoordMax ) {
    const mathfu::vec2 texcoordSize  = texcoordMax - texcoordMin;
    const mathfu::vec2 texcoordScale = ( texcoord - texcoordMin ) / texcoordSize;
    AssertInRange( texcoordScale );

    UIntPack_16_16 packed;
    packed.q.x = Unorm< 16 >( texcoordScale.x ).Bits( );
    packed.q.y = Unorm< 16 >( texcoordScale.y ).Bits( );

    return packed.u;
}

uint32_t PackTexcoord_16_16_half( const mathfu::vec2 texcoord,
                                  const mathfu::vec2 texcoordMin,
                                  const mathfu::vec2 texcoordMax ) {
    const bool sOverflowCheck = 1;
    // const bool sOverflowCheck = FBXP_DEBUG;

    UIntPack_16_16 packed;
    packed.q.x = Half< sOverflowCheck >( texcoord.x ).Bits( );
    packed.q.y = Half< sOverflowCheck >( texcoord.y ).Bits( );

    return packed.u;
}

uint32_t PackNormal_10_10_10_2( const mathfu::vec3 normal ) {
    AssertInRange( normal.Length( ) );

    const mathfu::vec3 n = normal * 0.5f + 0.5f;
    AssertInRange( n );

    UIntPack_10_10_10_2 packed;
    packed.q.x = Unorm< 10 >( n.x ).Bits( );
    packed.q.y = Unorm< 10 >( n.y ).Bits( );
    packed.q.z = Unorm< 10 >( n.z ).Bits( );
    packed.q.w = 0;

    return packed.u;
}

uint32_t PackTangent_10_10_10_2( const mathfu::vec4 tangent ) {
    AssertInRange( tangent.w, -1.f, +1.f );
    UIntPack_10_10_10_2 packed;
    packed.u = PackNormal_10_10_10_2( mathfu::vec3( tangent.x, tangent.y, tangent.z ).Normalized( ) );
    packed.q.w = Unorm< 2 >( tangent.w * 0.5f + 0.5f ).Bits( );
    return packed.u;
}

uint32_t PackNormal_8_8_8_8( const mathfu::vec3 normal ) {
    AssertInRange( normal.Length( ) );

    const mathfu::vec3 n = normal * 0.5f + 0.5f;
    AssertInRange( n );

    UIntPack_8_8_8_8 packedPosition;
    packedPosition.q.x = Unorm< 8 >( n.x ).Bits( );
    packedPosition.q.y = Unorm< 8 >( n.y ).Bits( );
    packedPosition.q.z = Unorm< 8 >( n.z ).Bits( );
    packedPosition.q.w = 0;

    return packedPosition.u;
}

uint32_t PackBoneWeights_10_10_10_2( const mathfu::vec4 boneWeights ) {
    UIntPack_10_10_10_2 packed;

    /* We know the sum is exactly one, so no need to save forth weight (if any) since
       weight[3] == 1 - (weight[2] + weight[1] + weight[0]).
       So we can increase precision here to 10/10/10/2 instead of 8/8/8/8
       at the cost of additional calculation in vertex shaders.
       */

    packed.q.x = Unorm< 10 >( boneWeights.x ).Bits( );
    packed.q.y = Unorm< 10 >( boneWeights.y ).Bits( );
    packed.q.z = Unorm< 10 >( boneWeights.z ).Bits( );
    packed.q.w = 0;

    return packed.u;
}

mathfu::vec4 UnpackBoneWeights_10_10_10_2( const uint32_t boneWeights ) {
    UIntPack_10_10_10_2 packed;
    packed.u = boneWeights;

    mathfu::vec4 unpacked;
    unpacked.x = Unorm< 10 >::FromBits( packed.q.x );
    unpacked.y = Unorm< 10 >::FromBits( packed.q.y );
    unpacked.z = Unorm< 10 >::FromBits( packed.q.z );
    unpacked.w = 1.0f - unpacked.x - unpacked.y - unpacked.z;
    return unpacked;
}


uint32_t PackBoneIndices_8_8_8_8( const mathfu::vec4 boneIndices ) {
    const auto maxIndex = (float)Unorm< 8 >::sMax;

    UIntPack_8_8_8_8 packedPosition;

    const auto normIndices = boneIndices / maxIndex;

    packedPosition.q.x = Unorm< 8 >( normIndices.x ).Bits( );
    packedPosition.q.y = Unorm< 8 >( normIndices.y ).Bits( );
    packedPosition.q.z = Unorm< 8 >( normIndices.z ).Bits( );
    packedPosition.q.w = Unorm< 8 >( normIndices.w ).Bits( );

    return packedPosition.u;
}

mathfu::vec4 UnpackBoneIndices_8_8_8_8( const uint32_t boneIndices ) {
    const auto maxIndex = (float)Unorm< 8 >::sMax;

    UIntPack_8_8_8_8 packed;
    packed.u = boneIndices;

    mathfu::vec4 unpacked;

    unpacked.x = (float) (int) ( Unorm< 8 >::FromBits( packed.q.x ) * maxIndex );
    unpacked.y = (float) (int) ( Unorm< 8 >::FromBits( packed.q.y ) * maxIndex );
    unpacked.z = (float) (int) ( Unorm< 8 >::FromBits( packed.q.z ) * maxIndex );
    unpacked.w = (float) (int) ( Unorm< 8 >::FromBits( packed.q.w ) * maxIndex );

    return unpacked;
}

uint32_t PackTangent_8_8_8_8( const mathfu::vec4 tangent ) {
    AssertInRange( tangent.w, -1.f, +1.f );
    UIntPack_8_8_8_8 packed;
    packed.u = PackNormal_8_8_8_8( mathfu::vec3( tangent.x, tangent.y, tangent.z ).Normalized( ) );
    packed.q.w = Unorm< 8 >( tangent.w * 0.5f + 0.5f ).Bits( );
    return packed.u;
}

void Pack( const StaticVertexFb* vertices,
           PackedVertexFb*       packed,
           const uint32_t        vertexCount,
           const mathfu::vec3    positionMin,
           const mathfu::vec3    positionMax,
           const mathfu::vec2    texcoordsMin,
           const mathfu::vec2    texcoordsMax ) {

    for ( uint32_t i = 0; i < vertexCount; ++i ) {

        const auto position  = Cast< mathfu::vec3 >( vertices[ i ].position( ) );
        const auto texcoords = Cast< mathfu::vec2 >( vertices[ i ].uv( ) );
        const auto normal    = Cast< mathfu::vec3 >( vertices[ i ].normal( ) );
        const auto tangent   = Cast< mathfu::vec4 >( vertices[ i ].tangent( ) );

        packed[ i ] = PackedVertexFb( PackPosition_10_10_10_2( position, positionMin, positionMax ),
                                      PackNormal_10_10_10_2( normal.Normalized( ) ),
                                      PackTangent_10_10_10_2( tangent ),
                                      PackTexcoord_16_16_fixed( texcoords, texcoordsMin, texcoordsMax ) );
    }
}

uint32_t PackedMaxBoneCount( ) {
    return (uint32_t) Unorm< 8 >::sMax;
}

void Pack( const apemodefb::StaticSkinnedVertexFb* vertices,
           apemodefb::PackedSkinnedVertexFb*       packed,
           const uint32_t                          vertexCount,
           const mathfu::vec3                      positionMin,
           const mathfu::vec3                      positionMax,
           const mathfu::vec2                      texcoordsMin,
           const mathfu::vec2                      texcoordsMax ) {

    for ( uint32_t i = 0; i < vertexCount; ++i ) {
        const auto position  = Cast< mathfu::vec3 >( vertices[ i ].position( ) );
        const auto texcoords = Cast< mathfu::vec2 >( vertices[ i ].uv( ) );
        const auto normal    = Cast< mathfu::vec3 >( vertices[ i ].normal( ) );
        const auto tangent   = Cast< mathfu::vec4 >( vertices[ i ].tangent( ) );
        const auto weights   = Cast< mathfu::vec4 >( vertices[ i ].weights( ) );
        const auto indices   = Cast< mathfu::vec4 >( vertices[ i ].indices( ) );

        packed[ i ] = PackedSkinnedVertexFb( PackPosition_10_10_10_2( position, positionMin, positionMax ),
                                             PackNormal_10_10_10_2( normal.Normalized( ) ),
                                             PackTangent_10_10_10_2( tangent ),
                                             PackTexcoord_16_16_fixed( texcoords, texcoordsMin, texcoordsMax ),
                                             PackBoneWeights_10_10_10_2( weights ),
                                             PackBoneIndices_8_8_8_8( indices ) );

#if 0
        const auto unpackedWeights = UnpackBoneWeights_10_10_10_2( packed[ i ].weights( ) );
        const auto unpackedIndices = UnpackBoneIndices_8_8_8_8( packed[ i ].indices( ) );

        assert( IsNearlyEqual( weights.x, unpackedWeights.x, 1e-3f ) );
        assert( IsNearlyEqual( weights.y, unpackedWeights.y, 1e-3f ) );
        assert( IsNearlyEqual( weights.z, unpackedWeights.z, 1e-3f ) );
        assert( IsNearlyEqual( weights.w, unpackedWeights.w, 1e-3f ) );

        assert( IsNearlyEqual( indices.x, unpackedIndices.x, 1e-9f ) );
        assert( IsNearlyEqual( indices.y, unpackedIndices.y, 1e-9f ) );
        assert( IsNearlyEqual( indices.z, unpackedIndices.z, 1e-9f ) );
        assert( IsNearlyEqual( indices.w, unpackedIndices.w, 1e-9f ) );
#endif

    }
}