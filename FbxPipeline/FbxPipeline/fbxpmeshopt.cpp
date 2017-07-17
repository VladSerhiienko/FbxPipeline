#include <fbxppch.h>
#include <fbxpstate.h>

#pragma warning( push )
#pragma warning( disable : 4244 ) // int64 to int32 conversion
#include <vcache_optimizer.hpp>
#pragma warning( pop )

#include <meshoptimizer.hpp>

using namespace apemode;
using namespace apemodefb;

template < typename TIndex >
struct VcacheTriangle {
    TIndex indices[ 3 ];

    inline VcacheTriangle( ) {
    }

    inline VcacheTriangle( TIndex const i1, TIndex const i2, TIndex const i3 ) {
        indices[ 0 ] = i1;
        indices[ 1 ] = i2;
        indices[ 2 ] = i3;
    }

    inline TIndex operator[]( uint32_t const index ) const {
        assert( index < 3 );
        return indices[ index ];
    }
};

using Vertex = apemodefb::StaticVertexFb;

template < typename TIndex >
struct VcacheMesh {
    apemode::Mesh* m = nullptr;
};

namespace vcache_optimizer {
    template < typename TIndex >
    struct mesh_traits< VcacheMesh< TIndex > > {
        typedef uint32_t                 submesh_id_t;
        typedef TIndex                   vertex_index_t;
        typedef TIndex                   triangle_index_t;
        typedef Vertex                   vertex_t;
        typedef VcacheTriangle< TIndex > triangle_t;
    };
}

#pragma region Optimization

VcacheTriangle< uint16_t > create_new_triangle( VcacheMesh< uint16_t >& m, uint16_t const i1, uint16_t const i2, uint16_t const i3 ) {
    return VcacheTriangle< uint16_t >( i1, i2, i3 );
}

std::size_t get_num_triangles( VcacheMesh< uint16_t > const& m, uint32_t const& sm ) {
    return m.m->subsets[ sm ].index_count( ) / 3;
}

std::size_t get_num_vertices( VcacheMesh< uint16_t > const& m, uint32_t const& sm ) {
    return m.m->vertices.size( ) / sizeof( Vertex );
}

VcacheTriangle< uint16_t > get_triangle( VcacheMesh< uint16_t > const& m, uint32_t const& sm, uint16_t& index ) {
    uint16_t* indices = reinterpret_cast< uint16_t* >( m.m->indices.data( ) );
    const uint32_t i1 = indices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 0 ];
    const uint32_t i2 = indices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 1 ];
    const uint32_t i3 = indices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 2 ];
    return VcacheTriangle< uint16_t >( i1, i2, i3 );
}

Vertex get_vertex( VcacheMesh< uint16_t > const& m, uint32_t const& sm, uint16_t& index ) {
    // uint16_t* indices = reinterpret_cast< uint16_t* >( m.m->indices.data( ) );
    // const uint16_t i = indices[ index ];
    // const uint16_t i = indices[ m.m->subsets[ sm ].base_index( ) + index ];
    // return *(Vertex*) ( m.m->vertices.data( ) + i * sizeof( Vertex ) );
    return *(Vertex*) ( m.m->vertices.data( ) + index * sizeof( Vertex ) );
}

void set_triangle( VcacheMesh< uint16_t >& m, uint32_t const& sm, uint16_t& index, VcacheTriangle< uint16_t > const& new_triangle ) {
    uint16_t* indices = reinterpret_cast< uint16_t* >( m.m->indices.data( ) );
    indices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 0 ] = new_triangle[ 0 ];
    indices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 1 ] = new_triangle[ 1 ];
    indices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 2 ] = new_triangle[ 2 ];
}

void set_vertex( VcacheMesh< uint16_t >& m, uint32_t const& sm, uint16_t& index, Vertex const& new_vertex ) {
    // uint16_t* indices = reinterpret_cast< uint16_t* >( m.m->indices.data( ) );
    // const uint16_t i = indices[ index ];
    // const uint16_t i = indices[ m.m->subsets[ sm ].base_index( ) + index ];
    // *(Vertex*) ( m.m->vertices.data( ) + i * sizeof( Vertex ) ) = new_vertex;
    *(Vertex*) ( m.m->vertices.data( ) + index * sizeof( Vertex ) ) = new_vertex;
}

VcacheTriangle< uint32_t > create_new_triangle( VcacheMesh< uint32_t >& m, uint32_t const i1, uint32_t const i2, uint32_t const i3 ) {
    return VcacheTriangle< uint32_t >( i1, i2, i3 );
}

std::size_t get_num_triangles( VcacheMesh< uint32_t > const& m, uint32_t const& sm ) {
    return m.m->subsets[ sm ].index_count( ) / 3;
}

std::size_t get_num_vertices( VcacheMesh< uint32_t > const& m, uint32_t const& sm ) {
    return m.m->vertices.size( ) / sizeof( Vertex );
    // return m.m->subsets[ sm ].index_count( );
}

VcacheTriangle< uint32_t > get_triangle( VcacheMesh< uint32_t > const& m, uint32_t const& sm, uint32_t& index ) {
    uint32_t* indices = reinterpret_cast< uint32_t* >( m.m->indices.data( ) );
    const uint32_t i1 = indices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 0 ];
    const uint32_t i2 = indices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 1 ];
    const uint32_t i3 = indices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 2 ];
    return VcacheTriangle< uint32_t >( i1, i2, i3 );
}

Vertex get_vertex( VcacheMesh< uint32_t > const& m, uint32_t const& sm, uint32_t& index ) {
    // uint32_t* indices = reinterpret_cast< uint32_t* >( m.m->indices.data( ) );
    // const uint32_t i = indices[ index ];
    // const uint32_t i = indices[ m.m->subsets[ sm ].base_index( ) + index ];
    // return *(Vertex*) ( m.m->vertices.data( ) + i * sizeof( Vertex ) );
    return *(Vertex*) ( m.m->vertices.data( ) + index * sizeof( Vertex ) );
}

void set_triangle( VcacheMesh< uint32_t >& m, uint32_t const& sm, uint32_t& index, VcacheTriangle< uint32_t > const& new_triangle ) {
    uint32_t* indices = reinterpret_cast< uint32_t* >( m.m->indices.data( ) );
    indices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 0 ] = new_triangle[ 0 ];
    indices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 1 ] = new_triangle[ 1 ];
    indices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 2 ] = new_triangle[ 2 ];
}

void set_vertex( VcacheMesh< uint32_t >& m, uint32_t const& sm, uint32_t& index, Vertex const& new_vertex ) {
    // uint32_t* indices = reinterpret_cast< uint32_t* >( m.m->indices.data( ) );
    // const uint32_t i = m.m->indices[ index ];
    // const uint32_t i = m.m->indices[ m.m->subsets[ sm ].base_index( ) + index ];
    // *(Vertex*) ( m.m->vertices.data( ) + i * sizeof( Vertex ) ) = new_vertex;
    *(Vertex*) ( m.m->vertices.data( ) + index * sizeof( Vertex ) ) = new_vertex;
}

#pragma endregion

const bool bUseVcacheOptimizer = false;

template < typename TIndex >
void GenerateSubset( apemode::Mesh& m, uint32_t& vertexCount, uint32_t vertexStride ) {
    if ( false == m.subsets.empty( ) )
        return;

    std::vector< uint32_t > indexBuffer;
    indexBuffer.resize( vertexCount );

    const uint32_t vc = (uint32_t) generateIndexBuffer( indexBuffer.data( ), m.vertices.data( ), vertexCount, vertexStride );

    std::vector< uint8_t > vertexBuffer;
    vertexBuffer.resize( vc * vertexStride );
    generateVertexBuffer( vertexBuffer.data( ), indexBuffer.data( ), m.vertices.data( ), vertexCount, vertexStride );

    m.vertices.swap( vertexBuffer );
    vertexCount = vc;

    m.indices.resize( sizeof( TIndex ) * vertexCount );
    auto indices = reinterpret_cast< TIndex* >( m.indices.data( ) );
    for ( uint32_t i = 0; i < vc; ++i ) {
        indices[ i ] = (TIndex) indexBuffer[ i ];
    }

    m.subsets.emplace_back( (uint32_t) 0, (uint32_t) 0, (uint32_t) vc );
}

template < typename TIndex >
void OptimizeSubsetVcache( apemode::Mesh& mesh, uint32_t subsetIndex ) {
    VcacheMesh< TIndex > meshWrapper;
    meshWrapper.m = &mesh;

    vcache_optimizer::vcache_optimizer< VcacheMesh< TIndex > > optimizer;
    optimizer( meshWrapper, subsetIndex, mesh.subsets.size( ) == 1 );
}

template < typename TIndex >
void OptimizeSubset(apemode::Mesh& m, const Vertex * vertices, uint32_t& vertexCount, uint32_t vertexStride, uint32_t ss ) {
    const uint32_t kCacheSize = 16;

    std::vector< uint8_t > indexBuffer;
    indexBuffer.resize( m.subsets[ ss ].index_count( ) * sizeof( TIndex ) );

#if 0

    std::vector< uint32_t > clusters;
    optimizePostTransform( reinterpret_cast< TIndex* >( indexBuffer.data( ) ),
                           reinterpret_cast< TIndex* >( m.indices.data( ) ) + m.subsets[ ss ].base_index( ),
                           m.subsets[ ss ].index_count( ),
                           vertexCount,
                           kCacheSize,
                           &clusters );

    memcpy( reinterpret_cast< TIndex* >( m.indices.data( ) ) + m.subsets[ ss ].base_index( ),
            indexBuffer.data( ),
            m.subsets[ ss ].index_count( ) * sizeof( TIndex ) );

    optimizeOverdraw( reinterpret_cast< TIndex* >( indexBuffer.data( ) ),
                      reinterpret_cast< TIndex* >( m.indices.data( ) ) + m.subsets[ ss ].base_index( ),
                      m.subsets[ ss ].index_count( ),
                      vertices,
                      vertexStride,
                      vertexCount,
                      clusters,
                      kCacheSize,
                      1.05f );

    memcpy( reinterpret_cast< TIndex* >( m.indices.data( ) ) + m.subsets[ ss ].base_index( ),
            indexBuffer.data( ),
            m.subsets[ ss ].index_count( ) * sizeof( TIndex ) );

#else

    optimizePostTransform( reinterpret_cast< TIndex* >( indexBuffer.data( ) ),
                           reinterpret_cast< TIndex* >( m.indices.data( ) ) + m.subsets[ ss ].base_index( ),
                           m.subsets[ ss ].index_count( ),
                           vertexCount,
                           kCacheSize );

#endif
}

template < typename TIndex >
void Optimize( apemode::Mesh& m, const Vertex * vertices, uint32_t& vertexCount, uint32_t vertexStride ) {

    auto& s = apemode::Get( );

    VcacheMesh< TIndex > mm;
    mm.m = &m;

    if ( m.subsets.empty( ) ) {
        GenerateSubset< TIndex >( m, vertexCount, vertexStride );
        // OptimizeSubset< TIndex >( m, vertices, vertexCount, vertexStride, 0 );
    }

    for ( uint32_t ss = 0; ss < m.subsets.size( ); ++ss ) {
        // OptimizeSubset< TIndex >( m, vertices, vertexCount, vertexStride, ss );
        OptimizeSubsetVcache< TIndex >( m, ss );
    }
}

// F:\Dev\Projects\ProjectFbxPipeline\ThirdParty\meshoptimizer\demo\bunny.obj
// E:\Media\Models\Mercedes+Benz+A45+AMG.FBX
// E:\Media\Models\m4a1-sopmod-overkill\source\M4A1 SOPMOD Overkill HIGH POLY.obj
// E:\Media\Models\mech-m-6k\source\93d43cf18ad5406ba0176c9fae7d4927.fbx

void Optimize32( apemode::Mesh& mesh, const Vertex* vertices, uint32_t& vertexCount, uint32_t vertexStride ) {
    Optimize< uint32_t >( mesh, vertices, vertexCount, vertexStride );
}

void Optimize16( apemode::Mesh& mesh, const Vertex* vertices, uint32_t& vertexCount, uint32_t vertexStride ) {
    Optimize< uint16_t >( mesh, vertices, vertexCount, vertexStride );
}