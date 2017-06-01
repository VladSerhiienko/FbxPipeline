#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <RenderPassResources.Vulkan.h>

namespace apemodevk {
    class CommandBuffer;
    class CommandQueue;
    class GraphicsDevice;
    class ColorResourceView;
    class RenderPassResources;

    class Swapchain : public apemodevk::ScalableAllocPolicy, public apemodevk::NoCopyAssignPolicy {
    public:
#ifdef _WIN32
        typedef HWND      WindowHandle;
        typedef HINSTANCE ModuleHandle;
#else
        typedef void *    WindowHandle;
        typedef void *    ModuleHandle;
#endif
        static uint32_t const     kExtentMatchFullscreen; // = -1;
        static uint32_t const     kExtentMatchWindow;     // =  0;
        static ModuleHandle const kCurrentExecutable;     // = nullptr;
        static uint64_t const     kMaxTimeout;            // = uint64_max;
        static uint32_t const     kMaxImgs = 3;

        Swapchain( );
        ~Swapchain( );

        /**
         * Initialized surface, render targets` and depth stencils` views.
         * @return True if initialization went ok, false otherwise.
         */
        bool RecreateResourceFor( apemodevk::GraphicsDevice& InGraphicsNode,
                                  uint32_t CmdQueueFamilyId,
#ifdef _WIN32
                                  ModuleHandle InInst,
                                  WindowHandle InWnd,
#endif
                                  uint32_t InDesiredColorWidth,
                                  uint32_t InDesiredColorHeight );

        /**
         * Acquires next swapchain buffer
         * @return true If the swapchain buffer was acquired, false otherwise.
         * @note Sets write frame for provided renderpass resources instance.
         */
        bool OnFrameMove( apemodevk::RenderPassResources& Resources,
                          VkSemaphore                     hSemaphore,
                          VkFence                         hFence,
                          uint64_t                        Timeout = kMaxTimeout );

        /**
         * Presents swapchain buffer content to the surface.
         * @return true If presented succeessfully, false otherwise.
         * @note Advances frame counters for provided renderpass resources instance.
         */
        bool OnFramePresent( apemodevk::CommandQueue&        CmdQueue,
                             apemodevk::RenderPassResources& Resources,
                             VkSemaphore*                    pWaitSemaphores    = nullptr,
                             uint32_t                        WaitSemaphoreCount = 0 );

        bool ExtractSwapchainBuffers( std::vector< VkImage >& OutSwapchainBufferImgs );
        bool ExtractSwapchainBuffers( VkImage *OutSwapchainBufferImgs );

        uint32_t GetBufferCount( ) const;

    public:
        apemodevk::GraphicsDevice*                                   pNode;
        apemodevk::CommandQueue*                                     pCmdQueue;
        uint16_t                                                     Id;
        VkExtent2D                                                   ColorExtent;
        VkFormat                                                     eColorFormat;
        VkColorSpaceKHR                                              eColorSpace;
        VkPresentModeKHR                                             ePresentMode;
        VkSurfaceTransformFlagsKHR                                   eSurfaceTransform;
        VkSurfaceCapabilitiesKHR                                     SurfaceCaps;
        apemodevk::TDispatchableHandle< VkSwapchainKHR >             hSwapchain;
        apemodevk::TDispatchableHandle< VkSurfaceKHR >               hSurface;
        VkImage                                       hImgs[kMaxImgs];
        apemodevk::TDispatchableHandle< VkImageView > hImgViews[kMaxImgs];
        uint32_t ImgCount;
    };
}
