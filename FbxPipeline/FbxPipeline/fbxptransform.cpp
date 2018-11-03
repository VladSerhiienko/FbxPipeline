#include <fbxppch.h>
#include <fbxpstate.h>

inline apemodefb::Vec3Fb Cast( FbxDouble3 const& d ) {
    return apemodefb::Vec3Fb{static_cast< float >( d.mData[ 0 ] ),
                             static_cast< float >( d.mData[ 1 ] ),
                             static_cast< float >( d.mData[ 2 ] )};
}


const char * GetRotationOrderString(EFbxRotationOrder e) {
    switch (e) {
    case fbxsdk::FbxEuler::eOrderXYZ: return "XYZ";
    case fbxsdk::FbxEuler::eOrderXZY: return "XZY";
    case fbxsdk::FbxEuler::eOrderYXZ: return "YXZ";
    case fbxsdk::FbxEuler::eOrderZXY: return "ZXY";
    case fbxsdk::FbxEuler::eOrderYZX: return "YZX";
    case fbxsdk::FbxEuler::eOrderZYX: return "ZYX";
    case fbxsdk::FbxEuler::eOrderSphericXYZ: return "SphericXYZ";
    default: return "<Error>";
    }
}

void ExportTransform( FbxNode* pFbxNode, apemode::Node & n ) {

    auto & s = apemode::State::Get( );

    auto eRotationOrder = pFbxNode->RotationOrder.Get();
    s.console->info( "Node \"{}\" has \"{}\" rotation order"
        , pFbxNode->GetName( )
        , GetRotationOrderString( eRotationOrder ) );
    
    apemodefb::TransformFb transform( Cast( pFbxNode->LclTranslation.Get( ) ),
                                      Cast( pFbxNode->RotationOffset.Get( ) ),
                                      Cast( pFbxNode->RotationPivot.Get( ) ),
                                      Cast( pFbxNode->PreRotation.Get( ) ),
                                      Cast( pFbxNode->PostRotation.Get( ) ),
                                      Cast( pFbxNode->LclRotation.Get( ) ),
                                      Cast( pFbxNode->ScalingOffset.Get( ) ),
                                      Cast( pFbxNode->ScalingPivot.Get( ) ),
                                      Cast( pFbxNode->LclScaling.Get( ) ),
                                      Cast( pFbxNode->GeometricTranslation.Get( ) ),
                                      Cast( pFbxNode->GeometricRotation.Get( ) ),
                                      Cast( pFbxNode->GeometricScaling.Get( ) ) );

    s.transforms.push_back( transform );
}
