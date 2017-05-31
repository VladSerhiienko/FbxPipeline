#pragma once

#include <GraphicsDevice.Vulkan.h>

namespace apemodevk
{
    // TODO Owned by graphics ecosystem (can support multiple devices)
    class ResourceAssembler : public apemodevk::ScalableAllocPolicy,
                                                          public apemodevk::NoCopyAssignPolicy
    {
    public:
        static std::shared_ptr<ResourceAssembler> MakeNewUnique ();
        static std::unique_ptr<ResourceAssembler> MakeNullUnique ();

    public:
    };
}