#include <fbxppch.h>
#include <fbxpstate.h>

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
 * Produces mesh subsets and subset indices.
 * A subset is a structure for mapping material index to a polygon range to allow a single mesh to
 * be rendered using multiple materials.
 * The usage could be: 1) render polygon range [ 0, 12] with 1st material.
 *                     2) render polygon range [12, 64] with 2nd material.
 *                     * range is [base index; index count]
 **/
using SubsetItem = std::tuple< uint32_t, uint32_t, uint32_t >;
bool GetSubsets( FbxMesh* mesh, fbxp::Mesh& m, std::vector< uint32_t >& indices, std::vector< fbxp::fb::SubsetFb >& subsets ) {
    auto& s = fbxp::Get( );

    s.console->info("Mesh {} has {} material(s) assigned.", mesh->GetNode( )->GetName( ), mesh->GetNode( )->GetMaterialCount( ) );

    // No submeshes for a node that has only 1 or no materials.
    if (mesh->GetNode()->GetMaterialCount() < 2) {
        return false;
    }

    //
    // Print materials attached to a node.
    //

    for ( auto k = 0; k < mesh->GetNode( )->GetMaterialCount( ); ++k ) {
        s.console->info( "#{} - {}.", k, mesh->GetNode( )->GetMaterial( k )->GetName( ) );
    }

    std::vector< std::tuple< uint32_t, uint32_t > > items;
    items.reserve( mesh->GetPolygonCount( ) );
    //indices.reserve( mesh->GetPolygonCount( ) * 3 );
    subsets.reserve( mesh->GetNode( )->GetMaterialCount( ) );

    // Go though all the material elements and map them.
    if ( const uint32_t mc = (uint32_t) mesh->GetElementMaterialCount( ) ) {
        s.console->info( "Mesh {} has {} material elements.", mesh->GetNode( )->GetName( ), mc );

        for ( uint32_t m = 0; m < mc; ++m ) {
            if ( const auto materialElement = mesh->GetElementMaterial( m ) ) {
                // The only mapping mode for materials that makes sense is polygon mapping.
                const auto materialMappingMode = materialElement->GetMappingMode( );
                if ( materialMappingMode != FbxLayerElement::eByPolygon ) {
                    s.console->error( "Material element {} has {} mapping mode (not supported).", m, materialMappingMode );
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
                    items.emplace_back( (uint32_t) materialIndices.GetAt( i ), i );
            }
        }
    }

    if ( items.empty( ) ) {
        s.console->error( "Mesh {} has no correctly mapped materials (fallback to first one).", mesh->GetNode( )->GetName( ) );
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

    using U = std::tuple< uint32_t, uint32_t >;
    auto sortByMaterialIndex = [&]( U& a, U& b ) { return std::get< 0 >( a ) < std::get< 0 >( b ); };
    auto sortByPolygonIndex  = [&]( U& a, U& b ) { return std::get< 1 >( a ) < std::get< 1 >( b ); };

    // Sort items by material index.
    std::sort( items.begin( ), items.end( ), sortByMaterialIndex );

    auto extractSubsetsFromRange = [&]( const uint32_t mii, const uint32_t ii, const uint32_t i ) {
        uint32_t ki = ii;
        uint32_t k  = std::get< 1 >( items[ ii ] );

        uint32_t j = ii;
        for ( ; j < i; ++j ) {
            //indices.push_back( j * 3 + 0 );
            //indices.push_back( j * 3 + 0 );
            //indices.push_back( j * 3 + 2 );

            const uint32_t kk  = std::get< 1 >( items[ j ] );
            if ((kk - k) > 1) {
                // s.console->info( "Break in {} - {} vs {}", j, k, kk );
                // Process a case where there is a break in polygon indices.
                s.console->info( "Adding subset: material {}, polygon {}, count {} ({} - {}).", mii, k, j - ki, ki, j - 1 );

                // subsets.emplace_back( mii, ki, j - ki );
                subsets.emplace_back( mii, ki * 3, ( j - ki ) * 3 );
                ki = j;
                k  = kk;
            }

            k = kk;
        }

        s.console->info( "Adding subset: material {}, polygon {}, count {} ({} - {}).", mii, k, j - ki, ki, j - 1 );
        // subsets.emplace_back( mii, ki, j - ki );
        subsets.emplace_back( mii, ki * 3, ( j - ki ) * 3 );
    };

    uint32_t ii = 0;
    uint32_t mii = std::get< 0 >( items.front( ) );

    uint32_t i  = 0;
    const uint32_t ic = (uint32_t) items.size( );
    for ( ; i < ic; ++i ) {
        const uint32_t mi = std::get< 0 >( items[ i ] );
        if ( mi != mii ) {
            s.console->info( "Material {} has {} assigned polygons ({} - {}).", mii, i - ii, ii, i - 1 );

            // Sort items by polygon index.
            std::sort( items.data( ) + ii, items.data( ) + i, sortByPolygonIndex );
            extractSubsetsFromRange( mii, ii, i );
            ii  = i;
            mii = mi;
        }
    }

    s.console->info( "Material {} has {} assigned polygons ({} - {}).", mii, i - ii, ii, i - 1 );
    std::sort( items.data( ) + ii, items.data( ) + i, sortByPolygonIndex );
    extractSubsetsFromRange( mii, ii, i );
    return true;
}

//
// Helper function to get values from geometry element layers
// with their reference and mapping modes.
//

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
                "Reference mode {} of layer {} "
                "is not supported.",
                referenceMode,
                elementLayer->GetName( ) );

            DebugBreak( );
            return TElementValue( );
    }
}

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
                "Mapping mode {} of layer {} "
                "is not supported.",
                mappingMode,
                elementLayer->GetName( ) );

            DebugBreak( );
            return TElementValue( );
    }
}

/**
 * Helper structure to assign vertex property values
 * since flatbuffers generator took care about mutable data.
 * Between const_cast's and reinterpret_cast I chose the last one.
 **/
struct StaticVertex {
    float position[ 3 ];
    float normal[ 3 ];
    float tangent[ 4 ];
    float texCoords[ 2 ];
};

/**
 * Flatbuffers takes care about correct platform-independent alignment.
 **/
static_assert( sizeof( StaticVertex ) == sizeof( fbxp::fb::StaticVertexFb ), "Must match" );

/**
 * Initialize vertices with very basic properties like 'position', 'normal', 'tangent', 'texCoords'.
 * Calculate mesh bounding box (local space).
 **/
template < typename TVertex >
void InitializeVertices( FbxMesh* mesh, fbxp::Mesh& m, TVertex* vertices, size_t vertexCount ) {
    auto& s = fbxp::Get( );

    auto& controlPoints = m.controlPoints;
    const uint32_t cc = (uint32_t) mesh->GetControlPointsCount( );
    controlPoints.reserve( cc );

    s.console->info( "Mesh {} has {} control points.", mesh->GetNode( )->GetName( ), cc );

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
        controlPoints.emplace_back( (float) c[ 0 ], (float) c[ 1 ], (float) c[ 2 ] );
        bboxMin[ 0 ] = std::min( bboxMin[ 0 ], controlPoints.back( ).x( ) );
        bboxMin[ 1 ] = std::min( bboxMin[ 1 ], controlPoints.back( ).y( ) );
        bboxMin[ 2 ] = std::min( bboxMin[ 2 ], controlPoints.back( ).z( ) );
        bboxMax[ 0 ] = std::max( bboxMax[ 0 ], controlPoints.back( ).x( ) );
        bboxMax[ 1 ] = std::max( bboxMax[ 1 ], controlPoints.back( ).y( ) );
        bboxMax[ 2 ] = std::max( bboxMax[ 2 ], controlPoints.back( ).z( ) );
    }

    m.min = fbxp::fb::vec3( bboxMin[ 0 ], bboxMin[ 1 ], bboxMin[ 2 ] );
    m.max = fbxp::fb::vec3( bboxMax[ 0 ], bboxMax[ 1 ], bboxMax[ 2 ] );

    auto& polygons = m.polygons;
    const uint32_t pc = (uint32_t) mesh->GetPolygonCount( );

    s.console->info( "Mesh {} has {} polygons.", mesh->GetNode( )->GetName( ), pc );

    const auto uve = mesh->GetElementUV( );
    const auto ne  = mesh->GetElementNormal( );
    const auto te  = mesh->GetElementTangent( );

    polygons.reserve( pc * 3 );

    uint32_t vi = 0;
    for ( uint32_t pi = 0; pi < pc; ++pi ) {
        // Must be triangular.
        assert( 3 == mesh->GetPolygonSize( pi ) );

        // Having this array we can easily control polygon winding order.
        // Since mesh is triangular we can make it static [3] at compile-time.
        for ( const uint32_t pvi : {0, 1, 2} ) {
            const uint32_t ci = (uint32_t) mesh->GetPolygonVertex( (int) pi, (int) pvi );
            polygons.push_back( ci );

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
        s.console->error( "Mesh {} does not have texcoords geometry layer.",
                          mesh->GetNode( )->GetName( ) );
    }

    if ( nullptr == ne ) {
        s.console->error( "Mesh {} does not have normal geometry layer.",
                          mesh->GetNode( )->GetName( ) );
    }

    if ( nullptr == te ) {
        s.console->warn( "Mesh {} does not have tangent geometry layer (will be calculated here).",
                         mesh->GetNode( )->GetName( ) );

        // Calculate tangents ourselves.
        CalculateTangents( vertices, vertexCount );
    }
}

void ExportMesh( FbxNode* node, fbxp::Node& n ) {
    auto& s = fbxp::Get( );
    if ( auto mesh = node->GetMesh( ) ) {
        s.console->info( "Node {} has mesh '{}' (can be empty, ).", node->GetName( ), mesh->GetName( ) );
        if ( !mesh->IsTriangleMesh( ) ) {
            s.console->warn( "Mesh {} is not triangular, processing...", node->GetName( ) );
            FbxGeometryConverter converter( mesh->GetNode( )->GetFbxManager( ) );
            mesh = (FbxMesh*) converter.Triangulate( mesh, false, s.legacyTriangulationSdk );

            if (nullptr == mesh) {
                s.console->error( "Mesh {} triangulation failed (mesh will be skipped).", node->GetName( ) );
                return;
            }

            s.console->warn( "Mesh {} was triangulated (success).", node->GetName( ) );
        }

        if ( const auto deformerCount = mesh->GetDeformerCount( ) ) {
            s.console->warn( "Mesh {} has {} deformers (ignored).", node->GetName( ), deformerCount );
        }

        n.meshId = (uint32_t) s.meshes.size( );
        s.meshes.emplace_back( );
        fbxp::Mesh& m = s.meshes.back( );

        const uint32_t vertexCount  = mesh->GetPolygonCount( ) * 3;
        const uint16_t vertexStride = (uint16_t) sizeof( StaticVertex );
        m.vertices.resize( vertexCount * vertexStride );

        InitializeVertices( mesh, m, reinterpret_cast< StaticVertex* >( m.vertices.data( ) ), vertexCount );
        GetSubsets( mesh, m, m.subsetIndices, m.subsets );

        m.submeshes.emplace_back( 0,                              // base vertex
                                  vertexCount,                    // vertex count
                                  0,                              // base index
                                  0,                              // index count
                                  0,                              // base subset
                                  (uint32_t) m.subsets.size( ),   // subset count
                                  fbxp::fb::EVertexFormat_Static, // vertex format
                                  vertexStride                    // vertex stride
                                  );
    }
}
