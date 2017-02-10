#include <fbxvpch.h>
#include <AppSurface.h>

namespace bgfx {
    bool sdlSetWindow( SDL_Window* _window ) {
        SDL_SysWMinfo wmi;
        SDL_VERSION( &wmi.version );
        if ( !SDL_GetWindowWMInfo( _window, &wmi ) )
            return false;

        bgfx::PlatformData pd;
        pd.ndt          = NULL;
        pd.nwh          = wmi.info.win.window;
        pd.context      = NULL;
        pd.backBuffer   = NULL;
        pd.backBufferDS = NULL;
        bgfx::setPlatformData( pd );

        return true;
    }
}

void fbxv::AppSurface::OnFrameMove( ) {
    AppSurfaceBase::OnFrameMove( );

    //
    // Handle resizing.
    //

    auto width  = GetWidth( );
    auto height = GetHeight( );
    if ( width != mWidth || height != mHeight ) {
        mWidth  = width;
        mHeight = height;
        bgfx::reset( mWidth, mHeight, BGFX_RESET_VSYNC );
    }

    // Set view 0 default viewport.
    bgfx::setViewRect( 0, 0, 0, uint16_t( GetWidth( ) ), uint16_t( GetHeight( ) ) );

    // This dummy draw call is here to make sure that view 0 is cleared
    // if no other draw calls are submitted to view 0.
    bgfx::touch( 0 );

    // Use debug font to print information about this example.
    bgfx::dbgTextClear( );
    bgfx::dbgTextPrintf( 0, 1, 0x4f, "bgfx/examples/00-helloworld" );
    bgfx::dbgTextPrintf( 0, 2, 0x6f, "Description: Initialization and debug text." );

    const bgfx::Stats* stats = bgfx::getStats( );
    bgfx::dbgTextPrintf( 0,
                         6,
                         0x0f,
                         "Backbuffer %dW x %dH in pixels, debug text %dW x %dH in characters.",
                         stats->width,
                         stats->height,
                         stats->textWidth,
                         stats->textHeight );
}

void fbxv::AppSurface::OnFrameDone( ) {
    // Advance to next frame. Rendering thread will be kicked to
    // process submitted rendering primitives.
    bgfx::frame( );

    // Swap.
    // AppSurfaceBase::OnFrameDone( );
}

bool fbxv::AppSurface::Initialize( ) {
    if ( AppSurfaceBase::Initialize( ) ) {
        if ( !bgfx::sdlSetWindow( (SDL_Window*) GetWindowHandle( ) ) ) {
            return false;
        }

        const auto bgfxRendererType = bgfx::RendererType::OpenGLES;

        if ( !bgfx::init( bgfxRendererType ) ) {
            return false;
        }

        mWidth  = GetWidth( );
        mHeight = GetHeight( );

        // Enable debug text and set view 0 clear state.
        bgfx::reset( mWidth, mHeight, BGFX_RESET_VSYNC );
        bgfx::setDebug( BGFX_DEBUG_TEXT );
        bgfx::setViewClear( 0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0 );

        return true;
    }

    return false;
}

void fbxv::AppSurface::Finalize( ) {
    bgfx::shutdown( );
    AppSurfaceBase::Finalize( );
}
