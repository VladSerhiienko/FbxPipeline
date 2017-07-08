#pragma once

#include <AppSurfaceSdlBase.h>
#include <GraphicsDevice.Vulkan.h>
#include <GraphicsManager.Vulkan.h>
#include <QueuePools.Vulkan.h>
#include <Swapchain.Vulkan.h>

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
        SceneRendererBase* CreateSceneRenderer( ) override;

        uint32_t                   LastWidth;
        uint32_t                   LastHeight;
        apemodevk::GraphicsManager DeviceManager;
        apemodevk::Surface         Surface;
        apemodevk::Swapchain       Swapchain;
        apemodevk::AcquiredQueue   PresentQueue;
        apemodevk::GraphicsDevice* pNode;
    };
}