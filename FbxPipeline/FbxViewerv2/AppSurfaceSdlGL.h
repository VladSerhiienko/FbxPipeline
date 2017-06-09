#pragma once

#include <AppSurfaceSdlBase.h>

namespace apemode {
    /**
     * Contains handle to window and graphics context.
     */
    class AppSurfaceSdlGL : public AppSurfaceSdlBase {
    public:
        AppSurfaceSdlGL( );
        virtual ~AppSurfaceSdlGL( );

        virtual bool Initialize( uint32_t width, uint32_t height, const char* name ) override;
        virtual void Finalize( ) override;
        virtual void OnFrameDone( ) override;
        virtual void* GetGraphicsHandle( ) override;

        SDL_GLContext pSdlGlContext;
    };
}