#pragma once

struct apemode::GraphicsManager::PrivateCreateDeviceArgs
    : public apemode::ScalableAllocPolicy
    , public apemode::NoCopyAssignPolicy
{
    VkPhysicalDevice AdapterHandle;
    GraphicsManager & GraphicsEcosystem;

    PrivateCreateDeviceArgs(apemode::GraphicsManager & GraphicsEcosystem, VkPhysicalDevice AdapterHandle)
        : GraphicsEcosystem(GraphicsEcosystem)
        , AdapterHandle(AdapterHandle)
    {
    }
};