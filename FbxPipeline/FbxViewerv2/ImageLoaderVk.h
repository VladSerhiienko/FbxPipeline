#pragma once

#include <GraphicsDevice.Vulkan.h>

namespace apemodevk {

    struct LoadedImage {
        VkImage               pImage;
        VkImageView           pImageView;
        VkImageView           pImageView;
        VkDeviceMemory        pMemory;
        VkImageCreateInfo     imageCreateInfo;
        VkImageViewCreateInfo imageViewCreateInfo;
    };

    class ImageLoader {
    public:
        enum EImageFileFormat {
            eImageFileFormat_DDS,
            eImageFileFormat_KTX,
            eImageFileFormat_PNG,
            // TODO PVR
        };

        std::unique_ptr< LoadedImage > LoadImageFromData( apemodevk::GraphicsDevice*    pNode,
                                                          const std::vector< uint8_t >& InFileContent,
                                                          EImageFileFormat              eFileFormat );
    };
}