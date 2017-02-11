#include <fbxvpch.h>
#include <AppSurface.h>

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
    // @note Done internally by bgfx.
    // AppSurfaceBase::OnFrameDone( );
}

bool fbxv::AppSurface::Initialize( ) {
    if ( AppSurfaceBase::Initialize( ) ) {
        if ( !bgfx::sdlSetWindow( (SDL_Window*) GetWindowHandle( ) ) ) {
            return false;
        }

        const auto bgfxRendererType = bgfx::RendererType::OpenGLES;

        if ( !bgfx::init( bgfxRendererType, 0, 0, new bgfx::Callback( ), new bx::Allocator( ) ) ) {
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
