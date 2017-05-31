#pragma once

#include <IAppSurface.h>

namespace apemode {
    /**
     * Contains handle to window and graphics context.
     */
    class AppSurfaceSdlGL : public IAppSurface {
        SDL_Window*   pSdlWindow;
        SDL_GLContext pSdlGlContext;

    public:
        AppSurfaceSdlGL( );
        virtual ~AppSurfaceSdlGL( );

        /** Creates window and initializes its graphics context. */
        virtual bool Initialize( ) override;

        /** Releases graphics context and destroyes window. */
        virtual void Finalize( ) override;

        /** Must be called when new frame starts. */
        virtual void OnFrameMove( ) override;

        /** Must be called when the current frame is done. */
        virtual void OnFrameDone( ) override;

        virtual uint32_t GetWidth( ) const override;
        virtual uint32_t GetHeight( ) const override;
        virtual void*    GetWindowHandle( ) override;
        virtual void*    GetGraphicsHandle( ) override;
    };
}