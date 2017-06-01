#include <fbxvpch.h>

#include <App.h>
#include <AppSurfaceSdlVk.h>
#include <Swapchain.Vulkan.h>
#include <Input.h>
#include <NuklearSdlVk.h>

#include <Scene.h>

#include <EmbeddedShaderPreprocessor.h>

using namespace apemode;

static const uint32_t kMaxFrames = apemodevk::Swapchain::kMaxImgs;

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
    std::unique_ptr< apemodevk::DescriptorPool >      pDescPool;
    apemodevk::TDispatchableHandle< VkRenderPass >    hRenderPass;
    apemodevk::TDispatchableHandle< VkFramebuffer >   hFramebuffers[ kMaxFrames ];
    apemodevk::TDispatchableHandle< VkCommandPool >   hCmdPool[ kMaxFrames ];
    apemodevk::TDispatchableHandle< VkCommandBuffer > hCmdBuffers[ kMaxFrames ];
    apemodevk::TDispatchableHandle< VkFence >         hFences[ kMaxFrames ];
    apemodevk::TDispatchableHandle< VkSemaphore >     hPresentCompleteSemaphores[ kMaxFrames ];
    apemodevk::TDispatchableHandle< VkSemaphore >     hRenderCompleteSemaphores[ kMaxFrames ];
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
        if (auto swapchain = appSurfaceVk->pSwapchain.get())
        {
            content->FrameCount = swapchain->ImgCount;

            VkAttachmentDescription attachment = {};
            attachment.format = swapchain->eColorFormat;
            attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentReference colorAttachment = {};
            colorAttachment.attachment = 0;
            colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass = {};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachment;

            apemodevk::TInfoStruct<VkRenderPassCreateInfo> renderPassCreateInfo = {};
            renderPassCreateInfo->attachmentCount = 1;
            renderPassCreateInfo->pAttachments = &attachment;
            renderPassCreateInfo->subpassCount = 1;
            renderPassCreateInfo->pSubpasses = &subpass;

            if (false == content->hRenderPass.Recreate(*appSurfaceVk->pNode, renderPassCreateInfo)) {
                DebugBreak();
                return false;
            }

            for (uint32_t i = 0; i < content->FrameCount; ++i) {
                apemodevk::TInfoStruct<VkFramebufferCreateInfo > framebufferCreateInfo;
                framebufferCreateInfo->renderPass = content->hRenderPass;
                framebufferCreateInfo->attachmentCount = 1;
                framebufferCreateInfo->pAttachments = swapchain->hImgViews[i];
                framebufferCreateInfo->width = swapchain->ColorExtent.width;
                framebufferCreateInfo->height = swapchain->ColorExtent.height;
                framebufferCreateInfo->layers = 1;

                if (false == content->hFramebuffers[i].Recreate(*appSurfaceVk->pNode, framebufferCreateInfo)) {
                    DebugBreak();
                    return false;
                }

                apemodevk::TInfoStruct<VkCommandPoolCreateInfo > cmdPoolCreateInfo;
                cmdPoolCreateInfo->flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                cmdPoolCreateInfo->queueFamilyIndex = appSurfaceVk->pCmdQueue->QueueFamilyId;

                if (false == content->hCmdPool[i].Recreate(*appSurfaceVk->pNode, cmdPoolCreateInfo)) {
                    DebugBreak();
                    return false;
                }

                apemodevk::TInfoStruct<VkCommandBufferAllocateInfo > cmdBufferAllocInfo;
                cmdBufferAllocInfo->commandPool = content->hCmdPool[i];
                cmdBufferAllocInfo->level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                cmdBufferAllocInfo->commandBufferCount = 1;

                if (false == content->hCmdBuffers[i].Recreate(*appSurfaceVk->pNode, cmdBufferAllocInfo)) {
                    DebugBreak();
                    return false;
                }

                apemodevk::TInfoStruct<VkFenceCreateInfo > fenceCreateInfo;
                fenceCreateInfo->flags = VK_FENCE_CREATE_SIGNALED_BIT;

                if (false == content->hFences[i].Recreate(*appSurfaceVk->pNode, fenceCreateInfo)) {
                    DebugBreak();
                    return false;
                }

                apemodevk::TInfoStruct<VkSemaphoreCreateInfo > semaphoreCreateInfo;
                if (false == content->hPresentCompleteSemaphores[i].Recreate(*appSurfaceVk->pNode, semaphoreCreateInfo) || 
                    false == content->hRenderCompleteSemaphores[i].Recreate(*appSurfaceVk->pNode, semaphoreCreateInfo)) {
                    DebugBreak();
                    return false;
                }
            }
        }

        /*content->pCmdBuffer = std::move(std::make_unique< apemodevk::CommandBuffer >());
        if (false == content->pCmdBuffer->RecreateResourcesFor(*appSurfaceVk->pNode, appSurfaceVk->pCmdQueue->QueueFamilyId, true, false)) {
            DebugBreak();
            return false;
        }*/

        content->pDescPool = std::move(std::make_unique< apemodevk::DescriptorPool >());
        if (false == content->pDescPool->RecreateResourcesFor(*appSurfaceVk->pNode, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256 )) {
            DebugBreak();
            return false;
        }

        content->Nk = new NuklearSdlVk();

        NuklearSdlVk::InitParametersVk initParams;
        initParams.pAlloc = nullptr;
        initParams.pDevice = *appSurfaceVk->pNode;
        initParams.pPhysicalDevice = *appSurfaceVk->pNode;
        //initParams.pCmdBuffer = *content->pCmdBuffer;
        initParams.pRenderPass = content->hRenderPass;
        initParams.pDescPool = *content->pDescPool;

        content->Nk->Init( &initParams );

        content->scenes[ 0 ] = LoadSceneFromFile( "../../../assets/iron-man.fbxp" );
        content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/kalestra-the-sorceress.fbxp" );
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

void App::OnFrameMove( ) {
    SDL_Event evt;
   /* nk_input_begin( content->nk );
    while ( SDL_PollEvent( &evt ) ) {
        nk_sdl_handle_event( &evt );
    }
    nk_input_end( content->nk );*/

    AppBase::OnFrameMove( );
}

void App::Update( float deltaSecs, Input const& inputState ) {
    totalSecs += deltaSecs;

    const uint32_t width  = GetSurface( )->GetWidth( );
    const uint32_t height = GetSurface( )->GetHeight( );

    bool hovered = false;
    bool reset   = false;

    const nk_flags windowFlags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE;

    //if ( nk_begin( content->nk, "FBXV: Demo.", nk_rect( 10, 60, 400, 600 ), windowFlags ) ) {
    //    nk_layout_row_dynamic( content->nk, 20, 1 );
    //    nk_labelf_wrap( content->nk, "Orbit - left button" );
    //    nk_labelf_wrap( content->nk, "Zoom - right button" );
    //    nk_labelf_wrap( content->nk, "Tab - statistics" );

    //    nk_layout_row_dynamic( content->nk, 30, 1 );
    //    //nk_property_float( content->nk, "Glossiness", 0, &content->uniforms.m_glossiness, 1, 0.01, 0.001 );
    //    //nk_property_float( content->nk, "Reflectivity", 0, &content->uniforms.m_reflectivity, 1, 0.01, 0.001 );

    //    nk_layout_row_dynamic( content->nk, 30, 1 );
    //    nk_label_wrap( content->nk, "Diffuse Color" );

    //    nk_layout_row_dynamic( content->nk, 100, 1 );
    //    content->diffColor = nk_color_picker( content->nk, content->diffColor, nk_color_format::NK_RGB );
    //    //nk_color_fv( content->uniforms.m_rgbDiff, content->diffColor );

    //    nk_layout_row_dynamic( content->nk, 10, 1 );
    //    nk_spacing( content->nk, 1 );

    //    nk_layout_row_dynamic( content->nk, 30, 1 );
    //    nk_combobox_string( content->nk,
    //                        "env/kyoto\0"
    //                        "env/bolonga\0\0",
    //                        (int*) &content->envId,
    //                        2,
    //                        30,
    //                        nk_vec2( 300, 300 ) );

    //    nk_combobox_string( content->nk,
    //                        "scene/A45-AMG\0"
    //                        "scene/Mech-6k\0\0",
    //                        (int*) &content->sceneId,
    //                        2,
    //                        30,
    //                        nk_vec2( 300, 300 ) );

    //    nk_layout_row_dynamic( content->nk, 10, 1 );
    //    nk_spacing( content->nk, 1 );

    //    hovered = nk_window_is_any_hovered( content->nk );
    //}
    //nk_end( content->nk );

    //glClearColor( 0.0f, 1.0f, 1.0f, 1.0f );
    //glClearDepth( 1.0 );
    //glClearStencil( 0 );

    //glEnable( GL_BLEND );
    //glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    //glEnable( GL_CULL_FACE );
    //glDisable( GL_DEPTH_TEST );

    //glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

    //nk_sdl_render( NK_ANTI_ALIASING_ON, 64 * 1024, 64 * 1024 );
}

bool App::IsRunning( ) {
    return AppBase::IsRunning( );
}

extern "C" AppBase* CreateApp( ) {
    return new App( );
}