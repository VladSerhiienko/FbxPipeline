#pragma once

#include <Texture.Vulkan.h>

namespace apemode
{
    class DepthStencilResourceView : public TextureResourceView
    {
    public:
        static std::shared_ptr<DepthStencilResourceView> MakeNewLinked ();
        static std::unique_ptr<DepthStencilResourceView> MakeNewUnique ();
        static std::unique_ptr<DepthStencilResourceView> MakeNullUnique ();

    public:
        /** 32-bit depth w/ 8-bit stencil. */
        static VkFormat const kDefaultFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

    public:
        DepthStencilResourceView ();
        ~DepthStencilResourceView ();

        bool RecreateResourcesFor (GraphicsDevice & InGraphicsNode,
                                   VkFormat         InFmt,
                                   uint32_t         InWidth,
                                   uint32_t         InHeight,
                                   float            InClearDepth,
                                   uint32_t         InClearStencil,
                                   VkImageLayout    InInitialLayout);

    public:
        VkFormat                         DepthFormat;
        VkFormat                         StencilFormat;
        VkImageAspectFlags               DepthAspect;
        VkImageAspectFlags               StencilAspect;
        TDispatchableHandle<VkImageView> DepthImgViewHandle;
        TDispatchableHandle<VkImageView> StencilImgViewHandle;
    };

}
