#pragma once

#include <GraphicsDevice.Vulkan.h>

namespace apemodevk {
    struct StoredSampler {
        uint64_t            Hash     = 0;
        VkSampler           pSampler = nullptr;
        VkSamplerCreateInfo SamplerCreateInfo;
    };

    class SamplerManager {
    public:
        apemodevk::GraphicsDevice*   pNode;
        std::vector< StoredSampler > StoredSamplers;

        bool Recreate( apemodevk::GraphicsDevice* pNode );
        uint32_t GetSamplerIndex( const VkSamplerCreateInfo& SamplerCreateInfo );
    };
}