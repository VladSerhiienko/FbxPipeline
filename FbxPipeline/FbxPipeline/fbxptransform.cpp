#include <fbxppch.h>
#include <fbxpstate.h>

inline apemodefb::Vec3Fb Cast( const FbxDouble3& d ) {
    return apemodefb::Vec3Fb{static_cast< float >( d.mData[ 0 ] ),
                             static_cast< float >( d.mData[ 1 ] ),
                             static_cast< float >( d.mData[ 2 ] )};
}

inline void PopulateTransformLimits( const bool          activeX,
                                     const bool          activeY,
                                     const bool          activeZ,
                                     const FbxDouble3&   src,
                                     apemodefb::Vec3Fb&  dst,
                                     apemodefb::Bool3Fb& activeXYZ ) {
    activeXYZ.mutate_x( activeX );
    activeXYZ.mutate_y( activeY );
    activeXYZ.mutate_z( activeZ );

    if ( activeX ) {
        dst.mutate_x( static_cast< float >( src.mData[ 0 ] ) );
    }
    if ( activeY ) {
        dst.mutate_y( static_cast< float >( src.mData[ 1 ] ) );
    }
    if ( activeZ ) {
        dst.mutate_z( static_cast< float >( src.mData[ 2 ] ) );
    }
}

void PopulateTransformLimits( FbxNode* pFbxNode, apemodefb::TransformLimitsFb& limits ) {
    PopulateTransformLimits( pFbxNode->TranslationMaxX.Get( ),
                             pFbxNode->TranslationMaxY.Get( ),
                             pFbxNode->TranslationMaxZ.Get( ),
                             pFbxNode->TranslationMax.Get( ),
                             limits.mutable_translation_max( ),
                             limits.mutable_translation_max_active( ) );

    PopulateTransformLimits( pFbxNode->TranslationMinX.Get( ),
                             pFbxNode->TranslationMinY.Get( ),
                             pFbxNode->TranslationMinZ.Get( ),
                             pFbxNode->TranslationMin.Get( ),
                             limits.mutable_translation_min( ),
                             limits.mutable_translation_min_active( ) );

    PopulateTransformLimits( pFbxNode->RotationMaxX.Get( ),
                             pFbxNode->RotationMaxY.Get( ),
                             pFbxNode->RotationMaxZ.Get( ),
                             pFbxNode->RotationMax.Get( ),
                             limits.mutable_rotation_max( ),
                             limits.mutable_rotation_max_active( ) );

    PopulateTransformLimits( pFbxNode->RotationMinX.Get( ),
                             pFbxNode->RotationMinY.Get( ),
                             pFbxNode->RotationMinZ.Get( ),
                             pFbxNode->RotationMin.Get( ),
                             limits.mutable_rotation_min( ),
                             limits.mutable_rotation_min_active( ) );

    PopulateTransformLimits( pFbxNode->ScalingMaxX.Get( ),
                             pFbxNode->ScalingMaxY.Get( ),
                             pFbxNode->ScalingMaxZ.Get( ),
                             pFbxNode->ScalingMax.Get( ),
                             limits.mutable_scaling_max( ),
                             limits.mutable_scaling_max_active( ) );

    PopulateTransformLimits( pFbxNode->ScalingMinX.Get( ),
                             pFbxNode->ScalingMinY.Get( ),
                             pFbxNode->ScalingMinZ.Get( ),
                             pFbxNode->ScalingMin.Get( ),
                             limits.mutable_scaling_min( ),
                             limits.mutable_scaling_min_active( ) );
}

void PopulateTransformLimits( FbxNode* pFbxNode, apemode::Node & n ) {
    if ( pFbxNode->TranslationMaxX.Get( ) || pFbxNode->TranslationMaxY.Get( ) || pFbxNode->TranslationMaxZ.Get( ) ||
         pFbxNode->TranslationMinX.Get( ) || pFbxNode->TranslationMinY.Get( ) || pFbxNode->TranslationMinZ.Get( ) ||
         pFbxNode->RotationMaxX.Get( ) || pFbxNode->RotationMaxY.Get( ) || pFbxNode->RotationMaxZ.Get( ) ||
         pFbxNode->RotationMinX.Get( ) || pFbxNode->RotationMinY.Get( ) || pFbxNode->RotationMinZ.Get( ) ||
         pFbxNode->ScalingMaxX.Get( ) || pFbxNode->ScalingMaxY.Get( ) || pFbxNode->ScalingMaxZ.Get( ) ||
         pFbxNode->ScalingMinX.Get( ) || pFbxNode->ScalingMinY.Get( ) || pFbxNode->ScalingMinZ.Get( ) ) {
        
        auto& s = apemode::State::Get( );
        s.console->info( "Node \"{}\" has transform limits.", pFbxNode->GetName( ) );

        const uint32_t transformLimitsId = static_cast< uint32_t >( s.transformLimits.size( ) );
        s.transformLimits.emplace_back( );
        n.transformLimitsId = transformLimitsId;
        PopulateTransformLimits( pFbxNode, s.transformLimits.back( ) );
    }
}

void ExportTransform( FbxNode* pFbxNode, apemode::Node & n ) {
    auto & s = apemode::State::Get( );

    s.transforms.emplace_back(    Cast( pFbxNode->LclTranslation.Get( ) ),
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

    PopulateTransformLimits( pFbxNode, n );
}
