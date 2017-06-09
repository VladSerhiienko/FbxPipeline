#include "AppSurfaceSdlBase.h"

apemode::AppSurfaceSdlBase::AppSurfaceSdlBase( ) {
}

apemode::AppSurfaceSdlBase::~AppSurfaceSdlBase( ) {
    Finalize( );
}

bool apemode::AppSurfaceSdlBase::Initialize( uint32_t width, uint32_t height, const char* name ) {
    SDL_Log( "apemode/AppSurfaceSdlBase/Initialize" );

    if ( nullptr == pSdlWindow && !SDL_Init( SDL_INIT_VIDEO ) ) {
        pSdlWindow = SDL_CreateWindow(
            name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE );

        if (pSdlWindow) {
            SDL_Log("apemode/AppSurfaceSdlBase/Initialize: Created a windows.");
        }

#ifdef _WINDOWS_
        SDL_SysWMinfo windowInfo{};
        if ( SDL_TRUE == SDL_GetWindowWMInfo( pSdlWindow, &windowInfo ) ) {
            hWnd      = windowInfo.info.win.window;
            hInstance = (HINSTANCE) GetWindowLongPtrA( windowInfo.info.win.window, GWLP_HINSTANCE );
            SDL_Log("apemode/AppSurfaceSdlBase/Initialize: Resolved Win32 handles.");
        }
#endif
    }

    return nullptr != pSdlWindow;
}

void apemode::AppSurfaceSdlBase::Finalize( ) {
    if ( pSdlWindow ) {
        SDL_Log( "apemode/AppSurfaceSdlVk/Finalize: Deleting window." );
        SDL_DestroyWindow( pSdlWindow );
        pSdlWindow = nullptr;
    }
}

uint32_t apemode::AppSurfaceSdlBase::GetWidth( ) const {
    SDL_assert( pSdlWindow && "Not initialized." );

    int OutWidth;
    SDL_GetWindowSize( pSdlWindow, &OutWidth, nullptr );
    return static_cast< uint32_t >( OutWidth );
}

uint32_t apemode::AppSurfaceSdlBase::GetHeight( ) const {
    SDL_assert( pSdlWindow && "Not initialized." );

    int OutHeight;
    SDL_GetWindowSize( pSdlWindow, nullptr, &OutHeight );
    return static_cast< uint32_t >( OutHeight );
}

void* apemode::AppSurfaceSdlBase::GetWindowHandle( ) {
    return reinterpret_cast< void* >( pSdlWindow );
}