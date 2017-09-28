#include <fbxppch.h>
#include <fbxpstate.h>

void ExportCamera( FbxNode* pNode, apemode::Node& n ) {
    auto& s = apemode::Get( );

    if ( auto pCamera = pNode->GetCamera( ) ) {
        float width, height;
        switch ( const auto format = pCamera->CameraFormat.Get( ) ) {
            case fbxsdk::FbxCamera::eCustomFormat: {
                switch ( const auto aspectRatioMode = pCamera->AspectRatioMode.Get( ) ) {
                    case fbxsdk::FbxCamera::eWindowSize:
                        width  = 0;
                        height = 0;
                        break;
                    case fbxsdk::FbxCamera::eFixedRatio:
                        width  = (float) pCamera->AspectWidth.Get( );
                        height = 1;
                        break;
                    case fbxsdk::FbxCamera::eFixedResolution:
                        width  = (float) pCamera->AspectWidth.Get( );
                        height = (float) pCamera->AspectHeight.Get( );
                        break;
                    case fbxsdk::FbxCamera::eFixedWidth:
                        width  = (float) pCamera->AspectWidth.Get( );
                        height = 0;
                        break;
                    case fbxsdk::FbxCamera::eFixedHeight:
                        width  = 0;
                        height = (float) pCamera->AspectHeight.Get( );
                        break;
                    default:
                        assert( false );
                        break;
                }
            } break;
            case fbxsdk::FbxCamera::eD1NTSC: {
                width  = 720;
                height = 486;
            } break;
            case fbxsdk::FbxCamera::eNTSC:
            case fbxsdk::FbxCamera::e640x480: {
                width  = 640;
                height = 480;
            } break;
            case fbxsdk::FbxCamera::ePAL: {
                width  = 570;
                height = 486;
            } break;
            case fbxsdk::FbxCamera::eD1PAL: {
                width  = 720;
                height = 576;
            } break;
            case fbxsdk::FbxCamera::eHD: {
                width  = 1920;
                height = 1080;
            } break;
            case fbxsdk::FbxCamera::e320x200: {
                width  = 320;
                height = 200;
            } break;
            case fbxsdk::FbxCamera::e320x240: {
                width  = 320;
                height = 240;
            } break;
            case fbxsdk::FbxCamera::e128x128: {
                width  = 128;
                height = 128;
            } break;
            case fbxsdk::FbxCamera::eFullscreen: {
                width  = 1280;
                height = 1024;
            } break;
            default:
                assert( false );
                break;
        }

        uint32_t cameraId = (uint32_t) s.cameras.size( );
        n.cameraId = cameraId;

        s.cameras.emplace_back( );
        auto& camera = s.cameras.back( );

        camera.mutate_id( cameraId );
        apemode::Mutable( camera.mutable_aspect( ) ).mutate_x( width );
        apemode::Mutable( camera.mutable_aspect( ) ).mutate_y( height );
        apemode::Mutable( camera.mutable_up( ) ).mutate_x( (float) pCamera->UpVector.Get( )[ 0 ] );
        apemode::Mutable( camera.mutable_up( ) ).mutate_y( (float) pCamera->UpVector.Get( )[ 1 ] );
        apemode::Mutable( camera.mutable_up( ) ).mutate_z( (float) pCamera->UpVector.Get( )[ 2 ] );

        if ( pCamera->GetName( ) && 0 != strlen( pCamera->GetName( ) ) )
            camera.mutate_name_id( s.PushName( pCamera->GetName( ) ) );
        else
            camera.mutate_name_id( s.PushName( pNode->GetName( ) ) );
    }
}
