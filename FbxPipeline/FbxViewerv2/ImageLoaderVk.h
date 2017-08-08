#pragma once

#include <GraphicsDevice.Vulkan.h>

namespace apemodevk {

    struct LoadedImage {
        VkImage           pImage;
        VkDeviceMemory    pMemory;
        VkImageCreateInfo imageCreateInfo;
    };

    class ImageLoader {
    public:
        std::unique_ptr< LoadedImage > LoadImageFromData( apemodevk::GraphicsDevice*    pNode,
                                                          const std::vector< uint8_t >& InFileContent );
    };
}