#pragma once

#include <AppSurfaceSdlBase.h>

namespace apemodevk {
    class GraphicsManager;
    class GraphicsDevice;
    class CommandQueue;
    class Swapchain;
}

namespace apemode {
    class AppSurfaceSettings;

    /**
     * Contains handle to window and graphics context.
     **/
    class AppSurfaceSdlVk : public AppSurfaceSdlBase {
    public:
        AppSurfaceSdlVk( );
        virtual ~AppSurfaceSdlVk( );

        bool Initialize( uint32_t width, uint32_t height, const char* name ) override;
        void Finalize( ) override;

        void OnFrameMove( ) override;
        void* GetGraphicsHandle( ) override;
        ISceneRenderer* CreateSceneRenderer( ) override;

        uint32_t LastWidth;
        uint32_t LastHeight;
        std::unique_ptr< apemodevk::GraphicsManager > pDeviceManager;
        std::unique_ptr< apemodevk::Swapchain >       pSwapchain;
        std::unique_ptr< apemodevk::CommandQueue >    pCmdQueue;
        apemodevk::GraphicsDevice*                    pNode;
    };
}