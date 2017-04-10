#include <fbxppch.h>
#include <fbxpstate.h>

void ExportScene( FbxScene* pScene );
void ConvertScene( FbxManager* lSdkManager, FbxScene* lScene, FbxString lFilePath );

int main( int argc, char** argv ) {
    auto& s = fbxp::Get( );

    bool convert = false;

    try {
        s.options.parse( argc, argv );
        convert = s.options[ "k" ].as< bool >( );
    } catch ( const cxxopts::OptionException& e ) {
        s.console->critical( "error parsing options: {0}", e.what( ) );
        std::exit( 1 );
    }

    if ( s.Initialize( ) ) {
        if ( s.Load( ) ) {
            if ( convert )
                ConvertScene( s.manager, s.scene, s.options[ "i" ].as< std::string >( ).c_str( ) );
            else {
                ExportScene( s.scene );
                s.Finish( );
            }
        }
    }

    return 0;
}

void ConvertScene( FbxManager* lSdkManager, FbxScene* lScene, FbxString lFilePath ) {
    auto& s = fbxp::Get( );

    const char* lFileTypes[] = {
        "_fbx7ascii.fbx", "FBX ascii (*.fbx)", "_fbx6ascii.fbx", "FBX 6.0 ascii (*.fbx)", "_obj.obj", "Alias OBJ (*.obj)",
        //"_dae.dae",           "Collada DAE (*.dae)",
        //"_fbx7binary.fbx",     "FBX binary (*.fbx)",
        //"_fbx6binary.fbx", "FBX 6.0 binary (*.fbx)",
        //"_dxf.dxf",           "AutoCAD DXF (*.dxf)"
    };

    const size_t lFileTypeCount = sizeof( lFileTypes ) / sizeof( lFileTypes[ 0 ] ) / 2;

    for ( size_t i = 0; i < lFileTypeCount; ++i ) {
        // Retrieve the writer ID according to the description of file format.
        int lFormat = lSdkManager->GetIOPluginRegistry( )->FindWriterIDByDescription( lFileTypes[ i * 2 + 1 ] );

        // Construct the output file name.
        FbxString lNewFileName = lFilePath + lFileTypes[ i * 2 ];

        // Create an exporter.
        FbxExporter* lExporter = FbxExporter::Create( lSdkManager, "" );

        //
        lSdkManager->GetIOSettings( )->SetBoolProp( EXP_FBX_EMBEDDED, true );
        //

        // Initialize the exporter.
        int lResult = lExporter->Initialize( lNewFileName, lFormat, lSdkManager->GetIOSettings( ) );
        if ( lResult ) {
            // Export the scene.
            lResult = lExporter->Export( lScene );
            if ( lResult ) {
                s.console->info( "Exported {} {}.\n", lNewFileName.Buffer( ), lFileTypes[ i * 2 + 1 ] );
            }
        }

        // Destroy the exporter.
        lExporter->Destroy( );
    }
}