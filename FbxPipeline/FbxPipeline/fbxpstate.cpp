#include <fbxppch.h>
#include <fbxpstate.h>
#include <city.h>
#include <fstream>

std::string GetExecutable( );
void SplitFilename( const std::string& filePath, std::string& parentFolderName, std::string& fileName );
bool InitializeSdkObjects( FbxManager*& pManager, FbxScene*& pScene );
void DestroySdkObjects( FbxManager* pManager );
bool LoadScene( FbxManager* pManager, FbxDocument* pScene, const char* pFilename );

fbxp::State  s;
fbxp::State& fbxp::GetState( ) {
    return s;
}

fbxp::State::State( ) : console( spdlog::stdout_color_mt( "fbxp" ) ), options( GetExecutable( ) ) {
    options.add_options( "input" )( "f,file", "File", cxxopts::value< std::string >( ) );
    options.add_options( "input" )( "d,dir", "Directory", cxxopts::value< std::string >( ) );
}

fbxp::State::~State( ) {
    Release( );
}

bool fbxp::State::Initialize( ) {
    if ( !manager || !scene ) {
        return InitializeSdkObjects( manager, scene );
    }

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
    console->info( "File name {}", fileName );
    console->info( "Folder name {}", folderPath );

    return LoadScene( manager, scene, f );
}

bool fbxp::State::Finish( ) {

    //
    // Finalize names
    //

    std::vector< flatbuffers::Offset< fb::NameFb > > nameOffsets; {
        nameOffsets.reserve( nameLookup.size( ) );
        for ( auto& namePair : nameLookup ) {
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
    // Finalize scene
    //

    fbxp::fb::SceneFbBuilder sceneBuilder( builder );
    sceneBuilder.add_transforms( transformsOffset );
    sceneBuilder.add_names( namesOffset );

    builder.Finish( sceneBuilder.Finish( ) );

    //
    // Write the file
    //

    const std::string outputPath = folderPath + fileName + "." + fb::SceneFbExtension( );

    std::ofstream outputFileStream;
    outputFileStream.open( outputPath );
    if ( outputFileStream.good( ) ) {
        outputFileStream.write( (const char*) builder.GetBufferPointer( ), builder.GetSize( ) );
        outputFileStream.close( );
    } else {
        console->error( "Failed to write to output to {}", outputPath );
    }

    return true;
}

uint64_t fbxp::State::PushName( std::string const& name ) {
    const uint64_t hash = CityHash64( name.data( ), name.size( ) );
    nameLookup.insert( std::make_pair( hash, name ) );
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