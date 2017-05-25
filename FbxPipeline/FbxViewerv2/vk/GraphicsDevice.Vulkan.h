#pragma once

#include <ResultHandle.Vulkan.h>

namespace Core
{
    class Swapchain;
    class CommandQueue;
    class ResourceReference;
    class RenderPassManager;
    class RootSignatureManager;
    class PipelineStateManager;
    class ShaderManager;
    class FramebufferManager;
    class GraphicsHeterogeneousMultiadapterEcosystem;

    class _Graphics_ecosystem_dll_api GraphicsDevice : public Aux::ScalableAllocPolicy,
                                                       public Aux::NoCopyAssignPolicy
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
        GraphicsHeterogeneousMultiadapterEcosystem & GetGraphicsEcosystem ();

        ShaderManager &        GetDefaultShaderManager();
        RenderPassManager &    GetDefaultRenderPassManager();
        FramebufferManager &   GetDefaultFramebufferManager();
        RootSignatureManager & GetDefaultRootSignatureManager();
        PipelineStateManager & GetDefaultPipelineStateManager();

    private:
        struct PrivateContent;

    private:
        friend Swapchain;
        friend ResourceReference;
        friend PrivateContent;
        friend GraphicsHeterogeneousMultiadapterEcosystem;

    private:
        operator PrivateContent &();

        PrivateContent *                             pContent;
        GraphicsHeterogeneousMultiadapterEcosystem * pGraphicsEcosystem;
    };


}