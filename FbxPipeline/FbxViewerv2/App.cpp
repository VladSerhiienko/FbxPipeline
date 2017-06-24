#include <fbxvpch.h>

#include <App.h>
#include <AppSurfaceSdlVk.h>
#include <Swapchain.Vulkan.h>
#include <Input.h>
#include <NuklearSdlVk.h>
#include <DebugRendererVk.h>

#include <AppState.h>

#include <Scene.h>

#include <EmbeddedShaderPreprocessor.h>

namespace apemode {
    using namespace apemodevk;

    const uint32_t kMaxFrames = Swapchain::kMaxImgs;
    AppContent * gAppContent = nullptr;
}

using namespace apemode;

namespace mathfu {
    // Left-handed system.
    const float kHandness = -1;

    static const float kPi    = M_PI;
    static const float kInvPi = 1 / M_PI;

    template < typename T >
    inline T NormalizedSafe( const T& _unsafeVec, float _safeEps = 1e-5f ) {
        const float length = _unsafeVec.Length( );
        const float invLength = 1.0f / ( length + _safeEps );

        return _unsafeVec * invLength;
    }

    template < typename T >
    inline T NormalizedSafeAndLength( const T& _unsafeVec, float& _outLength, float _safeEps = 1e-5f ) {
        const float length = _unsafeVec.Length( );
        const float invLength = 1.0f / ( length + _safeEps );

        _outLength = length;
        return _unsafeVec * invLength;
    }

    inline mathfu::vec3 VecFromLatLong( mathfu::vec2 _uv ) {
        const float phi   = _uv.x * 2.0f * kPi;
        const float theta = _uv.y * kPi;

        const float st = sin( theta );
        const float sp = sin( phi );
        const float ct = cos( theta );
        const float cp = cos( phi );

        return {-st * sp, ct, -st * cp};
    }

    inline mathfu::vec2 LatLongFromVec( const mathfu::vec3& _vec ) {
        const float phi   = atan2f( _vec[ 0 ], _vec[ 2 ] );
        const float theta = acosf( _vec[ 1 ] );

        return {( kPi + phi ) * kInvPi * 0.5f, theta * kInvPi};
    }
    static const float kSmallNumber     = 1.e-8f;
    static const float kKindSmallNumber = 1.e-4f;
    static const float kBigNumber       = 3.4e+38f;
    static const float kEulersNumber    = 2.71828182845904523536f;
    static const float kMaxFloat        = 3.402823466e+38f;
    static const float kInversePi       = 0.31830988618f;
    static const float kPiDiv2          = 1.57079632679f;
    static const float kSmallDelta      = 0.00001f;
    static const float k90              = kPiDiv2;
    static const float k180             = kPi;

    template < class T >
    inline auto RadiansToDegrees( T const& RadVal ) -> decltype( RadVal * ( 180.f / kPi ) ) {
        return RadVal * ( 180.f / kPi );
    }

    template < class T >
    inline auto DegreesToRadians( T const& DegVal ) -> decltype( DegVal * ( kPi / 180.f ) ) {
        return DegVal * ( kPi / 180.f );
    }

    inline bool IsNearlyEqual( float A, float B, float ErrorTolerance = kSmallNumber ) {
        return fabsf( A - B ) <= ErrorTolerance;
    }

    inline bool IsNearlyEqual( mathfu::vec2 const A, mathfu::vec2 const B, const float ErrorTolerance = kSmallNumber ) {
        return fabsf( A.x - B.x ) <= ErrorTolerance && fabsf( A.y - B.y ) <= ErrorTolerance;
    }

    inline bool IsNearlyEqual( mathfu::vec3 const A, mathfu::vec3 const B, const float ErrorTolerance = kSmallNumber ) {
        return fabsf( A.x - B.x ) <= ErrorTolerance && fabsf( A.y - B.y ) <= ErrorTolerance &&
            fabsf( A.z - B.z ) <= ErrorTolerance;
    }

    inline bool IsNearlyZero( float Value, float ErrorTolerance = kSmallNumber ) {
        return fabsf( Value ) <= ErrorTolerance;
    }

    inline bool IsNearlyZero( mathfu::vec2 const Value, float ErrorTolerance = kSmallNumber ) {
        return fabsf( Value.x ) <= ErrorTolerance && fabsf( Value.y ) <= ErrorTolerance;
    }

    inline bool IsNearlyZero( mathfu::vec3 const Value, float ErrorTolerance = kSmallNumber ) {
        return fabsf( Value.x ) <= ErrorTolerance && fabsf( Value.y ) <= ErrorTolerance &&
            fabsf( Value.z ) <= ErrorTolerance;
    }
}

namespace apemode {

    struct BasicCamera {
        mathfu::mat4 ViewMatrix;
        mathfu::mat4 ProjMatrix;
    };

    struct CameraProjectionController {

        mathfu::mat4 ProjMatrix( float _fieldOfViewDegs, float _width, float _height, float _nearZ, float _farZ ) {
            /* https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/ */
            const mathfu::mat4 kProjBias{{1.f, 0.0f, 0.0f, 0.0f},
                                         {0.f, -1.f, 0.0f, 0.0f}, /* Flip y */
                                         {0.f, 0.0f, 0.5f, 0.5f}, /* Z (-1, 1) to (0, 1) */
                                         {0.f, 0.0f, 0.0f, 1.0f}};


            const float fovRads = _fieldOfViewDegs / 180.0f * M_PI;
            const float aspectWOverH = _width / _height;
            return kProjBias * mathfu::mat4::Perspective( fovRads, aspectWOverH, _nearZ, _farZ, mathfu::kHandness );
        }

    };

    struct ModelViewCameraController {
        mathfu::vec3 Target;
        mathfu::vec3 Position;
        mathfu::vec3 TargetDst;
        mathfu::vec3 PositionDst;
        mathfu::vec2 OrbitCurr;
        mathfu::vec2 ZRange;

        ModelViewCameraController( ) {
            ZRange.x = 0.1f;
            ZRange.y = 1000.0f;
            Reset( );
        }

        void Reset( ) {
            Target      = {0, 0, 0};
            Position    = {0, 0, -5};
            TargetDst   = {0, 0, 0};
            PositionDst = {0, 0, -5};
            OrbitCurr   = {0, 0};
        }

        mathfu::mat4 ViewMatrix( ) {
            return mathfu::mat4::LookAt( Target, Position, {0, 1, 0}, mathfu::kHandness );
        }

        void Orbit( mathfu::vec2 _dxdy ) {
            OrbitCurr += _dxdy;
        }

        void Dolly( mathfu::vec3 _dzxy )  {
            float toTargetLen;
            const mathfu::vec3 toTargetNorm = mathfu::NormalizedSafeAndLength( TargetDst - PositionDst, toTargetLen );

            float delta  = toTargetLen * _dzxy.z;
            float newLen = toTargetLen + delta;
            if ( ( ZRange.x < newLen || _dzxy.z < 0.0f ) && ( newLen < ZRange.y || _dzxy.z > 0.0f ) ) {
                PositionDst += toTargetNorm * delta;
            }
        }

        void ConsumeOrbit( float _amount ) {
            mathfu::vec2 consume = OrbitCurr * _amount;
            OrbitCurr -= consume;

            float toPosLen;
            const mathfu::vec3 toPosNorm = mathfu::NormalizedSafeAndLength( Position - Target, toPosLen );

            mathfu::vec2 ll = mathfu::LatLongFromVec( toPosNorm ) + consume * mathfu::vec2( 1, -1 );
            ll.y = mathfu::Clamp( ll.y, 0.02f, 0.98f );

            const mathfu::vec3 tmp  = mathfu::VecFromLatLong( ll );
            const mathfu::vec3 diff = ( tmp - toPosNorm ) * toPosLen;

            Position += diff;
            PositionDst += diff;
        }

        void Update( float _dt ) {
            const float amount = std::min( _dt / 0.1f, 1.0f );

            ConsumeOrbit( amount );

            Target   = mathfu::Lerp( Target, TargetDst, amount );
            Position = mathfu::Lerp( Position, PositionDst, amount );
        }

        mathfu::mat4 EnvViewMatrix( ) {
            const mathfu::vec3 forward = mathfu::NormalizedSafe( Target - Position );
            const mathfu::vec3 right   = mathfu::vec3::CrossProduct( {0.0f, 1.0f, 0.0f}, forward ).Normalized( );
            const mathfu::vec3 up      = mathfu::vec3::CrossProduct( forward, right ).Normalized( );

            return mathfu::mat4( mathfu::vec4{right,   0.0f},
                                 mathfu::vec4{up,      0.0f},
                                 mathfu::vec4{forward, 0.0f},
                                 mathfu::vec4{0.0f, 0.0f, 0.0f, 1.0f} );
        }
    };

    struct FreeLookCameraController {
        mathfu::vec3 Target;
        mathfu::vec3 Position;
        mathfu::vec3 TargetDst;
        mathfu::vec3 PositionDst;
        mathfu::vec2 OrbitCurr;
        mathfu::vec2 ZRange;

        FreeLookCameraController( ) {
            ZRange.x = 0.1f;
            ZRange.y = 1000.0f;
            Reset( );
        }

        void Reset( ) {
            Target      = {0, 0, 5};
            Position    = {0, 0, 0};
            TargetDst   = {0, 0, 5};
            PositionDst = {0, 0, 0};
            OrbitCurr   = {0, 0};
        }

        mathfu::mat4 ViewMatrix( ) {
            return mathfu::mat4::LookAt( Target, Position, {0, 1, 0}, mathfu::kHandness );
        }

        void Orbit( mathfu::vec2 _dxdy ) {
            OrbitCurr += _dxdy;
        }

        void Dolly( mathfu::vec3 _dxyz ) {
            float toTargetLen;
            const mathfu::vec3 toTargetNorm = mathfu::NormalizedSafeAndLength( TargetDst - PositionDst, toTargetLen );
            const mathfu::vec3 right = mathfu::vec3::CrossProduct( { 0.0f, 1.0f, 0.0f }, toTargetNorm ); /* Already normalized */
            const mathfu::vec3 up = mathfu::vec3::CrossProduct( toTargetNorm, right );

            float deltaZ  = toTargetLen * _dxyz.z;
            TargetDst += toTargetNorm * deltaZ;
            PositionDst += toTargetNorm * deltaZ;

            float deltaX  = toTargetLen * _dxyz.x;
            TargetDst += right * deltaX;
            PositionDst += right * deltaX;

            float deltaY  = toTargetLen * _dxyz.y;
            TargetDst += up * deltaY;
            PositionDst += up * deltaY;
        }

        void ConsumeOrbit(float _amount) {

            float toPosLen;
            const mathfu::vec3 toPosNorm = mathfu::NormalizedSafeAndLength(Position - Target, toPosLen);
            mathfu::vec2 ll = mathfu::LatLongFromVec(toPosNorm);

            mathfu::vec2 consume = OrbitCurr * _amount;
            OrbitCurr -= consume;

            consume.y *= (ll.y < 0.02 && consume.y < 0) || (ll.y > 0.98 && consume.y > 0) ? 0 : -1;
            ll += consume;

            const mathfu::vec3 tmp = mathfu::VecFromLatLong(ll);
            mathfu::vec3 diff = (tmp - toPosNorm) * toPosLen;

            Target += diff;
            TargetDst += diff;

            const mathfu::vec3 dstDiff = mathfu::NormalizedSafe(TargetDst - PositionDst);
            TargetDst = PositionDst + dstDiff * (ZRange.y - ZRange.x) * 0.1f;
        }

        void Update( float _dt ) {
            const float amount = std::min( _dt / 0.1f, 1.0f );

            ConsumeOrbit( amount );

            Target   = mathfu::Lerp( Target, TargetDst, amount );
            Position = mathfu::Lerp( Position, PositionDst, amount );
        }

        mathfu::mat4 EnvViewMatrix( ) {
            const mathfu::vec3 forward = mathfu::NormalizedSafe( Target - Position );
            const mathfu::vec3 right   = mathfu::vec3::CrossProduct( {0.0f, 1.0f, 0.0f}, forward ).Normalized( );
            const mathfu::vec3 up      = mathfu::vec3::CrossProduct( forward, right ).Normalized( );

            return mathfu::mat4( mathfu::vec4{right,   0.0f},
                                 mathfu::vec4{up,      0.0f},
                                 mathfu::vec4{forward, 0.0f},
                                 mathfu::vec4{0.0f, 0.0f, 0.0f, 1.0f} );
        }
    };

    /* Adapts the input for the camera */
    struct CameraMouseInput {
        CameraMouseInput( )
            : Delta( 0.0f )
            , Scroll( 0.0f )
            , PrevPosition( std::numeric_limits<float>::max() )
            , PrevScroll( std::numeric_limits<float>::max() ) {
        }

        void Update( mathfu::vec3 _mxyz, mathfu::vec2 _widthHeight ) {
            if ( PrevScroll == std::numeric_limits< float >::max( ) ) {
                PrevScroll = _mxyz.z;
                PrevPosition = _mxyz.xy( );
            }

            // Delta movement.
            Delta = ( _mxyz.xy( ) - PrevPosition ) / _widthHeight;
            PrevPosition = _mxyz.xy( );

            // Scroll.
            Scroll = _mxyz.z - PrevScroll;
            PrevScroll = _mxyz.z;
        }

        mathfu::vec2 Delta;
        mathfu::vec2 PrevPosition;
        float Scroll;
        float PrevScroll;
    };
}

const VkFormat sDepthFormat = VK_FORMAT_D16_UNORM;

class apemode::AppContent {
public:

    nk_color diffColor;
    nk_color specColor;
    uint32_t width      = 0;
    uint32_t height     = 0;
    uint32_t resetFlags = 0;
    uint32_t envId      = 0;
    uint32_t sceneId    = 0;
    uint32_t maskId     = 0;
    Scene*   scenes[ 2 ];

    NuklearSdlBase* Nk = nullptr;
    DebugRendererVk * Dbg = nullptr;

    FreeLookCameraController CamController;
    //ModelViewCameraController CamController;
    CameraProjectionController CamProjController;
    CameraMouseInput CamMouseInput;


    uint32_t FrameCount = 0;
    uint32_t FrameIndex = 0;
    uint64_t FrameId    = 0;

    uint32_t BackbufferIndices[ kMaxFrames ] = {0};

    std::unique_ptr< DescriptorPool >      pDescPool;
    TDispatchableHandle< VkCommandPool >   hCmdPool[ kMaxFrames ];
    TDispatchableHandle< VkCommandBuffer > hCmdBuffers[ kMaxFrames ];
    TDispatchableHandle< VkFence >         hFences[ kMaxFrames ];
    TDispatchableHandle< VkSemaphore >     hPresentCompleteSemaphores[ kMaxFrames ];
    TDispatchableHandle< VkSemaphore >     hRenderCompleteSemaphores[ kMaxFrames ];

    TDispatchableHandle< VkRenderPass >    hNkRenderPass;
    TDispatchableHandle< VkFramebuffer >   hNkFramebuffers[ kMaxFrames ];

    TDispatchableHandle< VkRenderPass >    hDbgRenderPass;
    TDispatchableHandle< VkFramebuffer >   hDbgFramebuffers[ kMaxFrames ];
    TDispatchableHandle< VkImage >         hDepthImgs[ kMaxFrames ];
    TDispatchableHandle< VkImageView >     hDepthImgViews[ kMaxFrames ];
    TDispatchableHandle< VkDeviceMemory >  hDepthImgMemory[ kMaxFrames ];

    AppContent()  {
    }

    ~AppContent() {
    }
};

App::App( ) : appState(new AppState()) {
    if (nullptr != appState)
        appState->appOptions->add_options("vk")
            ("renderdoc", "Adds renderdoc layer to device layers")
            ("vkapidump", "Adds api dump layer to vk device layers")
            ("vktrace", "Adds vktrace layer to vk device layers");
}

App::~App( ) {
    if (nullptr != appState)
        delete appState;

    if ( nullptr != appContent )
        delete appContent;
}

IAppSurface* App::CreateAppSurface( ) {
    return new AppSurfaceSdlVk( );
}

bool App::Initialize( int Args, char* ppArgs[] ) {

    if (appState && appState->appOptions)
        appState->appOptions->parse(Args, ppArgs);

    if ( AppBase::Initialize( Args, ppArgs ) ) {

        if ( nullptr == appContent )
            appContent = new AppContent( );

        totalSecs = 0.0f;

        auto appSurface = GetSurface();
        if (appSurface->GetImpl() != kAppSurfaceImpl_SdlVk)
            return false;

        auto appSurfaceVk = (AppSurfaceSdlVk*)appSurface;
        if ( auto swapchain = appSurfaceVk->pSwapchain.get( ) ) {
            appContent->FrameCount = swapchain->ImgCount;
            appContent->FrameIndex = 0;
            appContent->FrameId    = 0;
           
            OnResized();

            for (uint32_t i = 0; i < appContent->FrameCount; ++i) {
                VkCommandPoolCreateInfo cmdPoolCreateInfo;
                InitializeStruct( cmdPoolCreateInfo );
                cmdPoolCreateInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                cmdPoolCreateInfo.queueFamilyIndex = appSurfaceVk->pCmdQueue->QueueFamilyId;

                if ( false == appContent->hCmdPool[ i ].Recreate( *appSurfaceVk->pNode, cmdPoolCreateInfo ) ) {
                    DebugBreak( );
                    return false;
                }

                VkCommandBufferAllocateInfo cmdBufferAllocInfo;
                InitializeStruct( cmdBufferAllocInfo );
                cmdBufferAllocInfo.commandPool        = appContent->hCmdPool[ i ];
                cmdBufferAllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                cmdBufferAllocInfo.commandBufferCount = 1;

                if ( false == appContent->hCmdBuffers[ i ].Recreate( *appSurfaceVk->pNode, cmdBufferAllocInfo ) ) {
                    DebugBreak( );
                    return false;
                }

                VkFenceCreateInfo fenceCreateInfo;
                InitializeStruct( fenceCreateInfo );
                fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

                if ( false == appContent->hFences[ i ].Recreate( *appSurfaceVk->pNode, fenceCreateInfo ) ) {
                    DebugBreak( );
                    return false;
                }

                VkSemaphoreCreateInfo semaphoreCreateInfo;
                InitializeStruct( semaphoreCreateInfo );
                if ( false == appContent->hPresentCompleteSemaphores[ i ].Recreate( *appSurfaceVk->pNode, semaphoreCreateInfo ) ||
                     false == appContent->hRenderCompleteSemaphores[ i ].Recreate( *appSurfaceVk->pNode, semaphoreCreateInfo ) ) {
                    DebugBreak( );
                    return false;
                }
            }
        }

        appContent->pDescPool = std::move(std::make_unique< DescriptorPool >());
        if (false == appContent->pDescPool->RecreateResourcesFor(*appSurfaceVk->pNode, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256 )) {
            DebugBreak();
            return false;
        }

        appContent->Nk = new NuklearSdlVk();

        NuklearSdlVk::InitParametersVk initParamsNk;
        initParamsNk.pAlloc          = nullptr;
        initParamsNk.pDevice         = *appSurfaceVk->pNode;
        initParamsNk.pPhysicalDevice = *appSurfaceVk->pNode;
        initParamsNk.pRenderPass     = appContent->hDbgRenderPass;
        //initParamsNk.pRenderPass     = appContent->hNkRenderPass;
        initParamsNk.pDescPool       = *appContent->pDescPool;
        initParamsNk.pQueue          = *appSurfaceVk->pCmdQueue;
        initParamsNk.QueueFamilyId   = appSurfaceVk->pCmdQueue->QueueFamilyId;

        appContent->Nk->Init( &initParamsNk );

        appContent->Dbg = new DebugRendererVk();

        DebugRendererVk::InitParametersVk initParamsDbg;
        initParamsDbg.pAlloc          = nullptr;
        initParamsDbg.pDevice         = *appSurfaceVk->pNode;
        initParamsDbg.pPhysicalDevice = *appSurfaceVk->pNode;
        initParamsDbg.pRenderPass     = appContent->hDbgRenderPass;
        initParamsDbg.pDescPool       = *appContent->pDescPool;
        initParamsDbg.FrameCount      = appContent->FrameCount;

        appContent->Dbg->RecreateResources(&initParamsDbg);

        // appContent->scenes[ 0 ] = LoadSceneFromFile( "../../../assets/iron-man.fbxp" );
        // appContent->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/kalestra-the-sorceress.fbxp" );
        // appContent->scenes[ 0 ] = LoadSceneFromFile( "../../../assets/Mech6kv3ps.fbxp" );
        // appContent->scenes[ 0 ] = LoadSceneFromFile( "../../../assets/Mech6k_v2.fbxp" );
        // appContent->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/P90_v2.fbxp" );
        // appContent->scenes[ 0 ] = LoadSceneFromFile( "../../../assets/MercedesBenzA45AMG.fbxp" );
        // appContent->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/MercedesBenzSLR.fbxp" );
        // appContent->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/P90.fbxp" );
        // appContent->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/IronMan.fbxp" );
        // appContent->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/Cathedral.fbxp" );
        // appContent->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/Leica1933.fbxp" );
        // appContent->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/UnrealOrb.fbxp" );
        // appContent->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/Artorias.fbxp" );
        // appContent->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/9mm.fbxp" );
        // appContent->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/Knife.fbxp" );
        // appContent->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/mech-m-6k.fbxp" );

        return true;
    }

    return false;
}

bool apemode::App::OnResized( ) {
    if ( auto appSurfaceVk = (AppSurfaceSdlVk*) GetSurface( ) ) {
        if ( auto swapchain = appSurfaceVk->pSwapchain.get( ) ) {
            appContent->width  = appSurfaceVk->GetWidth( );
            appContent->height = appSurfaceVk->GetHeight( );

            VkImageCreateInfo depthImgCreateInfo;
            InitializeStruct( depthImgCreateInfo );
            depthImgCreateInfo.imageType     = VK_IMAGE_TYPE_2D;
            depthImgCreateInfo.format        = sDepthFormat;
            depthImgCreateInfo.extent.width  = swapchain->ColorExtent.width;
            depthImgCreateInfo.extent.height = swapchain->ColorExtent.height;
            depthImgCreateInfo.extent.depth  = 1;
            depthImgCreateInfo.mipLevels     = 1;
            depthImgCreateInfo.arrayLayers   = 1;
            depthImgCreateInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
            depthImgCreateInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
            depthImgCreateInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

            VkImageViewCreateInfo depthImgViewCreateInfo;
            InitializeStruct( depthImgViewCreateInfo );
            depthImgViewCreateInfo.format                          = sDepthFormat;
            depthImgViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
            depthImgViewCreateInfo.subresourceRange.baseMipLevel   = 0;
            depthImgViewCreateInfo.subresourceRange.levelCount     = 1;
            depthImgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            depthImgViewCreateInfo.subresourceRange.layerCount     = 1;
            depthImgViewCreateInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;

            for ( uint32_t i = 0; i < appContent->FrameCount; ++i ) {
                if ( false == appContent->hDepthImgs[ i ].Recreate( *appSurfaceVk->pNode, *appSurfaceVk->pNode, depthImgCreateInfo ) ) {
                    DebugBreak( );
                    return false;
                }
            }

            for ( uint32_t i = 0; i < appContent->FrameCount; ++i ) {
                auto memoryAllocInfo = appContent->hDepthImgs[ i ].GetMemoryAllocateInfo( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
                if ( false == appContent->hDepthImgMemory[ i ].Recreate( *appSurfaceVk->pNode, memoryAllocInfo ) ) {
                    DebugBreak( );
                    return false;
                }
            }

            for ( uint32_t i = 0; i < appContent->FrameCount; ++i ) {
                if ( false == appContent->hDepthImgs[ i ].BindMemory( appContent->hDepthImgMemory[ i ], 0 ) ) {
                    DebugBreak( );
                    return false;
                }
            }

            for ( uint32_t i = 0; i < appContent->FrameCount; ++i ) {
                depthImgViewCreateInfo.image = appContent->hDepthImgs[ i ];
                if ( false == appContent->hDepthImgViews[ i ].Recreate( *appSurfaceVk->pNode, depthImgViewCreateInfo ) ) {
                    DebugBreak( );
                    return false;
                }
            }

            VkAttachmentDescription colorDepthAttachments[ 2 ];
            InitializeStruct( colorDepthAttachments );

            colorDepthAttachments[ 0 ].format         = swapchain->eColorFormat;
            colorDepthAttachments[ 0 ].samples        = VK_SAMPLE_COUNT_1_BIT;
            colorDepthAttachments[ 0 ].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorDepthAttachments[ 0 ].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            colorDepthAttachments[ 0 ].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorDepthAttachments[ 0 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorDepthAttachments[ 0 ].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            colorDepthAttachments[ 0 ].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            colorDepthAttachments[ 1 ].format         = sDepthFormat;
            colorDepthAttachments[ 1 ].samples        = VK_SAMPLE_COUNT_1_BIT;
            colorDepthAttachments[ 1 ].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorDepthAttachments[ 1 ].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE; // ?
            colorDepthAttachments[ 1 ].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorDepthAttachments[ 1 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorDepthAttachments[ 1 ].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            colorDepthAttachments[ 1 ].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkAttachmentReference colorAttachmentRef;
            InitializeStruct( colorAttachmentRef );
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentReference depthAttachmentRef;
            InitializeStruct( depthAttachmentRef );
            depthAttachmentRef.attachment = 1;
            depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpassNk;
            InitializeStruct( subpassNk );
            subpassNk.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassNk.colorAttachmentCount = 1;
            subpassNk.pColorAttachments    = &colorAttachmentRef;

            VkSubpassDescription subpassDbg;
            InitializeStruct( subpassDbg );
            subpassDbg.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDbg.colorAttachmentCount    = 1;
            subpassDbg.pColorAttachments       = &colorAttachmentRef;
            subpassDbg.pDepthStencilAttachment = &depthAttachmentRef;

            VkRenderPassCreateInfo renderPassCreateInfoNk;
            InitializeStruct( renderPassCreateInfoNk );
            renderPassCreateInfoNk.attachmentCount = 1;
            renderPassCreateInfoNk.pAttachments    = &colorDepthAttachments[ 0 ];
            renderPassCreateInfoNk.subpassCount    = 1;
            renderPassCreateInfoNk.pSubpasses      = &subpassNk;

            VkRenderPassCreateInfo renderPassCreateInfoDbg;
            InitializeStruct( renderPassCreateInfoDbg );
            renderPassCreateInfoDbg.attachmentCount = 2;
            renderPassCreateInfoDbg.pAttachments    = &colorDepthAttachments[ 0 ];
            renderPassCreateInfoDbg.subpassCount    = 1;
            renderPassCreateInfoDbg.pSubpasses      = &subpassDbg;

            if ( false == appContent->hNkRenderPass.Recreate( *appSurfaceVk->pNode, renderPassCreateInfoNk ) ) {
                DebugBreak( );
                return false;
            }

            if ( false == appContent->hDbgRenderPass.Recreate( *appSurfaceVk->pNode, renderPassCreateInfoDbg ) ) {
                DebugBreak( );
                return false;
            }

            VkFramebufferCreateInfo framebufferCreateInfoNk;
            InitializeStruct( framebufferCreateInfoNk );
            framebufferCreateInfoNk.renderPass      = appContent->hNkRenderPass;
            framebufferCreateInfoNk.attachmentCount = 1;
            framebufferCreateInfoNk.width           = swapchain->ColorExtent.width;
            framebufferCreateInfoNk.height          = swapchain->ColorExtent.height;
            framebufferCreateInfoNk.layers          = 1;

            VkFramebufferCreateInfo framebufferCreateInfoDbg;
            InitializeStruct( framebufferCreateInfoDbg );
            framebufferCreateInfoDbg.renderPass      = appContent->hDbgRenderPass;
            framebufferCreateInfoDbg.attachmentCount = 2;
            framebufferCreateInfoDbg.width           = swapchain->ColorExtent.width;
            framebufferCreateInfoDbg.height          = swapchain->ColorExtent.height;
            framebufferCreateInfoDbg.layers          = 1;

            for ( uint32_t i = 0; i < appContent->FrameCount; ++i ) {
                VkImageView attachments[ 1 ] = {swapchain->hImgViews[ i ]};
                framebufferCreateInfoNk.pAttachments = attachments;

                if ( false == appContent->hNkFramebuffers[ i ].Recreate( *appSurfaceVk->pNode, framebufferCreateInfoNk ) ) {
                    DebugBreak( );
                    return false;
                }
            }

            for ( uint32_t i = 0; i < appContent->FrameCount; ++i ) {
                VkImageView attachments[ 2 ] = {swapchain->hImgViews[ i ], appContent->hDepthImgViews[ i ]};
                framebufferCreateInfoDbg.pAttachments = attachments;

                if ( false == appContent->hDbgFramebuffers[ i ].Recreate( *appSurfaceVk->pNode, framebufferCreateInfoDbg ) ) {
                    DebugBreak( );
                    return false;
                }
            }
        }
    }

    return true;
}

void App::OnFrameMove( ) {
    nk_input_begin( &appContent->Nk->Context ); {
        SDL_Event evt;
        while ( SDL_PollEvent( &evt ) )
            appContent->Nk->HandleEvent( &evt );
        nk_input_end( &appContent->Nk->Context );
    }

    AppBase::OnFrameMove( );

    ++appContent->FrameId;
    appContent->FrameIndex = appContent->FrameId % (uint64_t) appContent->FrameCount;
}

void App::Update( float deltaSecs, Input const& inputState ) {
    totalSecs += deltaSecs;

    bool hovered = false;
    bool reset   = false;

    const nk_flags windowFlags
        = NK_WINDOW_BORDER
        | NK_WINDOW_MOVABLE
        | NK_WINDOW_SCALABLE
        | NK_WINDOW_MINIMIZABLE;

    auto ctx = &appContent->Nk->Context;
    float clearColor[ 4 ] = {0};

    if (nk_begin(ctx, "Calculator", nk_rect(10, 10, 180, 250),
        NK_WINDOW_BORDER|NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_MOVABLE))
    {
        static int set = 0, prev = 0, op = 0;
        static const char numbers[] = "789456123";
        static const char ops[] = "+-*/";
        static double a = 0, b = 0;
        static double *current = &a;

        size_t i = 0;
        int solve = 0;
        {int len; char buffer[256];
        nk_layout_row_dynamic(ctx, 35, 1);
        len = snprintf(buffer, 256, "%.2f", *current);
        nk_edit_string(ctx, NK_EDIT_SIMPLE, buffer, &len, 255, nk_filter_float);
        buffer[len] = 0;
        *current = atof(buffer);}

        nk_layout_row_dynamic(ctx, 35, 4);
        for (i = 0; i < 16; ++i) {
            if (i >= 12 && i < 15) {
                if (i > 12) continue;
                if (nk_button_label(ctx, "C")) {
                    a = b = op = 0; current = &a; set = 0;
                } if (nk_button_label(ctx, "0")) {
                    *current = *current*10.0f; set = 0;
                } if (nk_button_label(ctx, "=")) {
                    solve = 1; prev = op; op = 0;
                }
            } else if (((i+1) % 4)) {
                if (nk_button_text(ctx, &numbers[(i/4)*3+i%4], 1)) {
                    *current = *current * 10.0f + numbers[(i/4)*3+i%4] - '0';
                    set = 0;
                }
            } else if (nk_button_text(ctx, &ops[i/4], 1)) {
                if (!set) {
                    if (current != &b) {
                        current = &b;
                    } else {
                        prev = op;
                        solve = 1;
                    }
                }
                op = ops[i/4];
                set = 1;
            }
        }
        if (solve) {
            if (prev == '+') a = a + b;
            if (prev == '-') a = a - b;
            if (prev == '*') a = a * b;
            if (prev == '/') a = a / b;
            current = &a;
            if (set) current = &b;
            b = 0; set = 0;
        }
    }
    nk_end(ctx);

    appContent->CamMouseInput.Update( {inputState.Analogs[ kAnalogInput_MouseX ],
                                       inputState.Analogs[ kAnalogInput_MouseY ],
                                       inputState.Analogs[ kAnalogInput_MouseScroll ]},
                                      {(float) appContent->width, (float) appContent->height} );

    if ( inputState.Buttons[ 0 ][ kDigitalInput_Mouse0 ] ) {
        appContent->CamController.Orbit( appContent->CamMouseInput.Delta );
    } else if ( inputState.Buttons[ 0 ][ kDigitalInput_Mouse1 ] ) {
        auto scroll = appContent->CamMouseInput.Delta.x + appContent->CamMouseInput.Delta.y;
        appContent->CamController.Dolly({ 0, 0, scroll });
    }

    mathfu::vec3 dzxy = { 0, 0, 0 };
    dzxy.z += (inputState.Buttons[0][kDigitalInput_KeyW] || inputState.Buttons[0][kDigitalInput_KeyUp]) * deltaSecs;
    dzxy.z -= (inputState.Buttons[0][kDigitalInput_KeyS] || inputState.Buttons[0][kDigitalInput_KeyDown]) * deltaSecs;
    dzxy.x += (inputState.Buttons[0][kDigitalInput_KeyD] || inputState.Buttons[0][kDigitalInput_KeyRight]) * deltaSecs;
    dzxy.x -= (inputState.Buttons[0][kDigitalInput_KeyA] || inputState.Buttons[0][kDigitalInput_KeyLeft]) * deltaSecs;
    dzxy.y += (inputState.Buttons[0][kDigitalInput_KeyE]) * deltaSecs;
    dzxy.y -= (inputState.Buttons[0][kDigitalInput_KeyQ]) * deltaSecs;

    appContent->CamController.Dolly( dzxy );
    appContent->CamController.Update( deltaSecs );

    if ( auto appSurfaceVk = (AppSurfaceSdlVk*) GetSurface( ) ) {
        VkDevice        device                   = *appSurfaceVk->pNode;
        VkQueue         queue                    = *appSurfaceVk->pCmdQueue;
        VkSwapchainKHR  swapchain                = appSurfaceVk->pSwapchain->hSwapchain;
        VkFence         fence                    = appContent->hFences[ appContent->FrameIndex ];
        VkSemaphore     presentCompleteSemaphore = appContent->hPresentCompleteSemaphores[ appContent->FrameIndex ];
        VkSemaphore     renderCompleteSemaphore  = appContent->hRenderCompleteSemaphores[ appContent->FrameIndex ];
        VkCommandPool   cmdPool                  = appContent->hCmdPool[ appContent->FrameIndex ];
        VkCommandBuffer cmdBuffer                = appContent->hCmdBuffers[ appContent->FrameIndex ];

        const uint32_t width  = appSurfaceVk->GetWidth( );
        const uint32_t height = appSurfaceVk->GetHeight( );

        if ( width != appContent->width || height != appContent->height ) {
            CheckedCall( vkDeviceWaitIdle( device ) );
            OnResized( );

        }

        VkFramebuffer framebufferNk = appContent->hNkFramebuffers[ appContent->FrameIndex ];
        VkFramebuffer framebufferDbg = appContent->hDbgFramebuffers[ appContent->FrameIndex ];

        while (true) {
            const auto waitForFencesErrorHandle = vkWaitForFences( device, 1, &fence, VK_TRUE, 100 );
            if ( VK_SUCCESS == waitForFencesErrorHandle ) {
                break;
            } else if ( VK_TIMEOUT == waitForFencesErrorHandle ) {
                continue;
            } else {
                assert( false );
                return;
            }
        }

        CheckedCall( vkAcquireNextImageKHR( device,
                                            swapchain,
                                            UINT64_MAX,
                                            presentCompleteSemaphore,
                                            VK_NULL_HANDLE,
                                            &appContent->BackbufferIndices[ appContent->FrameIndex ] ) );

        CheckedCall( vkResetCommandPool( device, cmdPool, 0 ) );

        VkCommandBufferBeginInfo commandBufferBeginInfo;
        InitializeStruct( commandBufferBeginInfo );
        commandBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        CheckedCall( vkBeginCommandBuffer( cmdBuffer, &commandBufferBeginInfo ) );

        VkClearValue clearValue[2];
        clearValue[0].color.float32[ 0 ] = clearColor[ 0 ];
        clearValue[0].color.float32[ 1 ] = clearColor[ 1 ];
        clearValue[0].color.float32[ 2 ] = clearColor[ 2 ];
        clearValue[0].color.float32[ 3 ] = clearColor[ 3 ];
        clearValue[1].depthStencil.depth = 1;
        clearValue[1].depthStencil.stencil = 0;

        VkRenderPassBeginInfo renderPassBeginInfo;
        InitializeStruct( renderPassBeginInfo );
        renderPassBeginInfo.renderPass               = appContent->hDbgRenderPass;
        renderPassBeginInfo.framebuffer              = appContent->hDbgFramebuffers[ appContent->FrameIndex ];
        renderPassBeginInfo.renderArea.extent.width  = appSurfaceVk->GetWidth( );
        renderPassBeginInfo.renderArea.extent.height = appSurfaceVk->GetHeight( );
        renderPassBeginInfo.clearValueCount          = 2;
        renderPassBeginInfo.pClearValues             = clearValue;

        vkCmdBeginRenderPass( cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

        DebugRendererVk::FrameUniformBuffer frameData;

        static float rotationY = 0.0;

        rotationY += 0.001;
        if (rotationY >= 2 * M_PI) {
            rotationY -= 2 * M_PI;
        }

        frameData.worldMatrix = mathfu::mat4::FromRotationMatrix( mathfu::mat3::RotationY( rotationY ) );
        frameData.projectionMatrix = appContent->CamProjController.ProjMatrix(55, width, height, 0.1f, 100.0f);
        frameData.viewMatrix = appContent->CamController.ViewMatrix();
        frameData.color = {1, 0, 0, 1};

        appContent->Dbg->Reset( appContent->FrameIndex );

        DebugRendererVk::RenderParametersVk renderParamsDbg;
        renderParamsDbg.dims[ 0 ]  = (float) width;
        renderParamsDbg.dims[ 1 ]  = (float) height;
        renderParamsDbg.scale[ 0 ] = 1;
        renderParamsDbg.scale[ 1 ] = 1;
        renderParamsDbg.FrameIndex = appContent->FrameIndex;
        renderParamsDbg.pCmdBuffer = cmdBuffer;
        renderParamsDbg.pFrameData = &frameData;

        const float scale = 0.5f;

        frameData.worldMatrix
            = mathfu::mat4::FromScaleVector({ scale, scale * 2, scale })
            * mathfu::mat4::FromTranslationVector(mathfu::vec3{ 0, scale * 3, 0 });

        frameData.color = { 0, 1, 0, 1 };
        appContent->Dbg->Render(&renderParamsDbg);

        frameData.worldMatrix
            = mathfu::mat4::FromScaleVector({ scale, scale, scale * 2 })
            * mathfu::mat4::FromTranslationVector(mathfu::vec3{ 0, 0, scale * 3 });

        frameData.color = { 0, 0, 1, 1 };
        appContent->Dbg->Render(&renderParamsDbg);

        frameData.worldMatrix
            = mathfu::mat4::FromScaleVector({ scale * 2, scale, scale })
            * mathfu::mat4::FromTranslationVector(mathfu::vec3{ scale * 3, 0, 0 });

        frameData.color = { 1, 0, 0, 1 };
        appContent->Dbg->Render(&renderParamsDbg);

        NuklearSdlVk::RenderParametersVk renderParamsNk;
        renderParamsNk.dims[ 0 ]          = (float) width;
        renderParamsNk.dims[ 1 ]          = (float) height;
        renderParamsNk.scale[ 0 ]         = 1;
        renderParamsNk.scale[ 1 ]         = 1;
        renderParamsNk.aa                 = NK_ANTI_ALIASING_ON;
        renderParamsNk.max_vertex_buffer  = 64 * 1024;
        renderParamsNk.max_element_buffer = 64 * 1024;
        renderParamsNk.FrameIndex         = appContent->FrameIndex;
        renderParamsNk.pCmdBuffer         = cmdBuffer;

        appContent->Nk->Render( &renderParamsNk );
        nk_clear( &appContent->Nk->Context );

        vkCmdEndRenderPass( cmdBuffer );

        VkPipelineStageFlags waitPipelineStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submitInfo;
        InitializeStruct( submitInfo );
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = &renderCompleteSemaphore;
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = &presentCompleteSemaphore;
        submitInfo.pWaitDstStageMask    = &waitPipelineStage;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = &cmdBuffer;

        appContent->Dbg->Flush( appContent->FrameIndex );

        CheckedCall( vkEndCommandBuffer( cmdBuffer ) );
        CheckedCall( vkResetFences( device, 1, &fence ) );
        CheckedCall( vkQueueSubmit( queue, 1, &submitInfo, fence ) );

        if ( appContent->FrameId ) {
            uint32_t    presentIndex    = ( appContent->FrameIndex + appContent->FrameCount - 1 ) % appContent->FrameCount;
            VkSemaphore renderSemaphore = appContent->hRenderCompleteSemaphores[ presentIndex ];

            VkPresentInfoKHR presentInfoKHR;
            InitializeStruct( presentInfoKHR );
            presentInfoKHR.waitSemaphoreCount = 1;
            presentInfoKHR.pWaitSemaphores    = &renderSemaphore;
            presentInfoKHR.swapchainCount     = 1;
            presentInfoKHR.pSwapchains        = &swapchain;
            presentInfoKHR.pImageIndices      = &appContent->BackbufferIndices[ presentIndex ];

            CheckedCall( vkQueuePresentKHR( queue, &presentInfoKHR ) );
        }
    }
}

bool App::IsRunning( ) {
    return AppBase::IsRunning( );
}

extern "C" AppBase* CreateApp( ) {
    return new App( );
}