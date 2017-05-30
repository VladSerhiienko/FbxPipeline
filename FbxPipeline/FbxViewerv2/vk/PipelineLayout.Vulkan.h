#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <TInfoStruct.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>

namespace apemode
{
    class PipelineLayoutParameter;
    class PipelineLayout;
    class PipelineLayoutDescription;
    class PipelineLayout;
    class PipelineLayoutBuilder;

    class PipelineLayoutParameter : public ScalableAllocPolicy {
        friend PipelineLayout;
        friend PipelineLayoutBuilder;
        friend PipelineLayoutDescription;

    public:
        uint32_t DescSet;
        TInfoStruct< VkDescriptorSetLayoutBinding > Binding;

    public:
        PipelineLayoutParameter( );
        ~PipelineLayoutParameter( );

        void Clear( );

        void InitAsUniformBuffer( uint32_t Register, uint32_t Count, VkShaderStageFlagBits StageFlags, uint32_t Set = 0 );

        void InitAsUniformBufferDynamic( uint32_t Register, uint32_t Count, VkShaderStageFlagBits StageFlags, uint32_t Set = 0 );

        void InitAsSampler( uint32_t         Register,
                            uint32_t         Count,
                            VkShaderStageFlagBits StageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                            uint32_t         Set        = 0 );

        void InitAsCombinedImageSampler( uint32_t         Register,
                                         uint32_t         Count,
                                         VkSampler*       pImmutableSamplers,
                                         VkShaderStageFlagBits StageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                                         uint32_t         Set        = 0 );

    public:
        operator VkDescriptorSetLayoutBinding( ) const;
    };

    class PipelineLayout : public ScalableAllocPolicy, public NoCopyAssignPolicy {
    public:
        uint64_t                                Hash;
        GraphicsDevice const*                   pNode;
        PipelineLayoutDescription const*        pDesc;
        std::vector< VkDescriptorSetLayout >    DescSetLayouts;
        TDispatchableHandle< VkPipelineLayout > PipelineLayoutHandle;

    public:
        PipelineLayout( );
        ~PipelineLayout( );

    public:
        operator VkPipelineLayout( ) const;
    };

    class PipelineLayoutDescription : public ScalableAllocPolicy, public NoCopyAssignPolicy {
    public:
        uint64_t                               Hash;
        std::vector< PipelineLayoutParameter > Params;
        std::vector< VkPushConstantRange >     PushConstRanges;

    public:
        PipelineLayoutDescription( );
        ~PipelineLayoutDescription( );

        void                           Reset( );
        uint64_t                       UpdateHash( );
        uint32_t                       GetParamCount( ) const;
        PipelineLayoutParameter const& GetParameter( uint32_t ParameterIndex ) const;

    public:
        static PipelineLayoutDescription const* MakeNewFromTemporary( PipelineLayoutDescription const& TemporaryDesc );
    };

    class PipelineLayoutBuilder : public ScalableAllocPolicy, public NoCopyAssignPolicy {
        PipelineLayoutDescription TemporaryDesc;

    public:
        PipelineLayoutBuilder( );

        void                     Reset( uint32_t BindingCount, uint32_t PushConstRangeCount );
        PipelineLayoutParameter& AddParameter( );
        VkPushConstantRange&     AddPushConstRange( );
        PipelineLayout const*    RecreatePipelineLayout( GraphicsDevice& GraphicsNode );
    };

    class PipelineLayoutManager : public ScalableAllocPolicy, public NoCopyAssignPolicy {
        friend GraphicsDevice;
        friend PipelineLayoutBuilder;
        struct PipelineLayoutContent;
        struct PrivateContent;

        PrivateContent* pContent;

    public:
        PipelineLayoutManager( );
        ~PipelineLayoutManager( );

        VkDescriptorSetLayout GetDescSetLayout( uint64_t Hash );
        void                  SetDescSetLayout( uint64_t Hash, VkDescriptorSetLayout SetLayout );
        void                  AddNewPipelineLayoutObject( PipelineLayout& pRootSign );
        PipelineLayout const* TryGetPipelineLayoutObjectByHash( uint64_t Hash );
    };
}
