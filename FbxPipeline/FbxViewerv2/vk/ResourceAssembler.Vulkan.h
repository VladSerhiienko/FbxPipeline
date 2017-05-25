#pragma once

#include <GraphicsDevice.Vulkan.h>

namespace Core
{
    // TODO Owned by graphics ecosystem (can support multiple devices)
    class _Graphics_ecosystem_dll_api ResourceAssembler : public Aux::ScalableAllocPolicy,
                                                          public Aux::NoCopyAssignPolicy
    {
    public:
        static std::shared_ptr<ResourceAssembler> MakeNewUnique ();
        static std::unique_ptr<ResourceAssembler> MakeNullUnique ();

    public:
    };
}