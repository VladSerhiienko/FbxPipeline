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

struct apemode::AppSurfaceSdlVk::PrivateContent {
    SDL_Window* pSdlWindow;
    HWND        hWnd;
    HINSTANCE   hInstance;

    std::unique_ptr< GraphicsManager > pDeviceManager;
    std::unique_ptr< Swapchain >         pSwapchain;
    std::unique_ptr< CommandQueue >      pCmdQueue;
    GraphicsDevice*                      pNode;

    PrivateContent( ) : pSdlWindow( nullptr ) {
    }

    ~PrivateContent( ) {
        pSdlWindow = nullptr;
    }
};

apemode::AppSurfaceSdlVk::AppSurfaceSdlVk( ) : pContent( nullptr ) {
}

apemode::AppSurfaceSdlVk::~AppSurfaceSdlVk( ) {
    Finalize( );
}

bool apemode::AppSurfaceSdlVk::Initialize( ) {
    SDL_Log( "apemode/AppSurfaceSdlVk/Initialize" );

    pContent = new PrivateContent( );
    if ( !SDL_Init( SDL_INIT_VIDEO ) ) {
        AppSurfaceSettings defaultSettings;
        pContent->pSdlWindow = SDL_CreateWindow( defaultSettings.pName,
                                                 SDL_WINDOWPOS_CENTERED,
                                                 SDL_WINDOWPOS_CENTERED,
                                                 defaultSettings.Width,
                                                 defaultSettings.Height,
                                                 SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE );

        SDL_Log( "apemode/AppSurfaceSdlVk/Initialize: Created window." );

        SDL_SysWMinfo windowInfo{};
        if ( SDL_TRUE == SDL_GetWindowWMInfo( pContent->pSdlWindow, &windowInfo ) ) {
            pContent->hWnd      = windowInfo.info.win.window;
            pContent->hInstance = (HINSTANCE) GetWindowLongPtrA( windowInfo.info.win.window, GWLP_HINSTANCE );

            pContent->pDeviceManager = std::move( std::make_unique< GraphicsManager >( ) );
            if ( pContent->pDeviceManager->RecreateGraphicsNodes( ) ) {
                pContent->pNode = pContent->pDeviceManager->GetPrimaryGraphicsNode( );

                pContent->pCmdQueue = std::move( std::make_unique< CommandQueue >( ) );
                pContent->pCmdQueue->RecreateResourcesFor( *pContent->pNode, 0, 0 );

                pContent->pSwapchain = std::move( std::make_unique< Swapchain >( ) );
                pContent->pSwapchain->RecreateResourceFor(
                    *pContent->pNode, *pContent->pCmdQueue,
                    pContent->hInstance, pContent->hWnd,
                    GetWidth( ), GetHeight( ) );
            }
        }

        SDL_Log( "apemode/AppSurfaceSdlVk/Initialize: Initialized GLEW." );
        return nullptr != pContent->pNode;
    }

    return false;
}

void apemode::AppSurfaceSdlVk::Finalize( ) {
    if ( pContent->pSdlWindow ) {
        SDL_Log( "apemode/AppSurfaceSdlVk/Finalize: Deleting window." );
        SDL_DestroyWindow( pContent->pSdlWindow );
        pContent->pSdlWindow = nullptr;
    }

    if ( pContent ) {
        SDL_Log( "apemode/AppSurfaceSdlVk/Finalize: Deleting content." );
        delete pContent;
        pContent = nullptr;
    }
}

uint32_t apemode::AppSurfaceSdlVk::GetWidth( ) const {
    assert( pContent->pSdlWindow && "Not initialized." );

    int OutWidth;
    SDL_GetWindowSize( pContent->pSdlWindow, &OutWidth, nullptr );
    return static_cast< uint32_t >( OutWidth );
}

uint32_t apemode::AppSurfaceSdlVk::GetHeight( ) const {
    assert( pContent->pSdlWindow && "Not initialized." );

    int OutHeight;
    SDL_GetWindowSize( pContent->pSdlWindow, nullptr, &OutHeight );
    return static_cast< uint32_t >( OutHeight );
}

void apemode::AppSurfaceSdlVk::OnFrameMove( ) {
}

void apemode::AppSurfaceSdlVk::OnFrameDone( ) {
    SDL_GL_SwapWindow( pContent->pSdlWindow );
}

void* apemode::AppSurfaceSdlVk::GetWindowHandle( ) {
    return reinterpret_cast< void* >( pContent->pSdlWindow );
}

void* apemode::AppSurfaceSdlVk::GetGraphicsHandle( ) {
    return reinterpret_cast< void* >( pContent->pDeviceManager.get() );
}