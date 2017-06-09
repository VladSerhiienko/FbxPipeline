#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <TInfoStruct.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>

namespace apemodevk
{
    class CommandBuffer;
    class PipelineLayout;
    class DescriptorSet;
    class DescriptorPool;

    class DescriptorSetLayout;
    class DescriptorSetLayoutBuilder;

    template < uint32_t TCount >
    class TDescriptorSets {
    public:
        VkDevice              hNode              = VK_NULL_HANDLE;
        VkDescriptorPool      hPool              = VK_NULL_HANDLE;
        VkDescriptorSet       hSets[ TCount ]    = {VK_NULL_HANDLE};
        VkDescriptorSetLayout hLayouts[ TCount ] = {VK_NULL_HANDLE};
        uint32_t              Offsets[ TCount ]  = {0};
        uint32_t              Counts[ TCount ]   = {0};

        TDescriptorSets( ) {
        }

        ~TDescriptorSets( ) {
            if ( VK_NULL_HANDLE != hNode && VK_NULL_HANDLE != hPool )
                vkFreeDescriptorSets( hNode, hPool, TCount, hSets );
        }

        bool RecreateResourcesFor( VkDevice         InNode,
                                   VkDescriptorPool InDescPool,
                                   VkDescriptorSetLayout ( &InLayouts )[ TCount ] ) {
            hNode = InNode;
            hPool = InDescPool;
            memcpy( hLayouts, InLayouts, TCount * sizeof( VkDescriptorSetLayout ) );

            TInfoStruct< VkDescriptorSetAllocateInfo > AllocInfo;
            AllocInfo->pSetLayouts        = hLayouts;
            AllocInfo->descriptorPool     = hPool;
            AllocInfo->descriptorSetCount = TCount;

            //TODO: Only for debugging.
            if ( false == std::is_sorted( &hLayouts[ 0 ], &hLayouts[ TCount ] ) ) {
                platform::DebugBreak( );
                return false;
            }

            apemodevk::ZeroMemory( Offsets );
            apemodevk::ZeroMemory( Counts );

            uint32_t layoutIndex   = 0;
            uint32_t descSetOffset = 0;
            VkDescriptorSetLayout currentLayout = hLayouts[ 0 ];

            uint32_t i = 1;
            for ( ; i < TCount; ++i ) {
                if ( currentLayout != hLayouts[ i ] ) {
                    Offsets[ layoutIndex ] = descSetOffset;
                    Counts[ layoutIndex ]  = i - descSetOffset;
                    descSetOffset          = i;
                    ++layoutIndex;
                }
            }

            Offsets[ layoutIndex ] = descSetOffset;
            Counts[ layoutIndex ]  = i - descSetOffset;
            descSetOffset          = i;

            //TODO: Decrement descriptor set counter for the descriptor pool.
            //TODO: Checks

            const ResultHandle ErrorHandle = vkAllocateDescriptorSets( hNode, AllocInfo, hSets );
            return ErrorHandle;
        }

        void BindTo( CommandBuffer& CmdBuffer, VkPipelineBindPoint InBindPoint, const uint32_t ( &DynamicOffsets )[ TCount ] ) {



            for ( uint32_t i = 0; i < TCount; ++i ) {
                auto l = hLayouts[ i ];
                auto s = ;
                auto o = DynamicOffsets[ i ];
                vkCmdBindDescriptorSets( CmdBuffer, InBindPoint, l, 0, 1, &hSets[ i ], 1, o );
            }
        }
    };

    class DescriptorSet : public apemodevk::ScalableAllocPolicy, public apemodevk::NoCopyAssignPolicy {
    public:
        DescriptorSet( );
        ~DescriptorSet( );

        bool RecreateResourcesFor( apemodevk::GraphicsDevice& GraphicsNode,
                                   apemodevk::DescriptorPool& DescPool,
                                   VkDescriptorSetLayout    DescSetLayout );

        void BindTo( apemodevk::CommandBuffer& CmdBuffer,
                     VkPipelineBindPoint     PipelineBindPoint,
                     uint32_t                DynamicOffsetCount,
                     const uint32_t*         DynamicOffsets );

    public:
        operator VkDescriptorSet() const;

    public:
        apemodevk::GraphicsDevice const*                  pNode;
        apemodevk::DescriptorPool const*                  pDescPool;
        apemodevk::TDispatchableHandle< VkDescriptorSet > hDescSet;
        VkDescriptorSetLayout                           hDescSetLayout;
    };

    class DescriptorSetUpdater : public apemodevk::ScalableAllocPolicy, public apemodevk::NoCopyAssignPolicy {
        bool SetGraphicsNode (apemodevk::GraphicsDevice const & GraphicsNode);

    public:
        DescriptorSetUpdater ();

        /** Resets graphics node (member pointer), reserves memory. */
        void Reset( uint32_t MaxWrites, uint32_t MaxCopies );

        /** DescSet must be initialized. */
        bool WriteUniformBuffer( apemodevk::DescriptorSet const& DescSet,
                                 VkBuffer                      Buffer,
                                 uint32_t                      Offset,
                                 uint32_t                      Range,
                                 uint32_t                      Binding,
                                 uint32_t                      Count );

        /** DescSet must be initialized. */
        bool WriteCombinedImgSampler( apemodevk::DescriptorSet const& DescSet,
                                      VkImageView                   ImgView,
                                      VkImageLayout                 ImgLayout,
                                      VkSampler                     Sampler,
                                      uint32_t                      Binding,
                                      uint32_t                      Count );

        void Flush( );

    public:
        apemodevk::GraphicsDevice const*        pNode;
        std::vector< VkDescriptorBufferInfo > BufferInfos;
        std::vector< VkDescriptorImageInfo >  ImgInfos;
        std::vector< VkWriteDescriptorSet >   Writes;
        std::vector< VkCopyDescriptorSet >    Copies;
    };

    class DescriptorPool : public apemodevk::ScalableAllocPolicy, public apemodevk::NoCopyAssignPolicy {
    public:
        DescriptorPool( );
        ~DescriptorPool( );

        bool RecreateResourcesFor( apemodevk::GraphicsDevice& GraphicsNode,
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
        apemodevk::GraphicsDevice const *                pNode;
        apemodevk::TDispatchableHandle<VkDescriptorPool> hDescPool;

        friend DescriptorSet;

        /** Helps to track descriptor suballocations. */
        VkDescriptorPoolSize DescPoolCounters[ VK_DESCRIPTOR_TYPE_RANGE_SIZE ];
        uint32_t             DescSetCounter;
    };

    struct DescriptorSetLayoutBinding {
        TInfoStruct< VkDescriptorSetLayoutBinding > Binding;

        void Clear( ) {
            Binding.InitializeStruct( );
        }

        void InitAsUniformBuffer( uint32_t Register, uint32_t Count, VkShaderStageFlagBits StageFlags, uint32_t Set = 0 ) {
            Binding->stageFlags = static_cast< VkShaderStageFlagBits >( StageFlags );
        }

        void InitAsUniformBufferDynamic( uint32_t Register, uint32_t Count, VkShaderStageFlagBits StageFlags, uint32_t Set = 0 );

        void InitAsSampler( uint32_t         Register,
                            uint32_t         Count,
                            VkShaderStageFlagBits StageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                            uint32_t         Set        = 0 );

        void InitAsCombinedImageSampler( uint32_t              Register,
                                         uint32_t              Count,
                                         VkSampler*            pImmutableSamplers,
                                         VkShaderStageFlagBits StageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                                         uint32_t              Set        = 0 );

    public:
        operator VkDescriptorSetLayoutBinding( ) const;
    };

    class DescriptorSetLayout {

    };
}