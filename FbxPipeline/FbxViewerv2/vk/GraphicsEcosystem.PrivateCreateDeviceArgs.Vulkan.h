#pragma once

struct Core::GraphicsHeterogeneousMultiadapterEcosystem::PrivateCreateDeviceArgs
    : public Aux::ScalableAllocPolicy
    , public Aux::NoCopyAssignPolicy
{
    VkPhysicalDevice AdapterHandle;
    GraphicsHeterogeneousMultiadapterEcosystem & GraphicsEcosystem;

    PrivateCreateDeviceArgs(GraphicsHeterogeneousMultiadapterEcosystem & GraphicsEcosystem, VkPhysicalDevice AdapterHandle)
        : GraphicsEcosystem(GraphicsEcosystem)
        , AdapterHandle(AdapterHandle)
    {
    }
};