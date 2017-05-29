#pragma once

#include <GraphicsDevice.Vulkan.h>

namespace apemode
{
    // TODO Owned by graphics ecosystem (can support multiple devices)
    class ResourceAssembler : public apemode::ScalableAllocPolicy,
                                                          public apemode::NoCopyAssignPolicy
    {
    public:
        static std::shared_ptr<ResourceAssembler> MakeNewUnique ();
        static std::unique_ptr<ResourceAssembler> MakeNullUnique ();

    public:
    };
}