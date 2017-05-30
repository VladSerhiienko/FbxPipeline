#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>
#include <RenderPassResources.Vulkan.h>

namespace apemode
{
    class CommandBuffer;
    class CommandQueue;
    class GraphicsDevice;
    class ColorResourceView;
    class RenderPassResources;

    class Swapchain : public apemode::ScalableAllocPolicy,
                                                  public apemode::NoCopyAssignPolicy
    {
    public:
#ifdef _WIN32
        typedef HWND      WindowHandle;
        typedef HINSTANCE ModuleHandle;
#endif
        static uint32_t const     kExtentMatchFullscreen; // = -1;
        static uint32_t const     kExtentMatchWindow;     // =  0;
        static ModuleHandle const kCurrentExecutable;     // = nullptr;
        static uint64_t const     kMaxTimeout;            // = uint64_max;

    public:
        Swapchain ();
        ~Swapchain ();

        /**
         * Initialized surface, render targets` and depth stencils` views.
         * @return True if initialization went ok, false otherwise.
         */
        bool RecreateResourceFor (apemode::GraphicsDevice & InGraphicsNode,
                                  apemode::CommandQueue &   InCmdQueue,
#ifdef _WIN32
                                  ModuleHandle InInst,
                                  WindowHandle InWnd,
#endif
                                  uint32_t InDesiredColorWidth,
                                  uint32_t InDesiredColorHeight);

        /**
         * Checks if passed command queue supports swapchain presenting.
         * @return True if it does, false otherwise.
         */
        bool SupportsPresenting (apemode::GraphicsDevice const & InGraphicsNode,
                                 apemode::CommandQueue const &   InCmdQueue) const;

        /**
         * Acquires next swapchain buffer
         * @return true If the swapchain buffer was acquired, false otherwise.
         * @note Sets write frame for provided renderpass resources instance.
         */
        bool OnFrameMove (apemode::RenderPassResources & Resources,
                          VkSemaphore                 hSemaphore,
                          VkFence                     hFence,
                          uint64_t                    Timeout = kMaxTimeout);

        /**
         * Presents swapchain buffer content to the surface.
         * @return true If presented succeessfully, false otherwise.
         * @note Advances frame counters for provided renderpass resources instance.
         */
        bool OnFramePresent (apemode::CommandQueue &        CmdQueue,
                             apemode::RenderPassResources & Resources,
                             VkSemaphore *               pWaitSemaphores    = nullptr,
                             uint32_t                    WaitSemaphoreCount = 0);

        /** @return Semaphore the current swapchain image was acquired. */
        //VkSemaphore GetPresentSemaphore ();
        /**
        * @return Next semaphore the swapchain image was acquired with
        * on the next OnFrameMove call.
        */
        /*VkSemaphore GetNextPresentSemaphore ();
        void        AdvancePresentSemaphoreIdx ();*/

    public:
        uint32_t GetBufferCount () const;

    public:
        apemode::GraphicsDevice * pNode;
        apemode::CommandQueue *   pCmdQueue;

    public:
        uint16_t                                  Id;
        VkExtent2D                                ColorExtent;
        VkFormat                                  eColorFormat;
        VkColorSpaceKHR                           eColorSpace;
        VkPresentModeKHR                          ePresentMode;
        VkSurfaceTransformFlagsKHR                eSurfaceTransform;
        VkSurfaceCapabilitiesKHR                  SurfaceCaps;
        apemode::TDispatchableHandle<VkSwapchainKHR> hSwapchain;
        apemode::TDispatchableHandle<VkSurfaceKHR>   hSurface;
        std::vector<VkImage>             hBuffers;

        /*std::vector<VkSemaphore> hPresentSemaphores;
        uint32_t                          PresentSemapforeIdx;*/
    };

}
