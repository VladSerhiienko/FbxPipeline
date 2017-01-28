#include <fbxppch.h>
#include <fbxpstate.h>
#include <scene_generated.h>

void ExportScene( FbxScene* pScene );
void ExportNodes( FbxNode* pNode );
void ExportNode( FbxNode* pNode );
void ExportTransform( FbxNode* node );

int main( int argc, char** argv ) {
    std::string file;
    try {
        fbxp::GetState( ).options.parse( argc, argv );
        file = fbxp::GetState( ).options[ "f" ].as< std::string >( );
    } catch ( const cxxopts::OptionException& e ) {
        fbxp::GetState( ).console->critical( "error parsing options: {0}", e.what( ) );
        std::exit( 1 );
    }

    if ( fbxp::GetState( ).Initialize( ) ) {
        if ( fbxp::GetState( ).Load( file.c_str( ) ) ) {
            ExportScene( fbxp::GetState( ).scene );
            fbxp::GetState( ).Finish( );
        }
    }

    return 0;
}

void ExportNode( FbxNode* node ) {
    const auto nameId = fbxp::GetState( ).PushName( node->GetName( ) );
    ExportTransform( node );
}

void ExportNodes( FbxNode* node ) {
    ExportNode( node );
    for ( auto i = 0; i < node->GetChildCount( ); ++i ) {
        ExportNodes( node->GetChild( i ) );
    }
}

void ExportScene( FbxScene* scene ) {
    ExportNodes( scene->GetRootNode( ) );
}