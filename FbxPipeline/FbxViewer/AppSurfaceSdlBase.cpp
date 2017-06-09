#include <fbxvpch.h>

#include <AppSurfaceBase.h>

class fbxv::AppSurfaceSettings {
public:
    const char *pName;
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

struct fbxv::AppSurfaceBase::PrivateContent {
    SDL_Window *  pSdlWindow;
    SDL_GLContext pSdlGlContext;

    PrivateContent( ) : pSdlWindow( nullptr ), pSdlGlContext( nullptr ) {
    }

    ~PrivateContent( ) {
        pSdlWindow    = nullptr;
        pSdlGlContext = nullptr;
    }
};

fbxv::AppSurfaceBase::AppSurfaceBase( ) : pContent( nullptr ) {
}

fbxv::AppSurfaceBase::~AppSurfaceBase( ) {
    Finalize( );
}

bool fbxv::AppSurfaceBase::Initialize( ) {
    SDL_Log( "fbxv/AppSurfaceBase/Initialize" );

    pContent = new PrivateContent( );
    if ( !SDL_Init( SDL_INIT_VIDEO ) ) {
#if defined( __ANDROID__ )
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES );
#endif
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
        SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
        SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, true );

        AppSurfaceSettings defaultSettings;
        pContent->pSdlWindow = SDL_CreateWindow( defaultSettings.pName,
                                                 SDL_WINDOWPOS_CENTERED,
                                                 SDL_WINDOWPOS_CENTERED,
                                                 defaultSettings.Width,
                                                 defaultSettings.Height,
                                                 SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL );

        SDL_Log( "fbxv/AppSurfaceBase/Initialize: Created window." );
        return OnRecreateGraphicsContext( );
    }

    return false;
}

bool fbxv::AppSurfaceBase::OnRecreateGraphicsContext( ) {
    if ( pContent->pSdlGlContext ) {
        SDL_Log( "fbxv/AppSurfaceBase/OnRecreateGraphicsContext: Deleted OpenGL context." );
        SDL_GL_DeleteContext( pContent->pSdlGlContext );
        pContent->pSdlGlContext = nullptr;
    }

    SDL_Log( "fbxv/AppSurfaceBase/OnRecreateGraphicsContext: Created OpenGL context." );
    pContent->pSdlGlContext = SDL_GL_CreateContext( pContent->pSdlWindow );

    assert( pContent->pSdlGlContext != nullptr && "Failed to initialize GL (SDL)." );
    return pContent->pSdlGlContext != nullptr;
}

void fbxv::AppSurfaceBase::Finalize( ) {
    if ( pContent->pSdlGlContext ) {
        SDL_Log( "fbxv/AppSurfaceBase/Finalize: Deleting OpenGL context." );
        SDL_GL_DeleteContext( pContent->pSdlGlContext );
        pContent->pSdlGlContext = nullptr;
    }

    if ( pContent->pSdlWindow ) {
        SDL_Log( "fbxv/AppSurfaceBase/Finalize: Deleting window." );
        SDL_DestroyWindow( pContent->pSdlWindow );
        pContent->pSdlWindow = nullptr;
    }

    if ( pContent ) {
        SDL_Log( "fbxv/AppSurfaceBase/Finalize: Deleting content." );
        delete pContent;
        pContent = nullptr;
    }
}

uint32_t fbxv::AppSurfaceBase::GetWidth( ) const {
    assert( pContent->pSdlWindow && "Not initialized." );

    int OutWidth;
    SDL_GetWindowSize( pContent->pSdlWindow, &OutWidth, nullptr );
    return static_cast< uint32_t >( OutWidth );
}

uint32_t fbxv::AppSurfaceBase::GetHeight( ) const {
    assert( pContent->pSdlWindow && "Not initialized." );

    int OutHeight;
    SDL_GetWindowSize( pContent->pSdlWindow, nullptr, &OutHeight );
    return static_cast< uint32_t >( OutHeight );
}

void fbxv::AppSurfaceBase::OnFrameMove( ) {
}

void fbxv::AppSurfaceBase::OnFrameDone( ) {
    SDL_GL_SwapWindow( pContent->pSdlWindow );
}

void *fbxv::AppSurfaceBase::GetWindowHandle( ) {
    return reinterpret_cast< void * >( pContent->pSdlWindow );
}

void *fbxv::AppSurfaceBase::GetGraphicsHandle( ) {
    return reinterpret_cast< void * >( pContent->pSdlGlContext );
}