#include <fbxppch.h>
#include <fbxpstate.h>

void ExportMesh( FbxNode* node, fbxp::Node& n ) {
    auto& s = fbxp::Get( );
    if ( auto mesh = node->GetMesh( ) ) {
        s.console->info( "Node {} has mesh {}", node->GetName( ), mesh->GetName( ) );
        const auto deformerCount = mesh->GetDeformerCount( );
    }
}
