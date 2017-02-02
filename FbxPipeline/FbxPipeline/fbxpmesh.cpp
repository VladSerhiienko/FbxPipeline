#include <fbxppch.h>
#include <fbxpstate.h>

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
    indices.reserve( mesh->GetPolygonCount( ) * 3 );
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
        s.console->error( "Mesh {} has not correctly mapped materials.", mesh->GetNode( )->GetName( ) );
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

    auto extractSubmeshesFromRange = [&]( const uint32_t mii, const uint32_t ii, const uint32_t i ) {
        uint32_t ki = ii;
        uint32_t k  = std::get< 1 >( items[ ii ] );

        uint32_t j = ii;
        for ( ; j < i; ++j ) {
            indices.push_back( j * 3 + 0 );
            indices.push_back( j * 3 + 0 );
            indices.push_back( j * 3 + 2 );

            const uint32_t kk  = std::get< 1 >( items[ j ] );
            if ((kk - k) > 1) {
                // s.console->info( "Break in {} - {} vs {}", j, k, kk );
                // Process a case where there is a break in polygon indices.
                s.console->info( "Adding subset: material {}, index {}, count {} ({} - {})", mii, k, j - ki, ki, j - 1 );
                subsets.emplace_back( mii, ki, j - ki );
                ki = j;
                k  = kk;
            }

            k = kk;
        }

        s.console->info( "Adding subset: material {}, index {}, count {} ({} - {})", mii, k, j - ki, ki, j - 1 );
        subsets.emplace_back( mii, k, j - ki );
    };

    uint32_t ii = 0;
    uint32_t mii = std::get< 0 >( items.front( ) );

    uint32_t i  = 0;
    const uint32_t ic = items.size( );
    for ( ; i < ic; ++i ) {
        uint32_t mi = std::get< 0 >( items[ i ] );
        if ( mi != mii ) {
            s.console->info( "Material {} has {} assigned polygons ({} - {}).", mii, i - ii, ii, i - 1 );

            // Sort items by polygon index.
            std::sort( items.data( ) + ii, items.data( ) + i, sortByPolygonIndex );
            extractSubmeshesFromRange( mii, ii, i );
            ii  = i;
            mii = mi;
        }
    }

    s.console->info( "Material {} has {} assigned polygons ({} - {}).", mii, i - ii, ii, i - 1 );
    std::sort( items.data( ) + ii, items.data( ) + i, sortByPolygonIndex );
    extractSubmeshesFromRange( mii, ii, i );
}

template < typename TVertexFn >
void GetVertices( FbxMesh* mesh, fbxp::Mesh& m, TVertexFn vertexCallback ) {
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

    polygons.reserve( pc * 3 );
    for ( uint32_t pi = 0; pi < pc; ++pi ) {
        // Must be triangular.
        assert( 3 == mesh->GetPolygonSize( pi ) );

        // Having this array we can easily control polygon winding order.
        for ( const uint32_t pvi : {0, 1, 2} ) {
            const uint32_t ci = (uint32_t) mesh->GetPolygonVertex( (int) pi, (int) pvi );
            polygons.push_back( ci );
        }
    }
}

void ExportMesh( FbxNode* node, fbxp::Node& n ) {
    auto& s = fbxp::Get( );
    if ( auto mesh = node->GetMesh( ) ) {
        s.console->info( "Node {} has mesh '{}' (can be empty).", node->GetName( ), mesh->GetName( ) );
        if ( !mesh->IsTriangleMesh( ) ) {
            s.console->warn( "Mesh {} is not triangular, processing...", node->GetName( ) );
            FbxGeometryConverter converter( mesh->GetNode( )->GetFbxManager( ) );
            mesh = (FbxMesh*) converter.Triangulate( mesh, false );
            s.console->warn( "Mesh {} was triangulated.", node->GetName( ) );
        }

        if ( const auto deformerCount = mesh->GetDeformerCount( ) ) {
            s.console->warn( "Mesh {} has {} deformers (ignored).", node->GetName( ), deformerCount );
        }

        const uint32_t meshId = s.meshes.size( );
        n.meshId = meshId;
        s.meshes.emplace_back( );
        fbxp::Mesh& m = s.meshes.back( );

        GetVertices( mesh, m, [] {} );
        GetSubsets( mesh, m, m.subsetIndices, m.subsets );
    }
}
