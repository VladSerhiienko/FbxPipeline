#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <TInfoStruct.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>

namespace Core
{
    class RootParameter;
    class RootSignature;
    class RootSignatureDescription;
    class RootSignature;
    class RootSignatureBuilder;

    enum ShaderVisibility
    {
        kShaderVisibility_Vertex                = VK_SHADER_STAGE_VERTEX_BIT,
        kShaderVisibility_Fragment              = VK_SHADER_STAGE_FRAGMENT_BIT,
        kShaderVisibility_TesselationControl    = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
        kShaderVisibility_TesselationEvaluation = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
        kShaderVisibility_Geometry              = VK_SHADER_STAGE_GEOMETRY_BIT,
        kShaderVisibility_Compute               = VK_SHADER_STAGE_COMPUTE_BIT,
        kShaderVisibility_AllGraphics           = VK_SHADER_STAGE_ALL_GRAPHICS,
        kShaderVisibility_All                   = VK_SHADER_STAGE_ALL,
    };

    class _Graphics_ecosystem_dll_api RootParameter : public Aux::ScalableAllocPolicy
    {
        friend RootSignature;
        friend RootSignatureBuilder;
        friend RootSignatureDescription;

    public:
        uint32_t                                  DescSet;
        Core::TInfoStruct<VkDescriptorSetLayoutBinding> Binding;

    public:
        RootParameter();
        ~RootParameter();

        void Clear();

        void InitAsUniformBuffer(uint32_t         Register,
                                 uint32_t         Count,
                                 ShaderVisibility StageFlags,
                                 uint32_t         Set = 0);

        void InitAsUniformBufferDynamic(uint32_t         Register,
                                        uint32_t         Count,
                                        ShaderVisibility StageFlags,
                                        uint32_t         Set = 0);

        void InitAsSampler(uint32_t         Register,
                           uint32_t         Count,
                           ShaderVisibility StageFlags = kShaderVisibility_Fragment,
                           uint32_t         Set        = 0);

        void InitAsCombinedImageSampler(uint32_t         Register,
                                        uint32_t         Count,
                                        VkSampler *      pImmutableSamplers,
                                        ShaderVisibility StageFlags = kShaderVisibility_Fragment,
                                        uint32_t         Set        = 0);

    public:
        operator VkDescriptorSetLayoutBinding() const;
    };

    class _Graphics_ecosystem_dll_api RootSignature : public Aux::ScalableAllocPolicy,
                                                      public Aux::NoCopyAssignPolicy
    {
    public:
        uint64_t                                                             Hash;
        Core::TDispatchableHandle<VkDescriptorSetLayout>::NativeHandleVector DescSetLayouts;
        Core::TDispatchableHandle<VkPipelineLayout>                          PipelineLayoutHandle;
        Core::GraphicsDevice const *                                         pGraphicsNode;
        Core::RootSignatureDescription const *                               pDesc;

    public:
        RootSignature();
        ~RootSignature();

    public:
        operator VkPipelineLayout() const;
    };

    class _Graphics_ecosystem_dll_api RootSignatureDescription : public Aux::ScalableAllocPolicy,
                                                                 public Aux::NoCopyAssignPolicy
    {
    public:
        uint64_t                                  Hash;
        std::vector<RootParameter>       Params;
        std::vector<VkPushConstantRange> PushConstRanges;

    public:
        RootSignatureDescription();
        ~RootSignatureDescription();

        void Reset();
        uint64_t UpdateHash();
        uint32_t GetParamCount()  const;
        Core::RootParameter const & GetParameter(uint32_t ParameterIndex) const;

    public:
        static Core::RootSignatureDescription const *
        MakeNewFromTemporary (Core::RootSignatureDescription const & TemporaryDesc);
    };

    class _Graphics_ecosystem_dll_api RootSignatureBuilder : public Aux::ScalableAllocPolicy,
                                                             public Aux::NoCopyAssignPolicy
    {
        Core::RootSignatureDescription TemporaryDesc;

    public:
        RootSignatureBuilder ();

        void Reset (uint32_t BindingCount, uint32_t PushConstRangeCount);
        Core::RootParameter & AddParameter ();
        VkPushConstantRange & AddPushConstRange ();

        Core::RootSignature const * RecreateRootSignature (Core::GraphicsDevice & GraphicsNode);
    };

    class _Graphics_ecosystem_dll_api RootSignatureManager : public Aux::ScalableAllocPolicy,
                                                             public Aux::NoCopyAssignPolicy
    {
        friend Core::GraphicsDevice;
        friend Core::RootSignatureBuilder;
        struct RootSignatureContent;
        struct PrivateContent;

        PrivateContent * pContent;

    public:
        RootSignatureManager();
        ~RootSignatureManager();

        VkDescriptorSetLayout GetDescSetLayout (uint64_t Hash);
        void SetDescSetLayout (uint64_t Hash, VkDescriptorSetLayout SetLayout);
        void AddNewRootSignatureObject (Core::RootSignature & pRootSign);
        Core::RootSignature const * TryGetRootSignatureObjectByHash (uint64_t Hash);
    };
}

_Game_engine_Define_enum_flag_operators(Core::ShaderVisibility);