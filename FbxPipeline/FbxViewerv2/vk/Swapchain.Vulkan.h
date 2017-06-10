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
        typedef void* WindowHandle;
        typedef void* ModuleHandle;
#endif
        static uint32_t const     kExtentMatchFullscreen = -1;
        static uint32_t const     kExtentMatchWindow     = 0;
        static uint64_t const     kMaxTimeout            = uint64_t( -1 );
        static uint32_t const     kMaxImgs               = 3;

        static ModuleHandle const kCurrentExecutable;

        Swapchain( );
        ~Swapchain( );

        /**
         * Initialized surface, render targets` and depth stencils` views.
         * @return True if initialization went ok, false otherwise.
         */
        bool RecreateResourceFor( apemodevk::GraphicsDevice& InGraphicsNode,
                                  uint32_t                   CmdQueueFamilyId,
#ifdef _WIN32
                                  ModuleHandle InInst,
                                  WindowHandle InWnd,
#endif
                                  uint32_t InDesiredColorWidth,
                                  uint32_t InDesiredColorHeight );

        bool Resize( uint32_t InDesiredColorWidth, uint32_t InDesiredColorHeight );
        bool ExtractSwapchainBuffers( std::vector< VkImage >& OutSwapchainBufferImgs );
        bool ExtractSwapchainBuffers( VkImage* OutSwapchainBufferImgs );

        uint32_t GetBufferCount( ) const;

    public:
        GraphicsDevice*                       pNode;
        CommandQueue*                         pCmdQueue;
        uint16_t                              Id;
        VkExtent2D                            ColorExtent;
        VkFormat                              eColorFormat;
        VkColorSpaceKHR                       eColorSpace;
        VkPresentModeKHR                      ePresentMode;
        VkSurfaceTransformFlagsKHR            eSurfaceTransform;
        VkSurfaceCapabilitiesKHR              SurfaceCaps;
        TDispatchableHandle< VkSwapchainKHR > hSwapchain;
        TDispatchableHandle< VkSurfaceKHR >   hSurface;
        VkImage                               hImgs[ kMaxImgs ];
        TDispatchableHandle< VkImageView >    hImgViews[ kMaxImgs ];
        uint32_t                              ImgCount;
    };
}
