#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <TInfoStruct.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>

namespace Core
{
    class CommandList;
    class RenderPass;
    class RootSignature;
    class PipelineState;
    class PipelineStateDescription;

    enum ShaderType
    {
        kShaderType_Vertex = 0,
        kShaderType_TesselationControl,
        kShaderType_TesselationEvaluation,
        kShaderType_Geometry,
        kShaderType_Fragment,
        kShaderType_Compute,
        kShaderType_Unknown,
    };

    class _Graphics_ecosystem_dll_api ShaderBytecode : public Aux::ScalableAllocPolicy,
                                                       public Aux::NoCopyAssignPolicy
    {
    public:
        uint64_t                      Hash;
        ShaderType                    eType;
        std::string                  Id;
        std::string                  MainFn;
        std::vector<uint8_t> Bytecode;

    public:
        ShaderBytecode ();
        ShaderBytecode (ShaderBytecode && Other);
        ShaderBytecode (ShaderBytecode const & Other);

        /**
         * Must end with 4 chars, that denote shader stage:
         * - 'vert' -> kShaderType_Vertex
         * - 'tesc' -> kShaderType_TesselationControl
         * - 'tese' -> kShaderType_TesselationEvaluation
         * - 'geom' -> kShaderType_Geometry
         * - 'frag' -> kShaderType_Fragment
         * - 'comp' -> kShaderType_Compute
         */
        void SetIdAndType (const char * pId);
        void SetMainFn (const char * pMainFn);
        void SetBytecode (const void * pBytecode, size_t BytecodeLength);

    public:
        Core::ShaderBytecode & operator= (ShaderBytecode && Other);
        Core::ShaderBytecode & operator= (ShaderBytecode const & Other);
    };

    class _Graphics_ecosystem_dll_api PipelineStateDescription : public Aux::ScalableAllocPolicy,
                                                                 public Aux::NoCopyAssignPolicy
    {
    public:
        uint64_t                 Hash;
        bool                     bIsGraphics;
        RenderPass const *       pRenderPass;
        RootSignature const *    pRootSignature;

        Core::TInfoStruct<VkPipelineVertexInputStateCreateInfo>   VertexInputState;
        Core::TInfoStruct<VkPipelineInputAssemblyStateCreateInfo> InputAssemblyState;
        std::vector<VkVertexInputBindingDescription>     InputBindings;
        std::vector<VkVertexInputAttributeDescription>   InputAttributes;

        VkSampleMask                                              SampleMask;
        Core::TInfoStruct<VkPipelineMultisampleStateCreateInfo>   MultisampleState;
        Core::TInfoStruct<VkPipelineRasterizationStateCreateInfo> RasterizationState;

        Core::TInfoStruct<VkPipelineViewportStateCreateInfo> ViewportState;
        std::vector<VkViewport>                     Viewports;
        std::vector<VkRect2D>                       ScissorRects;

        Core::TInfoStruct<VkPipelineDepthStencilStateCreateInfo> DepthStencilState;
        Core::TInfoStruct<VkPipelineColorBlendStateCreateInfo>   ColorBlendState;
        VkPipelineColorBlendAttachmentState                      ColorBlendAttachmentStates[8];

        Core::TInfoStruct<VkPipelineDynamicStateCreateInfo> DynamicState;
        VkDynamicState EnabledDynamicStates[VK_DYNAMIC_STATE_RANGE_SIZE];

        Core::TInfoStruct<VkPipelineShaderStageCreateInfo> ShaderStages[5];
        ShaderBytecode                                     ShaderBytecodes[5];
        VkSpecializationInfo                               ShaderSpecializations[5];

    public:
        TInfoStruct<VkGraphicsPipelineCreateInfo> Graphics;
        TInfoStruct<VkComputePipelineCreateInfo>  Compute;
        TInfoStruct<VkPipelineCacheCreateInfo>    Cache;

    public:
        void Reset();
        uint64_t UpdateHash();
        bool IsDynamicStateEnabled(VkDynamicState eDynamicState) const;
    };

    class _Graphics_ecosystem_dll_api PipelineState : public Aux::ScalableAllocPolicy,
                                                      public Aux::NoCopyAssignPolicy
    {
    public:
        uint64_t                                   Hash;
        VkPipelineBindPoint                        eBindPoint;
        Core::TDispatchableHandle<VkPipeline>      hPipeline;
        Core::TDispatchableHandle<VkPipelineCache> hPipelineCache;
        Core::PipelineStateDescription const *     pDesc;
        Core::RenderPass const *                   pRenderPass;
        Core::RootSignature const *                pRootSignature;
        Core::GraphicsDevice const *               pGraphicsNode;

    public:
        PipelineState();
        ~PipelineState();

        void BindTo(Core::CommandList & CmdList);

    public:
        operator VkPipeline() const;
        operator VkPipelineCache() const;
    };

    class _Graphics_ecosystem_dll_api PipelineStateBuilder : public Aux::ScalableAllocPolicy,
                                                             public Aux::NoCopyAssignPolicy
    {
    public:
        class InputLayoutBuilder;
        friend InputLayoutBuilder;

    public:
        class _Graphics_ecosystem_dll_api InputLayoutBuilder : public Aux::ScalableAllocPolicy,
                                                               public Aux::NoCopyAssignPolicy
        {
            PipelineStateBuilder &            Bilder;
            VkVertexInputBindingDescription * pBinding;
            uint32_t                          AttributeLocation;
            uint32_t                          AttributeOffset;
            uint32_t                          AttributeCount;

        public:
            InputLayoutBuilder (PipelineStateBuilder & Bilder);

            InputLayoutBuilder & Reserve (uint32_t InputElementCount,
                                          uint32_t TotalInputAttributeCount);

            /** Sets current input element. */
            InputLayoutBuilder & AddInputElement (uint32_t          VertexStride,
                                                  VkVertexInputRate InputRate,
                                                  uint32_t          InputAttributeCount);

            /** Sets current input element. */
            template <typename TVertex>
            InputLayoutBuilder & AddInputElement (VkVertexInputRate InputRate,
                                                  uint32_t          InputAttributeCount)
            {
                static const uint32_t VertexStride = static_cast<uint32_t> (sizeof (TVertex));
                return AddInputElement (VertexStride, InputRate, InputAttributeCount);
            }

            /** Adds attribute to the current input element. */
            InputLayoutBuilder & AddAttribute (VkFormat Fmt);

            void ShrinkToFit ();
        };

    public:
        // Does Reset()
        PipelineStateBuilder ();
        ~PipelineStateBuilder ();

        void Reset();

        Core::PipelineStateDescription & GetDesc();

        //
        // DynamicState
        //

        void EnableDynamicState (VkDynamicState eDynamicState);
        void DisableDynamicState (VkDynamicState eDynamicState);

        //
        // RasterizationState
        // DynamicState
        //

        void SetLineWidth (float LineWidth);
        void ResetLineWidth ();

        void SetBlendContants (float BlendContantR,
                               float BlendContantG,
                               float BlendContantB,
                               float BlendContantA);
        void ResetBlendContants ();

        void SetDepthBias (float DepthBiasConstantFactor,
                           float DepthBiasClamp,
                           float DepthBiasSlopeFactor);
        void ResetDepthBias ();

        //
        // DepthStencilState
        // DynamicState
        //

        void SetDepthBounds (float MinDepthBounds,
                             float MaxDepthBounds,
                             bool  bEnableDepthBoundsTest = true);
        void ResetDepthBounds ();

        void SetStencilCompareMask (uint32_t FrontFaceStencilCompareMask,
                                    uint32_t BackFaceStencilCompareMask);
        void ResetStencilCompareMask ();

        void SetStencilWriteMask (uint32_t FrontFaceStencilWriteMask,
                                  uint32_t BackFaceStencilWriteMask);
        void ResetStencilWriteMask ();

        void SetStencilReference (uint32_t FrontFaceStencilReference,
                                  uint32_t BackFaceStencilReference);
        void ResetStencilReference ();

        //
        // ViewportState
        // DynamicState
        //

        void SetViewports (VkViewport const * pViewports, size_t ViewportCount);
        void ResetViewports ();

        void SetScissorRects (VkRect2D const * pScissorRects, size_t ScissorRectCount);
        void ResetScissorRects ();

        void SetRenderPass (Core::RenderPass const & RenderPass, uint32_t SubpassId);
        void SetRootSignature (Core::RootSignature const & RootSignature);

        //
        // Stages
        //

        void SetShader (const char * pId,
                        const char * pMainFn,
                        const void * pBytecode,
                        size_t       BytecodeLength);

        //
        // VertexInputState
        //

        InputLayoutBuilder & EditInputLayout (uint32_t InputElementCount,
                                              uint32_t TotalInputAttributeCount);

        void ClearInputLayout (uint32_t InputElementCount, uint32_t TotalInputAttributeCount);

        void AddInputElement (uint32_t                                  VertexStride,
                              VkVertexInputRate                         InputRate,
                              VkVertexInputAttributeDescription const * InputAttributes,
                              uint32_t                                  InputAttributeCount);

        template <typename TVertexInputElement>
        void AddInputElement (VkVertexInputRate                         InputRate,
                              VkVertexInputAttributeDescription const * InputAttributes,
                              uint32_t                                  InputAttributeCount)
        {
            AddInputElement (static_cast<uint32_t> (sizeof (TVertexInputElement)),
                             InputRate,
                             InputAttributes,
                             InputAttributeCount);
        }

        //
        // InputAssemblyState
        //

        void SetPrimitiveTopology (VkPrimitiveTopology Topology, bool bEnableRestart);

    public:
        Core::PipelineState const * RecreatePipelineState (Core::GraphicsDevice & GraphicsNode);

    private:
        InputLayoutBuilder       InputEditor;
        PipelineStateDescription TemporaryDesc;
    };

    class _Graphics_ecosystem_dll_api PipelineStateManager : public Aux::ScalableAllocPolicy,
                                                             public Aux::NoCopyAssignPolicy
    {
        friend Core::GraphicsDevice;
        friend Core::PipelineStateBuilder;
        struct PipelineStateContent;
        struct PrivateContent;

        PrivateContent * pContent;

    public:
        PipelineStateManager ();
        ~PipelineStateManager ();

        void AddNewPipelineStateObject (Core::PipelineState & PipelineState);
        Core::PipelineState const * TryGetPipelineStateObjectByHash (uint64_t Hash);
    };
}