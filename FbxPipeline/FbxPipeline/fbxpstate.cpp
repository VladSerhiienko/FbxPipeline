
#include <fbxppch.h>
#include <fbxpstate.h>
#include <city.h>
#include <fstream>
#include <flatbuffers/util.h>

std::string GetExecutable( );
void SplitFilename( const std::string& filePath, std::string& parentFolderName, std::string& fileName );
bool InitializeSdkObjects( FbxManager*& pManager, FbxScene*& pScene );
void DestroySdkObjects( FbxManager* pManager );
bool LoadScene( FbxManager* pManager, FbxDocument* pScene, const char* pFilename );

fbxp::State  s;
fbxp::State& fbxp::Get( ) {
    return s;
}

fbxp::State::State( ) : console( spdlog::stdout_color_mt( "fbxp" ) ), options( GetExecutable( ) ) {
    options.add_options( "input" )( "i,input-file", "Input", cxxopts::value< std::string >( ) );
    options.add_options( "input" )( "o,output-file", "Output", cxxopts::value< std::string >( ) );
    options.add_options( "input" )( "k,convert", "Convert", cxxopts::value< bool >( ) );
    options.add_options( "input" )( "p,pack-meshes", "Pack meshes", cxxopts::value< bool >( ) );
    options.add_options( "input" )( "s,split-meshes-per-material", "Split meshes per material", cxxopts::value< bool >( ) );
}

fbxp::State::~State( ) {
    Release( );
}

bool fbxp::State::Initialize( ) {
    if ( !manager || !scene )
        InitializeSdkObjects( manager, scene );
    return manager && scene;
}

void fbxp::State::Release( ) {
    if ( manager ) {
        DestroySdkObjects( manager );
        manager = nullptr;
    }
}

bool fbxp::State::Load( const char* f ) {
    SplitFilename( f, folderPath, fileName );
    console->info( "File name  : \"{}\"", fileName );
    console->info( "Folder name: \"{}\"", folderPath );
    return LoadScene( manager, scene, f );
}

bool fbxp::State::Finish( ) {

    //
    // Finalize names
    //

    std::vector< flatbuffers::Offset< fb::NameFb > > nameOffsets; {
        nameOffsets.reserve( names.size( ) );
        for ( auto& namePair : names ) {
            const auto valueOffset = builder.CreateString( namePair.second );

            fb::NameFbBuilder nameBuilder( builder );
            nameBuilder.add_h( namePair.first );
            nameBuilder.add_v( valueOffset );
            nameOffsets.push_back( nameBuilder.Finish( ) );
        }
    }

    const auto namesOffset = builder.CreateVector( nameOffsets );

    //
    // Finalize transforms
    //

    const auto transformsOffset = builder.CreateVectorOfStructs( transforms );

    //
    // Finalize nodes
    //

    std::vector< flatbuffers::Offset< fb::NodeFb > > nodeOffsets; {
        nodeOffsets.reserve( nodes.size( ) );
        for ( auto& node : nodes ) {
            const auto childIdsOffset = builder.CreateVector( node.childIds );
            const auto materialIdsOffset = builder.CreateVector( node.materialIds );

            fb::NodeFbBuilder nodeBuilder( builder );
            nodeBuilder.add_id( node.id );
            nodeBuilder.add_name_id( node.nameId );
            nodeBuilder.add_culling_type( node.cullingType );
            nodeBuilder.add_mesh_id( node.meshId );
            nodeBuilder.add_child_ids( childIdsOffset );
            nodeBuilder.add_material_ids( materialIdsOffset );
            nodeOffsets.push_back( nodeBuilder.Finish( ) );
        }
    }

    const auto nodesOffset = builder.CreateVector( nodeOffsets );

    //
    // Finalize materials
    // 

    std::vector< flatbuffers::Offset< fb::MaterialFb > > materialOffsets; {
        materialOffsets.reserve( materials.size( ) );
        for (auto& material : materials) {
            auto propsOffset = builder.CreateVectorOfStructs( material.props );

            fb::MaterialFbBuilder materialBuilder( builder );
            materialBuilder.add_id( material.id );
            materialBuilder.add_name_id( material.nameId );
            materialBuilder.add_props( propsOffset );
            materialOffsets.push_back( materialBuilder.Finish( ) );
        }
    }

    //
    // Finalize meshes
    //

    std::vector< flatbuffers::Offset< fb::MeshFb > > meshOffsets; {
        materialOffsets.reserve( meshes.size( ) );
        for ( auto& mesh : meshes ) {
            auto vsOffset = builder.CreateVector( mesh.vertices );
            auto smOffset = builder.CreateVectorOfStructs( mesh.submeshes );
            auto ssOffset = builder.CreateVectorOfStructs( mesh.subsets );
            // auto iiOffset = builder.CreateVector( mesh.indices );
            auto siOffset = builder.CreateVector( mesh.subsetIndices );
            // auto spOffset = builder.CreateVectorOfStructs( mesh.subsetsPolies );

            fb::MeshFbBuilder meshBuilder( builder );
            meshBuilder.add_vertices( vsOffset );
            meshBuilder.add_submeshes( smOffset );
            meshBuilder.add_subsets( ssOffset );
            // meshBuilder.add_indices( iiOffset );
            // meshBuilder.add_subset_polies( spOffset );
            meshBuilder.add_subset_indices( siOffset );
            meshBuilder.add_subset_index_type( mesh.subsetIndexType );
            meshOffsets.push_back( meshBuilder.Finish( ) );
        }
    }

    const auto meshesOffset = builder.CreateVector( meshOffsets );

    //
    // Finalize Materials
    //

    const auto materialsOffset = builder.CreateVector( materialOffsets );

    //
    // Finalize textures
    //

    const auto texturesOffset = builder.CreateVectorOfStructs( textures );

    //
    // Finalize scene
    //

    fbxp::fb::SceneFbBuilder sceneBuilder( builder );
    sceneBuilder.add_transforms( transformsOffset );
    sceneBuilder.add_names( namesOffset );
    sceneBuilder.add_nodes( nodesOffset );
    sceneBuilder.add_meshes( meshesOffset );
    sceneBuilder.add_textures( texturesOffset );
    sceneBuilder.add_materials( materialsOffset );

    fbxp::fb::FinishSceneFbBuffer( builder, sceneBuilder.Finish( ) );

    //
    // Write the file
    //

    flatbuffers::Verifier v( builder.GetBufferPointer( ), builder.GetSize( ) );
    assert( fbxp::fb::VerifySceneFbBuffer( v ) );

    std::string output = options[ "o" ].as< std::string >( );
    if ( output.empty( ) ) {
        output = folderPath + fileName + "." + fb::SceneFbExtension( );
    } else {
        std::string outputFolder, outputFileName;
        SplitFilename( output, outputFolder, outputFileName );
        (void) outputFileName;
        CreateDirectoryA( outputFolder.c_str( ), 0 );
    }

    if ( flatbuffers::SaveFile(output.c_str( ), (const char*) builder.GetBufferPointer( ), (size_t) builder.GetSize( ), true ) ) {
        return true;
    }

    console->error( "Failed to write to output to {}", output );
    DebugBreak( );
    return false;
}

uint64_t fbxp::State::PushName( std::string const& name ) {
    const uint64_t hash = CityHash64( name.data( ), name.size( ) );
    names.insert( std::make_pair( hash, name ) );
    return hash;
}

#pragma region FBX SDK Initialization

//
// The code is taken from the latest FBX SDK samples
// with some adapting changes.
//

#ifdef IOS_REF
#undef IOS_REF
#define IOS_REF ( *( pManager->GetIOSettings( ) ) )
#endif

bool InitializeSdkObjects( FbxManager*& pManager, FbxScene*& pScene ) {
    // The first thing to do is to create the FBX Manager which is the object allocator for almost all the classes in the SDK
    pManager = FbxManager::Create( );
    if ( !pManager ) {
        s.console->error( "Unable to create FBX Manager." );
        return false;
    }

    s.console->info( "Autodesk FBX SDK version {}.", pManager->GetVersion( ) );

    // Create an IOSettings object. This object holds all import/export settings.
    FbxIOSettings* ios = FbxIOSettings::Create( pManager, IOSROOT );
    pManager->SetIOSettings( ios );

    // Load plugins from the executable directory (optional)
    FbxString lPath = FbxGetApplicationDirectory( );
    s.console->info( "Application directory {}.", lPath.Buffer( ) );
    pManager->LoadPluginsDirectory( lPath.Buffer( ) );

    // Create an FBX scene. This object holds most objects imported/exported from/to files.
    pScene = FbxScene::Create( pManager, "" );
    if ( !pScene ) {
        s.console->error( "Unable to create FBX scene." );
        DestroySdkObjects( pManager );
        DebugBreak( );
        return false;
    }

    return true;
}

void DestroySdkObjects( FbxManager* pManager ) {
    // Delete the FBX Manager. All the objects that have been allocated using the FBX Manager and that haven't been explicitly
    // destroyed are also automatically destroyed.
    if ( pManager )
        pManager->Destroy( );
}

bool LoadScene( FbxManager* pManager, FbxDocument* pScene, const char* pFilename ) {
    int lFileMajor, lFileMinor, lFileRevision;
    int lSDKMajor, lSDKMinor, lSDKRevision;
    // int lFileFormat = -1;
    int  i, lAnimStackCount;
    bool lStatus;
    char lPassword[ 1024 ];

    // Get the file version number generate by the FBX SDK.
    FbxManager::GetFileFormatVersion( lSDKMajor, lSDKMinor, lSDKRevision );

    // Create an importer.
    FbxImporter* lImporter = FbxImporter::Create( pManager, "" );

    // Initialize the importer by providing a filename.
    const bool lImportStatus = lImporter->Initialize( pFilename, -1, pManager->GetIOSettings( ) );
    lImporter->GetFileVersion( lFileMajor, lFileMinor, lFileRevision );

    if ( !lImportStatus ) {
        FbxString error = lImporter->GetStatus( ).GetErrorString( );
        s.console->info( "Call to FbxImporter::Initialize() failed." );
        s.console->info( "Error returned: {}.", error.Buffer( ) );

        if ( lImporter->GetStatus( ).GetCode( ) == FbxStatus::eInvalidFileVersion ) {
            s.console->info( "FBX file format version for this FBX SDK is {}.{}.{}.", lSDKMajor, lSDKMinor, lSDKRevision );
            s.console->info( "FBX file format version for file '{}' is {}.{}.{}.", pFilename, lFileMajor, lFileMinor, lFileRevision );
        }

        DebugBreak( );
        return false;
    }

    s.console->info( "FBX file format version for this FBX SDK is {}.{}.{}.", lSDKMajor, lSDKMinor, lSDKRevision );

    if ( lImporter->IsFBX( ) ) {
        s.console->info(
            "FBX file format version for file '{}' is {}.{}.{}.", pFilename, lFileMajor, lFileMinor, lFileRevision );

        // From this point, it is possible to access animation stack information without
        // the expense of loading the entire file.

        s.console->info( "Animation Stack Information" );

        lAnimStackCount = lImporter->GetAnimStackCount( );

        s.console->info( "    Number of Animation Stacks: {}.", lAnimStackCount );
        s.console->info( "    Current Animation Stack: \"{}\"", lImporter->GetActiveAnimStackName( ).Buffer( ) );
        s.console->info( "" );

        for ( i = 0; i < lAnimStackCount; i++ ) {
            FbxTakeInfo* lTakeInfo = lImporter->GetTakeInfo( i );

            s.console->info( "    Animation Stack {}", i );
            s.console->info( "         Name: \"{}\"", lTakeInfo->mName.Buffer( ) );
            s.console->info( "         Description: \"{}\"", lTakeInfo->mDescription.Buffer( ) );

            // Change the value of the import name if the animation stack should be imported
            // under a different name.
            s.console->info( "         Import Name: \"{}\"", lTakeInfo->mImportName.Buffer( ) );

            // Set the value of the import state to false if the animation stack should be not
            // be imported.
            s.console->info( "         Import State: {}", lTakeInfo->mSelect ? "true" : "false" );
            s.console->info( "" );
        }

        // Set the import states. By default, the import states are always set to
        // true. The code below shows how to change these states.
        IOS_REF.SetBoolProp( IMP_FBX_MATERIAL, true );
        IOS_REF.SetBoolProp( IMP_FBX_TEXTURE, true );
        IOS_REF.SetBoolProp( IMP_FBX_LINK, true );
        IOS_REF.SetBoolProp( IMP_FBX_SHAPE, true );
        IOS_REF.SetBoolProp( IMP_FBX_GOBO, true );
        IOS_REF.SetBoolProp( IMP_FBX_ANIMATION, true );
        IOS_REF.SetBoolProp( IMP_FBX_GLOBAL_SETTINGS, true );
    }

    // Import the scene.
    lStatus = lImporter->Import( pScene );

    if ( lStatus == false && lImporter->GetStatus( ).GetCode( ) == FbxStatus::ePasswordError ) {
        s.console->info( "Please enter password: " );

        lPassword[ 0 ] = '\0';

        FBXSDK_CRT_SECURE_NO_WARNING_BEGIN
        scanf( "%s", lPassword );
        FBXSDK_CRT_SECURE_NO_WARNING_END

        FbxString lString( lPassword );

        IOS_REF.SetStringProp( IMP_FBX_PASSWORD, lString );
        IOS_REF.SetBoolProp( IMP_FBX_PASSWORD_ENABLE, true );

        lStatus = lImporter->Import( pScene );

        if ( lStatus == false && lImporter->GetStatus( ).GetCode( ) == FbxStatus::ePasswordError ) {
            s.console->info( "\nPassword is wrong, import aborted." );
        }
    }

    // Destroy the importer.
    lImporter->Destroy( );

    return lStatus;
}

bool SaveScene( FbxManager* pManager, FbxDocument* pScene, const char* pFilename, int pFileFormat, bool pEmbedMedia ) {
    int  lMajor, lMinor, lRevision;
    bool lStatus = true;

    // Create an exporter.
    FbxExporter* lExporter = FbxExporter::Create( pManager, "" );

    if ( pFileFormat < 0 || pFileFormat >= pManager->GetIOPluginRegistry( )->GetWriterFormatCount( ) ) {
        // Write in fall back format in less no ASCII format found
        pFileFormat = pManager->GetIOPluginRegistry( )->GetNativeWriterFormat( );

        // Try to export in ASCII if possible
        int lFormatIndex, lFormatCount = pManager->GetIOPluginRegistry( )->GetWriterFormatCount( );

        for ( lFormatIndex = 0; lFormatIndex < lFormatCount; lFormatIndex++ ) {
            if ( pManager->GetIOPluginRegistry( )->WriterIsFBX( lFormatIndex ) ) {
                FbxString   lDesc  = pManager->GetIOPluginRegistry( )->GetWriterFormatDescription( lFormatIndex );
                const char* lASCII = "ascii";
                if ( lDesc.Find( lASCII ) >= 0 ) {
                    pFileFormat = lFormatIndex;
                    break;
                }
            }
        }
    }

    // Set the export states. By default, the export states are always set to
    // true except for the option eEXPORT_TEXTURE_AS_EMBEDDED. The code below
    // shows how to change these states.
    IOS_REF.SetBoolProp( EXP_FBX_MATERIAL, true );
    IOS_REF.SetBoolProp( EXP_FBX_TEXTURE, true );
    IOS_REF.SetBoolProp( EXP_FBX_EMBEDDED, pEmbedMedia );
    IOS_REF.SetBoolProp( EXP_FBX_SHAPE, true );
    IOS_REF.SetBoolProp( EXP_FBX_GOBO, true );
    IOS_REF.SetBoolProp( EXP_FBX_ANIMATION, true );
    IOS_REF.SetBoolProp( EXP_FBX_GLOBAL_SETTINGS, true );

    // Initialize the exporter by providing a filename.
    if ( lExporter->Initialize( pFilename, pFileFormat, pManager->GetIOSettings( ) ) == false ) {
        FBXSDK_printf( "Call to FbxExporter::Initialize() failed.\n" );
        FBXSDK_printf( "Error returned: %s\n\n", lExporter->GetStatus( ).GetErrorString( ) );
        return false;
    }

    FbxManager::GetFileFormatVersion( lMajor, lMinor, lRevision );
    FBXSDK_printf( "FBX file format version %d.%d.%d\n\n", lMajor, lMinor, lRevision );

    // Export the scene.
    lStatus = lExporter->Export( pScene );

    // Destroy the exporter.
    lExporter->Destroy( );
    return lStatus;
}

#pragma endregion

#pragma region Utils

std::string GetExecutable( ) {
    char szFileName[ 1024 ];
    GetModuleFileNameA( NULL, szFileName, 1024 );
    return szFileName;
}

void SplitFilename( const std::string& filePath, std::string& parentFolderName, std::string& fileName ) {
    using namespace std;

    const size_t found = filePath.find_last_of( "/\\" );
    if ( found != filePath.npos ) {
        parentFolderName = filePath.substr( 0, found + 1 );
        fileName         = filePath.substr( found + 1 );
    } else {
        parentFolderName = "/";
        fileName         = filePath;
    }
}

#pragma endregion