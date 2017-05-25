#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <TInfoStruct.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>

namespace Core
{
    class CommandList;
    class RootSignature;
    class DescriptorSet;
    class DescriptorPool;

    class _Graphics_ecosystem_dll_api DescriptorSet : public Aux::ScalableAllocPolicy,
                                                      public Aux::NoCopyAssignPolicy
    {
    public:
        DescriptorSet();
        ~DescriptorSet();

        bool RecreateResourcesFor (Core::GraphicsDevice & GraphicsNode,
                                   Core::DescriptorPool & DescPool,
                                   Core::RootSignature &  RootSign,
                                   uint32_t               DescSetLayoutIndex);

        void BindTo(Core::CommandList & CmdList,
                    VkPipelineBindPoint PipelineBindPoint,
                    uint32_t            DynamicOffsetCount,
                    const uint32_t *    DynamicOffsets);

    public:
        operator VkDescriptorSet() const;

    public:
        Core::GraphicsDevice const *               pGraphicsNode;
        Core::RootSignature const *                pRootSign;
        Core::DescriptorPool const *               pDescPool;
        Core::TDispatchableHandle<VkDescriptorSet> hDescSet;
    };

    class _Graphics_ecosystem_dll_api DescriptorSetUpdater : public Aux::ScalableAllocPolicy,
                                                             public Aux::NoCopyAssignPolicy
    {
        bool SetGraphicsNode (Core::GraphicsDevice const & GraphicsNode);

    public:
        DescriptorSetUpdater ();

        /** Resets graphics node (member pointer), reserves memory. */
        void Reset (uint32_t MaxWrites, uint32_t MaxCopies);

        /** DescSet must be initialized. */
        bool WriteUniformBuffer (Core::DescriptorSet const & DescSet,
                                 VkBuffer                    Buffer,
                                 uint32_t                    Offset,
                                 uint32_t                    Range,
                                 uint32_t                    Binding,
                                 uint32_t                    Count);

        /** DescSet must be initialized. */
        bool WriteCombinedImgSampler (Core::DescriptorSet const & DescSet,
                                      VkImageView                 ImgView,
                                      VkImageLayout               ImgLayout,
                                      VkSampler                   Sampler,
                                      uint32_t                    Binding,
                                      uint32_t                    Count);

        void Flush ();

    public:
        Core::GraphicsDevice const *                 pGraphicsNode;
        std::vector<VkDescriptorBufferInfo> BufferInfos;
        std::vector<VkDescriptorImageInfo>  ImgInfos;
        std::vector<VkWriteDescriptorSet>   Writes;
        std::vector<VkCopyDescriptorSet>    Copies;
    };

    class _Graphics_ecosystem_dll_api DescriptorPool : public Aux::ScalableAllocPolicy,
                                                       public Aux::NoCopyAssignPolicy
    {
    public:
        DescriptorPool();
        ~DescriptorPool();

        bool RecreateResourcesFor (
            Core::GraphicsDevice & GraphicsNode,
            uint32_t MaxSets,
            uint32_t MaxSamplerCount,              // VK_DESCRIPTOR_TYPE_SAMPLER
            uint32_t MaxCombinedImgSamplerCount,   // VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
            uint32_t MaxSampledImgCount,           // VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
            uint32_t MaxStorageImgCount,           // VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
            uint32_t MaxUniformTexelBufferCount,   // VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
            uint32_t MaxStorageTexelBufferCount,   // VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
            uint32_t MaxUniformBufferCount,        // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
            uint32_t MaxStorageBufferCount,        // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
            uint32_t MaxDynamicUniformBufferCount, // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
            uint32_t MaxDynamicStorageBufferCount, // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
            uint32_t MaxInputAttachmentCount       // VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
            );

    public:
        uint32_t GetAvailableSetCount () const;
        uint32_t GetAvailableDescCount (VkDescriptorType DescType) const;

        operator VkDescriptorPool() const;

    public:
        Core::GraphicsDevice const *                pGraphicsNode;
        Core::TDispatchableHandle<VkDescriptorPool> hDescPool;

        friend DescriptorSet;

        /** Helps to track descriptor suballocations. */
        VkDescriptorPoolSize DescPoolCounters[ VK_DESCRIPTOR_TYPE_RANGE_SIZE ];
        uint32_t             DescSetCounter;
    };
}