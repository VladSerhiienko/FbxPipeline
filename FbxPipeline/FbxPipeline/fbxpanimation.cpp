#include <fbxppch.h>
#include <fbxpstate.h>

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

    FbxAnimCurveFilterResample           resample;
    FbxAnimCurveFilterGimbleKiller       gimbleKiller;
    FbxAnimCurveFilterKeySync            keySync;
    FbxAnimCurveFilterKeyReducer         keyReducer;
    FbxAnimCurveFilterConstantKeyReducer constantKeyReducer;

    FbxTime periodTime;
    if ( s.resampleFPS > 0.0f ) {
        auto milliseconds = 1.0f / s.resampleFPS * 1000.0f;
        periodTime.SetMilliSeconds( (FbxLongLong) milliseconds );
    }

    for ( int i = 0; i < animCurves.size( ); i += 3 ) {

        /* Get pAnimChannels from structs */

        FbxAnimCurve* pAnimChannels[] = {
            animCurves[ i ].pAnimCurve,
            animCurves[ i + 1 ].pAnimCurve,
            animCurves[ i + 2 ].pAnimCurve};

        /* Apply filters on channels */

        int availableCurves = 0;

        if ( s.reduceConstKeys || s.reduceKeys )
            for ( auto pAnimCurve : pAnimChannels ) {

                auto keyCount = 0;

                if ( nullptr != pAnimCurve ) {
                    keyCount = pAnimCurve->KeyGetCount( );

                    if ( s.reduceConstKeys )
                        ApplyFilter< 1 >( &constantKeyReducer, &pAnimCurve );

                    if ( s.reduceKeys )
                        ApplyFilter< 1 >( &keyReducer, &pAnimCurve );
                }

                /* NOTE: After key reducers key count can become zero. */
                availableCurves += nullptr != pAnimCurve && pAnimCurve->KeyGetCount( ) > 0;

                if ( pAnimCurve )
                    if ( pAnimCurve->KeyGetCount( ) < 1 )
                        s.console->warn( "Reduced: \"{}\": {} -> {} keys (no export)",
                                         pAnimCurve->GetName( ),
                                         keyCount,
                                         pAnimCurve->KeyGetCount( ) );
                    else
                        s.console->info( "Reduced: \"{}\": {} -> {} keys",
                                         pAnimCurve->GetName( ),
                                         keyCount,
                                         pAnimCurve->KeyGetCount( ) );
            }

        /* Apply filters on properties */

        if ( availableCurves == 3 ) {

            if ( s.propertyCurveSync )
                ApplyFilter< 3 >( &keySync, pAnimChannels );

            switch ( animCurves[ i ].eAnimCurveProperty ) {
                case apemodefb::EAnimCurvePropertyFb_GeometricRotation:
                case apemodefb::EAnimCurvePropertyFb_LclRotation:
                case apemodefb::EAnimCurvePropertyFb_PreRotation:
                case apemodefb::EAnimCurvePropertyFb_PostRotation:
                    ApplyFilter< 3 >( &gimbleKiller, pAnimChannels );
                    break;
            }
        }

        if ( s.resampleFPS > 0.0f ) {
            if ( availableCurves == 3 && s.propertyCurveSync ) {
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

                resample.SetPeriodTime( periodTime );
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
            auto keyCount   = pAnimCurve->KeyGetCount( );

            uint32_t curveId = static_cast< uint32_t >( s.animCurves.size( ) );
            s.animCurves.emplace_back( );
            n.curveIds.push_back( curveId );

            auto& curve       = s.animCurves.back( );
            curve.id          = curveId;
            curve.nameId      = s.PushValue( pAnimCurve->GetName( ) );
            curve.property    = pAnimCurveTuple.eAnimCurveProperty;
            curve.channel     = pAnimCurveTuple.eAnimCurveChannel;
            curve.animStackId = s.animStackDict[ pAnimStack->GetUniqueID( ) ];
            curve.animLayerId = s.animLayerDict[ pAnimLayer->GetUniqueID( ) ];
            curve.nodeId      = n.id;

            curve.keys.resize( keyCount );

            /* Constant and linear modes */
            for ( int i = 0; i < keyCount; ++i ) {
                auto& key = curve.keys[ i ];
                key.time  = (float) pAnimCurve->KeyGetTime( i ).GetMilliSeconds( );
                key.value = pAnimCurve->KeyGetValue( i );

                switch ( pAnimCurve->KeyGetInterpolation( i ) ) {
                    case FbxAnimCurveDef::eInterpolationConstant: {
                        key.interpolationMode = apemodefb::EInterpolationModeFb_Const;
                        switch ( pAnimCurve->KeyGetConstantMode( i ) ) {
                            case FbxAnimCurveDef::eConstantStandard:
                                break;

                            case FbxAnimCurveDef::eConstantNext:
                                /* There is at least one key ahead. */
                                if ( i < ( pAnimCurve->KeyGetCount( ) - 1 ) )
                                    key.value = pAnimCurve->KeyGetValue( i + 1 );
                                break;
                        }
                    } break;

                    case FbxAnimCurveDef::eInterpolationLinear: {
                        key.interpolationMode = apemodefb::EInterpolationModeFb_Linear;
                    } break;

                    case FbxAnimCurveDef::eInterpolationCubic: {
                        /* Resampling */
                        key.interpolationMode = apemodefb::EInterpolationModeFb_Linear;
                        // key.interpolationMode = apemodefb::EInterpolationMode_Cubic;
                    } break;
                }
            }

            /* Cubic modes, calculate tangents */

            /* TODO: Evaluate tangents externally without FBX.
                     I want to have compact Hermite splines. */

#if 0
            /* Animation in FBX is fucked up. Just resampling the keys, sync where possible
               Not sure how the handle those cases, what the fuck are weight and velocity properties. */

            for ( int i = 0; i < keyCount; ++i ) {
                switch ( pAnimCurve->KeyGetInterpolation( i ) ) {
                    case FbxAnimCurveDef::eInterpolationCubic: {
                        auto tangentMode = pAnimCurve->KeyGetTangentMode(i);
                        switch ( pAnimCurve->KeyGetTangentMode( i ) ) {

                            /* This case looks straightforward and correct */
                            case FbxAnimCurveDef::eTangentTCB: {
                                // https://en.wikipedia.org/wiki/Kochanek%E2%80%93Bartels_spline
                                // http://cubic.org/docs/hermite.htm

                                auto k1 = pAnimCurve->KeyGet( i );

                                auto& kk0 = curve.keys[ i - 1 ];
                                auto& kk1 = curve.keys[ i ];
                                auto& kk2 = curve.keys[ i + 1 ];
                                auto& kk3 = curve.keys[ i + 2 ];

                                auto t = k1.GetDataFloat( FbxAnimCurveDef::eTCBTension );
                                auto c = k1.GetDataFloat( FbxAnimCurveDef::eTCBContinuity );
                                auto b = k1.GetDataFloat( FbxAnimCurveDef::eTCBBias );

                                mathfu::vec2 p0 = {kk0.time, kk0.value};
                                mathfu::vec2 p1 = {kk1.time, kk1.value};
                                mathfu::vec2 p2 = {kk2.time, kk2.value};
                                mathfu::vec2 p3 = {kk3.time, kk3.value};

                                mathfu::vec2 d1 = ( p1 - p0 ) * ( 1 - t ) * ( 1 + b ) * ( 1 + c ) * 0.5f +
                                                  ( p2 - p1 ) * ( 1 - t ) * ( 1 - b ) * ( 1 - c ) * 0.5f;

                                mathfu::vec2 d2 = ( p2 - p1 ) * ( 1 - t ) * ( 1 + b ) * ( 1 - c ) * 0.5f +
                                                  ( p3 - p2 ) * ( 1 - t ) * ( 1 - b ) * ( 1 + c ) * 0.5f;

                                kk1.tangents[ 1 ][ 0 ] = d1.x;
                                kk1.tangents[ 1 ][ 1 ] = d1.y;

                                kk2.tangents[ 0 ][ 0 ] = d2.x;
                                kk2.tangents[ 0 ][ 1 ] = d2.y;

                            } break;

                            /* TODO: Can be first or last key (will cause out of range reads) */
                            case FbxAnimCurveDef::eTangentAuto: {
                                // http://cubic.org/docs/hermite.htm

                                auto k1 = pAnimCurve->KeyGet( i );

                                auto& kk0 = curve.keys[ i - 1 ];
                                auto& kk1 = curve.keys[ i ];
                                auto& kk2 = curve.keys[ i + 1 ];

                                mathfu::vec2 p0 = {kk0.time, kk0.value};
                                mathfu::vec2 p2 = {kk2.time, kk2.value};

                                float a = pAnimCurve->KeyGetRightAuto( i );
                                mathfu::vec2 d1 = ( p2 - p0 ) * a;

                                kk1.tangents[ 0 ][ 0 ] = d1.x;
                                kk1.tangents[ 0 ][ 1 ] = d1.y;

                                kk1.tangents[ 1 ][ 0 ] = d1.x;
                                kk1.tangents[ 1 ][ 1 ] = d1.y;

                            } break;

                            case FbxAnimCurveDef::eTangentAutoBreak: {
                                auto k1 = pAnimCurve->KeyGet( i );

                                auto& kk0 = curve.keys[ i - 1 ];
                                auto& kk1 = curve.keys[ i ];
                                auto& kk2 = curve.keys[ i + 1 ];

                                mathfu::vec2 p0 = {kk0.time, kk0.value};
                                mathfu::vec2 p2 = {kk2.time, kk2.value};

                                float a  = pAnimCurve->KeyGetLeftAuto( i );
                                mathfu::vec2 d1 = ( p2 - p0 ) * a;

                                kk1.tangents[ 1 ][ 0 ] = d1.x;
                                kk1.tangents[ 1 ][ 1 ] = d1.y;
                            } break;

                            case FbxAnimCurveDef::eTangentUser: {
                                bool bWeighted = pAnimCurve->KeyIsRightTangentWeighted( i );

                                float d = pAnimCurve->KeyGetRightDerivative( i );
                                float w = bWeighted ? pAnimCurve->KeyGetRightTangentWeight( i ) : 0.333f;

                                auto& kk1 = curve.keys[ i ];
                                auto& kk2 = curve.keys[ i + 1 ];

                                float duration = kk2.time - kk1.time;
                                float distance = kk2.value - kk1.value;

                                /* Right */
                                kk1.tangents[ 1 ][ 0 ] = w;
                                kk1.tangents[ 1 ][ 1 ] = w * d;

                                /* No break */
                                kk1.tangents[ 0 ][ 0 ] = kk1.tangents[ 1 ][ 0 ];
                                kk1.tangents[ 0 ][ 1 ] = kk1.tangents[ 1 ][ 1 ];

                                /* Next left == Right */
                                kk2.tangents[ 0 ][ 0 ] = kk1.tangents[ 1 ][ 0 ];
                                kk2.tangents[ 0 ][ 1 ] = kk1.tangents[ 1 ][ 1 ];

                            } break;

                            case FbxAnimCurveDef::eTangentBreak:{
                                bool  bWeighted = pAnimCurve->KeyIsRightTangentWeighted( i );

                                float d = pAnimCurve->KeyGetRightDerivative( i );
                                float w = bWeighted ? pAnimCurve->KeyGetRightTangentWeight( i ) : 0.333f;

                                auto& kk1 = curve.keys[ i ];
                                auto& kk2 = curve.keys[ i + 1 ];

                                /* Right */
                                kk1.tangents[ 1 ][ 0 ] = w;
                                kk1.tangents[ 1 ][ 1 ] = w * d;

                                /* Next left == Right */
                                kk2.tangents[ 0 ][ 0 ] = kk1.tangents[ 1 ][ 0 ];
                                kk2.tangents[ 0 ][ 1 ] = kk1.tangents[ 1 ][ 1 ];

                            } break;

                            case FbxAnimCurveDef::eTangentGenericBreak:
                            case FbxAnimCurveDef::eTangentGenericClamp:
                            case FbxAnimCurveDef::eTangentGenericTimeIndependent:
                            case FbxAnimCurveDef::eTangentGenericClampProgressive:
                                assert( false && "TODO" );
                                break;
                        }

                        /* Check if previous key is user key */
                        /*if ( i && 0 != ( FbxAnimCurveDef::eTangentUser & pAnimCurve->KeyGetTangentMode( i - 1 ) ) ) {
                            curve.keys[ i - 1 ].tangents[ 1 ][ 0 ] = curve.keys[ i ].tangents[ 0 ][ 0 ];
                            curve.keys[ i - 1 ].tangents[ 1 ][ 1 ] = curve.keys[ i ].tangents[ 0 ][ 1 ];
                        }*/

                    } break;
                }
            }
#endif

        }
    }
}
