#include <fbxvpch.h>

#include <App.h>
#include <AppSurfaceSdlVk.h>
#include <Swapchain.Vulkan.h>
#include <Input.h>
#include <NuklearSdlVk.h>

#include <Scene.h>

#include <EmbeddedShaderPreprocessor.h>

namespace apemode {
    using namespace apemodevk;
}

using namespace apemode;

static const uint32_t kMaxFrames = Swapchain::kMaxImgs;

struct apemode::AppContent {
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

    uint32_t FrameCount = 0;
    uint32_t FrameIndex = 0;
    uint64_t FrameId    = 0;

    uint32_t BackbufferIndices[ kMaxFrames ] = {0};

    std::unique_ptr< DescriptorPool >      pDescPool;
    TDispatchableHandle< VkRenderPass >    hRenderPass;
    TDispatchableHandle< VkFramebuffer >   hFramebuffers[ kMaxFrames ];
    TDispatchableHandle< VkCommandPool >   hCmdPool[ kMaxFrames ];
    TDispatchableHandle< VkCommandBuffer > hCmdBuffers[ kMaxFrames ];
    TDispatchableHandle< VkFence >         hFences[ kMaxFrames ];
    TDispatchableHandle< VkSemaphore >     hPresentCompleteSemaphores[ kMaxFrames ];
    TDispatchableHandle< VkSemaphore >     hRenderCompleteSemaphores[ kMaxFrames ];
};

App::App( ) : content( new AppContent( ) ) {
}

App::~App( ) {
}

IAppSurface* App::CreateAppSurface( ) {
    return new AppSurfaceSdlVk( );
}

bool App::Initialize( int Args, char* ppArgs[] ) {
    if ( AppBase::Initialize( Args, ppArgs ) ) {
        totalSecs = 0.0f;

        auto appSurface = GetSurface();
        if (appSurface->GetImpl() != kAppSurfaceImpl_SdlVk)
            return false;

        auto appSurfaceVk = (AppSurfaceSdlVk*)appSurface;
        if ( auto swapchain = appSurfaceVk->pSwapchain.get( ) ) {
            content->FrameCount = swapchain->ImgCount;
            content->FrameIndex = 0;
            content->FrameId    = 0;
           
            OnResized();

            for (uint32_t i = 0; i < content->FrameCount; ++i) {

                TInfoStruct<VkCommandPoolCreateInfo > cmdPoolCreateInfo;
                cmdPoolCreateInfo->flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                cmdPoolCreateInfo->queueFamilyIndex = appSurfaceVk->pCmdQueue->QueueFamilyId;

                if (false == content->hCmdPool[i].Recreate(*appSurfaceVk->pNode, cmdPoolCreateInfo)) {
                    DebugBreak();
                    return false;
                }

                TInfoStruct<VkCommandBufferAllocateInfo > cmdBufferAllocInfo;
                cmdBufferAllocInfo->commandPool = content->hCmdPool[i];
                cmdBufferAllocInfo->level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                cmdBufferAllocInfo->commandBufferCount = 1;

                if (false == content->hCmdBuffers[i].Recreate(*appSurfaceVk->pNode, cmdBufferAllocInfo)) {
                    DebugBreak();
                    return false;
                }

                TInfoStruct<VkFenceCreateInfo > fenceCreateInfo;
                fenceCreateInfo->flags = VK_FENCE_CREATE_SIGNALED_BIT;

                if (false == content->hFences[i].Recreate(*appSurfaceVk->pNode, fenceCreateInfo)) {
                    DebugBreak();
                    return false;
                }

                TInfoStruct< VkSemaphoreCreateInfo > semaphoreCreateInfo;
                if ( false == content->hPresentCompleteSemaphores[ i ].Recreate( *appSurfaceVk->pNode, semaphoreCreateInfo ) ||
                     false == content->hRenderCompleteSemaphores[ i ].Recreate( *appSurfaceVk->pNode, semaphoreCreateInfo ) ) {
                    DebugBreak( );
                    return false;
                }
            }
        }

        content->pDescPool = std::move(std::make_unique< DescriptorPool >());
        if (false == content->pDescPool->RecreateResourcesFor(*appSurfaceVk->pNode, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256 )) {
            DebugBreak();
            return false;
        }

        content->Nk = new NuklearSdlVk();

        NuklearSdlVk::InitParametersVk initParams;
        initParams.pAlloc          = nullptr;
        initParams.pDevice         = *appSurfaceVk->pNode;
        initParams.pPhysicalDevice = *appSurfaceVk->pNode;
        initParams.pRenderPass     = content->hRenderPass;
        initParams.pDescPool       = *content->pDescPool;
        initParams.pQueue          = *appSurfaceVk->pCmdQueue;
        initParams.QueueFamilyId   = appSurfaceVk->pCmdQueue->QueueFamilyId;

        content->Nk->Init( &initParams );

        // content->scenes[ 0 ] = LoadSceneFromFile( "../../../assets/iron-man.fbxp" );
        // content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/kalestra-the-sorceress.fbxp" );
        // content->scenes[ 0 ] = LoadSceneFromFile( "../../../assets/Mech6kv3ps.fbxp" );
        // content->scenes[ 0 ] = LoadSceneFromFile( "../../../assets/Mech6k_v2.fbxp" );
        // content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/P90_v2.fbxp" );
        // content->scenes[ 0 ] = LoadSceneFromFile( "../../../assets/MercedesBenzA45AMG.fbxp" );
        // content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/MercedesBenzSLR.fbxp" );
        // content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/P90.fbxp" );
        // content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/IronMan.fbxp" );
        // content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/Cathedral.fbxp" );
        // content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/Leica1933.fbxp" );
        // content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/UnrealOrb.fbxp" );
        // content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/Artorias.fbxp" );
        // content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/9mm.fbxp" );
        // content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/Knife.fbxp" );
        // content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/mech-m-6k.fbxp" );

        return true;
    }

    return false;
}

bool apemode::App::OnResized( ) {
    if ( auto appSurfaceVk = (AppSurfaceSdlVk*) GetSurface( ) ) {
        if ( auto swapchain = appSurfaceVk->pSwapchain.get( ) ) {
            content->width  = appSurfaceVk->GetWidth( );
            content->height = appSurfaceVk->GetHeight( );

            VkAttachmentDescription attachment = {};
            attachment.format                  = swapchain->eColorFormat;
            attachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
            attachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
            attachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentReference colorAttachment = {};
            colorAttachment.attachment            = 0;
            colorAttachment.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass = {};
            subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments    = &colorAttachment;

            TInfoStruct< VkRenderPassCreateInfo > renderPassCreateInfo = {};
            renderPassCreateInfo->attachmentCount                      = 1;
            renderPassCreateInfo->pAttachments                         = &attachment;
            renderPassCreateInfo->subpassCount                         = 1;
            renderPassCreateInfo->pSubpasses                           = &subpass;

            if ( false == content->hRenderPass.Recreate( *appSurfaceVk->pNode, renderPassCreateInfo ) ) {
                DebugBreak( );
                return false;
            }

            for ( uint32_t i = 0; i < content->FrameCount; ++i ) {
                TInfoStruct< VkFramebufferCreateInfo > framebufferCreateInfo;
                framebufferCreateInfo->renderPass      = content->hRenderPass;
                framebufferCreateInfo->attachmentCount = 1;
                framebufferCreateInfo->pAttachments    = swapchain->hImgViews[ i ];
                framebufferCreateInfo->width           = swapchain->ColorExtent.width;
                framebufferCreateInfo->height          = swapchain->ColorExtent.height;
                framebufferCreateInfo->layers          = 1;

                if ( false == content->hFramebuffers[ i ].Recreate( *appSurfaceVk->pNode, framebufferCreateInfo ) ) {
                    DebugBreak( );
                    return false;
                }
            }
        }
    }

    return true;
}

void App::OnFrameMove( ) {
    nk_input_begin( &content->Nk->Context ); {
        SDL_Event evt;
        while ( SDL_PollEvent( &evt ) )
            content->Nk->HandleEvent( &evt );
        nk_input_end( &content->Nk->Context );
    }

    AppBase::OnFrameMove( );

    ++content->FrameId;
    content->FrameIndex = content->FrameId % (uint64_t) content->FrameCount;
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

    auto ctx = &content->Nk->Context;
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
        VkFence         fence                    = content->hFences[ content->FrameIndex ];
        VkSemaphore     presentCompleteSemaphore = content->hPresentCompleteSemaphores[ content->FrameIndex ];
        VkSemaphore     renderCompleteSemaphore  = content->hRenderCompleteSemaphores[ content->FrameIndex ];
        VkCommandPool   cmdPool                  = content->hCmdPool[ content->FrameIndex ];
        VkCommandBuffer cmdBuffer                = content->hCmdBuffers[ content->FrameIndex ];

        const uint32_t width  = appSurfaceVk->GetWidth( );
        const uint32_t height = appSurfaceVk->GetHeight( );

        if ( width != content->width || height != content->height ) {
            CheckedCall( vkDeviceWaitIdle( device ) );
            OnResized( );

        }

        VkRenderPass  renderPass  = content->hRenderPass;
        VkFramebuffer framebuffer = content->hFramebuffers[ content->FrameIndex ];

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
                                            &content->BackbufferIndices[ content->FrameIndex ] ) );

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

        NuklearSdlVk::RenderParametersVk renderParams;
        renderParams.dims[0] = width;
        renderParams.dims[1] = height;
        renderParams.scale[0] = 1;
        renderParams.scale[1] = 1;
        renderParams.aa                 = NK_ANTI_ALIASING_ON;
        renderParams.max_vertex_buffer  = 64 * 1024;
        renderParams.max_element_buffer = 64 * 1024;
        renderParams.FrameIndex         = content->FrameIndex;
        renderParams.pCmdBuffer         = cmdBuffer;

        content->Nk->Render(&renderParams);

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

        CheckedCall( vkEndCommandBuffer( cmdBuffer ) );
        CheckedCall( vkResetFences( device, 1, &fence ) );
        CheckedCall( vkQueueSubmit( queue, 1, &submitInfo, fence ) );

        if ( content->FrameId ) {
            uint32_t    presentIndex    = ( content->FrameIndex + content->FrameCount - 1 ) % content->FrameCount;
            VkSemaphore renderSemaphore = content->hRenderCompleteSemaphores[ presentIndex ];

            VkPresentInfoKHR presentInfoKHR;
            InitializeStruct( presentInfoKHR );
            presentInfoKHR.waitSemaphoreCount = 1;
            presentInfoKHR.pWaitSemaphores    = &renderSemaphore;
            presentInfoKHR.swapchainCount     = 1;
            presentInfoKHR.pSwapchains        = &swapchain;
            presentInfoKHR.pImageIndices      = &content->BackbufferIndices[ presentIndex ];

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