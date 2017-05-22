
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <fbxvpch.h>
#include <AppSurface.h>
#include <nuklear.h>

NK_API struct nk_context* nk_sdl_init( SDL_Window* win );
NK_API void nk_sdl_font_stash_begin( struct nk_font_atlas** atlas );
NK_API void nk_sdl_font_stash_end( void );
NK_API int nk_sdl_handle_event( SDL_Event* evt );
NK_API void nk_sdl_render( enum nk_anti_aliasing, int max_vertex_buffer, int max_element_buffer );
NK_API void nk_sdl_shutdown( void );
NK_API void nk_sdl_device_destroy( void );
NK_API void nk_sdl_device_create( void );

namespace fbxv {
    void* Malloc( size_t bytes );
    void* Memalign( size_t alignment, size_t bytes );
    void Free( void* p );
}

fbxv::AppSurface::AppSurface( ) {
}

fbxv::AppSurface::~AppSurface()
{
}

void fbxv::AppSurface::OnFrameMove( ) {
    AppSurfaceBase::OnFrameMove( );
}

void fbxv::AppSurface::OnFrameDone( ) {
    AppSurfaceBase::OnFrameDone( );
}

bool fbxv::AppSurface::Initialize( ) {
    if ( AppSurfaceBase::Initialize( ) ) {

#ifdef _WIN32
        glewExperimental = GL_TRUE;
        if ( GLEW_OK != glewInit( ) ) {
            SDL_Log( "Failed to initialize GLEW for Windows." );
            return false;
        }
#endif
        return true;
    }

    return false;
}

void fbxv::AppSurface::Finalize( ) {
    nk_sdl_device_destroy( );
    AppSurfaceBase::Finalize( );
}
