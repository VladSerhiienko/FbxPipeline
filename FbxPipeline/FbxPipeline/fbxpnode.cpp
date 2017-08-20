#include <fbxppch.h>
#include <fbxpstate.h>
#include <queue>

void ExportMesh( FbxNode* node, apemode::Node& n, bool pack, bool optimize );
void ExportMaterials( FbxScene* scene );
void ExportMaterials( FbxNode* node, apemode::Node& n );
void ExportTransform( FbxNode* node, apemode::Node& n );
void ExportAnimation( FbxNode* node, apemode::Node& n );

void ExportNodeAttributes( FbxNode* node, apemode::Node& n ) {
    auto& s = apemode::Get( );

    n.cullingType = (apemodefb::ECullingType) node->mCullingType;
    s.console->info( "Node \"{}\" has {} culling type.", node->GetName(), n.cullingType );

    ExportTransform( node, n );
    ExportAnimation( node, n );
    ExportMesh( node, n, s.options[ "p" ].as< bool >( ), s.options[ "t" ].as< bool >( ) );
    ExportMaterials( node, n );
}

uint32_t ExportNode( FbxNode* node ) {
    auto& s = apemode::Get( );

    const uint32_t nodeId = static_cast< uint32_t >( s.nodes.size( ) );
    s.nodes.emplace_back( );

    auto& n = s.nodes.back( );
    n.id = nodeId;
    n.nameId = s.PushName( node->GetName( ) );

    ExportNodeAttributes( node, n );
    if ( auto c = node->GetChildCount( ) ) {
        n.childIds.reserve( c );
        for ( auto i = 0; i < c; ++i ) {
            const auto childId = ExportNode( node->GetChild( i ) );
            s.nodes[ nodeId ].childIds.push_back( childId );
        }
    }

    return nodeId;
}

/**
 * Preprocess scene with Fbx tools:
 * FbxGeometryConverter > Remove bad polygons
 *                      > Triangulate
 *                      > Split meshes per material
 **/
void PreprocessMeshes( FbxScene* scene ) {
    auto& s = apemode::Get( );

    FbxGeometryConverter geometryConverter( s.manager );

    s.console->info( "Triangulating..." );
    if ( false == geometryConverter.Triangulate( s.scene, true ) ) {
        s.console->warn( "Triangulation failed for some nodes." );
        s.console->warn( "Nodes that failed triangulation will be detected in mesh exporting stage." );
    } else {
        s.console->info( "Triangulation succeeded for all nodes." );
    }

    FbxArray< FbxNode* > affectedNodes;
    s.console->info( "Removing bad polygons..." );
    geometryConverter.RemoveBadPolygonsFromMeshes( s.scene, &affectedNodes );
    if ( 0 != affectedNodes.Size( ) ) {
        s.console->warn( "Removed bad polygons from {} nodes:", affectedNodes.Size( ) );
        for ( int32_t i = 0; i < affectedNodes.Size( ); ++i ) {
            assert( nullptr != affectedNodes[ i ] );
            s.console->warn( "\t > {}", affectedNodes[ i ]->GetName( ) );
        }
    } else {
        s.console->info( "No bad polygons in the scene." );
    }

    if ( s.options[ "s" ].as< bool >( ) ) {
        s.console->info( "Splitting per material..." );
        if ( false == geometryConverter.SplitMeshesPerMaterial( s.scene, true ) ) {
            s.console->warn( "Splitting per material failed for some nodes." );
            s.console->warn( "Nodes that were not splitted will have subsets." );
        } else {
            s.console->info( "Splitting per material succeeded for all nodes." );
        }
    }
}

void PreprocessAnimation( FbxScene* pScene ) {
    auto& s = apemode::Get( );

    int animStackCount = pScene->GetSrcObjectCount< FbxAnimStack >( );
    s.console->info( "Scene has {} animation stacks:", animStackCount );

    for ( int i = 0; i < animStackCount; i++ ) {
        FbxAnimStack* pAnimStack = pScene->GetSrcObject< FbxAnimStack >( i );
        s.animStacks.emplace_back( );
        s.animStacks.back( ).nameId = s.PushName( pAnimStack->GetName( ) );

        int animLayerCount = pAnimStack->GetMemberCount< FbxAnimLayer >( );
        s.console->info( "\t- animation stack #{} \"{}\" has {} layers ", i, pAnimStack->GetName( ), animLayerCount );

        for ( int j = 0; j < animLayerCount; j++ ) {
            FbxAnimLayer* pAnimLayer = pAnimStack->GetMember< FbxAnimLayer >( j );
            s.animLayers.emplace_back( );
            s.animLayers.back( ).nameId      = s.PushName( pAnimLayer->GetName( ) );
            s.animLayers.back( ).animStackId = s.animStacks.size( ) - 1;

            s.console->info( "\t\t- animation layer #{} \"{}\"", j, pAnimLayer->GetName( ) );
        }
    }
}

void ExportScene( FbxScene* scene ) {
    auto& s = apemode::Get( );

    PreprocessMeshes( scene );
    PreprocessAnimation( scene );

    // Pre-allocate nodes and attributes.
    s.nodes.reserve( (size_t) scene->GetNodeCount( ) );
    s.meshes.reserve( (size_t) scene->GetNodeCount( ) );

    // We want shared materials, so export all the scene material first
    // and reference them from the node scope by their indices.
    ExportMaterials( scene );

    // Export nodes recursively.
    ExportNode( scene->GetRootNode( ) );
}
