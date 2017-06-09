#include <fbxvpch.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <AppSurfaceSdlGL.h>

apemode::AppSurfaceSdlGL::AppSurfaceSdlGL( ) {
    Impl = kAppSurfaceImpl_SdlGL;
}

apemode::AppSurfaceSdlGL::~AppSurfaceSdlGL( ) {
    Finalize( );
}

bool apemode::AppSurfaceSdlGL::Initialize( uint32_t width, uint32_t height, const char *name ) {
    SDL_Log( "apemode/AppSurfaceSdlGL/Initialize" );

    //
    // Override window creation.
    // Set OpenGL flag for context initialization.
    //

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

        pSdlWindow = SDL_CreateWindow( name,
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       width,
                                       height,
                                       SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL );

        SDL_Log( "apemode/AppSurfaceSdlGL/Initialize: Created window." );
    }

#ifdef _WINDOWS_

    glewExperimental = GL_TRUE;
    if ( GLEW_OK != glewInit( ) ) {
        SDL_Log( "apemode/AppSurfaceSdlGL/Initialize: Failed to initialize GLEW." );
        return false;
    }

    SDL_Log( "apemode/AppSurfaceSdlGL/Initialize: Initialized GLEW." );

#endif

    if ( pSdlGlContext ) {
        SDL_Log( "apemode/AppSurfaceSdlGL/OnRecreateGraphicsContext: Deleted OpenGL context." );
        SDL_GL_DeleteContext( pSdlGlContext );
        pSdlGlContext = nullptr;
    }

    SDL_Log( "apemode/AppSurfaceSdlGL/OnRecreateGraphicsContext: Created OpenGL context." );
    pSdlGlContext = SDL_GL_CreateContext( pSdlWindow );

    assert( pSdlGlContext != nullptr && "Failed to initialize GL (SDL)." );
    return pSdlGlContext != nullptr;
}

void apemode::AppSurfaceSdlGL::Finalize( ) {
    if ( pSdlGlContext ) {
        SDL_Log( "apemode/AppSurfaceSdlGL/Finalize: Deleting OpenGL context." );
        SDL_GL_DeleteContext( pSdlGlContext );
        pSdlGlContext = nullptr;
    }

    AppSurfaceSdlBase::Finalize();
}

void apemode::AppSurfaceSdlGL::OnFrameDone( ) {
    SDL_GL_SwapWindow( pSdlWindow );
}

void *apemode::AppSurfaceSdlGL::GetGraphicsHandle( ) {
    return reinterpret_cast< void * >( pSdlGlContext );
}