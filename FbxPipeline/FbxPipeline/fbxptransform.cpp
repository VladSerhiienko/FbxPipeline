#include <fbxppch.h>
#include <fbxpstate.h>

inline fbxp::fb::vec3 cast( FbxDouble3 const& d ) {
    return fbxp::fb::vec3{static_cast< float >( d.mData[ 0 ] ),
                          static_cast< float >( d.mData[ 1 ] ),
                          static_cast< float >( d.mData[ 2 ] )};
}

void ExportTransform( FbxNode* node, fbxp::Node & n ) {
    fbxp::fb::TransformFb transform( cast( node->LclTranslation.Get( ) ),
                                     cast( node->RotationOffset.Get( ) ),
                                     cast( node->RotationPivot.Get( ) ),
                                     cast( node->PreRotation.Get( ) ),
                                     cast( node->PostRotation.Get( ) ),
                                     cast( node->LclRotation.Get( ) ),
                                     cast( node->ScalingOffset.Get( ) ),
                                     cast( node->ScalingPivot.Get( ) ),
                                     cast( node->LclScaling.Get( ) ),
                                     cast( node->GeometricTranslation.Get( ) ),
                                     cast( node->GeometricRotation.Get( ) ),
                                     cast( node->GeometricScaling.Get( ) ) );

    fbxp::Get( ).transforms.push_back( transform );
}
