#include <fbxppch.h>
#include <fbxpstate.h>
#include <fbxpnorm.h>

using namespace fbxp;
using namespace fb;

template < typename TMathFu, typename TFb >
inline TMathFu Cast( const TFb v ) {
    static_assert( sizeof( TMathFu ) == sizeof( TFb ), "Cannot cast." );
    return TMathFu( reinterpret_cast< const float* >( &v ) );
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
    assert( mathfu::InRange( positionScale, mathfu::vec3( 0.0f ), mathfu::vec3( 1.0f ) ) );

    UIntPack_10_10_10_2 packed;
    packed.q.x = Unorm< 10 >( positionScale.x( ) ).Bits( );
    packed.q.y = Unorm< 10 >( positionScale.y( ) ).Bits( );
    packed.q.z = Unorm< 10 >( positionScale.z( ) ).Bits( );
    packed.q.w = 0;

    return packed.u;
}

uint32_t PackTexcoord_16_16_fixed( const mathfu::vec2 texcoord,
                                   const mathfu::vec2 texcoordMin,
                                   const mathfu::vec2 texcoordMax ) {
    const mathfu::vec2 texcoordSize  = texcoordMax - texcoordMin;
    const mathfu::vec2 texcoordScale = ( texcoord - texcoordMin ) / texcoordSize;
    assert( mathfu::InRange( texcoordScale, mathfu::vec2( 0.0f ), mathfu::vec2( 1.0f ) ) );

    UIntPack_16_16 packed;
    packed.q.x = Unorm< 16 >( texcoordScale.x( ) ).Bits( );
    packed.q.y = Unorm< 16 >( texcoordScale.y( ) ).Bits( );

    return packed.u;
}

uint32_t PackTexcoord_16_16_half( const mathfu::vec2 texcoord,
                                  const mathfu::vec2 texcoordMin,
                                  const mathfu::vec2 texcoordMax ) {
    const bool sOverflowCheck = true;

    UIntPack_16_16 packed;
    packed.q.x = Half< sOverflowCheck >( texcoord.x( ) ).Bits( );
    packed.q.y = Half< sOverflowCheck >( texcoord.y( ) ).Bits( );

    return packed.u;
}

uint32_t PackNormal_10_10_10_2( const mathfu::vec3 normal ) {
    assert( normal.Length( ) <= 1.0f );
    const mathfu::vec3 n = normal * 0.5f + 0.5f;

    UIntPack_10_10_10_2 packed;
    packed.q.x = Unorm< 10 >( n.x( ) ).Bits( );
    packed.q.y = Unorm< 10 >( n.y( ) ).Bits( );
    packed.q.z = Unorm< 10 >( n.z( ) ).Bits( );
    packed.q.w = 0;

    return packed.u;
}

uint32_t PackTangent_10_10_10_2( const mathfu::vec4 tangent ) {
    UIntPack_10_10_10_2 packed;
    packed.u = PackNormal_10_10_10_2( mathfu::vec4::ToType< mathfu::vec3 >( tangent ).Normalized( ) );
    packed.q.w = Unorm< 2 >( tangent.w( ) * 0.5f + 0.5f ).Bits( );
    return packed.u;
}

uint32_t PackNormal_8_8_8_8( const mathfu::vec3 normal ) {
    assert( normal.Length( ) <= 1.0f );

    const mathfu::vec3 n = normal * 0.5f + 0.5f;

    UIntPack_8_8_8_8 packedPosition;
    packedPosition.q.x = Unorm< 8 >( n.x( ) ).Bits( );
    packedPosition.q.y = Unorm< 8 >( n.y( ) ).Bits( );
    packedPosition.q.z = Unorm< 8 >( n.z( ) ).Bits( );
    packedPosition.q.w = 0;

    return packedPosition.u;
}

uint32_t PackTangent_8_8_8_8( const mathfu::vec4 tangent ) {
    UIntPack_8_8_8_8 packed;
    packed.u   = PackNormal_8_8_8_8( mathfu::vec4::ToType< mathfu::vec3 >( tangent ).Normalized( ) );
    packed.q.w = Unorm< 8 >( tangent.w( ) * 0.5f + 0.5f ).Bits( );
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