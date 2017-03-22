#include <fbxppch.h>
#include <fbxpstate.h>

#pragma warning( push )
#pragma warning( disable : 4244 ) // int64 to int32 conversion
#include <vcache_optimizer.hpp>
#pragma warning( pop )

using namespace fbxp;
using namespace fb;

template < typename TIndex >
struct Triangle {
    TIndex indices[ 3 ];

    inline Triangle( ) {
    }

    inline Triangle( TIndex const i1, TIndex const i2, TIndex const i3 ) {
        indices[ 0 ] = i1;
        indices[ 1 ] = i2;
        indices[ 2 ] = i3;
    }

    inline TIndex operator[]( uint32_t const index ) const {
        assert( index < 3 );
        return indices[ index ];
    }
};

template < typename TIndex >
struct PackedMesh {
    fbxp::Mesh* m = nullptr;
    PackedMesh( fbxp::Mesh* m ) : m( m ) {
    }
};

using PackedMesh16 = PackedMesh< uint16_t >;
using PackedMesh32 = PackedMesh< uint32_t >;

namespace vcache_optimizer {
    template < typename TIndex >
    struct mesh_traits< PackedMesh< TIndex > > {
        typedef uint32_t                 submesh_id_t;
        typedef TIndex                   vertex_index_t;
        typedef TIndex                   triangle_index_t;
        typedef Triangle< TIndex >       triangle_t;
        typedef fbxp::fb::PackedVertexFb vertex_t;
    };
}

#pragma region Optimization

Triangle< uint16_t > create_new_triangle( PackedMesh< uint16_t >& m, uint16_t const i1, uint16_t const i2, uint16_t const i3 ) {
    return Triangle< uint16_t >( i1, i2, i3 );
}

std::size_t get_num_triangles( PackedMesh< uint16_t > const& m, uint32_t const& sm ) {
    return m.m->subsets[ sm ].index_count( ) / 3;
}

std::size_t get_num_vertices( PackedMesh< uint16_t > const& m, uint32_t const& sm ) {
    return m.m->vertices.size( ) / sizeof( fbxp::fb::PackedVertexFb );
    // return m.m->subsets[ sm ].index_count( );
}

Triangle< uint16_t > get_triangle( PackedMesh< uint16_t > const& m, uint32_t const& sm, uint16_t& index ) {
    uint16_t* subsetIndices = reinterpret_cast< uint16_t* >( m.m->subsetIndices.data( ) );
    const uint32_t i1 = subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 0 ];
    const uint32_t i2 = subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 1 ];
    const uint32_t i3 = subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 2 ];
    return Triangle< uint16_t >( i1, i2, i3 );
}

fbxp::fb::PackedVertexFb get_vertex( PackedMesh< uint16_t > const& m, uint32_t const& sm, uint16_t& index ) {
    // uint16_t* subsetIndices = reinterpret_cast< uint16_t* >( m.m->subsetIndices.data( ) );
    // const uint16_t i = subsetIndices[ index ];
    // const uint16_t i = subsetIndices[ m.m->subsets[ sm ].base_index( ) + index ];
    // return *(fbxp::fb::PackedVertexFb*) ( m.m->vertices.data( ) + i * sizeof( fbxp::fb::PackedVertexFb ) );
    return *(fbxp::fb::PackedVertexFb*) ( m.m->vertices.data( ) + index * sizeof( fbxp::fb::PackedVertexFb ) );
}

void set_triangle( PackedMesh< uint16_t >& m, uint32_t const& sm, uint16_t& index, Triangle< uint16_t > const& new_triangle ) {
    uint16_t* subsetIndices = reinterpret_cast< uint16_t* >( m.m->subsetIndices.data( ) );
    subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 0 ] = new_triangle[ 0 ];
    subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 1 ] = new_triangle[ 1 ];
    subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 2 ] = new_triangle[ 2 ];
}

void set_vertex( PackedMesh< uint16_t >& m, uint32_t const& sm, uint16_t& index, fbxp::fb::PackedVertexFb const& new_vertex ) {
    // uint16_t* subsetIndices = reinterpret_cast< uint16_t* >( m.m->subsetIndices.data( ) );
    // const uint16_t i = subsetIndices[ index ];
    // const uint16_t i = subsetIndices[ m.m->subsets[ sm ].base_index( ) + index ];
    // *(fbxp::fb::PackedVertexFb*) ( m.m->vertices.data( ) + i * sizeof( fbxp::fb::PackedVertexFb ) ) = new_vertex;
    *(fbxp::fb::PackedVertexFb*) ( m.m->vertices.data( ) + index * sizeof( fbxp::fb::PackedVertexFb ) ) = new_vertex;
}

Triangle< uint32_t > create_new_triangle( PackedMesh< uint32_t >& m, uint32_t const i1, uint32_t const i2, uint32_t const i3 ) {
    return Triangle< uint32_t >( i1, i2, i3 );
}

std::size_t get_num_triangles( PackedMesh< uint32_t > const& m, uint32_t const& sm ) {
    return m.m->subsets[ sm ].index_count( ) / 3;
}

std::size_t get_num_vertices( PackedMesh< uint32_t > const& m, uint32_t const& sm ) {
    return m.m->vertices.size( ) / sizeof( fbxp::fb::PackedVertexFb );
    // return m.m->subsets[ sm ].index_count( );
}

Triangle< uint32_t > get_triangle( PackedMesh< uint32_t > const& m, uint32_t const& sm, uint32_t& index ) {
    uint32_t* subsetIndices = reinterpret_cast< uint32_t* >( m.m->subsetIndices.data( ) );
    const uint32_t i1 = subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 0 ];
    const uint32_t i2 = subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 1 ];
    const uint32_t i3 = subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 2 ];
    return Triangle< uint32_t >( i1, i2, i3 );
}

fbxp::fb::PackedVertexFb get_vertex( PackedMesh< uint32_t > const& m, uint32_t const& sm, uint32_t& index ) {
    // uint32_t* subsetIndices = reinterpret_cast< uint32_t* >( m.m->subsetIndices.data( ) );
    // const uint32_t i = subsetIndices[ index ];
    // const uint32_t i = subsetIndices[ m.m->subsets[ sm ].base_index( ) + index ];
    // return *(fbxp::fb::PackedVertexFb*) ( m.m->vertices.data( ) + i * sizeof( fbxp::fb::PackedVertexFb ) );
    return *(fbxp::fb::PackedVertexFb*) ( m.m->vertices.data( ) + index * sizeof( fbxp::fb::PackedVertexFb ) );
}

void set_triangle( PackedMesh< uint32_t >& m, uint32_t const& sm, uint32_t& index, Triangle< uint32_t > const& new_triangle ) {
    uint32_t* subsetIndices = reinterpret_cast< uint32_t* >( m.m->subsetIndices.data( ) );
    subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 0 ] = new_triangle[ 0 ];
    subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 1 ] = new_triangle[ 1 ];
    subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 2 ] = new_triangle[ 2 ];
}

void set_vertex( PackedMesh< uint32_t >& m, uint32_t const& sm, uint32_t& index, fbxp::fb::PackedVertexFb const& new_vertex ) {
    // uint32_t* subsetIndices = reinterpret_cast< uint32_t* >( m.m->subsetIndices.data( ) );
    // const uint32_t i = m.m->subsetIndices[ index ];
    // const uint32_t i = m.m->subsetIndices[ m.m->subsets[ sm ].base_index( ) + index ];
    // *(fbxp::fb::PackedVertexFb*) ( m.m->vertices.data( ) + i * sizeof( fbxp::fb::PackedVertexFb ) ) = new_vertex;
    *(fbxp::fb::PackedVertexFb*) ( m.m->vertices.data( ) + index * sizeof( fbxp::fb::PackedVertexFb ) ) = new_vertex;
}

#pragma endregion

template < typename TIndex >
void Optimize( fbxp::Mesh& m ) {
    PackedMesh< TIndex > mm{(fbxp::Mesh*) &m};
    for ( uint32_t ss = 0; ss < m.subsets.size( ); ++ss ) {
        vcache_optimizer::vcache_optimizer< PackedMesh< TIndex > > optimizer;
        optimizer( mm, ss, false );
    }
}

void Optimize32( fbxp::Mesh& mesh ) {
    Optimize< uint32_t >( mesh );
}

void Optimize16( fbxp::Mesh& mesh ) {
    Optimize< uint16_t >( mesh );
}