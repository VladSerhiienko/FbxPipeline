#include <fbxppch.h>
#include <fbxpstate.h>

#include <vcache_optimizer.hpp>

/**
 * Helper function to calculate tangents when the tangent element layer is missing.
 * Can be used in multiple threads.
 * http://gamedev.stackexchange.com/a/68617/39505
 **/
template < typename TVertex >
void CalculateTangents( TVertex* vertices, size_t vertexCount ) {
    std::vector< mathfu::vec3 > tan;
    tan.resize( vertexCount * 2 );

    auto tan1 = tan.data( );
    auto tan2 = tan1 + vertexCount;

    for ( size_t i = 0; i < vertexCount; i += 3 ) {
        TVertex& v0 = vertices[ i + 0 ];
        TVertex& v1 = vertices[ i + 1 ];
        TVertex& v2 = vertices[ i + 2 ];

        float x1 = v1.position[ 0 ] - v0.position[ 0 ];
        float x2 = v2.position[ 0 ] - v0.position[ 0 ];
        float y1 = v1.position[ 1 ] - v0.position[ 1 ];
        float y2 = v2.position[ 1 ] - v0.position[ 1 ];
        float z1 = v1.position[ 2 ] - v0.position[ 2 ];
        float z2 = v2.position[ 2 ] - v0.position[ 2 ];

        float s1 = v1.texCoords[ 0 ] - v0.texCoords[ 0 ];
        float s2 = v2.texCoords[ 0 ] - v0.texCoords[ 0 ];
        float t1 = v1.texCoords[ 1 ] - v0.texCoords[ 1 ];
        float t2 = v2.texCoords[ 1 ] - v0.texCoords[ 1 ];

        float        r = 1.0F / ( s1 * t2 - s2 * t1 );
        mathfu::vec3 sdir( ( t2 * x1 - t1 * x2 ) * r, ( t2 * y1 - t1 * y2 ) * r, ( t2 * z1 - t1 * z2 ) * r );
        mathfu::vec3 tdir( ( s1 * x2 - s2 * x1 ) * r, ( s1 * y2 - s2 * y1 ) * r, ( s1 * z2 - s2 * z1 ) * r );

        tan1[ i + 0 ] += sdir;
        tan1[ i + 1 ] += sdir;
        tan1[ i + 2 ] += sdir;

        tan2[ i + 0 ] += tdir;
        tan2[ i + 1 ] += tdir;
        tan2[ i + 2 ] += tdir;
    }

    for ( size_t i = 0; i < vertexCount; i += 1 ) {
        TVertex& v = vertices[ i ];
        auto     n = mathfu::vec3( v.normal[ 0 ], v.normal[ 1 ], v.normal[ 2 ] );
        auto     t = tan1[ i ];

        mathfu::vec3 tt = mathfu::normalize( t - n * mathfu::dot( n, t ) );
        v.tangent[ 0 ]  = tt.x( );
        v.tangent[ 1 ]  = tt.y( );
        v.tangent[ 2 ]  = tt.z( );
        v.tangent[ 3 ]  = ( mathfu::dot( mathfu::cross( n, t ), tan2[ i ] ) < 0 ) ? -1.0f : 1.0f;
    }
}

/**
 * Calculate normals for the faces (does not weight triangles).
 * Fast and usable results, however incorrect.
 * TODO: Implement vertex normal calculations.
 **/
template < typename TVertex >
void CalculateFaceNormals( TVertex* vertices, size_t vertexCount ) {
    for ( size_t i = 0; i < vertexCount; i += 3 ) {
        TVertex& v0 = vertices[ i + 0 ];
        TVertex& v1 = vertices[ i + 1 ];
        TVertex& v2 = vertices[ i + 2 ];

        const mathfu::vec3 p0( v0.position );
        const mathfu::vec3 p1( v1.position );
        const mathfu::vec3 p2( v2.position );
        const mathfu::vec3 n( mathfu::normalize( mathfu::cross( p1 - p0, p2 - p0 ) ) );

        v0.normal[ 0 ] = n.x( );
        v0.normal[ 1 ] = n.y( );
        v0.normal[ 2 ] = n.z( );

        v1.normal[ 0 ] = n.x( );
        v1.normal[ 1 ] = n.y( );
        v1.normal[ 2 ] = n.z( );

        v2.normal[ 0 ] = n.x( );
        v2.normal[ 1 ] = n.y( );
        v2.normal[ 2 ] = n.z( );
    }
}

/**
 * Produces mesh subsets and subset indices.
 * A subset is a structure for mapping material index to a polygon range to allow a single mesh to
 * be rendered using multiple materials.
 * The usage could be: 1) render polygon range [ 0, 12] with 1st material.
 *                     2) render polygon range [12, 64] with 2nd material.
 *                     * range is [base index; index count]
 *
 * @param indices The indices of the mesh that will be used to draw the mesh with multiple materials.
 * @param subsets The ranges of the vertex indices for each material of the node.
 * @param subsetPolies A mapping of material indices to polygon ranges (useful for knowing the basic structure).
 * @return True on success.
 **/
template < typename TIndex >
bool GetSubsets( FbxMesh*                           mesh,
                 fbxp::Mesh&                        m,
                 std::vector< TIndex >&             indices,
                 std::vector< fbxp::fb::SubsetFb >& subsets,
                 std::vector< fbxp::fb::SubsetFb >& subsetPolies,
                 std::vector< std::tuple< TIndex, TIndex > >& items ) {
    auto& s = fbxp::Get( );

    s.console->info("Mesh \"{}\" has {} material(s) assigned.", mesh->GetNode( )->GetName( ), mesh->GetNode( )->GetMaterialCount( ) );

    // No submeshes for a node that has only 1 or no materials.
    if (mesh->GetNode()->GetMaterialCount() < 2) {
        return false;
    }

    //
    // Print materials attached to a node.
    //

    for ( auto k = 0; k < mesh->GetNode( )->GetMaterialCount( ); ++k ) {
        s.console->info( "\t#{} - \"{}\".", k, mesh->GetNode( )->GetMaterial( k )->GetName( ) );
    }

    items.clear( );
    indices.clear( );
    subsets.clear( );
    subsetPolies.clear( );

    items.reserve( mesh->GetPolygonCount( ) );
    indices.reserve( mesh->GetPolygonCount( ) * 3 );
    subsets.reserve( mesh->GetNode( )->GetMaterialCount( ) );
    subsetPolies.reserve( mesh->GetNode( )->GetMaterialCount( ) );

    // Go though all the material elements and map them.
    if ( const uint32_t mc = (uint32_t) mesh->GetElementMaterialCount( ) ) {
        s.console->info( "Mesh \"{}\" has {} material elements.", mesh->GetNode( )->GetName( ), mc );

        for ( uint32_t m = 0; m < mc; ++m ) {
            if ( const auto materialElement = mesh->GetElementMaterial( m ) ) {
                // The only mapping mode for materials that makes sense is polygon mapping.
                const auto materialMappingMode = materialElement->GetMappingMode( );
                if ( materialMappingMode != FbxLayerElement::eByPolygon ) {
                    s.console->error( "Material element #{} has {} mapping mode (not supported).", m, materialMappingMode );
                    DebugBreak( );
                    continue;
                }

                // Mapping is done though the polygon indices.
                // For each polygon we have assigned material index.
                const auto& materialIndices = materialElement->GetIndexArray( );
                if (materialIndices.GetCount() < 1) {
                    s.console->error( "Material element {} has no indices, skipped.", m );
                    DebugBreak( );
                    continue;
                }

                const uint32_t pc = (uint32_t) mesh->GetPolygonCount( );
                for ( uint32_t i = 0; i < pc; ++i )
                    items.emplace_back( (TIndex) materialIndices.GetAt( i ), i );
            }
        }
    }

    if ( items.empty( ) ) {
        s.console->error( "Mesh \"{}\" has no correctly mapped materials (fallback to first one).", mesh->GetNode( )->GetName( ) );
        DebugBreak( );
        return false;
    }

    //
    // The most important part:
    // 1) Make sure our mapping is sorted, the FBX SDK doc does not state their data is sorted
    //    but in practice I did not see the unsorted polygon indices.
    // 2) Fill all the polygon ranges (consider breaks in the sorted ranges).
    //    If the polygon indices are [2, 3, 4, 10, 11, 12, 13, 15, 17]
    //    we will get {2, 3} (range starts at 2 and is 3 polygons long),
    //                {10, 4}, {15, 1}, {17, 1}.
    //

    using U = std::tuple< TIndex, TIndex >;
    auto sortByMaterialIndex = [&]( U& a, U& b ) { return std::get< 0 >( a ) < std::get< 0 >( b ); };
    auto sortByPolygonIndex  = [&]( U& a, U& b ) { return std::get< 1 >( a ) < std::get< 1 >( b ); };

    // Sort items by material index.
    std::sort( items.begin( ), items.end( ), sortByMaterialIndex );

    auto extractSubsetsFromRange = [&]( const uint32_t mii, const uint32_t ii, const uint32_t i ) {
        uint32_t ki = ii;
        TIndex k  = std::get< 1 >( items[ ii ] );

        uint32_t j = ii;
        for ( ; j < i; ++j ) {
            //indices.push_back( j * 3 + 0 );
            //indices.push_back( j * 3 + 0 );
            //indices.push_back( j * 3 + 2 );

            const TIndex kk  = std::get< 1 >( items[ j ] );
            if ((kk - k) > 1) {
                // s.console->info( "Break in {} - {} vs {}", j, k, kk );
                // Process a case where there is a break in polygon indices.
                s.console->info( "\tAdding subset: material #{}, polygon #{}, count {} ({} - {}).", mii, k, j - ki, ki, j - 1 );

                // subsetPolies.emplace_back( mii, k, j - ki );
                subsetPolies.emplace_back( mii, ki, j - ki );
                // subsets.emplace_back( mii, ki, j - ki );
                // subsets.emplace_back( mii, ki * 3, ( j - ki ) * 3 );
                ki = j;
                k  = kk;
            }

            k = kk;
        }

        s.console->info( "\tAdding subset: material #{}, polygon #{}, count {} ({} - {}).", mii, k, j - ki, ki, j - 1 );
        // subsetPolies.emplace_back( mii, k, j - ki );
        subsetPolies.emplace_back( mii, ki, j - ki );
        // subsets.emplace_back( mii, ki, j - ki );
        // subsets.emplace_back( mii, ki * 3, ( j - ki ) * 3 );
    };

    uint32_t ii = 0;
    TIndex mii = std::get< 0 >( items.front( ) );

    uint32_t i  = 0;
    const uint32_t ic = (uint32_t) items.size( );
    for ( ; i < ic; ++i ) {
        const TIndex mi = std::get< 0 >( items[ i ] );
        if ( mi != mii ) {
            s.console->info( "Material #{} has {} assigned polygons ({} - {}).", mii, i - ii, ii, i - 1 );

            // Sort items by polygon index.
            std::sort( items.data( ) + ii, items.data( ) + i, sortByPolygonIndex );
            extractSubsetsFromRange( mii, ii, i );
            ii  = i;
            mii = mi;
        }
    }

    s.console->info( "Material #{} has {} assigned polygons ({} - {}).", mii, i - ii, ii, i - 1 );
    std::sort( items.data( ) + ii, items.data( ) + i, sortByPolygonIndex );
    extractSubsetsFromRange( mii, ii, i );

    TIndex subsetStartIndex = 0;
    uint32_t materialIndex    = (uint32_t) -1;

    if ( !subsetPolies.empty( ) ) {
        s.console->info( "Mesh index subsets from {} polygon ranges:", subsetPolies.size( ) );
    }

    for ( auto& sp : subsetPolies ) {
        if ( materialIndex != sp.material_id( ) ) {
            const TIndex indexCount = (TIndex) indices.size( );

            if ( materialIndex != (uint32_t) -1 ) {
                const auto subsetLength = indexCount - subsetStartIndex;
                subsets.emplace_back( materialIndex, (uint32_t)subsetStartIndex, (uint32_t)subsetLength );

                s.console->info( "\tMesh subset #{} for material #{} index range: [{}; {}].",
                                 subsets.size( ) - 1,
                                 materialIndex,
                                 subsetStartIndex,
                                 subsetLength );
            }

            materialIndex    = sp.material_id( );
            subsetStartIndex = indexCount;
        }

        for ( uint32_t pp = 0; pp < sp.index_count( ); ++pp ) {
            const TIndex pii = ( TIndex )( sp.base_index( ) + pp );
            indices.push_back( pii * 3 + 0 );
            indices.push_back( pii * 3 + 1 );
            indices.push_back( pii * 3 + 2 );
        }
    }

    if ( !subsetPolies.empty( ) ) {
        const TIndex indexCount   = (TIndex) indices.size( );
        const auto subsetLength = indexCount - subsetStartIndex;
        subsets.emplace_back( materialIndex, subsetStartIndex, subsetLength );

        s.console->info( "\tMesh subset #{} for material #{} index range: [{}; {}].",
                         subsets.size( ) - 1,
                         materialIndex,
                         subsetStartIndex,
                         subsetLength );
    }

    assert( indices.size( ) == ( size_t )( mesh->GetPolygonCount( ) * 3 ) );
    assert( subsets.size( ) <= ( size_t )( mesh->GetNode( )->GetMaterialCount( ) ) );
    return true;
}

//
// Helper function to get values from geometry element layers
// with their reference and mapping modes.
//

/**
 * Returns the value from the element layer by index with respect to reference mode.
 **/
template < typename TElementLayer, typename TElementValue >
TElementValue GetElementValue( const TElementLayer* elementLayer, uint32_t i ) {
    assert( elementLayer );
    switch ( const auto referenceMode = elementLayer->GetReferenceMode( ) ) {
        case FbxLayerElement::EReferenceMode::eDirect:
            return elementLayer->GetDirectArray( ).GetAt( i );
        case FbxLayerElement::EReferenceMode::eIndex:
        case FbxLayerElement::EReferenceMode::eIndexToDirect: {
            const int j = elementLayer->GetIndexArray( ).GetAt( i );
            return elementLayer->GetDirectArray( ).GetAt( j );
        }

        default:
            fbxp::Get( ).console->error(
                "Reference mode {} of layer \"{}\" "
                "is not supported.",
                referenceMode,
                elementLayer->GetName( ) );

            DebugBreak( );
            return TElementValue( );
    }
}

/**
 * Returns the value from the element layer with respect to reference and mapping modes.
 **/
template < typename TElementLayer, typename TElementValue >
TElementValue GetElementValue( const TElementLayer* elementLayer,
                               uint32_t             controlPointIndex,
                               uint32_t             vertexIndex,
                               uint32_t             polygonIndex ) {
    if ( nullptr == elementLayer )
        return TElementValue( );

    switch ( const auto mappingMode = elementLayer->GetMappingMode( ) ) {
        case FbxLayerElement::EMappingMode::eByControlPoint:
            return GetElementValue< TElementLayer, TElementValue >( elementLayer, controlPointIndex );
        case FbxLayerElement::EMappingMode::eByPolygon:
            return GetElementValue< TElementLayer, TElementValue >( elementLayer, polygonIndex );
        case FbxLayerElement::EMappingMode::eByPolygonVertex:
            return GetElementValue< TElementLayer, TElementValue >( elementLayer, vertexIndex );

        default:
            fbxp::Get( ).console->error(
                "Mapping mode {} of layer \"{}\" "
                "is not supported.",
                mappingMode,
                elementLayer->GetName( ) );

            DebugBreak( );
            return TElementValue( );
    }
}

/**
 * Returns nullptr in case element layer has unsupported properties or is null.
 **/
template < typename TElementLayer >
const TElementLayer* VerifyElementLayer( const TElementLayer* elementLayer ) {
    if ( nullptr == elementLayer ) {
        fbxp::Get( ).console->error( "Missing element layer." );
        return nullptr;
    }

    switch ( const auto mappingMode = elementLayer->GetMappingMode( ) ) {
        case FbxLayerElement::EMappingMode::eByControlPoint:
        case FbxLayerElement::EMappingMode::eByPolygon:
        case FbxLayerElement::EMappingMode::eByPolygonVertex:
            break;
        default:
            fbxp::Get( ).console->error(
                "Mapping mode {} of layer \"{}\" "
                "is not supported.",
                mappingMode,
                elementLayer->GetName( ) );
            return nullptr;
    }

    switch ( const auto referenceMode = elementLayer->GetReferenceMode( ) ) {
        case FbxLayerElement::EReferenceMode::eDirect:
        case FbxLayerElement::EReferenceMode::eIndex:
        case FbxLayerElement::EReferenceMode::eIndexToDirect:
            break;
        default:
            fbxp::Get( ).console->error(
                "Reference mode {} of layer \"{}\" "
                "is not supported.",
                referenceMode,
                elementLayer->GetName( ) );
            return nullptr;
    }

    return elementLayer;
}

/**
 * Helper structure to assign vertex property values
 **/
struct StaticVertex {
    float position[ 3 ];
    float normal[ 3 ];
    float tangent[ 4 ];
    float texCoords[ 2 ];
};

/**
 * Helper structure to assign vertex property values
 **/
struct PackedVertex {
    float    position[ 3 ];
    uint32_t normal;
    uint32_t tangent;
    uint32_t texCoords;
};

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
        typedef uint32_t           submesh_id_t;
        typedef PackedVertex       vertex_t;
        typedef Triangle< TIndex > triangle_t;
        typedef TIndex             vertex_index_t;
        typedef TIndex             triangle_index_t;
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
    return m.m->vertices.size( ) / sizeof( PackedVertex );
    // return m.m->subsets[ sm ].index_count( );
}

Triangle< uint16_t > get_triangle( PackedMesh< uint16_t > const& m, uint32_t const& sm, uint16_t& index ) {
    uint16_t* subsetIndices = reinterpret_cast< uint16_t* >( m.m->subsetIndices.data( ) );
    const uint32_t i1 = subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 0 ];
    const uint32_t i2 = subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 1 ];
    const uint32_t i3 = subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 2 ];
    return Triangle< uint16_t >( i1, i2, i3 );
}

PackedVertex get_vertex( PackedMesh< uint16_t > const& m, uint32_t const& sm, uint16_t& index ) {
    // uint16_t* subsetIndices = reinterpret_cast< uint16_t* >( m.m->subsetIndices.data( ) );
    // const uint16_t i = subsetIndices[ index ];
    // const uint16_t i = subsetIndices[ m.m->subsets[ sm ].base_index( ) + index ];
    // return *(PackedVertex*) ( m.m->vertices.data( ) + i * sizeof( PackedVertex ) );
    return *(PackedVertex*) ( m.m->vertices.data( ) + index * sizeof( PackedVertex ) );
}

void set_triangle( PackedMesh< uint16_t >& m, uint32_t const& sm, uint16_t& index, Triangle< uint16_t > const& new_triangle ) {
    uint16_t* subsetIndices = reinterpret_cast< uint16_t* >( m.m->subsetIndices.data( ) );
    subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 0 ] = new_triangle[ 0 ];
    subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 1 ] = new_triangle[ 1 ];
    subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 2 ] = new_triangle[ 2 ];
}

void set_vertex( PackedMesh< uint16_t >& m, uint32_t const& sm, uint16_t& index, PackedVertex const& new_vertex ) {
    // uint16_t* subsetIndices = reinterpret_cast< uint16_t* >( m.m->subsetIndices.data( ) );
    // const uint16_t i = subsetIndices[ index ];
    // const uint16_t i = subsetIndices[ m.m->subsets[ sm ].base_index( ) + index ];
    //*(PackedVertex*) ( m.m->vertices.data( ) + i * sizeof( PackedVertex ) ) = new_vertex;
    *(PackedVertex*) ( m.m->vertices.data( ) + index * sizeof( PackedVertex ) ) = new_vertex;
}

Triangle< uint32_t > create_new_triangle( PackedMesh< uint32_t >& m, uint32_t const i1, uint32_t const i2, uint32_t const i3 ) {
    return Triangle< uint32_t >( i1, i2, i3 );
}

std::size_t get_num_triangles( PackedMesh< uint32_t > const& m, uint32_t const& sm ) {
    return m.m->subsets[ sm ].index_count( ) / 3;
}

std::size_t get_num_vertices( PackedMesh< uint32_t > const& m, uint32_t const& sm ) {
    return m.m->vertices.size( ) / sizeof( PackedVertex );
    //return m.m->subsets[ sm ].index_count( );
}

Triangle< uint32_t > get_triangle( PackedMesh< uint32_t > const& m, uint32_t const& sm, uint32_t& index ) {
    uint32_t* subsetIndices = reinterpret_cast< uint32_t* >( m.m->subsetIndices.data( ) );
    const uint32_t i1 = subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 0 ];
    const uint32_t i2 = subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 1 ];
    const uint32_t i3 = subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 2 ];
    return Triangle< uint32_t >( i1, i2, i3 );
}

PackedVertex get_vertex( PackedMesh< uint32_t > const& m, uint32_t const& sm, uint32_t& index ) {
    // uint32_t* subsetIndices = reinterpret_cast< uint32_t* >( m.m->subsetIndices.data( ) );
    // const uint32_t i = subsetIndices[ index ];
    // const uint32_t i = subsetIndices[ m.m->subsets[ sm ].base_index( ) + index ];
    // return *(PackedVertex*) ( m.m->vertices.data( ) + i * sizeof( PackedVertex ) );
    return *(PackedVertex*) ( m.m->vertices.data( ) + index * sizeof( PackedVertex ) );
}

void set_triangle( PackedMesh< uint32_t >& m, uint32_t const& sm, uint32_t& index, Triangle< uint32_t > const& new_triangle ) {
    uint32_t* subsetIndices = reinterpret_cast< uint32_t* >( m.m->subsetIndices.data( ) );
    subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 0 ] = new_triangle[ 0 ];
    subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 1 ] = new_triangle[ 1 ];
    subsetIndices[ m.m->subsets[ sm ].base_index( ) + index * 3 + 2 ] = new_triangle[ 2 ];
}

void set_vertex( PackedMesh< uint32_t >& m, uint32_t const& sm, uint32_t& index, PackedVertex const& new_vertex ) {
    // uint32_t* subsetIndices = reinterpret_cast< uint32_t* >( m.m->subsetIndices.data( ) );
    // const uint32_t i = m.m->subsetIndices[ index ];
    // const uint32_t i = m.m->subsetIndices[ m.m->subsets[ sm ].base_index( ) + index ];
    //*(PackedVertex*) ( m.m->vertices.data( ) + i * sizeof( PackedVertex ) ) = new_vertex;
    *(PackedVertex*) ( m.m->vertices.data( ) + index * sizeof( PackedVertex ) ) = new_vertex;
}

#pragma endregion

//
// Flatbuffers takes care about correct platform-independent alignment.
//

static_assert( sizeof( StaticVertex ) == sizeof( fbxp::fb::StaticVertexFb ), "Must match" );
static_assert( sizeof( PackedVertex ) == sizeof( fbxp::fb::PackedVertexFb ), "Must match" );

/**
 * Initialize vertices with very basic properties like 'position', 'normal', 'tangent', 'texCoords'.
 * Calculate mesh bounding box (local space).
 **/
template < typename TVertex >
void InitializeVertices( FbxMesh* mesh, fbxp::Mesh& m, TVertex* vertices, size_t vertexCount ) {
    auto& s = fbxp::Get( );

    // auto& controlPoints = m.controlPoints;
    const uint32_t cc = (uint32_t) mesh->GetControlPointsCount( );
    // controlPoints.reserve( cc );

    s.console->info( "Mesh \"{}\" has {} control points.", mesh->GetNode( )->GetName( ), cc );

    float bboxMin[ 3 ];
    float bboxMax[ 3 ];
    bboxMin[ 0 ] = std::numeric_limits< float >::max( );
    bboxMin[ 1 ] = std::numeric_limits< float >::max( );
    bboxMin[ 2 ] = std::numeric_limits< float >::max( );
    bboxMax[ 0 ] = std::numeric_limits< float >::min( );
    bboxMax[ 1 ] = std::numeric_limits< float >::min( );
    bboxMax[ 2 ] = std::numeric_limits< float >::min( );

    for ( uint32_t ci = 0; ci < cc; ++ci ) {
        const auto c = mesh->GetControlPointAt( ci );
        // controlPoints.emplace_back( (float) c[ 0 ], (float) c[ 1 ], (float) c[ 2 ] );
        bboxMin[ 0 ] = std::min( bboxMin[ 0 ], (float) c[ 0 ] );
        bboxMin[ 1 ] = std::min( bboxMin[ 1 ], (float) c[ 1 ] );
        bboxMin[ 2 ] = std::min( bboxMin[ 2 ], (float) c[ 2 ] );
        bboxMax[ 0 ] = std::max( bboxMax[ 0 ], (float) c[ 0 ] );
        bboxMax[ 1 ] = std::max( bboxMax[ 1 ], (float) c[ 1 ] );
        bboxMax[ 2 ] = std::max( bboxMax[ 2 ], (float) c[ 2 ] );
    }

    m.min = fbxp::fb::vec3( bboxMin[ 0 ], bboxMin[ 1 ], bboxMin[ 2 ] );
    m.max = fbxp::fb::vec3( bboxMax[ 0 ], bboxMax[ 1 ], bboxMax[ 2 ] );

    //auto& polygons = m.polygons;
    const uint32_t pc = (uint32_t) mesh->GetPolygonCount( );
    //polygons.reserve( pc * 3 );

    s.console->info( "Mesh \"{}\" has {} polygons.", mesh->GetNode( )->GetName( ), pc );

    const auto uve = VerifyElementLayer( mesh->GetElementUV( ) );
    const auto ne  = VerifyElementLayer( mesh->GetElementNormal( ) );
    const auto te  = VerifyElementLayer( mesh->GetElementTangent( ) );

    uint32_t vi = 0;
    for ( uint32_t pi = 0; pi < pc; ++pi ) {
        // Must be triangular.
        assert( 3 == mesh->GetPolygonSize( pi ) );

        // Having this array we can easily control polygon winding order.
        // Since mesh is triangular we can make it static [3] at compile-time.
        for ( const uint32_t pvi : {0, 1, 2} ) {
            const uint32_t ci = (uint32_t) mesh->GetPolygonVertex( (int) pi, (int) pvi );
            //polygons.push_back( ci );

            //const auto vii = vi + pvi;
            const auto cp  = mesh->GetControlPointAt( ci );
            const auto uv  = GetElementValue< FbxGeometryElementUV, FbxVector2 >( uve, ci, vi, pi );
            const auto n   = GetElementValue< FbxGeometryElementNormal, FbxVector4 >( ne, ci, vi, pi );
            const auto t   = GetElementValue< FbxGeometryElementTangent, FbxVector4 >( te, ci, vi, pi );

            auto& vvii          = vertices[ vi ];
            vvii.position[ 0 ]  = (float) cp[ 0 ];
            vvii.position[ 1 ]  = (float) cp[ 1 ];
            vvii.position[ 2 ]  = (float) cp[ 2 ];
            vvii.normal[ 0 ]    = (float) n[ 0 ];
            vvii.normal[ 1 ]    = (float) n[ 1 ];
            vvii.normal[ 2 ]    = (float) n[ 2 ];
            vvii.tangent[ 0 ]   = (float) t[ 0 ];
            vvii.tangent[ 1 ]   = (float) t[ 1 ];
            vvii.tangent[ 2 ]   = (float) t[ 2 ];
            vvii.tangent[ 3 ]   = (float) t[ 3 ];
            vvii.texCoords[ 0 ] = (float) uv[ 0 ];
            vvii.texCoords[ 1 ] = (float) uv[ 1 ];

            ++vi;
        }
    }

    if ( nullptr == uve ) {
        s.console->error( "Mesh \"{}\" does not have texcoords geometry layer.",
                          mesh->GetNode( )->GetName( ) );
    }

    if ( nullptr == ne ) {
        s.console->warn( "Mesh \"{}\" does not have normal geometry layer.",
                          mesh->GetNode( )->GetName( ) );

        // Calculate face normals ourselves.
        // Usable but incorrect.
        CalculateFaceNormals( vertices, vertexCount );
    }

    if ( nullptr == te ) {
        s.console->warn( "Mesh \"{}\" does not have tangent geometry layer.",
                         mesh->GetNode( )->GetName( ) );

        // Calculate tangents ourselves.
        CalculateTangents( vertices, vertexCount );
    }
}

#pragma region Packing

union UIntPack_aaa2 {
    uint32_t u;
    struct {
        uint32_t x : 10;
        uint32_t y : 10;
        uint32_t z : 10;
        uint32_t w : 2;
    } q;
};

union UIntPack_8888 {
    uint32_t u;
    struct {
        uint32_t x : 8;
        uint32_t y : 8;
        uint32_t z : 8;
        uint32_t w : 8;
    } q;
};

union UIntPack_hh {
    uint32_t u;
    struct {
        uint32_t x : 16;
        uint32_t y : 16;
    } q;
};

union Float16Pack {
    struct {
        uint16_t m : 10;
        uint16_t e : 5;
        uint16_t s : 1;
    } q;

    uint16_t u;
};

union Float32Pack {
    struct {
        uint32_t m : 23;
        uint32_t e : 8;
        uint32_t s : 1;
    } q;

    float_t  f;
    uint32_t u;
};

static_assert( sizeof( Float16Pack ) == sizeof( uint16_t ), "Must match" );
static_assert( sizeof( Float32Pack ) == sizeof( uint32_t ), "Must match" );
static_assert( sizeof( UIntPack_8888 ) == sizeof( uint32_t ), "Must match" );
static_assert( sizeof( UIntPack_aaa2 ) == sizeof( uint32_t ), "Must match" );
static_assert( sizeof( UIntPack_hh ) == sizeof( uint32_t ), "Must match" );

template < bool bOverflowCheck = true >
uint16_t PackFloat_h( float v ) {
    Float32Pack f;
    Float16Pack h;

    f.f   = v;
    h.q.s = f.q.s;

    // Check for zero, denormal or too small value.
    // Too small exponent? (0+127-15)
    if ( bOverflowCheck && f.q.e <= 112 ) {
        // DebugBreak( );
        // Set to 0.
        h.q.e = 0;
        h.q.m = 0;
    }
    // Check for INF or NaN, or too high value
    // Too large exponent? (31+127-15)
    else if ( bOverflowCheck && f.q.e >= 143 ) {
        // DebugBreak( );
        // Set to 65504.0 (max value)
        h.q.e = 30;
        h.q.m = 1023;
    }
    // Handle normal number.
    else {
        h.q.e = int32_t( f.q.e ) - 127 + 15;
        h.q.m = uint16_t( f.q.m >> 13 );
    }

    return h.u;
}

template < bool tangent >
uint32_t PackNormal_aaa2( const float* v ) {
    UIntPack_aaa2 packed;
    packed.q.x = ( uint32_t )( mathfu::Clamp( v[ 0 ], -1.0f, 1.0f ) * 511.0f + 512.0f );
    packed.q.y = ( uint32_t )( mathfu::Clamp( v[ 1 ], -1.0f, 1.0f ) * 511.0f + 512.0f );
    packed.q.z = ( uint32_t )( mathfu::Clamp( v[ 2 ], -1.0f, 1.0f ) * 511.0f + 512.0f );
    packed.q.w = tangent ? ( uint32_t )( mathfu::Clamp( v[ 3 ], -1.0f, 1.0f ) * 1.0f + 2.0f ) : 0;
    return packed.u;
}

template < bool tangent >
uint32_t PackNormal_8888( const float* v ) {
    UIntPack_8888 packed;
    packed.q.x = ( uint32_t )( mathfu::Clamp( v[ 0 ], -1.0f, 1.0f ) * 127.0f + 128.0f );
    packed.q.y = ( uint32_t )( mathfu::Clamp( v[ 1 ], -1.0f, 1.0f ) * 127.0f + 128.0f );
    packed.q.z = ( uint32_t )( mathfu::Clamp( v[ 2 ], -1.0f, 1.0f ) * 127.0f + 128.0f );
    packed.q.w = tangent ? ( uint32_t )( mathfu::Clamp( v[ 3 ], -1.0f, 1.0f ) * 127.0f + 128.0f ) : 0;
    return packed.u;
}

template < bool bOverflowCheck = true >
uint32_t PackUV_hh( const float* v ) {
    UIntPack_hh packed;
    packed.q.x = PackFloat_h< bOverflowCheck >( v[ 0 ] );
    packed.q.y = PackFloat_h< bOverflowCheck >( v[ 1 ] );
    return packed.u;
}

template < typename TVertex >
void PackVertices( const TVertex* vertices, PackedVertex* packed, const uint32_t vertexCount ) {
    for ( uint32_t i = 0; i < vertexCount; ++i ) {
        packed[ i ].position[ 0 ] = vertices[ i ].position[ 0 ];
        packed[ i ].position[ 1 ] = vertices[ i ].position[ 1 ];
        packed[ i ].position[ 2 ] = vertices[ i ].position[ 2 ];
        packed[ i ].normal        = PackNormal_8888< false >( vertices[ i ].normal );
        packed[ i ].tangent       = PackNormal_8888< true >( vertices[ i ].tangent );
        packed[ i ].texCoords     = PackUV_hh< true >( vertices[ i ].texCoords );
    }
}

#pragma endregion

template < typename TIndex, bool bOptimizeSubsetIndices = true >
void ExportMesh( FbxNode* node, FbxMesh * mesh, fbxp::Node& n, fbxp::Mesh& m, const uint32_t vertexCount ) {
    auto& s = fbxp::Get( );

    const uint16_t vertexStride           = (uint16_t) sizeof( StaticVertex );
    const uint32_t vertexBufferSize       = vertexCount * vertexStride;
    const uint16_t packedVertexStride     = (uint16_t) sizeof( PackedVertex );
    const uint32_t packedVertexBufferSize = vertexCount * packedVertexStride;

    static std::vector< std::tuple< TIndex, TIndex > > tempItems;

    static std::vector< TIndex >       tempSubsetIndices;
    static std::vector< StaticVertex > tempBuffer;
    if ( tempBuffer.size( ) < vertexCount )
        tempBuffer.resize( vertexCount );

    InitializeVertices( mesh, m, tempBuffer.data( ), vertexCount );
    GetSubsets( mesh, m, tempSubsetIndices, m.subsets, m.subsetsPolies, tempItems );

    const size_t subsetIndexBufferSize = sizeof( TIndex ) * tempSubsetIndices.size( );
    m.subsetIndices.resize( subsetIndexBufferSize );
    std::memcpy( m.subsetIndices.data( ), tempSubsetIndices.data( ), subsetIndexBufferSize );

    if ( std::is_same< TIndex, uint16_t >::value ) {
        m.subsetIndexType = fbxp::fb::EIndexTypeFb_UInt16;
    } else if ( std::is_same< TIndex, uint32_t >::value ) {
        m.subsetIndexType = fbxp::fb::EIndexTypeFb_UInt32;
    } else {
        assert( false );
    }

    m.vertices.resize( packedVertexBufferSize );
    PackVertices( reinterpret_cast< StaticVertex* >( tempBuffer.data( ) ),
                  reinterpret_cast< PackedVertex* >( m.vertices.data( ) ),
                  vertexCount );

    if ( bOptimizeSubsetIndices && false == m.subsets.empty( ) ) {
        PackedMesh< TIndex > mm{(fbxp::Mesh*) &m};
        for ( uint32_t ss = 0; ss < m.subsets.size( ); ++ss ) {
            vcache_optimizer::vcache_optimizer< PackedMesh< TIndex > > optimizer;
            optimizer( mm, ss, false );
        }
    }

    m.submeshes.emplace_back( 0,                              // base vertex
                              vertexCount,                    // vertex count
                              0,                              // base index
                              0,                              // index count
                              0,                              // base subset
                              (uint32_t) m.subsets.size( ),   // subset count
                              fbxp::fb::EVertexFormat_Packed, // vertex format
                              packedVertexStride              // vertex stride
                              );
}

void ExportMesh( FbxNode* node, fbxp::Node& n ) {
    auto& s = fbxp::Get( );
    if ( auto mesh = node->GetMesh( ) ) {
        s.console->info( "Node \"{}\" has mesh.", node->GetName( ) );
        if ( !mesh->IsTriangleMesh( ) ) {
            s.console->warn( "Mesh \"{}\" is not triangular, processing...", node->GetName( ) );
            FbxGeometryConverter converter( mesh->GetNode( )->GetFbxManager( ) );
            mesh = (FbxMesh*) converter.Triangulate( mesh, false, s.legacyTriangulationSdk );

            if ( nullptr == mesh ) {
                s.console->error( "Mesh \"{}\" triangulation failed (mesh will be skipped).", node->GetName( ) );
                return;
            }

            s.console->warn( "Mesh \"{}\" was triangulated (success).", node->GetName( ) );
        }

        if ( const auto deformerCount = mesh->GetDeformerCount( ) ) {
            s.console->warn( "Mesh \"{}\" has {} deformers (ignored).", node->GetName( ), deformerCount );
        }

        n.meshId = (uint32_t) s.meshes.size( );
        s.meshes.emplace_back( );
        fbxp::Mesh& m = s.meshes.back( );

        const uint32_t vertexCount = mesh->GetPolygonCount( ) * 3;

        if ( vertexCount < 0xffff )
            ExportMesh< uint16_t >( node, mesh, n, m, vertexCount );
        else
            ExportMesh< uint32_t >( node, mesh, n, m, vertexCount );
    }
}
