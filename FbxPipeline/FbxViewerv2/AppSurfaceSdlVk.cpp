#include <fbxvpch.h>

#include <AppState.h>
#include <AppSurfaceSdlVk.h>
#include <CommandQueue.Vulkan.h>
#include <GraphicsDevice.Vulkan.h>
#include <GraphicsManager.Vulkan.h>
#include <Swapchain.Vulkan.h>

apemode::AppSurfaceSdlVk::AppSurfaceSdlVk( ) {
    Impl = kAppSurfaceImpl_SdlVk;
}

apemode::AppSurfaceSdlVk::~AppSurfaceSdlVk( ) {
    Finalize( );
}

bool apemode::AppSurfaceSdlVk::Initialize( uint32_t width, uint32_t height, const char* name ) {
    SDL_Log( "apemode/AppSurfaceSdlVk/Initialize" );

    if ( AppSurfaceSdlBase::Initialize( width, height, name ) ) {
        pDeviceManager = std::move( std::make_unique< apemodevk::GraphicsManager >( ) );

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
#endif
            }

        if ( pDeviceManager->RecreateGraphicsNodes( graphicsManagerFlags ) ) {
            pNode = pDeviceManager->GetPrimaryGraphicsNode( );

            uint32_t queueFamilyId    = 0;
            uint32_t queueFamilyCount = (uint32_t) pNode->QueueProps.size( );
            for ( ; queueFamilyId < queueFamilyCount; ++queueFamilyId ) {
                pSwapchain = std::move( std::make_unique< apemodevk::Swapchain >( ) );

                if ( pSwapchain->RecreateResourceFor( *pNode, queueFamilyId, hInstance, hWnd, GetWidth( ), GetHeight( ) ) ) {
                    SDL_Log( "apemode/AppSurfaceSdlVk/Initialize: Created swapchain." );
                    LastWidth  = GetWidth( );
                    LastHeight = GetHeight( );

                    if ( pNode->SupportsGraphics( queueFamilyId ) && pNode->SupportsPresenting( queueFamilyId, pSwapchain->hSurface ) ) {
                        if ( nullptr == pCmdQueue ) {
                            pCmdQueue = std::move( std::make_unique< apemodevk::CommandQueue >( ) );
                        }

                        if ( nullptr != pCmdQueue && pCmdQueue->RecreateResourcesFor( *pNode, queueFamilyId, 0 ) ) {
                            SDL_Log( "apemode/AppSurfaceSdlVk/Initialize: Created command queue for presenting." );
                            break;
                        }
                    }
                }
            }

            if ( nullptr == pCmdQueue ) {
                return false;
            }
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

        const bool bResized = pSwapchain->Resize( width, height );
        SDL_assert( bResized );
        (void) bResized;
    } 
}

void* apemode::AppSurfaceSdlVk::GetGraphicsHandle( ) {
    return reinterpret_cast< void* >( pDeviceManager.get( ) );
}