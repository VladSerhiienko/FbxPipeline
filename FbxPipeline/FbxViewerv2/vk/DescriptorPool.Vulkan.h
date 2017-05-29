#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <TInfoStruct.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>

namespace apemode
{
    class CommandList;
    class PipelineLayout;
    class DescriptorSet;
    class DescriptorPool;

    class DescriptorSet : public apemode::ScalableAllocPolicy, public apemode::NoCopyAssignPolicy {
    public:
        DescriptorSet( );
        ~DescriptorSet( );

        bool RecreateResourcesFor( apemode::GraphicsDevice& GraphicsNode,
                                   apemode::DescriptorPool& DescPool,
                                   apemode::PipelineLayout& RootSign,
                                   uint32_t                 DescSetLayoutIndex );

        void BindTo( apemode::CommandList& CmdList,
                     VkPipelineBindPoint   PipelineBindPoint,
                     uint32_t              DynamicOffsetCount,
                     const uint32_t*       DynamicOffsets );

    public:
        operator VkDescriptorSet() const;

    public:
        apemode::GraphicsDevice const *               pNode;
        apemode::PipelineLayout const *                pRootSign;
        apemode::DescriptorPool const *               pDescPool;
        apemode::TDispatchableHandle<VkDescriptorSet> hDescSet;
    };

    class DescriptorSetUpdater : public apemode::ScalableAllocPolicy, public apemode::NoCopyAssignPolicy {
        bool SetGraphicsNode (apemode::GraphicsDevice const & GraphicsNode);

    public:
        DescriptorSetUpdater ();

        /** Resets graphics node (member pointer), reserves memory. */
        void Reset( uint32_t MaxWrites, uint32_t MaxCopies );

        /** DescSet must be initialized. */
        bool WriteUniformBuffer( apemode::DescriptorSet const& DescSet,
                                 VkBuffer                      Buffer,
                                 uint32_t                      Offset,
                                 uint32_t                      Range,
                                 uint32_t                      Binding,
                                 uint32_t                      Count );

        /** DescSet must be initialized. */
        bool WriteCombinedImgSampler( apemode::DescriptorSet const& DescSet,
                                      VkImageView                   ImgView,
                                      VkImageLayout                 ImgLayout,
                                      VkSampler                     Sampler,
                                      uint32_t                      Binding,
                                      uint32_t                      Count );

        void Flush( );

    public:
        apemode::GraphicsDevice const*        pNode;
        std::vector< VkDescriptorBufferInfo > BufferInfos;
        std::vector< VkDescriptorImageInfo >  ImgInfos;
        std::vector< VkWriteDescriptorSet >   Writes;
        std::vector< VkCopyDescriptorSet >    Copies;
    };

    class DescriptorPool : public apemode::ScalableAllocPolicy, public apemode::NoCopyAssignPolicy {
    public:
        DescriptorPool( );
        ~DescriptorPool( );

        bool RecreateResourcesFor( apemode::GraphicsDevice& GraphicsNode,
                                   uint32_t MaxSets,
                                   uint32_t MaxSamplerCount,                 // VK_DESCRIPTOR_TYPE_SAMPLER
                                   uint32_t MaxCombinedImgSamplerCount,      // VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                                   uint32_t MaxSampledImgCount,              // VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
                                   uint32_t MaxStorageImgCount,              // VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
                                   uint32_t MaxUniformTexelBufferCount,      // VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
                                   uint32_t MaxStorageTexelBufferCount,      // VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
                                   uint32_t MaxUniformBufferCount,           // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                                   uint32_t MaxStorageBufferCount,           // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
                                   uint32_t MaxDynamicUniformBufferCount,    // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
                                   uint32_t MaxDynamicStorageBufferCount,    // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
                                   uint32_t MaxInputAttachmentCount          // VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
        );

    public:
        uint32_t GetAvailableSetCount( ) const;
        uint32_t GetAvailableDescCount( VkDescriptorType DescType ) const;

        operator VkDescriptorPool( ) const;

    public:
        apemode::GraphicsDevice const *                pNode;
        apemode::TDispatchableHandle<VkDescriptorPool> hDescPool;

        friend DescriptorSet;

        /** Helps to track descriptor suballocations. */
        VkDescriptorPoolSize DescPoolCounters[ VK_DESCRIPTOR_TYPE_RANGE_SIZE ];
        uint32_t             DescSetCounter;
    };

    struct DescriptorSetLayoutBinding {
        TInfoStruct< VkDescriptorSetLayoutBinding > Binding;

        void Clear( ) {
            Binding.ZeroMemory( );
        }

        void InitAsUniformBuffer( uint32_t Register, uint32_t Count, ShaderVisibility StageFlags, uint32_t Set = 0 ) {
            Binding->stageFlags = static_cast< VkShaderStageFlagBits >( StageFlags );
        }

        void InitAsUniformBufferDynamic( uint32_t Register, uint32_t Count, ShaderVisibility StageFlags, uint32_t Set = 0 );

        void InitAsSampler( uint32_t         Register,
                            uint32_t         Count,
                            ShaderVisibility StageFlags = kShaderVisibility_Fragment,
                            uint32_t         Set        = 0 );

        void InitAsCombinedImageSampler( uint32_t         Register,
                                         uint32_t         Count,
                                         VkSampler*       pImmutableSamplers,
                                         ShaderVisibility StageFlags = kShaderVisibility_Fragment,
                                         uint32_t         Set        = 0 );

    public:
        operator VkDescriptorSetLayoutBinding( ) const;
    };

    class DescriptorSetLayout {

    };
}