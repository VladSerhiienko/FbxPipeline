#pragma once

#include <GraphicsDevice.Vulkan.h>

namespace apemodevk {

    class ImageLoader {
    public:
        bool LoadImageFromData(apemodevk::GraphicsDevice* pNode, const std::vector< uint8_t >& InFileContent);

    };

}