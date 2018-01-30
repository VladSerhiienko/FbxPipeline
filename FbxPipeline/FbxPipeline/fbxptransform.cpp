#include <fbxppch.h>
#include <fbxpstate.h>

inline apemodefb::Vec3Fb Cast( FbxDouble3 const& d ) {
    return apemodefb::Vec3Fb{static_cast< float >( d.mData[ 0 ] ),
                             static_cast< float >( d.mData[ 1 ] ),
                             static_cast< float >( d.mData[ 2 ] )};
}

void ExportTransform( FbxNode* node, apemode::Node & n ) {
    apemodefb::TransformFb transform( Cast( node->LclTranslation.Get( ) ),
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

    apemode::Get( ).transforms.push_back( transform );
}
