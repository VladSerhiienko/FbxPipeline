#include <fbxvpch.h>

#include <AppState.h>
#include <AppSurfaceSdlVk.h>
#include <SceneRendererVk.h>

apemode::AppSurfaceSdlVk::AppSurfaceSdlVk( ) {
    Impl = kAppSurfaceImpl_SdlVk;
}

apemode::AppSurfaceSdlVk::~AppSurfaceSdlVk( ) {
    Finalize( );
}

bool apemode::AppSurfaceSdlVk::Initialize( uint32_t width, uint32_t height, const char* name ) {
    SDL_Log( "apemode/AppSurfaceSdlVk/Initialize" );

    if ( AppSurfaceSdlBase::Initialize( width, height, name ) ) {

        uint32_t graphicsManagerFlags = 0;

        if (auto appState = apemode::AppState::GetCurrentState())
            if (appState->appOptions) {

                if ((*appState->appOptions)["renderdoc"].count())
                    graphicsManagerFlags |= apemodevk::GraphicsManager::kEnableRenderDocLayer;
                if ((*appState->appOptions)["vktrace"].count())
                    graphicsManagerFlags |= apemodevk::GraphicsManager::kEnableVkTraceLayer;
                if ((*appState->appOptions)["vkapidump"].count())
                    graphicsManagerFlags |= apemodevk::GraphicsManager::kEnableVkApiDumpLayer;
#if _DEBUG
                graphicsManagerFlags |= apemodevk::GraphicsManager::kEnableLayers;
                //graphicsManagerFlags |= apemodevk::GraphicsManager::kEnableVkApiDumpLayer;
#endif
            }

        if ( DeviceManager.RecreateGraphicsNodes( graphicsManagerFlags ) ) {
            pNode = DeviceManager.GetPrimaryGraphicsNode( );

            Surface.Recreate( pNode->pPhysicalDevice, DeviceManager.hInstance, hInstance, hWnd );

            uint32_t queueFamilyIndex = 0;
            apemodevk::QueueFamilyPool* queueFamilyPool  = nullptr;
            while ( auto currentQueueFamilyPool = pNode->GetQueuePool( )->GetPool( queueFamilyIndex++ ) ) {
                if ( currentQueueFamilyPool->SupportsPresenting( Surface.hSurface ) ) {
                    PresentQueue = currentQueueFamilyPool->Acquire( true );
                    break;
                }
            }

            if ( nullptr == PresentQueue.pQueue ) {
                apemodevk::platform::DebugBreak( );
                return false;
            }


            // Determine the number of VkImage's to use in the swap chain.
            // We desire to own only 1 image at a time, besides the
            // images being displayed and queued for display.

            uint32_t ImgCount = std::min< uint32_t >( apemodevk::Swapchain::kMaxImgs, Surface.SurfaceCaps.minImageCount + 1 );
            if ( ( Surface.SurfaceCaps.maxImageCount > 0 ) && ( Surface.SurfaceCaps.maxImageCount < ImgCount ) ) {
                // Application must settle for fewer images than desired.
                ImgCount = Surface.SurfaceCaps.maxImageCount;
            }

            VkExtent2D currentExtent = {0, 0};

            if ( Surface.SurfaceCaps.currentExtent.width == apemodevk::Swapchain::kExtentMatchFullscreen &&
                 Surface.SurfaceCaps.currentExtent.height == apemodevk::Swapchain::kExtentMatchFullscreen ) {
                // If the surface size is undefined, the size is set to
                // the size of the images requested.
                apemode_assert( bIsDefined, "Unexpected." );

                uint32_t DesiredColorWidth  = GetWidth( );
                uint32_t DesiredColorHeight = GetHeight( );

                const bool bMatchesWindow = DesiredColorWidth == apemodevk::Swapchain::kExtentMatchWindow &&
                                            DesiredColorHeight == apemodevk::Swapchain::kExtentMatchWindow;

                const bool bMatchesFullscreen = DesiredColorWidth == apemodevk::Swapchain::kExtentMatchFullscreen &&
                                                DesiredColorHeight == apemodevk::Swapchain::kExtentMatchFullscreen;

                if ( !bMatchesWindow && !bMatchesFullscreen ) {
                    currentExtent.width  = DesiredColorWidth;
                    currentExtent.height = DesiredColorHeight;
                }
            } else {
                // If the surface size is defined, the swap chain size must match
                currentExtent = Surface.SurfaceCaps.currentExtent;
            }

            if ( false == Swapchain.Recreate( pNode->hLogicalDevice,
                                              pNode->pPhysicalDevice,
                                              Surface.hSurface,
                                              ImgCount,
                                              currentExtent.width,
                                              currentExtent.height,
                                              Surface.eColorFormat,
                                              Surface.eColorSpace,
                                              Surface.eSurfaceTransform,
                                              Surface.ePresentMode ) ) {

                apemodevk::platform::DebugBreak( );
                return false;
            }

            LastWidth  = Swapchain.ImgExtent.width;
            LastHeight = Swapchain.ImgExtent.height;
        }

        return true;
    }

    return false;
}

void apemode::AppSurfaceSdlVk::Finalize( ) {
    AppSurfaceSdlBase::Finalize( );
}

void apemode::AppSurfaceSdlVk::OnFrameMove( ) {
    const uint32_t width  = GetWidth( );
    const uint32_t height = GetHeight( );

    if ( width != LastWidth || height != LastHeight ) {
        apemodevk::CheckedCall( vkDeviceWaitIdle( *pNode ) );

        LastWidth  = width;
        LastHeight = height;

        const bool bResized = Swapchain.Recreate( pNode->hLogicalDevice,
                                                  pNode->pPhysicalDevice,
                                                  Surface.hSurface,
                                                  Swapchain.ImgCount,
                                                  width,
                                                  height,
                                                  Surface.eColorFormat,
                                                  Surface.eColorSpace,
                                                  Surface.eSurfaceTransform,
                                                  Surface.ePresentMode );
        SDL_assert( bResized );
        (void) bResized;
    } 
}

void* apemode::AppSurfaceSdlVk::GetGraphicsHandle( ) {
    return reinterpret_cast< void* >( &DeviceManager );
}

apemode::SceneRendererBase* apemode::AppSurfaceSdlVk::CreateSceneRenderer( ) {
    return new SceneRendererVk( );
}
