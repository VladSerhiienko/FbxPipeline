#pragma once

#include <ResultHandle.Vulkan.h>

namespace apemode
{
    class Swapchain;
    class CommandQueue;
    class ResourceReference;
    class RenderPassManager;
    class PipelineLayoutManager;
    class PipelineStateManager;
    class ShaderManager;
    class FramebufferManager;
    class GraphicsManager;

    class GraphicsDevice : public apemode::ScalableAllocPolicy, public apemode::NoCopyAssignPolicy
    {
       public:
        static std::unique_ptr<GraphicsDevice> MakeNewUnique ();
        static std::unique_ptr<GraphicsDevice> MakeNullUnique ();

    public:
        GraphicsDevice ();
        ~GraphicsDevice ();

        bool RecreateResourcesFor (void * pDeviceCreateArgs);

        operator VkDevice () const;
        operator VkPhysicalDevice () const;
        operator VkInstance () const;

        bool     IsValid () const;
        bool     Await ();
        uint32_t GetQueueFamilyCount ();
        uint32_t GetQueueCountInQueueFamily (uint32_t QueueFamilyId);
        GraphicsManager & GetGraphicsEcosystem ();

        ShaderManager &        GetDefaultShaderManager();
        RenderPassManager &    GetDefaultRenderPassManager();
        FramebufferManager &   GetDefaultFramebufferManager();
        PipelineLayoutManager & GetDefaultPipelineLayoutManager();
        PipelineStateManager & GetDefaultPipelineStateManager();

    private:
        struct PrivateContent;

    private:
        friend Swapchain;
        friend ResourceReference;
        friend PrivateContent;
        friend GraphicsManager;

    private:
        operator PrivateContent &();

        PrivateContent *                             pContent;
        GraphicsManager * pGraphicsEcosystem;
    };


}