#pragma once

struct apemode::GraphicsEcosystem::PrivateCreateDeviceArgs
    : public apemode::ScalableAllocPolicy
    , public apemode::NoCopyAssignPolicy
{
    VkPhysicalDevice AdapterHandle;
    GraphicsEcosystem & GraphicsEcosystem;

    PrivateCreateDeviceArgs(apemode::GraphicsEcosystem & GraphicsEcosystem, VkPhysicalDevice AdapterHandle)
        : GraphicsEcosystem(GraphicsEcosystem)
        , AdapterHandle(AdapterHandle)
    {
    }
};