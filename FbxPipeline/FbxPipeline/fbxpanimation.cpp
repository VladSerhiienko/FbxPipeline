#include <fbxppch.h>
#include <fbxpstate.h>

template < int TCurveCount >
void ApplyFilter( FbxAnimCurveFilter* pFilter, FbxAnimCurve** ppCurves ) {
    auto& s = apemode::Get( );
    FbxStatus status  = FbxStatus::eSuccess;

    pFilter->Apply( ppCurves, TCurveCount, &status );
    switch ( TCurveCount ) {
        case 1:
            if ( status.GetCode( ) != FbxStatus::eSuccess ) {
                s.console->error( "Failed to apply \"{}\" to \"{}\": {}",
                                  pFilter->GetName( ),
                                  ( *ppCurves )->GetName( ),
                                  status.GetErrorString( ) );
            } else {
                s.console->info( "Applied \"{}\" to \"{}\"", pFilter->GetName( ), ( *ppCurves )->GetName( ) );
            }
            break;

        case 3:
            if ( status.GetCode( ) != FbxStatus::eSuccess ) {
                s.console->error( "Failed to apply \"{}\" to \"{}\", \"{}\", \"{}\": {}",
                                  pFilter->GetName( ),
                                  ppCurves[ 0 ]->GetName( ),
                                  ppCurves[ 1 ]->GetName( ),
                                  ppCurves[ 2 ]->GetName( ),
                                  status.GetErrorString( ) );
            } else {
                s.console->info( "Applied \"{}\" to \"{}\", \"{}\", \"{}\"",
                                 pFilter->GetName( ),
                                 ppCurves[ 0 ]->GetName( ),
                                 ppCurves[ 1 ]->GetName( ),
                                 ppCurves[ 2 ]->GetName( ) );
            }
            break;

        default:
            assert( false );
            break;
    }
}

void ExportAnimation( FbxNode* pNode, apemode::Node& n ) {
    auto& s      = apemode::Get( );
    auto  pScene = pNode->GetScene( );

    std::vector< std::tuple< FbxAnimLayer*, FbxAnimStack* > > animLayers;
    animLayers.reserve( s.animLayers.size( ) );

    int animStackCount = pScene->GetSrcObjectCount< FbxAnimStack >( );
    for ( int i = 0; i < animStackCount; i++ ) {
        FbxAnimStack* pAnimStack = pScene->GetSrcObject< FbxAnimStack >( i );
        int animLayerCount = pAnimStack->GetMemberCount< FbxAnimLayer >( );
        for ( int j = 0; j < animLayerCount; j++ ) {
            FbxAnimLayer* pAnimLayer = pAnimStack->GetMember< FbxAnimLayer >( j );
            animLayers.emplace_back( pAnimLayer, pAnimStack );
        }
    }

    std::vector< std::tuple< apemodefb::EAnimCurveProperty, apemodefb::EAnimCurveChannel, FbxAnimCurve*, FbxAnimLayer*, FbxAnimStack* > > animCurves;
    animCurves.reserve( animLayers.size( ) * ( apemodefb::EAnimCurveChannel_MAX + 1 ) * ( apemodefb::EAnimCurveProperty_MAX + 1 ) );

    for ( auto pAnimLayerTuple : animLayers ) {
        auto pAnimLayer = std::get< FbxAnimLayer* >( pAnimLayerTuple );
        auto pAnimStack = std::get< FbxAnimStack* >( pAnimLayerTuple );

#pragma region
#define EmplaceBackChannel( _P, _eC, _C ) \
    animCurves.emplace_back( apemodefb::EAnimCurveProperty_##_P, _eC, pNode->##_P.GetCurve( pAnimLayer, _C ), pAnimLayer, pAnimStack );

#define EmplaceBack( _P )                                                                   \
    EmplaceBackChannel( _P, apemodefb::EAnimCurveChannel_X, FBXSDK_CURVENODE_COMPONENT_X ); \
    EmplaceBackChannel( _P, apemodefb::EAnimCurveChannel_Y, FBXSDK_CURVENODE_COMPONENT_Y ); \
    EmplaceBackChannel( _P, apemodefb::EAnimCurveChannel_Z, FBXSDK_CURVENODE_COMPONENT_Z );
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
    for (auto pAnimCurve : animCurves) {
        if ( nullptr == std::get< FbxAnimCurve* >( pAnimCurve ) )
            continue;

        std::string curveName = std::get< FbxAnimCurve* >( pAnimCurve )->GetName( );
        if ( curveName.empty( ) ) {
            ss << pNode->GetName( );

            ss << " ";
            switch ( std::get< apemodefb::EAnimCurveProperty >( pAnimCurve ) ) {
            case apemodefb::EAnimCurveProperty_LclTranslation:          ss << "LclTranslation"; break;
            case apemodefb::EAnimCurveProperty_RotationOffset:          ss << "RotationOffset"; break;
            case apemodefb::EAnimCurveProperty_RotationPivot:           ss << "RotationPivot"; break;
            case apemodefb::EAnimCurveProperty_PreRotation:             ss << "PreRotation"; break;
            case apemodefb::EAnimCurveProperty_PostRotation:            ss << "PostRotation"; break;
            case apemodefb::EAnimCurveProperty_LclRotation:             ss << "LclRotation"; break;
            case apemodefb::EAnimCurveProperty_ScalingOffset:           ss << "ScalingOffset"; break;
            case apemodefb::EAnimCurveProperty_ScalingPivot:            ss << "ScalingPivot"; break;
            case apemodefb::EAnimCurveProperty_LclScaling:              ss << "LclScaling"; break;
            case apemodefb::EAnimCurveProperty_GeometricTranslation:    ss << "GeometricTranslation"; break;
            case apemodefb::EAnimCurveProperty_GeometricRotation:       ss << "GeometricRotation"; break;
            case apemodefb::EAnimCurveProperty_GeometricScaling:        ss << "GeometricScaling"; break;
            }

            ss << " ";
            switch ( std::get< apemodefb::EAnimCurveChannel >( pAnimCurve ) ) {
            case apemodefb::EAnimCurveChannel_X:    ss << "X"; break;
            case apemodefb::EAnimCurveChannel_Y:    ss << "Y"; break;
            case apemodefb::EAnimCurveChannel_Z:    ss << "Z"; break;
            }

            /* Avoid noise in the names */

            if ( animLayers.size( ) > 1 ) {
                ss << " [";
                ss << std::get< FbxAnimStack* >( pAnimCurve )->GetName( );
                ss << ", ";
                ss << std::get< FbxAnimLayer* >( pAnimCurve )->GetName( );
                ss << "]";
            }

            std::get< FbxAnimCurve* >( pAnimCurve )->SetName( ss.str( ).c_str( ) );
            ss.str( "" );
            ss.clear( );
        }
    }

    FbxAnimCurveFilterResample           resample;
    FbxAnimCurveFilterGimbleKiller       gimbleKiller;
    FbxAnimCurveFilterKeySync            keySync;
    FbxAnimCurveFilterKeyReducer         keyReducer;
    FbxAnimCurveFilterConstantKeyReducer constantKeyReducer;

    for ( int i = 0; i < animCurves.size( ); i += 3 ) {

        /* Get curves from tuples */

        FbxAnimCurve* curves[] = {std::get< FbxAnimCurve* >( animCurves[ i ] ),
                                  std::get< FbxAnimCurve* >( animCurves[ i + 1 ] ),
                                  std::get< FbxAnimCurve* >( animCurves[ i + 2 ] )};

        /* Apply filters on channels */

        int availableCurves = 0;
        for ( auto pAnimCurve : curves ) {
            availableCurves += nullptr != pAnimCurve;

            if ( nullptr != pAnimCurve ) {
                ApplyFilter< 1 >( &constantKeyReducer, &pAnimCurve );
                ApplyFilter< 1 >( &keyReducer, &pAnimCurve );
            }
        }

        /* Apply filters on properties */

        if ( availableCurves == 3 ) {
            ApplyFilter< 3 >( &keySync, curves );

            switch ( std::get< apemodefb::EAnimCurveProperty >( animCurves[ i ] ) ) {
                case apemodefb::EAnimCurveProperty_GeometricRotation:
                case apemodefb::EAnimCurveProperty_LclRotation:
                case apemodefb::EAnimCurveProperty_PreRotation:
                case apemodefb::EAnimCurveProperty_PostRotation:
                    ApplyFilter< 3 >( &gimbleKiller, curves );
                    break;
            }

            /* Resample property */
            /* ApplyFilter< 3 >( &resample, curves ); */

        } else {

            /* Resample each channel */
            /* for ( auto pAnimCurve : curves )
                if ( nullptr != pAnimCurve )
                    ApplyFilter< 1 >( &resample, &pAnimCurve ); */
        }
    }

    s.animCurves.reserve( s.animCurves.size( ) + animCurves.size( ) );

    for (auto pAnimCurveTuple : animCurves) {
        if ( auto pAnimCurve = std::get< FbxAnimCurve* >( pAnimCurveTuple ) ) {
            s.animCurves.emplace_back( );

            auto& curve = s.animCurves.back( );
            curve.keys.resize( pAnimCurve->KeyGetCount( ) );

            /* Constant and linear modes */

            for ( int i = 0; i < pAnimCurve->KeyGetCount( ); ++i ) {
                auto& key = curve.keys[ i ];
                key.time  = (float) pAnimCurve->KeyGetTime( i ).GetMilliSeconds( );
                key.value = pAnimCurve->KeyGetValue( i );

                switch ( pAnimCurve->KeyGetInterpolation( i ) ) {
                    case fbxsdk::FbxAnimCurveDef::eInterpolationConstant: {
                        key.interpolationMode = apemodefb::EInterpolationMode_Const;
                        switch ( pAnimCurve->KeyGetConstantMode( i ) ) {
                            case fbxsdk::FbxAnimCurveDef::eConstantStandard:
                                break;

                            case fbxsdk::FbxAnimCurveDef::eConstantNext:
                                assert( ( pAnimCurve->KeyGetCount( ) - 1 ) != i );
                                key.value = pAnimCurve->KeyGetValue( i + 1 );
                                break;
                        }
                    } break;

                    case fbxsdk::FbxAnimCurveDef::eInterpolationLinear: {
                        key.interpolationMode = apemodefb::EInterpolationMode_Linear;
                        key.value = pAnimCurve->KeyGetValue( i );
                    } break;
                    case fbxsdk::FbxAnimCurveDef::eInterpolationCubic: {
                        key.interpolationMode = apemodefb::EInterpolationMode_Cubic;
                    }
                }
            }

            /* Cubic modes, calculate tangents */

            for ( int i = 0; i < pAnimCurve->KeyGetCount( ); ++i ) {
                switch ( pAnimCurve->KeyGetInterpolation( i ) ) {
                    case fbxsdk::FbxAnimCurveDef::eInterpolationCubic: {
                        auto tangentMode = pAnimCurve->KeyGetTangentMode(i);
                        switch ( pAnimCurve->KeyGetTangentMode( i ) ) {

                            case fbxsdk::FbxAnimCurveDef::eTangentTCB: {
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

                            case fbxsdk::FbxAnimCurveDef::eTangentAuto: {
                                // http://cubic.org/docs/hermite.htm

                                auto k1 = pAnimCurve->KeyGet( i );

                                auto& kk0 = curve.keys[ i - 1 ];
                                auto& kk1 = curve.keys[ i ];
                                auto& kk2 = curve.keys[ i + 1 ];

                                mathfu::vec2 p0 = {kk0.time, kk0.value};
                                mathfu::vec2 p2 = {kk2.time, kk2.value};

                                float a  = pAnimCurve->KeyGetRightAuto( i );
                                mathfu::vec2 d1 = ( p2 - p0 ) * a;

                                kk1.tangents[ 0 ][ 0 ] = d1.x;
                                kk1.tangents[ 0 ][ 1 ] = d1.y;

                                kk1.tangents[ 1 ][ 0 ] = d1.x;
                                kk1.tangents[ 1 ][ 1 ] = d1.y;

                            } break;

                            case fbxsdk::FbxAnimCurveDef::eTangentAutoBreak: {
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

                            case fbxsdk::FbxAnimCurveDef::eTangentUser:
                            case fbxsdk::FbxAnimCurveDef::eTangentBreak:
                                // User will be set after switch with the next iteration.
                                break;

                            case fbxsdk::FbxAnimCurveDef::eTangentGenericBreak:
                            case fbxsdk::FbxAnimCurveDef::eTangentGenericClamp:
                            case fbxsdk::FbxAnimCurveDef::eTangentGenericTimeIndependent:
                            case fbxsdk::FbxAnimCurveDef::eTangentGenericClampProgressive:
                                assert( false && "TODO" );
                                break;
                        }

                        /* Check if previous key is user key */
                        if ( i && 0 != ( fbxsdk::FbxAnimCurveDef::eTangentUser & pAnimCurve->KeyGetTangentMode( i - 1 ) ) ) {
                            curve.keys[ i - 1 ].tangents[ 1 ][ 0 ] = curve.keys[ i ].tangents[ 0 ][ 0 ];
                            curve.keys[ i - 1 ].tangents[ 1 ][ 1 ] = curve.keys[ i ].tangents[ 0 ][ 1 ];
                        }

                    } break;
                }
            }
        }
    }
}
