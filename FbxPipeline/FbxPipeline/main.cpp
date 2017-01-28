
//
// STL
//

#include <iostream>
#include <memory>

//
// ThirdParty
//

#include <flatbuffers/flatbuffers.h>
#include <spdlog/spdlog.h>
#include <cxxopts.hpp>

//
// Generated
//

#include <scene_generated.h>

//
// FBX SDK
//

#include <fbxsdk.h>

bool InitializeSdkObjects( FbxManager*& pManager, FbxScene*& pScene );
void DestroySdkObjects( FbxManager* pManager );
bool LoadScene( FbxManager* pManager, FbxDocument* pScene, const char* pFilename );
void ExportScene( FbxScene* pScene );
void ExportNode( FbxNode* pNode );
void ExportNodes( FbxNode* pNode );

//
//
//

fbxsdk::FbxManager*               manager = nullptr;
fbxsdk::FbxScene*                 scene   = nullptr;
std::shared_ptr< spdlog::logger > console = spdlog::stdout_color_mt( "fbxp" );
flatbuffers::FlatBufferBuilder    builder;
std::vector<fbxp::fb::TransformFb> transforms;

int main( int argc, char** argv ) {
    std::string file; {
        try {
            cxxopts::Options options( argv[ 0 ] );
            options.add_options( "input" )( "f,file", "File", cxxopts::value< std::string >( ) );
            options.add_options( "input" )( "d,dir", "Directory", cxxopts::value< std::string >( ) );
            options.parse( argc, argv );
            file = options[ "f" ].as< std::string >( );
        } catch ( const cxxopts::OptionException& e ) {
            console->critical( "error parsing options: {0}", e.what( ) );
            std::exit( 1 );
        }
    }

    if ( InitializeSdkObjects( manager, scene ) ) {
        if ( LoadScene( manager, scene, file.c_str( ) ) ) {
            ExportScene( scene );

            auto transformsOffset = builder.CreateVectorOfStructs( transforms );
            fbxp::fb::SceneFbBuilder sceneBuilder(builder);
            sceneBuilder.add_transforms( transformsOffset );

            auto sceneOffset = sceneBuilder.Finish();
            builder.Finish( sceneOffset );
        }

        DestroySdkObjects( manager );
    }

    return 0;
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
        console->error( "Unable to create FBX Manager." );
        return false;
    }

    console->info( "Autodesk FBX SDK version {}.", pManager->GetVersion( ) );

    // Create an IOSettings object. This object holds all import/export settings.
    FbxIOSettings* ios = FbxIOSettings::Create( pManager, IOSROOT );
    pManager->SetIOSettings( ios );

    // Load plugins from the executable directory (optional)
    FbxString lPath = FbxGetApplicationDirectory( );
    console->info( "Application directory {}.", lPath.Buffer( ) );
    pManager->LoadPluginsDirectory( lPath.Buffer( ) );

    // Create an FBX scene. This object holds most objects imported/exported from/to files.
    pScene = FbxScene::Create( pManager, "" );
    if ( !pScene ) {
        console->error( "Unable to create FBX scene." );
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
        console->info( "Call to FbxImporter::Initialize() failed." );
        console->info( "Error returned: {}.", error.Buffer( ) );

        if ( lImporter->GetStatus( ).GetCode( ) == FbxStatus::eInvalidFileVersion ) {
            console->info( "FBX file format version for this FBX SDK is {}.{}.{}.", lSDKMajor, lSDKMinor, lSDKRevision );
            console->info( "FBX file format version for file '{}' is {}.{}.{}.", pFilename, lFileMajor, lFileMinor, lFileRevision );
        }

        return false;
    }

    console->info( "FBX file format version for this FBX SDK is {}.{}.{}.", lSDKMajor, lSDKMinor, lSDKRevision );

    if ( lImporter->IsFBX( ) ) {
        console->info(
            "FBX file format version for file '{}' is {}.{}.{}.", pFilename, lFileMajor, lFileMinor, lFileRevision );

        // From this point, it is possible to access animation stack information without
        // the expense of loading the entire file.

        console->info( "Animation Stack Information" );

        lAnimStackCount = lImporter->GetAnimStackCount( );

        console->info( "    Number of Animation Stacks: {}.", lAnimStackCount );
        console->info( "    Current Animation Stack: \"{}\"", lImporter->GetActiveAnimStackName( ).Buffer( ) );
        console->info( "" );

        for ( i = 0; i < lAnimStackCount; i++ ) {
            FbxTakeInfo* lTakeInfo = lImporter->GetTakeInfo( i );

            console->info( "    Animation Stack {}", i );
            console->info( "         Name: \"{}\"", lTakeInfo->mName.Buffer( ) );
            console->info( "         Description: \"{}\"", lTakeInfo->mDescription.Buffer( ) );

            // Change the value of the import name if the animation stack should be imported
            // under a different name.
            console->info( "         Import Name: \"{}\"", lTakeInfo->mImportName.Buffer( ) );

            // Set the value of the import state to false if the animation stack should be not
            // be imported.
            console->info( "         Import State: {}", lTakeInfo->mSelect ? "true" : "false" );
            console->info( "" );
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
        console->info( "Please enter password: " );

        lPassword[ 0 ] = '\0';

        FBXSDK_CRT_SECURE_NO_WARNING_BEGIN
        scanf( "%s", lPassword );
        FBXSDK_CRT_SECURE_NO_WARNING_END

        FbxString lString( lPassword );

        IOS_REF.SetStringProp( IMP_FBX_PASSWORD, lString );
        IOS_REF.SetBoolProp( IMP_FBX_PASSWORD_ENABLE, true );

        lStatus = lImporter->Import( pScene );

        if ( lStatus == false && lImporter->GetStatus( ).GetCode( ) == FbxStatus::ePasswordError ) {
            console->info( "\nPassword is wrong, import aborted." );
        }
    }

    // Destroy the importer.
    lImporter->Destroy( );

    return lStatus;
}

#pragma endregion

inline fbxp::fb::vec3 Cast( FbxDouble3 const& d ) {
    return fbxp::fb::vec3{d.mData[ 0 ], d.mData[ 1 ], d.mData[ 2 ]};
}

void ExportNode( FbxNode* node ) {
    fbxp::fb::TransformFb transform( Cast( node->LclTranslation.Get( ) ),
                                     Cast( node->RotationOffset.Get( ) ),
                                     Cast( node->RotationPivot.Get( ) ),
                                     Cast( node->PreRotation.Get( ) ),
                                     Cast( node->PostRotation.Get( ) ),
                                     Cast( node->LclRotation.Get( ) ),
                                     Cast( node->ScalingOffset.Get( ) ),
                                     Cast( node->ScalingPivot.Get( ) ),
                                     Cast( node->LclScaling.Get( ) ),
                                     Cast( node->GeometricTranslation.Get( ) ),
                                     Cast( node->GeometricRotation.Get( ) ),
                                     Cast( node->GeometricScaling.Get( ) ) );

    transforms.push_back(transform);


    console->info( "Node {}", node->GetName() );
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