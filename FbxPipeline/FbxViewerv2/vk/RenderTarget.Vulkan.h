#pragma once

#include <Texture.Vulkan.h>

namespace apemode
{
    class Swapchain;
    class ColorResourceView : public TextureResourceView
    {
        bool bIsOwnedBySwapchain;

    public:
        static std::shared_ptr<ColorResourceView> MakeNewLinked ();
        static std::unique_ptr<ColorResourceView> MakeNewUnique ();
        static std::unique_ptr<ColorResourceView> MakeNullUnique ();

    public:
        ColorResourceView();
        ~ColorResourceView();

        bool RecreateResourcesFor (GraphicsDevice & InGraphicsNode,
                                   VkFormat         InImageFormat,
                                   VkImage          InImageHandle);
    };

}
