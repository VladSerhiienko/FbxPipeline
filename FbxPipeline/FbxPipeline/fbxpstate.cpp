
#include <fbxppch.h>
#include <fbxpstate.h>
#include <CityHash.h>
#include <fstream>
#include <iostream>
#include <iomanip>

#include <flatbuffers/util.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

std::string CurrentDirectory( );
std::string GetExecutable( );
std::string ResolveFullPath( const char* path );
bool        MakeDirectory( const char* directory );

void SplitFilename( const std::string& filePath, std::string& parentFolderName, std::string& fileName );
bool InitializeSdkObjects( FbxManager*& pManager, FbxScene*& pScene );
void DestroySdkObjects( FbxManager* pManager );
bool LoadScene( FbxManager* pManager, FbxDocument* pScene, const char* pFilename );
void InitializeSeachLocations( );

apemode::State  s;
apemode::State& apemode::Get( ) {
    return s;
}

std::shared_ptr< spdlog::logger > CreateLogger( spdlog::level::level_enum lvl, std::string logFile ) {
    if ( logFile.empty( ) ) {

        /* This code is about to create a name for log file.
           The name should contain date and time.
           TODO: Something portable and less ugly. */

        tm currentTime;
        time_t currentSystemTime = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now( ) );
        localtime_s( &currentTime, &currentSystemTime );

        std::string curentTimeStr;
        std::stringstream curentTimeStrStream;
        curentTimeStrStream << std::put_time( &currentTime, "%F-%T-" );
        curentTimeStrStream << currentSystemTime;
        curentTimeStr = curentTimeStrStream.str( );
        std::replace( curentTimeStr.begin( ), curentTimeStr.end( ), ':', '-' );

        auto logsDirectory = ResolveFullPath( ( CurrentDirectory( ) + "/.logs" ).c_str( ) );
        MakeDirectory( logsDirectory.c_str( ) );

        // logFile = ResolveFullPath( ( logsDirectory + "/fbxp-" + curentTimeStr ).c_str( ) );
        logFile = ResolveFullPath( ( logsDirectory + "/fbxp-" + curentTimeStr + ".fbxp-log.txt" ).c_str( ) );
    }

    // std::make_shared< spdlog::sinks::simple_file_sink_mt >( logFile )
    // std::make_shared< spdlog::sinks::rotating_file_sink_mt >( logFile, "fbxp-rlog.txt", 65536, 1024 )

    std::vector< spdlog::sink_ptr > sinks{
        std::make_shared< spdlog::sinks::wincolor_stdout_sink_mt >( ),
        std::make_shared< spdlog::sinks::msvc_sink_mt >( ),
        std::make_shared< spdlog::sinks::simple_file_sink_mt >( logFile )
        /*, std::make_shared< spdlog::sinks::rotating_file_sink_mt >( logFile, "fbxp-rlog.txt", 65536, 1024 )*/};

    auto logger = spdlog::create<>( "apemode", sinks.begin( ), sinks.end( ) );
    logger->set_level( lvl );

    return logger;
}

apemode::State& apemode::Main( int argc, char** argv ) {
    try {
        s.options.parse( argc, argv );

        auto lvl = spdlog::level::info;
        if ( s.options[ "log-level" ].count( ) > 0 )
            lvl = (spdlog::level::level_enum) s.options[ "log-level" ].as< int >( );

        s.console = CreateLogger( lvl, s.options[ "l" ].as< std::string >( ) );
    } catch ( const cxxopts::OptionException& e ) {
        std::cerr << s.options.help( {"main"} ) << std::endl;
        std::cerr << "Error parsing options:" << e.what( ) << std::endl;
        std::exit( 1 );
    }

    return s;
}

apemode::State::State( ) : options( GetExecutable( ) ) {
    options.add_options( "main" )( "i,input-file", "Input", cxxopts::value< std::string >( ) );
    options.add_options( "main" )( "o,output-file", "Output", cxxopts::value< std::string >( ) );
    options.add_options( "main" )( "password", "Password", cxxopts::value< std::string >( ) );
    options.add_options( "main" )( "k,convert", "Convert", cxxopts::value< bool >( ) );
    options.add_options( "main" )( "c,compress", "Compress", cxxopts::value< bool >( ) );
    options.add_options( "main" )( "p,pack-meshes", "Pack meshes", cxxopts::value< bool >( ) );
    options.add_options( "main" )( "s,split-meshes-per-material", "Split meshes per material", cxxopts::value< bool >( ) );
    options.add_options( "main" )( "t,optimize-meshes", "Optimize meshes", cxxopts::value< bool >( ) );
    options.add_options( "main" )( "e,search-location", "Add search location", cxxopts::value< std::vector< std::string > >( ) );
    options.add_options( "main" )( "m,embed-file", "Embed file", cxxopts::value< std::vector< std::string > >( ) );
    options.add_options( "main" )( "l,log-file", "Log file (relative or absolute path)", cxxopts::value< std::string >( ) );
    options.add_options( "main" )( "log-level", "Log level: 0 (most detailed) - 6 (off)", cxxopts::value< int >( ) );
    options.add_options( "main" )( "sync-keys", "Synchronize curve keys for properties", cxxopts::value< bool >( ) );
    options.add_options( "main" )( "reduce-keys", "Reduce the keys in the animation curves.", cxxopts::value< bool >( ) );
    options.add_options( "main" )( "resample-framerate", "Frame rate at which animation curves will be resampled (60 - default, 0 - disable).", cxxopts::value< float >( ) );
}

apemode::State::~State( ) {
    Release( );
    console->flush( );
}

bool apemode::State::Initialize( ) {
    if (!manager || !scene) {
        InitializeSdkObjects( manager, scene );
        InitializeSeachLocations( );
    }

    return manager && scene;
}

void apemode::State::Release( ) {
    if ( manager ) {
        DestroySdkObjects( manager );
        manager = nullptr;
    }
}

bool apemode::State::Load( ) {
    const std::string inputFile = options[ "i" ].as< std::string >( );
    // SplitFilename( inputFile.c_str( ), folderPath, fileName );
    // logger->info( "File name  : \"{}\"", fileName );
    // logger->info( "Folder name: \"{}\"", folderPath );
    return LoadScene( manager, scene, inputFile.c_str( ) );
}

std::vector< uint8_t > ReadFile( const char* filepath );

bool apemode::State::Finish( ) {

    //
    // Finalize names
    //

    std::vector< flatbuffers::Offset<apemodefb::NameFb > > nameOffsets; {
        nameOffsets.reserve( names.size( ) );
        for ( auto& namePair : names ) {
            const auto valueOffset = builder.CreateString( namePair.second );

           apemodefb::NameFbBuilder nameBuilder( builder );
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

    std::vector< flatbuffers::Offset<apemodefb::NodeFb > > nodeOffsets; {
        nodeOffsets.reserve( nodes.size( ) );
        for ( auto& node : nodes ) {
            const auto childIdsOffset = builder.CreateVector( node.childIds );
            const auto materialIdsOffset = builder.CreateVector( node.materialIds );

           apemodefb::NodeFbBuilder nodeBuilder( builder );
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

    std::vector< flatbuffers::Offset<apemodefb::MaterialFb > > materialOffsets; {
        materialOffsets.reserve( materials.size( ) );
        for (auto& material : materials) {
            auto propsOffset = builder.CreateVectorOfStructs( material.props );

           apemodefb::MaterialFbBuilder materialBuilder( builder );
            materialBuilder.add_id( material.id );
            materialBuilder.add_name_id( material.nameId );
            materialBuilder.add_props( propsOffset );
            materialOffsets.push_back( materialBuilder.Finish( ) );
        }
    }

    //
    // Finalize meshes
    //

    std::vector< flatbuffers::Offset<apemodefb::MeshFb > > meshOffsets; {
        materialOffsets.reserve( meshes.size( ) );
        for ( auto& mesh : meshes ) {
            auto vsOffset = builder.CreateVector( mesh.vertices );
            auto smOffset = builder.CreateVectorOfStructs( mesh.submeshes );
            auto ssOffset = builder.CreateVectorOfStructs( mesh.subsets );
            auto siOffset = builder.CreateVector( mesh.indices );

            apemodefb::MeshFbBuilder meshBuilder( builder );
            meshBuilder.add_vertices( vsOffset );
            meshBuilder.add_submeshes( smOffset );
            meshBuilder.add_subsets( ssOffset );
            meshBuilder.add_indices( siOffset );
            meshBuilder.add_index_type( mesh.indexType );
            meshOffsets.push_back( meshBuilder.Finish( ) );
        }
    }

    //
    // Finalize files
    //

    /*std::vector< flatbuffers::Offset<apemodefb::FileFb > > fileOffsets; {
        fileOffsets.reserve( embedQueue.size( ) );

        std::vector< uint8_t > fileBuffer;

        for ( auto& embedded : embedQueue ) {
            fileBuffer = ReadFile( embedded.c_str( ) );
            if ( !fileBuffer.empty( ) ) {
                auto bytesOffset = builder.CreateVectorOfStructs( fileBuffer );
                fileOffsets.push_back(apemodefb::CreateFileFbDirect( builder, (uint32_t) fileOffsets.size( ), 0, &fileBuffer ) );
            }
        }
    }*/

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
    // Finalize files
    //

    //const auto filesOffset = builder.CreateVector( fileOffsets );

    //
    // Finalize scene
    //

    apemodefb::SceneFbBuilder sceneBuilder( builder );
    sceneBuilder.add_transforms( transformsOffset );
    sceneBuilder.add_names( namesOffset );
    sceneBuilder.add_nodes( nodesOffset );
    sceneBuilder.add_meshes( meshesOffset );
    sceneBuilder.add_textures( texturesOffset );
    sceneBuilder.add_materials( materialsOffset );
    //sceneBuilder.add_files( filesOffset );

    apemodefb::FinishSceneFbBuffer( builder, sceneBuilder.Finish( ) );

    //
    // Write the file
    //

    flatbuffers::Verifier v( builder.GetBufferPointer( ), builder.GetSize( ) );
    assert( apemodefb::VerifySceneFbBuffer( v ) );

    std::string output = options[ "o" ].as< std::string >( );
    if ( output.empty( ) ) {
        output = folderPath + fileName + "." +apemodefb::SceneFbExtension( );
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

uint64_t apemode::State::PushName( std::string const& name ) {
    const uint64_t hash = CityHash64( name.data( ), name.size( ) );

#if _DEBUG
    auto it = names.find( hash );
    if ( it == names.end( ) ) {
        console->trace( "Adding {} -> {}", hash, name );
    }
#endif

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
    bool lStatus;
    char lPassword[ 1024 ];

    // Get the file version number generate by the FBX SDK.
    FbxManager::GetFileFormatVersion( lSDKMajor, lSDKMinor, lSDKRevision );
    s.console->info( "FBX SDK is {}.{}.{}.", lSDKMajor, lSDKMinor, lSDKRevision );

    // Create an importer.
    FbxImporter* lImporter = FbxImporter::Create( pManager, "" );

    // Initialize the importer by providing a filename.
    const bool lImportStatus = lImporter->Initialize( pFilename, -1, pManager->GetIOSettings( ) );
    lImporter->GetFileVersion( lFileMajor, lFileMinor, lFileRevision );

    if ( !lImportStatus ) {
        FbxString error = lImporter->GetStatus( ).GetErrorString( );
        s.console->error( "Call to FbxImporter::Initialize() failed." );
        s.console->error( "Error returned: {}.", error.Buffer( ) );

        if ( lImporter->GetStatus( ).GetCode( ) == FbxStatus::eInvalidFileVersion ) {
            s.console->error( "FBX SDK is {}.{}.{}.", lSDKMajor, lSDKMinor, lSDKRevision );
            s.console->error( "\"{}\" is {}.{}.{}.", pFilename, lFileMajor, lFileMinor, lFileRevision );
        }

        DebugBreak( );
        return false;
    }


    if ( lImporter->IsFBX( ) ) {
        s.console->info( "\"{}\" is {}.{}.{}.", pFilename, lFileMajor, lFileMinor, lFileRevision );

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