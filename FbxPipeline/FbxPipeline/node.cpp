#include <fbxppch.h>
#include <fbxpstate.h>
#include <queue>

void ExportMesh( FbxNode* node, fbxp::Node& n );
void ExportMaterials( FbxScene* scene );
void ExportMaterials( FbxNode* node, fbxp::Node& n );
void ExportTransform( FbxNode* node, fbxp::Node& n );
void ExportAnimation( FbxNode* node, fbxp::Node& n );

void ExportNodeAttributes( FbxNode* node, fbxp::Node& n ) {
    ExportTransform( node, n );
    ExportAnimation( node, n );
    ExportMesh( node, n );
    ExportMaterials( node, n );
}

uint32_t ExportNode( FbxNode* node ) {
    auto& s = fbxp::GetState( );

    const auto nodeId = s.nodes.size( );
    s.nodes.emplace_back( );

    auto& n  = s.nodes.back( );
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

void ExportScene( FbxScene* scene ) {
    auto& s = fbxp::GetState( );
    s.nodes.reserve( scene->GetNodeCount( ) );
    ExportMaterials( scene );
    ExportNode( scene->GetRootNode( ) );
}
