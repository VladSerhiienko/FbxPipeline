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
}

using namespace apemode;

static const uint32_t kMaxFrames = Swapchain::kMaxImgs;
static apemode::AppContent * gAppContent = nullptr;

struct Camera {
    mathfu::mat4 View, Proj;
};

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

    uint32_t FrameCount = 0;
    uint32_t FrameIndex = 0;
    uint64_t FrameId    = 0;

    uint32_t BackbufferIndices[ kMaxFrames ] = {0};

    std::unique_ptr< DescriptorPool >      pDescPool;
    TDispatchableHandle< VkRenderPass >    hNkRenderPass;
    TDispatchableHandle< VkFramebuffer >   hNkFramebuffers[ kMaxFrames ];
    TDispatchableHandle< VkCommandPool >   hCmdPool[ kMaxFrames ];
    TDispatchableHandle< VkCommandBuffer > hCmdBuffers[ kMaxFrames ];
    TDispatchableHandle< VkFence >         hFences[ kMaxFrames ];
    TDispatchableHandle< VkSemaphore >     hPresentCompleteSemaphores[ kMaxFrames ];
    TDispatchableHandle< VkSemaphore >     hRenderCompleteSemaphores[ kMaxFrames ];

    TDispatchableHandle< VkRenderPass >    hRenderPass;
    TDispatchableHandle< VkFramebuffer >   hFramebuffers[ kMaxFrames ];
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
        initParamsNk.pRenderPass     = appContent->hNkRenderPass;
        initParamsNk.pDescPool       = *appContent->pDescPool;
        initParamsNk.pQueue          = *appSurfaceVk->pCmdQueue;
        initParamsNk.QueueFamilyId   = appSurfaceVk->pCmdQueue->QueueFamilyId;

        appContent->Nk->Init( &initParamsNk );

        appContent->Dbg = new DebugRendererVk();

        DebugRendererVk::InitParametersVk initParamsDbg;
        initParamsDbg.pAlloc          = nullptr;
        initParamsDbg.pDevice         = *appSurfaceVk->pNode;
        initParamsDbg.pPhysicalDevice = *appSurfaceVk->pNode;
        initParamsDbg.pRenderPass     = appContent->hNkRenderPass;
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
            renderPassCreateInfoDbg.pSubpasses      = &subpassNk;

            if ( false == appContent->hNkRenderPass.Recreate( *appSurfaceVk->pNode, renderPassCreateInfoNk ) ) {
                DebugBreak( );
                return false;
            }

            if ( false == appContent->hRenderPass.Recreate( *appSurfaceVk->pNode, renderPassCreateInfoDbg ) ) {
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

            VkFramebufferCreateInfo framebufferCreateInfo;
            InitializeStruct( framebufferCreateInfo );
            framebufferCreateInfo.renderPass      = appContent->hRenderPass;
            framebufferCreateInfo.attachmentCount = 2;
            framebufferCreateInfo.width           = swapchain->ColorExtent.width;
            framebufferCreateInfo.height          = swapchain->ColorExtent.height;
            framebufferCreateInfo.layers          = 1;

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
                framebufferCreateInfo.pAttachments = attachments;

                if ( false == appContent->hFramebuffers[ i ].Recreate( *appSurfaceVk->pNode, framebufferCreateInfo ) ) {
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

        VkRenderPass  renderPass  = appContent->hNkRenderPass;
        VkFramebuffer framebuffer = appContent->hNkFramebuffers[ appContent->FrameIndex ];

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

        VkClearValue clearValue;
        clearValue.color.float32[ 0 ] = clearColor[ 0 ];
        clearValue.color.float32[ 1 ] = clearColor[ 1 ];
        clearValue.color.float32[ 2 ] = clearColor[ 2 ];
        clearValue.color.float32[ 3 ] = clearColor[ 3 ];

        VkRenderPassBeginInfo renderPassBeginInfo;
        InitializeStruct( renderPassBeginInfo );
        renderPassBeginInfo.renderPass               = renderPass;
        renderPassBeginInfo.framebuffer              = framebuffer;
        renderPassBeginInfo.renderArea.extent.width  = appSurfaceVk->GetWidth( );
        renderPassBeginInfo.renderArea.extent.height = appSurfaceVk->GetHeight( );
        renderPassBeginInfo.clearValueCount          = 1;
        renderPassBeginInfo.pClearValues             = &clearValue;

        vkCmdBeginRenderPass( cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

        DebugRendererVk::FrameUniformBuffer frameData;

        static float rotationY = 0.0;

        rotationY += 0.001;
        if (rotationY >= M_PI) {
            rotationY -= M_PI;
        }

        frameData.worldMatrix = mathfu::mat4::FromRotationMatrix( mathfu::mat3::RotationY( rotationY ) );
        frameData.projectionMatrix = mathfu::mat4::Perspective( 55.0f / 180.0f * M_PI, (float) width / (float) height, 0.1f, 100.0f, 1 );
        frameData.viewMatrix = mathfu::mat4::LookAt( {0, 0, 0}, {0, 3, 5}, {0, 1, 0}, 1 );
        frameData.color = {1, 0, 0, 1};

        // Flip projection matrix from GL to Vulkan orientation.
        frameData.projectionMatrix.GetColumn( 1 )[ 1 ] *= -1;

        appContent->Dbg->Reset( appContent->FrameIndex );

        DebugRendererVk::RenderParametersVk renderParamsDbg;
        renderParamsDbg.dims[ 0 ]  = (float) width;
        renderParamsDbg.dims[ 1 ]  = (float) height;
        renderParamsDbg.scale[ 0 ] = 1;
        renderParamsDbg.scale[ 1 ] = 1;
        renderParamsDbg.FrameIndex = appContent->FrameIndex;
        renderParamsDbg.pCmdBuffer = cmdBuffer;
        renderParamsDbg.pFrameData = &frameData;

        appContent->Dbg->Render( &renderParamsDbg );

        frameData.worldMatrix = mathfu::mat4::FromScaleVector({0.1f, 2.0f, 0.1f});
        frameData.color = { 0, 1, 0, 1 };
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