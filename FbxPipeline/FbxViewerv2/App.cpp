#include <fbxvpch.h>

#include <App.h>
#include <AppState.h>
#include <Input.h>
#include <Scene.h>
#include <Camera.h>
#include <FileTracker.h>
#include <CameraControllerInputMouseKeyboard.h>
#include <CameraControllerProjection.h>
#include <CameraControllerModelView.h>
#include <CameraControllerFreeLook.h>

#include <Swapchain.Vulkan.h>
#include <ShaderCompiler.Vulkan.h>

#include <AppSurfaceSdlVk.h>
#include <NuklearSdlVk.h>
#include <DebugRendererVk.h>
#include <SceneRendererVk.h>
#include <ImageLoaderVk.h>
#include <SkyboxRendererVk.h>
#include <SamplerManagerVk.h>

#include <tbb/tbb.h>

namespace apemode {
    using namespace apemodevk;

    const uint32_t kMaxFrames  = Swapchain::kMaxImgs;
    AppContent*    gAppContent = nullptr;
}

namespace apemodevk {

    class ShaderFileReader : public ShaderCompiler::IShaderFileReader {
    public:
        apemodeos::FileManager* pFileManager;

        bool ReadShaderTxtFile( const std::string& FilePath,
                                std::string&       OutFileFullPath,
                                std::string&       OutFileContent ) const override {
            OutFileFullPath = apemodeos::ResolveFullPath( FilePath );
            OutFileContent  = pFileManager->ReadTxtFile( FilePath );
            return false == OutFileContent.empty( );
        }
    };
}

using namespace apemode;

const VkFormat sDepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
//const VkFormat sDepthFormat = VK_FORMAT_D16_UNORM;

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

    std::vector< Scene* >       Scenes;
    apemodeos::FileTracker      FileTracker;
    apemodeos::FileManager      FileManager;
    apemodevk::ShaderCompiler*  pShaderCompiler;
    apemodevk::ShaderFileReader ShaderFileReader;
    CameraProjectionController  CamProjController;
    CameraControllerInputBase*  pCamInput          = nullptr;
    CameraControllerBase*       pCamController     = nullptr;
    NuklearRendererSdlBase*     pNkRenderer        = nullptr;
    DebugRendererVk*            pDebugRenderer     = nullptr;
    SceneRendererBase*          pSceneRendererBase = nullptr;

    apemodevk::Skybox*         pSkybox         = nullptr;
    apemodevk::SkyboxRenderer* pSkyboxRenderer = nullptr;
    apemodevk::LoadedImage*    pLoadedDDS      = nullptr;
    apemodevk::SamplerManager* pSamplerManager = nullptr;

    uint32_t FrameCount = 0;
    uint32_t FrameIndex = 0;
    uint64_t FrameId    = 0;

    uint32_t BackbufferIndices[ kMaxFrames ] = {0};

    DescriptorPool                         DescPool;
    TDispatchableHandle< VkCommandPool >   hCmdPool[ kMaxFrames ];
    TDispatchableHandle< VkCommandBuffer > hCmdBuffers[ kMaxFrames ];
    TDispatchableHandle< VkSemaphore >     hPresentCompleteSemaphores[ kMaxFrames ];
    TDispatchableHandle< VkSemaphore >     hRenderCompleteSemaphores[ kMaxFrames ];

    TDispatchableHandle< VkRenderPass >  hNkRenderPass;
    TDispatchableHandle< VkFramebuffer > hNkFramebuffers[ kMaxFrames ];

    TDispatchableHandle< VkRenderPass >   hDbgRenderPass;
    TDispatchableHandle< VkFramebuffer >  hDbgFramebuffers[ kMaxFrames ];
    TDispatchableHandle< VkImage >        hDepthImgs[ kMaxFrames ];
    TDispatchableHandle< VkImageView >    hDepthImgViews[ kMaxFrames ];
    TDispatchableHandle< VkDeviceMemory > hDepthImgMemory[ kMaxFrames ];

    AppContent( ) {
        pCamController = new ModelViewCameraController( );
        //pCamController = new FreeLookCameraController( );
        pCamInput      = new MouseKeyboardCameraControllerInput( );
    }

    ~AppContent() {
    }
};

App::App( ) : appState(new AppState()) {
    if ( nullptr != appState )
        appState->appOptions->add_options( "vk" )
            ( "renderdoc", "Adds renderdoc layer to device layers" )
            ( "vkapidump", "Adds api dump layer to vk device layers" )
            ( "vktrace", "Adds vktrace layer to vk device layers" );
}

App::~App( ) {
    if (nullptr != appState)
        delete appState;

    if ( nullptr != appContent )
        delete appContent;
}

AppSurfaceBase* App::CreateAppSurface( ) {
    return new AppSurfaceSdlVk( );
}

bool App::Initialize( int Args, char* ppArgs[] ) {

    if (appState && appState->appOptions)
        appState->appOptions->parse(Args, ppArgs);

    if ( AppBase::Initialize( Args, ppArgs ) ) {

        if ( nullptr == appContent )
            appContent = new AppContent( );

        appContent->FileTracker.FilePatterns.push_back( ".*\\.(vert|frag|comp|geom|tesc|tese|h|hpp|inl|inc|fx)$" );
        appContent->FileTracker.ScanDirectory( "./shaders/**", true );

        appContent->ShaderFileReader.pFileManager = &appContent->FileManager;
        appContent->pShaderCompiler = new apemodevk::ShaderCompiler( );
        appContent->pShaderCompiler->SetShaderFileReader( &appContent->ShaderFileReader );

        totalSecs = 0.0f;

        auto appSurfaceBase = GetSurface();
        if (appSurfaceBase->GetImpl() != kAppSurfaceImpl_SdlVk)
            return false;

        auto appSurface = (AppSurfaceSdlVk*) appSurfaceBase;
        if ( auto swapchain = &appSurface->Swapchain ) {
            appContent->FrameId    = 0;
            appContent->FrameIndex = 0;
            appContent->FrameCount = swapchain->ImgCount;

            OnResized();

            for (uint32_t i = 0; i < appContent->FrameCount; ++i) {
                VkCommandPoolCreateInfo cmdPoolCreateInfo;
                InitializeStruct( cmdPoolCreateInfo );
                cmdPoolCreateInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                cmdPoolCreateInfo.queueFamilyIndex = appSurface->PresentQueueFamilyIds[0];

                if ( false == appContent->hCmdPool[ i ].Recreate( *appSurface->pNode, cmdPoolCreateInfo ) ) {
                    DebugBreak( );
                    return false;
                }

                VkCommandBufferAllocateInfo cmdBufferAllocInfo;
                InitializeStruct( cmdBufferAllocInfo );
                cmdBufferAllocInfo.commandPool = appContent->hCmdPool[ i ];
                cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                cmdBufferAllocInfo.commandBufferCount = 1;

                if ( false == appContent->hCmdBuffers[ i ].Recreate( *appSurface->pNode, cmdBufferAllocInfo ) ) {
                    DebugBreak( );
                    return false;
                }

                VkSemaphoreCreateInfo semaphoreCreateInfo;
                InitializeStruct( semaphoreCreateInfo );
                if ( false == appContent->hPresentCompleteSemaphores[ i ].Recreate( *appSurface->pNode, semaphoreCreateInfo ) ||
                     false == appContent->hRenderCompleteSemaphores[ i ].Recreate( *appSurface->pNode, semaphoreCreateInfo ) ) {
                    DebugBreak( );
                    return false;
                }
            }
        }

        if (false == appContent->DescPool.RecreateResourcesFor(*appSurface->pNode, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256 )) {
            DebugBreak();
            return false;
        }

        appContent->pNkRenderer = new NuklearRendererSdlVk();

        auto queueFamilyPool = appSurface->pNode->GetQueuePool()->GetPool(appSurface->PresentQueueFamilyIds[0]);
        apemodevk::AcquiredQueue acquiredQueue;

        while (acquiredQueue.pQueue == nullptr) {
            acquiredQueue = queueFamilyPool->Acquire(false);
        }

        NuklearRendererSdlVk::InitParametersVk initParamsNk;
        initParamsNk.pAlloc          = nullptr;
        initParamsNk.pDevice         = *appSurface->pNode;
        initParamsNk.pPhysicalDevice = *appSurface->pNode;
        initParamsNk.pRenderPass     = appContent->hDbgRenderPass;
        //initParamsNk.pRenderPass     = appContent->hNkRenderPass;
        initParamsNk.pDescPool       = appContent->DescPool;
        initParamsNk.pQueue          = acquiredQueue.pQueue;
        initParamsNk.queueFamilyId   = acquiredQueue.queueFamilyId;

        appContent->pNkRenderer->Init( &initParamsNk );

        queueFamilyPool->Release( acquiredQueue );

        appContent->pDebugRenderer = new DebugRendererVk( );

        DebugRendererVk::InitParametersVk initParamsDbg;
        initParamsDbg.pAlloc          = nullptr;
        initParamsDbg.pDevice         = *appSurface->pNode;
        initParamsDbg.pPhysicalDevice = *appSurface->pNode;
        initParamsDbg.pRenderPass     = appContent->hDbgRenderPass;
        initParamsDbg.pDescPool       = appContent->DescPool;
        initParamsDbg.FrameCount      = appContent->FrameCount;

        appContent->pDebugRenderer->RecreateResources( &initParamsDbg );
        appContent->pSceneRendererBase = appSurface->CreateSceneRenderer( );

        SceneRendererVk::RecreateParametersVk recreateParams;
        recreateParams.pNode           = appSurface->pNode;
        recreateParams.pShaderCompiler = appContent->pShaderCompiler;
        recreateParams.pRenderPass     = appContent->hDbgRenderPass;
        recreateParams.pDescPool       = appContent->DescPool;
        recreateParams.FrameCount      = appContent->FrameCount;

        SceneRendererVk::SceneUpdateParametersVk updateParams;
        updateParams.pNode           = appSurface->pNode;
        updateParams.pShaderCompiler = appContent->pShaderCompiler;
        updateParams.pRenderPass     = appContent->hDbgRenderPass;
        updateParams.pDescPool       = appContent->DescPool;
        updateParams.FrameCount      = appContent->FrameCount;

        // -i "E:\Media\Models\blood-and-fire\source\DragonMain.fbx" -o "$(SolutionDir)assets\DragonMainp.fbxp" -p
        // -i "E:\Media\Models\knight-artorias\source\Artorias.fbx.fbx" -o "$(SolutionDir)assets\Artoriasp.fbxp" -p
        // -i "E:\Media\Models\vanille-flirty-animation\source\happy.fbx" -o "$(SolutionDir)assets\vanille-flirty-animation.fbxp" -p
        // -i "E:\Media\Models\special-sniper-rifle-vss-vintorez\source\vintorez.FBX" -o "$(SolutionDir)assets\vintorez.fbxp" -p
        // -i "F:\Dev\AutodeskMaya\Mercedes+Benz+A45+AMG+Centered.FBX" -o "$(SolutionDir)assets\A45p.fbxp" -p
        // -i "F:\Dev\AutodeskMaya\Mercedes+Benz+A45+AMG+Centered.FBX" -o "$(SolutionDir)assets\A45.fbxp"
        // -i "E:\Media\Models\mech-m-6k\source\93d43cf18ad5406ba0176c9fae7d4927.fbx" -o "$(SolutionDir)assets\Mech6kv4p.fbxp" -p
        // -i "E:\Media\Models\mech-m-6k\source\93d43cf18ad5406ba0176c9fae7d4927.fbx" -o "$(SolutionDir)assets\Mech6kv4.fbxp"
        // -i "E:\Media\Models\carambit\source\Knife.fbx" -o "$(SolutionDir)assets\Knifep.fbxp" -p
        // -i "E:\Media\Models\pontiac-firebird-formula-1974\source\carz.obj 2.zip\carz.obj\mesh.obj" -o "$(SolutionDir)assets\pontiacp.fbxp" -p

        appContent->Scenes.push_back( LoadSceneFromFile( "../../../assets/DragonMainp.fbxp" ) );
        // appContent->Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/Artoriasp.fbxp" ) );
        // appContent->Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/vanille-flirty-animation.fbxp" ) );
        // appContent->Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/vintorez.fbxp" ) );
        // appContent->Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/Mech6kv4p.fbxp" ) );
        // appContent->Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/A45p.fbxp" ));
        // appContent->Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/A45.fbxp" ));
        // appContent->Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/Mech6kv4.fbxp" ));
        // appContent->Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/Mech6kv4.fbxp" ));
        // appContent->Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/Cube10p.fbxp" ));
        // appContent->Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/Knifep.fbxp" ));
        // appContent->Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/pontiacp.fbxp" ));
        // appContent->Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/Mech6kv4p.fbxp" ));
        updateParams.pSceneSrc = appContent->Scenes.back( )->sourceScene;

        if ( false == appContent->pSceneRendererBase->Recreate( &recreateParams ) ) {
            DebugBreak( ); 
            return false;
        }

        if ( false == appContent->pSceneRendererBase->UpdateScene( appContent->Scenes.back( ), &updateParams ) ) {
            DebugBreak( );
            return false;
        }

        appContent->pSkybox         = new apemodevk::Skybox( );
        appContent->pSkyboxRenderer = new apemodevk::SkyboxRenderer( );
        appContent->pSamplerManager = new apemodevk::SamplerManager( );

        apemodevk::SkyboxRenderer::RecreateParameters skyboxRendererRecreateParams;
        skyboxRendererRecreateParams.pNode           = appSurface->pNode;
        skyboxRendererRecreateParams.pShaderCompiler = appContent->pShaderCompiler;
        skyboxRendererRecreateParams.pRenderPass     = appContent->hDbgRenderPass;
        skyboxRendererRecreateParams.pDescPool       = appContent->DescPool;
        skyboxRendererRecreateParams.FrameCount      = appContent->FrameCount;

        if ( false == appContent->pSkyboxRenderer->Recreate( &skyboxRendererRecreateParams ) ) {
            DebugBreak( ); 
            return false;
        }

        if ( false == appContent->pSamplerManager->Recreate( appSurface->pNode ) ) {
            DebugBreak( );
            return false;
        }

        apemodeos::FileManager imgFileManager;
        apemodevk::ImageLoader imgLoader;
        imgLoader.Recreate( appSurface->pNode, nullptr );

        //auto pngContent = imgFileManager.ReadBinFile( "../../../assets/img/DragonMain_Diff.png" );
        //auto loadedPNG  = imgLoader.LoadImageFromData( pngContent, apemodevk::ImageLoader::eImageFileFormat_PNG, true, true );

        //auto ddsContent = imgFileManager.ReadBinFile( "../../../assets/env/kyoto_lod.dds" );
        auto ddsContent = imgFileManager.ReadBinFile( "../../../assets/env/PaperMill/Specular_HDR.dds" );
        //auto ddsContent = imgFileManager.ReadBinFile( "../../../assets/env/Canyon/Unfiltered_HDR.dds" );
        appContent->pLoadedDDS = imgLoader.LoadImageFromData( ddsContent, apemodevk::ImageLoader::eImageFileFormat_DDS, true, true ).release( );

        VkSamplerCreateInfo samplerCreateInfo;
        apemodevk::InitializeStruct( samplerCreateInfo );
        samplerCreateInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.anisotropyEnable        = true;
        samplerCreateInfo.maxAnisotropy           = 16;
        samplerCreateInfo.compareEnable           = false;
        samplerCreateInfo.compareOp               = VK_COMPARE_OP_NEVER;
        samplerCreateInfo.magFilter               = VK_FILTER_LINEAR;
        samplerCreateInfo.minFilter               = VK_FILTER_LINEAR;
        samplerCreateInfo.minLod                  = 0;
        samplerCreateInfo.maxLod                  = appContent->pLoadedDDS->imageCreateInfo.mipLevels;
        samplerCreateInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.borderColor             = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        samplerCreateInfo.unnormalizedCoordinates = false;

        auto samplerIndex = appContent->pSamplerManager->GetSamplerIndex(samplerCreateInfo);
        assert ( samplerIndex != UINT_ERROR ); 

        appContent->pSkybox->pSampler      = appContent->pSamplerManager->StoredSamplers[samplerIndex].pSampler;
        appContent->pSkybox->pImgView      = appContent->pLoadedDDS->hImgView;
        appContent->pSkybox->Dimension     = appContent->pLoadedDDS->imageCreateInfo.extent.width;
        appContent->pSkybox->eImgLayout    = appContent->pLoadedDDS->eImgLayout;
        appContent->pSkybox->LevelOfDetail = 0;
        appContent->pSkybox->Exposure      = 1;
         
        return true;   
    }

    return false;
}

bool apemode::App::OnResized( ) {
    if ( auto appSurfaceVk = (AppSurfaceSdlVk*) GetSurface( ) ) {
        if ( auto swapchain = &appSurfaceVk->Swapchain ) {
            appContent->width  = appSurfaceVk->GetWidth( );
            appContent->height = appSurfaceVk->GetHeight( );

            VkImageCreateInfo depthImgCreateInfo;
            InitializeStruct( depthImgCreateInfo );
            depthImgCreateInfo.imageType     = VK_IMAGE_TYPE_2D;
            depthImgCreateInfo.format        = sDepthFormat;
            depthImgCreateInfo.extent.width  = swapchain->ImgExtent.width;
            depthImgCreateInfo.extent.height = swapchain->ImgExtent.height;
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

            colorDepthAttachments[ 0 ].format         = appSurfaceVk->Surface.eColorFormat;
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
            framebufferCreateInfoNk.width           = swapchain->ImgExtent.width;
            framebufferCreateInfoNk.height          = swapchain->ImgExtent.height;
            framebufferCreateInfoNk.layers          = 1;

            VkFramebufferCreateInfo framebufferCreateInfoDbg;
            InitializeStruct( framebufferCreateInfoDbg );
            framebufferCreateInfoDbg.renderPass      = appContent->hDbgRenderPass;
            framebufferCreateInfoDbg.attachmentCount = 2;
            framebufferCreateInfoDbg.width           = swapchain->ImgExtent.width;
            framebufferCreateInfoDbg.height          = swapchain->ImgExtent.height;
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
    nk_input_begin( &appContent->pNkRenderer->Context ); {
        SDL_Event evt;
        while ( SDL_PollEvent( &evt ) )
            appContent->pNkRenderer->HandleEvent( &evt );
        nk_input_end( &appContent->pNkRenderer->Context );
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

    auto ctx = &appContent->pNkRenderer->Context;
    float clearColor[ 4 ] = {0.5f, 0.5f, 1.0f, 1.0f};

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

    appContent->pCamInput->Update( deltaSecs, inputState, {(float) appContent->width, (float) appContent->height} );
    appContent->pCamController->Orbit( appContent->pCamInput->OrbitDelta );
    appContent->pCamController->Dolly( appContent->pCamInput->DollyDelta );
    appContent->pCamController->Update( deltaSecs );

    if ( auto appSurfaceVk = (AppSurfaceSdlVk*) GetSurface( ) ) {
        auto queueFamilyPool = appSurfaceVk->pNode->GetQueuePool( )->GetPool( appSurfaceVk->PresentQueueFamilyIds[ 0 ] );
        auto acquiredQueue   = queueFamilyPool->Acquire( true );
        while ( acquiredQueue.pQueue == nullptr ) {
            acquiredQueue = queueFamilyPool->Acquire( true );
        }

        VkDevice        device                   = *appSurfaceVk->pNode;
        VkQueue         queue                    = acquiredQueue.pQueue;
        VkSwapchainKHR  swapchain                = appSurfaceVk->Swapchain.hSwapchain;
        VkFence         fence                    = acquiredQueue.pFence;
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

        VkClearValue clearValue[ 2 ];
        clearValue[ 0 ].color.float32[ 0 ]   = clearColor[ 0 ];
        clearValue[ 0 ].color.float32[ 1 ]   = clearColor[ 1 ];
        clearValue[ 0 ].color.float32[ 2 ]   = clearColor[ 2 ];
        clearValue[ 0 ].color.float32[ 3 ]   = clearColor[ 3 ];
        clearValue[ 1 ].depthStencil.depth   = 1;
        clearValue[ 1 ].depthStencil.stencil = 0;

        VkRenderPassBeginInfo renderPassBeginInfo;
        InitializeStruct( renderPassBeginInfo );
        renderPassBeginInfo.renderPass               = appContent->hDbgRenderPass;
        renderPassBeginInfo.framebuffer              = appContent->hDbgFramebuffers[ appContent->FrameIndex ];
        renderPassBeginInfo.renderArea.extent.width  = appSurfaceVk->GetWidth( );
        renderPassBeginInfo.renderArea.extent.height = appSurfaceVk->GetHeight( );
        renderPassBeginInfo.clearValueCount          = 2;
        renderPassBeginInfo.pClearValues             = clearValue; 

        vkCmdBeginRenderPass( cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

        auto View    = appContent->pCamController->ViewMatrix( );
        auto InvView = View.Inverse( );
        auto Proj    = appContent->CamProjController.ProjMatrix( 55, (float) width, (float) height, 0.1f, 1000.0f );
        auto InvProj = Proj.Inverse( );

        DebugRendererVk::FrameUniformBuffer frameData;
        frameData.projectionMatrix = Proj;
        frameData.viewMatrix       = appContent->pCamController->ViewMatrix( );
        frameData.color            = {1, 0, 0, 1};

        appContent->pSkyboxRenderer->Reset( appContent->FrameIndex );
        appContent->pDebugRenderer->Reset( appContent->FrameIndex );
        appContent->pSceneRendererBase->Reset( appContent->Scenes[ 0 ], appContent->FrameIndex );

        apemodevk::SkyboxRenderer::RenderParameters skyboxRenderParams;
        skyboxRenderParams.Dims[ 0 ]      = (float) width;
        skyboxRenderParams.Dims[ 1 ]      = (float) height;
        skyboxRenderParams.Scale[ 0 ]     = 1;
        skyboxRenderParams.Scale[ 1 ]     = 1;
        skyboxRenderParams.FrameIndex     = appContent->FrameIndex;
        skyboxRenderParams.pCmdBuffer     = cmdBuffer;
        skyboxRenderParams.pNode          = appSurfaceVk->pNode;
        skyboxRenderParams.InvViewMatrix  = InvView;
        skyboxRenderParams.InvProjMatrix  = InvProj;
        skyboxRenderParams.ProjBiasMatrix = appContent->CamProjController.ProjBiasMatrix( );
        skyboxRenderParams.FieldOfView    = apemodem::DegreesToRadians( 67 );

        appContent->pSkyboxRenderer->Render( appContent->pSkybox, &skyboxRenderParams );

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
        appContent->pDebugRenderer->Render(&renderParamsDbg);

        frameData.worldMatrix
            = mathfu::mat4::FromScaleVector({ scale, scale, scale * 2 })
            * mathfu::mat4::FromTranslationVector(mathfu::vec3{ 0, 0, scale * 3 });

        frameData.color = { 0, 0, 1, 1 };
        appContent->pDebugRenderer->Render(&renderParamsDbg);

        frameData.worldMatrix
            = mathfu::mat4::FromScaleVector({ scale * 2, scale, scale })
            * mathfu::mat4::FromTranslationVector(mathfu::vec3{ scale * 3, 0, 0 });

        frameData.color = { 1, 0, 0, 1 };
        appContent->pDebugRenderer->Render(&renderParamsDbg);

        apemode::SceneRendererVk::SceneRenderParametersVk sceneRenderParameters;
        sceneRenderParameters.dims[ 0 ]  = (float) width;
        sceneRenderParameters.dims[ 1 ]  = (float) height;
        sceneRenderParameters.scale[ 0 ] = 1;
        sceneRenderParameters.scale[ 1 ] = 1;
        sceneRenderParameters.FrameIndex = appContent->FrameIndex;
        sceneRenderParameters.pCmdBuffer = cmdBuffer;
        sceneRenderParameters.pNode      = appSurfaceVk->pNode;
        sceneRenderParameters.ViewMatrix = frameData.viewMatrix;
        sceneRenderParameters.ProjMatrix = frameData.projectionMatrix;

        appContent->pSceneRendererBase->RenderScene(appContent->Scenes[0], &sceneRenderParameters);

        NuklearRendererSdlVk::RenderParametersVk renderParamsNk;
        renderParamsNk.dims[ 0 ]          = (float) width;
        renderParamsNk.dims[ 1 ]          = (float) height;
        renderParamsNk.scale[ 0 ]         = 1;
        renderParamsNk.scale[ 1 ]         = 1;
        renderParamsNk.aa                 = NK_ANTI_ALIASING_ON;
        renderParamsNk.max_vertex_buffer  = 64 * 1024;
        renderParamsNk.max_element_buffer = 64 * 1024;
        renderParamsNk.FrameIndex         = appContent->FrameIndex;
        renderParamsNk.pCmdBuffer         = cmdBuffer;

        appContent->pNkRenderer->Render( &renderParamsNk );
        nk_clear( &appContent->pNkRenderer->Context );

        vkCmdEndRenderPass( cmdBuffer );

        appContent->pDebugRenderer->Flush( appContent->FrameIndex );
        appContent->pSceneRendererBase->Flush( appContent->Scenes[0], appContent->FrameIndex );
        appContent->pSkyboxRenderer->Flush( appContent->FrameIndex );

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

        queueFamilyPool->Release(acquiredQueue);

    }
}

bool App::IsRunning( ) {
    return AppBase::IsRunning( );
}

extern "C" AppBase* CreateApp( ) {
    return new App( );
}