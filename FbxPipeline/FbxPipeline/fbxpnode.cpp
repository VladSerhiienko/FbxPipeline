#include <fbxppch.h>
#include <fbxpstate.h>
#include <queue>

const char* GetPivotStateString( FbxNode::EPivotState eState ) {
    return eState == FbxNode::ePivotActive ? "PivotActive" : "PivotReference";
}
const char* GetPivotSetString( FbxNode::EPivotSet eState ) {
    return eState == FbxNode::eSourcePivot ? "SourcePivot" : "DestinationPivot";
}
const char* GetSkeletonTypeString( FbxSkeleton::EType eType ) {
    switch ( eType ) {
        case fbxsdk::FbxSkeleton::eRoot:
            return "FbxSkeleton.Root";
        case fbxsdk::FbxSkeleton::eLimb:
            return "FbxSkeleton.Limb";
        case fbxsdk::FbxSkeleton::eLimbNode:
            return "FbxSkeleton.LimbNode";
        case fbxsdk::FbxSkeleton::eEffector:
            return "FbxSkeleton.Effector";
        default:
            return "FbxSkeleton.ERROR";
    }
}

void InitializeSeachLocations( );
void ExportMesh( FbxNode* node, apemode::Node& n, bool pack, bool optimize );
void ExportMaterials( FbxScene* scene );
void ExportMaterials( FbxNode* node, apemode::Node& n );
void ExportTransform( FbxNode* node, apemode::Node& n );
void ExportAnimation( FbxNode* node, apemode::Node& n );
void ExportCamera( FbxNode* node, apemode::Node& n );
void ExportLight( FbxNode* node, apemode::Node& n );


void ExportSkeleton( FbxNode* node, apemode::Node& n ) {
    if ( auto pSkeleton = node->GetSkeleton( ) ) {
        auto& s = apemode::State::Get( );
        s.console->info( "\tSkeleton: {} -> {}", node->GetName( ), GetSkeletonTypeString( pSkeleton->GetSkeletonType( ) ) );
        n.skeletonType = apemodefb::ESkeletonTypeFb( pSkeleton->GetSkeletonType( ) );
    }
}

void ExportNodeAttributes( FbxNode* node, apemode::Node& n ) {
    auto& s = apemode::State::Get( );

    n.cullingType = (apemodefb::ECullingTypeFb) node->mCullingType;
    node->GetTransformationInheritType( (FbxTransform::EInheritType&) n.inheritType );
    node->GetRotationOrder( FbxNode::eSourcePivot, (FbxEuler::EOrder&) n.rotationOrder );

    // auto& pivots = node->GetPivots( );
    FbxNode::EPivotState srcPivotState, dstPivotState;
    node->GetPivotState( FbxNode::EPivotSet::eDestinationPivot, dstPivotState );
    node->GetPivotState( FbxNode::EPivotSet::eSourcePivot, srcPivotState );

    s.console->info( "\tCulling: {}", apemodefb::EnumNameECullingTypeFb( n.cullingType ) );
    s.console->info( "\tInherit type: {}", apemodefb::EnumNameEInheritTypeFb( n.inheritType ) );
    s.console->info( "\tRotation order: {}", apemodefb::EnumNameERotationOrderFb( n.rotationOrder ) );
    s.console->info( "\tSrc pivot state: {}", GetPivotStateString( srcPivotState ) );
    s.console->info( "\tDst pivot state: {}", GetPivotStateString( dstPivotState ) );

    ExportTransform( node, n );
    ExportAnimation( node, n );
    ExportMaterials( node, n );
    ExportCamera( node, n );
    ExportLight( node, n );
}

uint32_t ExportNode( FbxNode* node ) {
    auto& s = apemode::State::Get( );

    s.console->info( "" );
    s.console->info( "" );
    s.console->info( "ExportNode: {}", node->GetName( ) );

    if ( false == node->GetObjectFlags( FbxObject::EObjectFlag::eHidden ) ) {
        const uint32_t nodeId = static_cast< uint32_t >( s.nodes.size( ) );
        s.nodes.emplace_back( );

        auto& n  = s.nodes.back( );
        n.id     = nodeId;
        n.fbxId  = node->GetUniqueID( );
        n.nameId = s.PushValue( node->GetName( ) );

        s.nodeDict[ n.fbxId ] = nodeId;

        ExportNodeAttributes( node, n );
        if ( auto c = node->GetChildCount( ) ) {
            n.childIds.reserve( c );
            for ( auto i = 0; i < c; ++i ) {
                const auto childId = ExportNode( node->GetChild( i ) );
                if ( childId != -1 )
                    s.nodes[ nodeId ].childIds.push_back( childId );
            }
        }

        return nodeId;
    }

    s.console->info( "Node {} is hidden", node->GetName( ) );
    return uint32_t( -1 );
}

void ExportMeshes( FbxNode* pFbxNode ) {
    auto& s = apemode::State::Get( );

    uint32_t nodeId = s.nodeDict[ pFbxNode->GetUniqueID( ) ];
    apemode::Node& node = s.nodes[ nodeId ];
    assert( node.fbxId == pFbxNode->GetUniqueID( ) );

    ExportMesh( pFbxNode, node, s.options[ "p" ].as< bool >( ), s.options[ "t" ].as< bool >( ) );
    if ( auto c = pFbxNode->GetChildCount( ) ) {
        for ( auto i = 0; i < c; ++i ) {
            ExportMeshes( pFbxNode->GetChild( i ) );
        }
    }
}

void ExportSkeletons( FbxNode* pFbxNode ) {
    auto& s = apemode::State::Get( );

    uint32_t nodeId = s.nodeDict[ pFbxNode->GetUniqueID( ) ];
    apemode::Node& node = s.nodes[ nodeId ];
    assert( node.fbxId == pFbxNode->GetUniqueID( ) );

    ExportSkeleton( pFbxNode, node );
    if ( auto c = pFbxNode->GetChildCount( ) ) {
        for ( auto i = 0; i < c; ++i ) {
            ExportSkeletons( pFbxNode->GetChild( i ) );
        }
    }
}

void ExportSkeletons( FbxScene* pScene ) {
    auto& s = apemode::State::Get( );
    s.console->info( "Skeleton: " );
    ExportSkeletons( pScene->GetRootNode( ) );
}

/**
 * Preprocess scene with Fbx tools:
 * FbxGeometryConverter > Remove bad polygons
 *                      > Triangulate
 *                      > Split meshes per material
 **/
void PreprocessMeshes( FbxScene* scene ) {
    auto& s = apemode::State::Get( );

    FbxGeometryConverter geometryConverter( s.manager );

#if FBXSDK_VERSION_MAJOR > 2015

    if ( s.options[ "remove-bad-polies" ].as< bool >( ) ) {
        s.console->info( "Removing bad polygons..." );

        FbxArray< FbxNode* > affectedNodes;
        geometryConverter.RemoveBadPolygonsFromMeshes( s.scene, &affectedNodes );

        if ( 0 != affectedNodes.Size( ) ) {
            s.console->warn( "Removed bad polygons from {} nodes:", affectedNodes.Size( ) );
            for ( int32_t i = 0; i < affectedNodes.Size( ); ++i ) {
                assert( nullptr != affectedNodes[ i ] );
                s.console->warn( "\t > {}", affectedNodes[ i ]->GetName( ) );
            }
        } else {
            s.console->info( "No bad polygons in the scene." );
        }
    }

#endif

    if ( s.options[ "split-meshes-per-material" ].as< bool >( ) ) {
        s.console->info( "Splitting per material..." );

        if ( false == geometryConverter.SplitMeshesPerMaterial( s.scene, true ) ) {
            s.console->warn( "Splitting per material failed for some nodes." );
            s.console->warn( "Nodes that were not splitted will have subsets." );
        } else {
            s.console->info( "Splitting per material succeeded for all nodes." );
        }
    }
}

void PreprocessAnimation( FbxScene* pScene ) {
    auto& s = apemode::State::Get( );
    assert(pScene);

    int animStackCount = pScene->GetSrcObjectCount< FbxAnimStack >( );
    s.console->info( "Scene has {} animation stacks:", animStackCount );

    for ( int i = 0; i < animStackCount; i++ ) {
        FbxAnimStack* pAnimStack = pScene->GetSrcObject< FbxAnimStack >( i );

        std::string animStackName = pAnimStack->GetName( );
        if ( animStackName.empty( ) ) {
            animStackName = "AnimStack";

            if ( animStackCount > 1 )
                animStackName += std::to_string( i );

            pAnimStack->SetName( animStackName.c_str( ) );
        }

        const uint32_t animStackId = (uint32_t) s.animStacks.size( );
        s.animStackDict[ pAnimStack->GetUniqueID( ) ] = animStackId;

        s.animStacks.emplace_back( );
        s.animStacks.back( ).nameId = s.PushValue( pAnimStack->GetName( ) );
        s.animStacks.back( ).id = animStackId;

        const int animLayerCount = pAnimStack->GetMemberCount< FbxAnimLayer >( );
        s.console->info( "\t> Animation Stack #{} \"{}\" has {} layers ", i, pAnimStack->GetName( ), animLayerCount );

        for ( int j = 0; j < animLayerCount; j++ ) {
            FbxAnimLayer* pAnimLayer = pAnimStack->GetMember< FbxAnimLayer >( j );

            std::string animLayerName = pAnimLayer->GetName( );
            if ( animLayerName.empty( ) ) {
                animLayerName = "AnimLayer";

                if ( animLayerCount > 1 )
                    animLayerName += std::to_string( i );

                if ( animStackCount > 1 ) {
                    animLayerName += "[";
                    animLayerName += animStackName;
                    animLayerName += "]";
                }

                 pAnimLayer->SetName( animLayerName.c_str( ) );
            }

            const uint32_t animLayerId = (uint32_t) s.animLayers.size( );
            s.animLayerDict[ pAnimLayer->GetUniqueID( ) ] = animLayerId;

            s.animLayers.emplace_back( );
            s.animLayers.back( ).id           = animLayerId;
            s.animLayers.back( ).animStackId  = animStackId;
            s.animLayers.back( ).animStackIdx = uint32_t( j );
            s.animLayers.back( ).nameId       = s.PushValue( pAnimLayer->GetName( ) );

            s.console->info( "\t\t> Animation Layer #{} \"{}\"", j, pAnimLayer->GetName( ) );
        }
    }
}

struct AxisSystem {
    enum EAxis { eXAxis, eYAxis, eZAxis };
    struct AxisDef {
        EAxis mAxis;
        int   mSign;
    };

    virtual ~AxisSystem() = default;
    AxisDef mUpVector{eYAxis, -1};
    AxisDef mFrontVector{eZAxis, +1};
    AxisDef mCoorSystem{eXAxis, -1};
};

void PrintAxisSystem( const AxisSystem& axisSystem ) {
    const char* sz[] = {"X", "Y", "Z"};

    auto& s = apemode::State::Get( );
    s.console->error( "\t\tFront={}{} Up={}{} CoordSystem={}{} ",
                      axisSystem.mFrontVector.mSign >= 0 ? "+" : "-",
                      sz[ axisSystem.mFrontVector.mAxis ],
                      axisSystem.mUpVector.mSign >= 0 ? "+" : "-",
                      sz[ axisSystem.mUpVector.mAxis ],
                      axisSystem.mCoorSystem.mSign >= 0 ? "+" : "-",
                      sz[ axisSystem.mCoorSystem.mAxis ] );
}

void PrintAxisSystem( const FbxAxisSystem& axisSystem ) {
    PrintAxisSystem( reinterpret_cast< const AxisSystem& >( axisSystem ) );
}

void ExportAxisSystem( FbxScene* pScene ) {
    auto& s = apemode::State::Get( );
    s.console->info( "\tCurrent axis system:" );
    PrintAxisSystem( pScene->GetGlobalSettings( ).GetAxisSystem( ) );
    s.console->info( "\tDirectX axis system:" );
    PrintAxisSystem( FbxAxisSystem::DirectX );
    s.console->info( "\tOpenGL axis system:" );
    PrintAxisSystem( FbxAxisSystem::OpenGL );
    s.console->info( "\tMayaYUp axis system:" );
    PrintAxisSystem( FbxAxisSystem::MayaYUp );
    s.console->info( "\tMayaZUp axis system:" );
    PrintAxisSystem( FbxAxisSystem::MayaZUp );
    s.console->info( "\tLightwave axis system:" );
    PrintAxisSystem( FbxAxisSystem::Lightwave );
    s.console->info( "\tMax axis system:" );
    PrintAxisSystem( FbxAxisSystem::Max );
    s.console->info( "\tMotionBuilder axis system:" );
    PrintAxisSystem( FbxAxisSystem::Motionbuilder );

    s.console->error( "\tConverting to viewer axis system ..." );
    // TODO: Check for the current axis system, and switch to Viewer's.
    auto rootScaling = pScene->GetRootNode( )->LclScaling.Get( );
    rootScaling[ 2 ] *= -1;
    pScene->GetRootNode( )->LclScaling.Set( rootScaling );
    s.console->error( "\tDone" );
}

void ExportBoundingBox( FbxScene* pScene ) {
    auto&      s = apemode::State::Get( );
    FbxVector4 bboxMin, bboxMax, bboxCenter;
    s.console->error( "\tCalculating the bounding box (bind pose) ..." );
    if ( pScene->ComputeBoundingBoxMinMaxCenter( bboxMin, bboxMax, bboxCenter ) ) {
        s.bboxMin.mutate_x( (float) bboxMin[ 0 ] );
        s.bboxMin.mutate_y( (float) bboxMin[ 1 ] );
        s.bboxMin.mutate_z( (float) bboxMin[ 2 ] );
        s.bboxMax.mutate_x( (float) bboxMax[ 0 ] );
        s.bboxMax.mutate_y( (float) bboxMax[ 1 ] );
        s.bboxMax.mutate_z( (float) bboxMax[ 2 ] );

        s.console->info( "\tBounding box: {} {} {} <-> {} {} {}",
                         s.bboxMin.x( ),
                         s.bboxMin.y( ),
                         s.bboxMin.z( ),
                         s.bboxMax.x( ),
                         s.bboxMax.y( ),
                         s.bboxMax.z( ) );
    } else {
        s.console->error( "\tBounding box: failed to calculate" );
    }
}

FBXPIPELINE_API void ExportScene( FbxScene* pScene ) {
    auto& s = apemode::State::Get( );
    InitializeSeachLocations( );

    // Pre-allocate nodes and attributes.
    s.nodes.reserve( (size_t) pScene->GetNodeCount( ) );
    s.meshes.reserve( (size_t) pScene->GetNodeCount( ) );

    ExportAxisSystem( pScene );
    ExportBoundingBox( pScene );

    // We want shared materials, so export all the scene material first
    // and reference them from the node scope by their indices.
    ExportMaterials( pScene );

    // Export nodes recursively.
    PreprocessAnimation( pScene );
    ExportNode( pScene->GetRootNode( ) );
    ExportSkeletons( pScene->GetRootNode( ) );

    // Export meshes.
    PreprocessMeshes( pScene );
    ExportMeshes( pScene->GetRootNode( ) );
}
