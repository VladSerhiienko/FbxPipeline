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

namespace bx {
    struct Allocator : AllocatorI {
        virtual ~Allocator( ) {
        }

        /// Allocated, resizes memory block or frees memory.
        /// @param[in] _ptr If _ptr is NULL new block will be allocated.
        /// @param[in] _size If _ptr is set, and _size is 0, memory will be freed.
        /// @param[in] _align Alignment.
        /// @param[in] _file Debug file path info.
        /// @param[in] _line Debug file line info.
        void* realloc( void* _ptr, size_t _size, size_t _align, const char* _file, uint32_t _line ) override {
            (void) _file;
            (void) _line;
            if ( nullptr == _ptr )
                return fbxv::Memalign( std::max< size_t >( _align, MALLOC_ALIGNMENT ), _size );
            else if ( 0 == _size ) {
                assert( _ptr );
                fbxv::Free( _ptr );
                return nullptr;
            } else {
                assert( _ptr );
                fbxv::Free( _ptr );
                return fbxv::Memalign( std::max< size_t >( _align, MALLOC_ALIGNMENT ), _size );
            }
        }
    };
}

namespace bgfx {

    struct Callback : CallbackI {
        virtual ~Callback( ) {
        }

        /// If fatal code code is not Fatal::DebugCheck this callback is
        /// called on unrecoverable error. It's not safe to continue, inform
        /// user and terminate application from this call.
        /// @param[in] _code Fatal error code.
        /// @param[in] _str More information about error.
        /// @remarks Not thread safe and it can be called from any thread.
        /// @attention C99 equivalent is `bgfx_callback_vtbl.fatal`.
        virtual void fatal( Fatal::Enum _code, const char* _str ) {
            DebugBreak( );
        }

        /// Print debug message.
        /// @param[in] _filePath File path where debug message was generated.
        /// @param[in] _line Line where debug message was generated.
        /// @param[in] _format `printf` style format.
        /// @param[in] _argList Variable arguments list initialized with `va_start`.
        /// @remarks Not thread safe and it can be called from any thread.
        virtual void traceVargs( const char* _filePath, uint16_t _line, const char* _format, va_list _argList ) {
        }

        /// Return size of for cached item. Return 0 if no cached item was found.
        /// @param[in] _id Cache id.
        /// @returns Number of bytes to read.
        virtual uint32_t cacheReadSize( uint64_t _id ) {
            return 0;
        }

        /// Read cached item.
        /// @param[in] _id Cache id.
        /// @param[in] _data Buffer where to read data.
        /// @param[in] _size Size of data to read.
        /// @returns True if data is read.
        virtual bool cacheRead( uint64_t _id, void* _data, uint32_t _size ) {
            return false;
        }

        /// Write cached item.
        /// @param[in] _id Cache id.
        /// @param[in] _data Data to write.
        /// @param[in] _size Size of data to write.
        virtual void cacheWrite( uint64_t _id, const void* _data, uint32_t _size ) {
        }

        /// Screenshot captured. Screenshot format is always 4-byte BGRA.
        /// @param[in] _filePath File path.
        /// @param[in] _width Image width.
        /// @param[in] _height Image height.
        /// @param[in] _pitch Number of bytes to skip to next line.
        /// @param[in] _data Image data.
        /// @param[in] _size Image size.
        /// @param[in] _yflip If true image origin is bottom left.
        virtual void screenShot( const char* _filePath,
                                 uint32_t    _width,
                                 uint32_t    _height,
                                 uint32_t    _pitch,
                                 const void* _data,
                                 uint32_t    _size,
                                 bool        _yflip ) {
        }

        /// Called when capture begins.
        virtual void captureBegin(uint32_t _width, uint32_t _height, uint32_t _pitch, TextureFormat::Enum _format, bool _yflip ) {
        }

        /// Called when capture ends.
        virtual void captureEnd() {
        }

        /// Captured frame.
        /// @param[in] _data Image data.
        /// @param[in] _size Image size.
        virtual void captureFrame( const void* _data, uint32_t _size ) {
        }
    };

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

fbxv::AppSurface::AppSurface( ) {
}

fbxv::AppSurface::~AppSurface()
{
}

void fbxv::AppSurface::OnFrameMove( ) {
    AppSurfaceBase::OnFrameMove( );
}

void fbxv::AppSurface::OnFrameDone( ) {
    // Advance to next frame. Rendering thread will be kicked to
    // process submitted rendering primitives.
    bgfx::frame( );

    // Swap.
    // @note Done internally by bgfx.
    // AppSurfaceBase::OnFrameDone( );
}

bool fbxv::AppSurface::Initialize( ) {
    if ( AppSurfaceBase::Initialize( ) ) {
        if ( !bgfx::sdlSetWindow( (SDL_Window*) GetWindowHandle( ) ) ) {
            return false;
        }

        const auto bgfxRendererType = bgfx::RendererType::OpenGL;

        if ( !bgfx::init( bgfxRendererType, 0, 0 ) ) { // , new bgfx::Callback(), new bx::Allocator()) ) {
            return false;
        }

        assert( bgfxRendererType == bgfx::getRendererType( ) );
        SDL_Log( "Renderer type: %u", bgfx::getRendererType( ) );
        SDL_Log( "Renderer name: %s", bgfx::getRendererName( bgfxRendererType ) );


        // Enable debug text and set view 0 clear state.
        bgfx::reset( GetWidth( ), GetHeight( ), BGFX_RESET_VSYNC );
        bgfx::setDebug(BGFX_DEBUG_TEXT);
        bgfx::setViewClear( 0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0 );

        return true;
    }

    return false;
}

void fbxv::AppSurface::Finalize( ) {
    nk_sdl_device_destroy( );
    bgfx::shutdown( );
    AppSurfaceBase::Finalize( );
}
