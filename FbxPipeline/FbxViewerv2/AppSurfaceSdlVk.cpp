#include <fbxvpch.h>

#include <AppSurfaceSdlVk.h>
#include <CommandQueue.Vulkan.h>
#include <GraphicsDevice.Vulkan.h>
#include <GraphicsManager.Vulkan.h>
#include <Swapchain.Vulkan.h>

class apemode::AppSurfaceSettings {
public:
    const char* pName;
    bool        bIsFullscreen;
    int         Width;
    int         Height;

public:
    AppSurfaceSettings( )
        : pName( "NesquikSdkSurface" )
#if _WIN32
        , bIsFullscreen( false )
        , Width( 1280 )
        , Height( 800 )
#endif

#if __ANDROID__
        , bIsFullscreen( true )
        , Width( 0 )
        , Height( 0 )
#endif

    {
    }
};


apemode::AppSurfaceSdlVk::AppSurfaceSdlVk( ) {
    Impl = kAppSurfaceImpl_SdlVk;
}

apemode::AppSurfaceSdlVk::~AppSurfaceSdlVk( ) {
    Finalize( );
}

bool apemode::AppSurfaceSdlVk::Initialize( ) {
    SDL_Log( "apemode/AppSurfaceSdlVk/Initialize" );

    if ( !SDL_Init( SDL_INIT_VIDEO ) ) {
        AppSurfaceSettings defaultSettings;
        pSdlWindow = SDL_CreateWindow( defaultSettings.pName,
                                                 SDL_WINDOWPOS_CENTERED,
                                                 SDL_WINDOWPOS_CENTERED,
                                                 defaultSettings.Width,
                                                 defaultSettings.Height,
                                                 SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE );

        SDL_Log( "apemode/AppSurfaceSdlVk/Initialize: Created window." );

        SDL_SysWMinfo windowInfo{};
        if ( SDL_TRUE == SDL_GetWindowWMInfo( pSdlWindow, &windowInfo ) ) {
            hWnd      = windowInfo.info.win.window;
            hInstance = (HINSTANCE) GetWindowLongPtrA( windowInfo.info.win.window, GWLP_HINSTANCE );

            pDeviceManager = std::move( std::make_unique< apemodevk::GraphicsManager >( ) );
            if ( pDeviceManager->RecreateGraphicsNodes( ) ) {
                pNode = pDeviceManager->GetPrimaryGraphicsNode( );

                uint32_t queueFamilyId    = 0;
                uint32_t queueFamilyCount = pNode->QueueProps.size( );
                for ( ; queueFamilyId < queueFamilyCount; ++queueFamilyId ) {
                    pSwapchain = std::move( std::make_unique< apemodevk::Swapchain >( ) );
                    if ( true == pSwapchain->RecreateResourceFor( *pNode, queueFamilyId, hInstance, hWnd, GetWidth( ), GetHeight( ) ) ) {
                        if ( pNode->SupportsGraphics( queueFamilyId ) && pNode->SupportsPresenting( queueFamilyId, pSwapchain->hSurface ) ) {
                            if ( nullptr == pCmdQueue )
                                pCmdQueue = std::move( std::make_unique< apemodevk::CommandQueue >( ) );
                            if ( pCmdQueue->RecreateResourcesFor( *pNode, queueFamilyId, 0 ) )
                                break;
                        }
                    }
                }

                if ( nullptr == pCmdQueue ) {
                    return false;
                }
            }
        }

        SDL_Log( "apemode/AppSurfaceSdlVk/Initialize: Initialized Vk." );
        return true;
    }

    return false;
}

void apemode::AppSurfaceSdlVk::Finalize( ) {
    if ( pSdlWindow ) {
        SDL_Log( "apemode/AppSurfaceSdlVk/Finalize: Deleting window." );
        SDL_DestroyWindow( pSdlWindow );
        pSdlWindow = nullptr;
    }
}

uint32_t apemode::AppSurfaceSdlVk::GetWidth( ) const {
    assert( pSdlWindow && "Not initialized." );

    int OutWidth;
    SDL_GetWindowSize( pSdlWindow, &OutWidth, nullptr );
    return static_cast< uint32_t >( OutWidth );
}

uint32_t apemode::AppSurfaceSdlVk::GetHeight( ) const {
    assert( pSdlWindow && "Not initialized." );

    int OutHeight;
    SDL_GetWindowSize( pSdlWindow, nullptr, &OutHeight );
    return static_cast< uint32_t >( OutHeight );
}

void apemode::AppSurfaceSdlVk::OnFrameMove( ) {
}

void apemode::AppSurfaceSdlVk::OnFrameDone( ) {
    SDL_GL_SwapWindow( pSdlWindow );
}

void* apemode::AppSurfaceSdlVk::GetWindowHandle( ) {
    return reinterpret_cast< void* >( pSdlWindow );
}

void* apemode::AppSurfaceSdlVk::GetGraphicsHandle( ) {
    return reinterpret_cast< void* >( pDeviceManager.get( ) );
}