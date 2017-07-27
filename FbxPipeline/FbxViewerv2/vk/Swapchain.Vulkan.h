#pragma once

#include <GraphicsDevice.Vulkan.h>

namespace apemodevk {
    class CommandBuffer;
    class CommandQueue;
    class GraphicsDevice;
    class ColorResourceView;
    class RenderPassResources;

    class Surface : public apemodevk::NoCopyAssignPolicy {
    public:
#ifdef _WIN32
        typedef HWND      WindowHandle;
        typedef HINSTANCE ModuleHandle;
#else
        typedef void* WindowHandle;
        typedef void* ModuleHandle;
#endif
        bool Recreate( VkPhysicalDevice pInPhysicalDevice, VkInstance pInInstance, ModuleHandle InInst, WindowHandle InWnd );

        VkPhysicalDevice                    pPhysicalDevice;
        VkInstance                          pInstance;
        ModuleHandle                        Inst;
        WindowHandle                        Wnd;
        VkSurfaceCapabilitiesKHR            SurfaceCaps;
        VkSurfaceTransformFlagsKHR          eSurfaceTransform;
        VkFormat                            eColorFormat;
        VkColorSpaceKHR                     eColorSpace;
        VkPresentModeKHR                    ePresentMode;
        TDispatchableHandle< VkSurfaceKHR > hSurface;
    };

    class Swapchain : public apemodevk::NoCopyAssignPolicy {
    public:
#ifdef _WIN32
        typedef HWND      WindowHandle;
        typedef HINSTANCE ModuleHandle;
#else
        typedef void* WindowHandle;
        typedef void* ModuleHandle;
#endif
        static uint32_t const     kExtentMatchFullscreen = -1;
        static uint32_t const     kExtentMatchWindow     = 0;
        static uint64_t const     kMaxTimeout            = uint64_t( -1 );
        static uint32_t const     kMaxImgs               = 3;

        Swapchain( );
        ~Swapchain( );

        /**
         * Initialized surface, render targets` and depth stencils` views.
         * @return True if initialization went ok, false otherwise.
         */
        bool Recreate( VkDevice                   pInDevice,
                       VkPhysicalDevice           pInPhysicalDevice,
                       VkSurfaceKHR               pInSurface,
                       uint32_t                   InImgCount,
                       uint32_t                   InDesiredColorWidth,
                       uint32_t                   InDesiredColorHeight,
                       VkFormat                   eColorFormat,
                       VkColorSpaceKHR            eColorSpace,
                       VkSurfaceTransformFlagsKHR eSurfaceTransform,
                       VkPresentModeKHR           ePresentMode );

        bool ExtractSwapchainBuffers( VkImage* OutSwapchainBufferImgs );

        uint32_t GetBufferCount( ) const;

    public:
        VkDevice                              pDevice;
        VkPhysicalDevice                      pPhysicalDevice;
        VkSurfaceKHR                          pSurface;
        uint32_t                              ImgCount;
        VkExtent2D                            ImgExtent;
        VkImage                               hImgs[ kMaxImgs ];
        TDispatchableHandle< VkImageView >    hImgViews[ kMaxImgs ];
        TDispatchableHandle< VkSwapchainKHR > hSwapchain;
    };
}
