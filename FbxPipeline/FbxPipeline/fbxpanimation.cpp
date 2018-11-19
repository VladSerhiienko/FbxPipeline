#include <fbxppch.h>
#include <fbxpstate.h>

void BezierFitterFitSamples( FbxAnimCurve* pAnimCurve,
                             int           keyIndex,
                             double&       OutFittedBezier1,
                             double&       OutFittedBezier2 );

template < int TCurveCount >
void ApplyFilter( FbxAnimCurveFilter* pFilter, FbxAnimCurve** ppCurves ) {
    auto& s = apemode::State::Get( );
    FbxStatus status  = FbxStatus::eSuccess;

    pFilter->Apply( ppCurves, TCurveCount, &status );

    const char* P = TCurveCount == 1 ? "" : " (P)";

    if ( status.GetCode( ) != FbxStatus::eSuccess ) {
        for ( int i = 0; i < TCurveCount; ++i ) {
            s.console->error( "Failed{}: \"{}\": \"{}\"", P, pFilter->GetName( ), ppCurves[ i ]->GetName( ) );
        }
    } else {
        for ( int i = 0; i < TCurveCount; ++i ) {
            s.console->info( "Applied{}: \"{}\": \"{}\"", P, pFilter->GetName( ), ppCurves[ i ]->GetName( ) );
        }
    }
}

struct FbxAnimLayerComposite {
    FbxAnimLayer* pAnimLayer;
    FbxAnimStack* pAnimStack;
};

struct FbxAnimCurveComposite {
    apemodefb::EAnimCurvePropertyFb eAnimCurveProperty;
    apemodefb::EAnimCurveChannelFb  eAnimCurveChannel;
    FbxAnimCurve*                   pAnimCurve;
    FbxAnimLayer*                   pAnimLayer;
    FbxAnimStack*                   pAnimStack;
};

void ExportAnimation( FbxNode* pNode, apemode::Node& n ) {
    auto& s      = apemode::State::Get( );
    auto  pScene = pNode->GetScene( );

    std::vector< FbxAnimLayerComposite > animLayers;
    animLayers.reserve( s.animLayers.size( ) );

    int animStackCount = pScene->GetSrcObjectCount< FbxAnimStack >( );
    for ( int i = 0; i < animStackCount; i++ ) {
        FbxAnimStack* pAnimStack = pScene->GetSrcObject< FbxAnimStack >( i );
        int animLayerCount = pAnimStack ? pAnimStack->GetMemberCount< FbxAnimLayer >( ) : 0;
        for ( int j = 0; j < animLayerCount; j++ ) {
            FbxAnimLayerComposite item;
            item.pAnimLayer = pAnimStack->GetMember< FbxAnimLayer >( j );
            item.pAnimStack = pAnimStack;
            animLayers.push_back( item );
        }
    }

    std::vector< FbxAnimCurveComposite > animCurves;

    animCurves.reserve( animLayers.size( ) *
                        ( apemodefb::EAnimCurveChannelFb_MAX + 1 ) *
                        ( apemodefb::EAnimCurvePropertyFb_MAX + 1 ) );

    for ( auto pAnimLayerTuple : animLayers ) {
        auto pAnimLayer = pAnimLayerTuple.pAnimLayer;
        auto pAnimStack = pAnimLayerTuple.pAnimStack;

#pragma region
#define EmplaceBackChannel( _P, _eC, _C )                                             \
    {                                                                                 \
        FbxAnimCurveComposite animCurveComposite;                                     \
        animCurveComposite.eAnimCurveProperty = apemodefb::EAnimCurvePropertyFb_##_P; \
        animCurveComposite.eAnimCurveChannel  = _eC;                                  \
        animCurveComposite.pAnimCurve         = pNode->_P.GetCurve( pAnimLayer, _C ); \
        animCurveComposite.pAnimLayer         = pAnimLayer;                           \
        animCurveComposite.pAnimStack         = pAnimStack;\
        animCurves.push_back( animCurveComposite ); \
    }

    // animCurves.emplace_back( apemodefb::EAnimCurveProperty_##_P, _eC, pNode->##_P.GetCurve( pAnimLayer, _C ), pAnimLayer, pAnimStack );

#define EmplaceBack( _P )                                                                   \
    EmplaceBackChannel( _P, apemodefb::EAnimCurveChannelFb_X, FBXSDK_CURVENODE_COMPONENT_X ); \
    EmplaceBackChannel( _P, apemodefb::EAnimCurveChannelFb_Y, FBXSDK_CURVENODE_COMPONENT_Y ); \
    EmplaceBackChannel( _P, apemodefb::EAnimCurveChannelFb_Z, FBXSDK_CURVENODE_COMPONENT_Z );
#pragma endregion

        EmplaceBack( LclTranslation );
        EmplaceBack( LclRotation );
        EmplaceBack( LclScaling );
        EmplaceBack( GeometricTranslation );
        EmplaceBack( GeometricRotation );
        EmplaceBack( GeometricScaling );
        EmplaceBack( PreRotation );
        EmplaceBack( PostRotation );
        EmplaceBack( RotationOffset );
        EmplaceBack( RotationPivot );
        EmplaceBack( ScalingOffset );
        EmplaceBack( ScalingPivot );

#pragma region
#undef EmplaceBackChannel
#undef EmplaceBack
#pragma endregion

    }

    /* Ensure each curve has a name */

    std::stringstream ss;
    for (auto pAnimCurveComposite : animCurves) {
        if ( nullptr == pAnimCurveComposite.pAnimCurve )
            continue;

        std::string curveName = pAnimCurveComposite.pAnimCurve->GetName( );
        if ( curveName.empty( ) ) {
            ss << pNode->GetName( );

            ss << " ";
            switch ( pAnimCurveComposite.eAnimCurveProperty ) {
            case apemodefb::EAnimCurvePropertyFb_LclTranslation:          ss << "LclTranslation"; break;
            case apemodefb::EAnimCurvePropertyFb_RotationOffset:          ss << "RotationOffset"; break;
            case apemodefb::EAnimCurvePropertyFb_RotationPivot:           ss << "RotationPivot"; break;
            case apemodefb::EAnimCurvePropertyFb_PreRotation:             ss << "PreRotation"; break;
            case apemodefb::EAnimCurvePropertyFb_PostRotation:            ss << "PostRotation"; break;
            case apemodefb::EAnimCurvePropertyFb_LclRotation:             ss << "LclRotation"; break;
            case apemodefb::EAnimCurvePropertyFb_ScalingOffset:           ss << "ScalingOffset"; break;
            case apemodefb::EAnimCurvePropertyFb_ScalingPivot:            ss << "ScalingPivot"; break;
            case apemodefb::EAnimCurvePropertyFb_LclScaling:              ss << "LclScaling"; break;
            case apemodefb::EAnimCurvePropertyFb_GeometricTranslation:    ss << "GeometricTranslation"; break;
            case apemodefb::EAnimCurvePropertyFb_GeometricRotation:       ss << "GeometricRotation"; break;
            case apemodefb::EAnimCurvePropertyFb_GeometricScaling:        ss << "GeometricScaling"; break;
            }

            ss << " ";
            switch ( pAnimCurveComposite.eAnimCurveChannel ) {
            case apemodefb::EAnimCurveChannelFb_X:    ss << "X"; break;
            case apemodefb::EAnimCurveChannelFb_Y:    ss << "Y"; break;
            case apemodefb::EAnimCurveChannelFb_Z:    ss << "Z"; break;
            }

            /* Avoid noise in the names */

            if ( animLayers.size( ) > 1 ) {
                ss << " [";
                ss << pAnimCurveComposite.pAnimStack->GetName( );
                ss << ", ";
                ss << pAnimCurveComposite.pAnimLayer->GetName( );
                ss << "]";
            }

            pAnimCurveComposite.pAnimCurve->SetName( ss.str( ).c_str( ) );
            ss.str( "" );
            ss.clear( );
        }

        s.console->info( "\"{}\" <- \"{}\" ({} keys)",
                         pNode->GetName( ),
                         pAnimCurveComposite.pAnimCurve->GetName( ),
                         pAnimCurveComposite.pAnimCurve->KeyGetCount( ) );
    }

    float resampleFramerate = 0;
    bool  reduceConstKeys   = true;
    bool  reduceKeys        = false;
    bool  propertyCurveSync = false;

    if ( s.options[ "resample-framerate" ].count( ) ) {
        resampleFramerate = s.options[ "resample-framerate" ].as< float >( );
        resampleFramerate = std::min( resampleFramerate, 180.0f );
        resampleFramerate = std::max( resampleFramerate, 1.0f );
    }

    if ( s.options[ "sync-keys" ].count( ) )
        propertyCurveSync = s.options[ "sync-keys" ].as< bool >( );

    if ( s.options[ "reduce-keys" ].count( ) )
        reduceKeys = s.options[ "reduce-keys" ].as< bool >( );

    if ( s.options[ "reduce-const-keys" ].count( ) )
        reduceConstKeys = s.options[ "reduce-const-keys" ].as< bool >( );

    FbxAnimCurveFilterResample           resample;
    FbxAnimCurveFilterGimbleKiller       gimbleKiller;
    FbxAnimCurveFilterKeySync            keySync;
    FbxAnimCurveFilterConstantKeyReducer constantKeyReducer;

    FbxAnimCurveFilterKeyReducer keyReducer;
    keyReducer.SetPrecision( 1e-8 );

    FbxTime periodTime;
    if ( resampleFramerate > 0.0f ) {
        const FbxLongLong milliseconds = FbxLongLong( 1000.0f / resampleFramerate );
        periodTime.SetMilliSeconds( milliseconds );
        resample.SetPeriodTime( periodTime );
    }

    for ( int i = 0; i < animCurves.size( ); i += 3 ) {

        /* Get pAnimChannels from structs */

        FbxAnimCurve* pAnimChannels[] = {
            animCurves[ i ].pAnimCurve,
            animCurves[ i + 1 ].pAnimCurve,
            animCurves[ i + 2 ].pAnimCurve};

        /* Apply filters on channels */

        int availableCurves = 0;

        if ( reduceConstKeys || reduceKeys )
            for ( auto pAnimCurve : pAnimChannels ) {

                auto keyCount = 0;

                if ( nullptr != pAnimCurve ) {
                    keyCount = pAnimCurve->KeyGetCount( );

                    if ( reduceConstKeys )
                        ApplyFilter< 1 >( &constantKeyReducer, &pAnimCurve );

                    if ( reduceKeys )
                        ApplyFilter< 1 >( &keyReducer, &pAnimCurve );
                }

                /* NOTE: After key reducers key count can become zero. */
                availableCurves += nullptr != pAnimCurve && pAnimCurve->KeyGetCount( ) > 0;

                if ( pAnimCurve ) {
                    if ( pAnimCurve->KeyGetCount( ) < 1 ){
                        s.console->warn( "Reduced: \"{}\": {} -> {} keys (no export)",
                                         pAnimCurve->GetName( ),
                                         keyCount,
                                         pAnimCurve->KeyGetCount( ) );}
                    else{
                        s.console->info( "Reduced: \"{}\": {} -> {} keys",
                                         pAnimCurve->GetName( ),
                                         keyCount,
                                         pAnimCurve->KeyGetCount( ) );}

                }
            }

        /* Apply filters on properties */

        if ( availableCurves == 3 ) {

            if ( propertyCurveSync )
                ApplyFilter< 3 >( &keySync, pAnimChannels );

            switch ( animCurves[ i ].eAnimCurveProperty ) {
                case apemodefb::EAnimCurvePropertyFb_GeometricRotation:
                case apemodefb::EAnimCurvePropertyFb_LclRotation:
                case apemodefb::EAnimCurvePropertyFb_PreRotation:
                case apemodefb::EAnimCurvePropertyFb_PostRotation:
                    ApplyFilter< 3 >( &gimbleKiller, pAnimChannels );
                    break;

                default:
                    break;
            }
        }

        if ( resampleFramerate > 0.0f ) {
            if ( availableCurves == 3 && propertyCurveSync ) {
                /* Resample property */
                /* NOTE: After sync start time, stop time and key count must be the same. */

                auto keyCount = pAnimChannels[ 0 ]->KeyGetCount( );
                assert( keyCount == pAnimChannels[ 1 ]->KeyGetCount( ) );
                assert( keyCount == pAnimChannels[ 2 ]->KeyGetCount( ) );

                auto startTime = pAnimChannels[ 0 ]->KeyGet( 0 ).GetTime( );
                assert( startTime == pAnimChannels[ 1 ]->KeyGet( 0 ).GetTime( ) );
                assert( startTime == pAnimChannels[ 2 ]->KeyGet( 0 ).GetTime( ) );

                auto stopTime = pAnimChannels[ 0 ]->KeyGet( keyCount - 1 ).GetTime( );
                assert( stopTime == pAnimChannels[ 1 ]->KeyGet( keyCount - 1 ).GetTime( ) );
                assert( stopTime == pAnimChannels[ 2 ]->KeyGet( keyCount - 1 ).GetTime( ) );

                resample.SetStartTime( startTime );
                resample.SetStopTime( stopTime );

                /* NOTE: After resampling all the keys become auto keys. */
                ApplyFilter< 3 >( &resample, pAnimChannels );

                auto expectedApproxKeyCount = ( stopTime.Get( ) - startTime.Get( ) ) / periodTime.Get( );
                auto resampledKeyCount      = pAnimChannels[ 0 ]->KeyGetCount( );
                auto diffKeyCount           = abs( resampledKeyCount - expectedApproxKeyCount );
                assert( resampledKeyCount == pAnimChannels[ 1 ]->KeyGetCount( ) );
                assert( resampledKeyCount == pAnimChannels[ 2 ]->KeyGetCount( ) );

                for ( int i = 0; i < 3; ++i ) {

                    spdlog::level::level_enum level = diffKeyCount > 3
                                                    ? spdlog::level::warn
                                                    : spdlog::level::info;

                    s.console->log( level,
                                    "Resampled (P): \"{}\": {} -> {} ({} -> {})",
                                    pAnimChannels[ i ]->GetName( ),
                                    keyCount,
                                    resampledKeyCount,
                                    expectedApproxKeyCount,
                                    diffKeyCount );
                }

            } else {

                /* Resample each channel */
                for ( auto pAnimCurve : pAnimChannels )
                    if ( nullptr != pAnimCurve ) {
                        auto keyCount = pAnimCurve->KeyGetCount( );
                        if (keyCount < 2) {
                            s.console->warn( "Failed: \"{}\": No keys for resampling", pAnimCurve->GetName( ) );
                            continue;
                        }

                        auto startTime = pAnimCurve->KeyGet( 0 ).GetTime( );
                        auto stopTime  = pAnimCurve->KeyGet( keyCount - 1 ).GetTime( );

                        resample.SetPeriodTime( periodTime );
                        resample.SetStartTime( startTime );
                        resample.SetStopTime( stopTime );

                        /* NOTE: After resampling all the keys become auto keys. */
                        ApplyFilter< 1 >( &resample, &pAnimCurve );

                        auto expectedApproxKeyCount = ( stopTime.Get( ) - startTime.Get( ) ) / periodTime.Get( );
                        auto resampledKeyCount      = pAnimCurve->KeyGetCount( );
                        auto diffKeyCount           = abs( resampledKeyCount - expectedApproxKeyCount );

                        spdlog::level::level_enum level = diffKeyCount > 2
                                                        ? spdlog::level::warn
                                                        : spdlog::level::info;

                        s.console->log( level,
                                        "Resampled: \"{}\": {} -> {} ({} -> {})",
                                        pAnimCurve->GetName( ),
                                        keyCount,
                                        resampledKeyCount,
                                        expectedApproxKeyCount,
                                        diffKeyCount );
                    }
            }
        }
    }

    s.animCurves.reserve( s.animCurves.size( ) + animCurves.size( ) );

    for ( auto pAnimCurveTuple : animCurves ) {
        if ( auto pAnimCurve = pAnimCurveTuple.pAnimCurve ) {
            auto pAnimStack = pAnimCurveTuple.pAnimStack;
            auto pAnimLayer = pAnimCurveTuple.pAnimLayer;
            const int32_t keyCount   = pAnimCurve->KeyGetCount( );

            const uint32_t curveId = static_cast< uint32_t >( s.animCurves.size( ) );
            s.animCurves.emplace_back( );
            n.curveIds.push_back( curveId );

            apemode::AnimCurve& curve = s.animCurves.back( );

            curve.id          = curveId;
            curve.nodeId      = n.id;
            curve.nameId      = s.PushValue( pAnimCurve->GetName( ) );
            curve.property    = pAnimCurveTuple.eAnimCurveProperty;
            curve.channel     = pAnimCurveTuple.eAnimCurveChannel;
            curve.animStackId = s.animStackDict[ pAnimStack->GetUniqueID( ) ];
            curve.animLayerId = s.animLayerDict[ pAnimLayer->GetUniqueID( ) ];

            curve.keys.resize( keyCount );

            /* Constant and linear modes */
            for ( int i = 0; i < keyCount; ++i ) {
                auto& key = curve.keys[ i ];
                key.time  = static_cast< float >( pAnimCurve->KeyGetTime( i ).GetSecondDouble( ) );
                key.value = pAnimCurve->KeyGetValue( i );

                switch ( pAnimCurve->KeyGetInterpolation( i ) ) {
                    case FbxAnimCurveDef::eInterpolationLinear: {
                        key.interpolationMode = apemodefb::EInterpolationModeFb_Linear;
                    } break;

                    case FbxAnimCurveDef::eInterpolationConstant: {
                        key.interpolationMode = apemodefb::EInterpolationModeFb_Const;
                        switch ( pAnimCurve->KeyGetConstantMode( i ) ) {
                            case FbxAnimCurveDef::eConstantNext: {
                                if ( i < ( keyCount - 1 ) ) {
                                    /* There is at least one key ahead. */
                                    key.value = pAnimCurve->KeyGetValue( i + 1 );
                                }
                            } break;

                            default: {
                            } break;
                        }
                    } break;

                    case FbxAnimCurveDef::eInterpolationCubic: {
                        if ( resampleFramerate > 0 ) {
                            key.interpolationMode = apemodefb::EInterpolationModeFb_Linear;
                        }
                    } break;
                }
            }

            /* Cubic modes (only for original curve keys) */
            if ( resampleFramerate < std::numeric_limits< float >::epsilon( ) ) {
                for ( int i = 0; i < keyCount; ++i ) {
                    auto& key = curve.keys[ i ];
                    switch ( pAnimCurve->KeyGetInterpolation( i ) ) {
                        case FbxAnimCurveDef::eInterpolationCubic: {

                            if ( i < ( keyCount - 1 ) ) {
                                double fittedBezier1 = 1, fittedBezier2 = 1;
                                BezierFitterFitSamples( pAnimCurve, i, fittedBezier1, fittedBezier2 );

                                key.bez1 = static_cast< float >( fittedBezier1 );
                                key.bez2 = static_cast< float >( fittedBezier2 );
                                key.interpolationMode = apemodefb::EInterpolationModeFb_Cubic;
                            } else {
                                key.interpolationMode = apemodefb::EInterpolationModeFb_Const;
                            }

                        } break;

                        default: {
                        } break;
                    }
                }
            }
        }
    }
}
