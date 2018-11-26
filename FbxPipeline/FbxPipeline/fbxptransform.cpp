#include <fbxppch.h>
#include <fbxpstate.h>

inline apemodefb::Vec3Fb Cast( const FbxDouble3& other ) {
    return apemodefb::Vec3Fb{static_cast< float >( other.mData[ 0 ] ),
                             static_cast< float >( other.mData[ 1 ] ),
                             static_cast< float >( other.mData[ 2 ] )};
}

inline void PopulateTransformLimits( const bool          activeX,
                                     const bool          activeY,
                                     const bool          activeZ,
                                     const FbxDouble3&   src,
                                     apemodefb::Vec3Fb&  dst,
                                     apemodefb::Bool3Fb& dstActiveXYZ ) {
    dstActiveXYZ.mutate_x( activeX );
    dstActiveXYZ.mutate_y( activeY );
    dstActiveXYZ.mutate_z( activeZ );

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

void PopulateTransformLimits( FbxNode* pFbxNode, apemode::Node& n ) {
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

inline void LogIfNotEqual( const FbxVector4& v, const FbxVector4& compare, const char* name ) {
    if ( v == compare )
        return;

    auto& s = apemode::State::Get( );
    s.console->warn( "{}: ({} {} {})", name, v[ 0 ], v[ 1 ], v[ 2 ] );
}

void ExportTransform( FbxNode* pFbxNode, apemode::Node& n ) {
    auto& s = apemode::State::Get( );

    const FbxVector4 kFbxZero( 0.0, 0.0, 0.0, 0.0 );
    const FbxVector4 kFbxOne( 1.0, 1.0, 1.0, 1.0 );

    LogIfNotEqual( pFbxNode->LclTranslation.Get(), kFbxZero, "LclTranslation" );
    LogIfNotEqual( pFbxNode->LclRotation.Get(), kFbxZero, "LclRotation" );
    LogIfNotEqual( pFbxNode->LclScaling.Get(), kFbxOne, "LclScaling" );
#define LOG_IF_NOT_EQUAL_OFFSET_PROPERTY( P, d ) LogIfNotEqual( pFbxNode->Get##P( FbxNode::eSourcePivot ), d, #P );
    LOG_IF_NOT_EQUAL_OFFSET_PROPERTY( RotationOffset, kFbxZero );
    LOG_IF_NOT_EQUAL_OFFSET_PROPERTY( RotationPivot, kFbxZero );
    LOG_IF_NOT_EQUAL_OFFSET_PROPERTY( PreRotation, kFbxZero );
    LOG_IF_NOT_EQUAL_OFFSET_PROPERTY( PostRotation, kFbxZero );
    LOG_IF_NOT_EQUAL_OFFSET_PROPERTY( ScalingOffset, kFbxZero );
    LOG_IF_NOT_EQUAL_OFFSET_PROPERTY( ScalingPivot, kFbxZero );
    LOG_IF_NOT_EQUAL_OFFSET_PROPERTY( GeometricTranslation, kFbxZero );
    LOG_IF_NOT_EQUAL_OFFSET_PROPERTY( GeometricRotation, kFbxZero );
#undef LOG_IF_NOT_EQUAL_OFFSET_PROPERTY


    s.transforms.emplace_back( Cast( pFbxNode->LclTranslation.Get( ) ),
                               Cast( pFbxNode->GetRotationOffset( FbxNode::eSourcePivot ) ),
                               Cast( pFbxNode->GetRotationPivot( FbxNode::eSourcePivot ) ),
                               Cast( pFbxNode->GetPreRotation( FbxNode::eSourcePivot ) ),
                               Cast( pFbxNode->GetPostRotation( FbxNode::eSourcePivot ) ),
                               Cast( pFbxNode->LclRotation.Get( ) ),
                               Cast( pFbxNode->GetScalingOffset( FbxNode::eSourcePivot ) ),
                               Cast( pFbxNode->GetScalingPivot( FbxNode::eSourcePivot ) ),
                               Cast( pFbxNode->LclScaling.Get( ) ),
                               Cast( pFbxNode->GetGeometricTranslation( FbxNode::eSourcePivot ) ),
                               Cast( pFbxNode->GetGeometricRotation( FbxNode::eSourcePivot ) ),
                               Cast( pFbxNode->GetGeometricScaling( FbxNode::eSourcePivot ) ) );

    PopulateTransformLimits( pFbxNode, n );
}
