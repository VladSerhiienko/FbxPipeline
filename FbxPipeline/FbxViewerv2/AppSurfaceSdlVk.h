#pragma once

#include <IAppSurface.h>

namespace apemodevk {
    class GraphicsManager;
    class GraphicsDevice;
    class Swapchain;
    class CommandQueue;
}

namespace apemode {
    class AppSurfaceSettings;

    /**
     * Contains handle to window and graphics context.
     **/
    class AppSurfaceSdlVk : public IAppSurface {
    public:
        AppSurfaceSdlVk( );
        virtual ~AppSurfaceSdlVk( );

        virtual bool Initialize( ) override;
        virtual void Finalize( ) override;

        virtual void OnFrameMove( ) override;
        virtual void OnFrameDone( ) override;

        virtual uint32_t GetWidth( ) const override;
        virtual uint32_t GetHeight( ) const override;
        virtual void*    GetWindowHandle( ) override;
        virtual void*    GetGraphicsHandle( ) override;

        SDL_Window*                                   pSdlWindow;
        HWND                                          hWnd;
        HINSTANCE                                     hInstance;
        std::unique_ptr< apemodevk::GraphicsManager > pDeviceManager;
        std::unique_ptr< apemodevk::Swapchain >       pSwapchain;
        std::unique_ptr< apemodevk::CommandQueue >    pCmdQueue;
        apemodevk::GraphicsDevice*                    pNode;
    };
}