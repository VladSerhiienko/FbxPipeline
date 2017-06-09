//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <PipelineState.Vulkan.h>

#include <RenderPass.Vulkan.h>
#include <CommandQueue.Vulkan.h>
#include <PipelineLayout.Vulkan.h>
#include <Resource.Vulkan.h>

#include <CityHash.h>

/// -------------------------------------------------------------------------------------------------------------------
/// PipelineStateDescription
/// -------------------------------------------------------------------------------------------------------------------

void apemodevk::PipelineStateDescription::Reset()
{
    DynamicState.InitializeStruct( );
    InputAssemblyState.InitializeStruct( );
    RasterizationState.InitializeStruct( );
    ColorBlendState.InitializeStruct( );
    DepthStencilState.InitializeStruct( );
    MultisampleState.InitializeStruct( );
    VertexInputState.InitializeStruct( );
    ViewportState.InitializeStruct( );
    Cache.InitializeStruct( );

    for (auto & ShaderStage : ShaderStages)
        ShaderStage.InitializeStruct( );

    for (auto & DynamicState : EnabledDynamicStates)
        DynamicState = VK_DYNAMIC_STATE_MAX_ENUM;

    SampleMask = 0;

    for (auto & ShaderSpecialization : ShaderSpecializations)
        apemodevk::ZeroMemory(ShaderSpecialization);

    for (auto & ColorBlendAttachmentState : ColorBlendAttachmentStates)
        apemodevk::ZeroMemory(ColorBlendAttachmentState);

    Compute.InitializeStruct( );
    Graphics.InitializeStruct( );

    Hash           = 0;
    bIsGraphics    = true;
    pRenderPass    = nullptr;
    pPipelineLayout = nullptr;

    DynamicState->pDynamicStates = EnabledDynamicStates;

    InputAssemblyState->primitiveRestartEnable = false;
    InputAssemblyState->topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    InputAssemblyState->flags                  = 0;

    RasterizationState->polygonMode             = VK_POLYGON_MODE_FILL;
    RasterizationState->cullMode                = VK_CULL_MODE_BACK_BIT;
    RasterizationState->frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    RasterizationState->depthClampEnable        = VK_FALSE;
    RasterizationState->rasterizerDiscardEnable = VK_FALSE;
    RasterizationState->depthBiasEnable         = VK_FALSE;

    ColorBlendState->attachmentCount = 1;
    ColorBlendState->pAttachments    = ColorBlendAttachmentStates;

    for (auto & ColorBlendAttachmentState : ColorBlendAttachmentStates)
    {
        ColorBlendAttachmentState.colorWriteMask = 0xf;
        ColorBlendAttachmentState.blendEnable    = false;
    }

    DepthStencilState->depthTestEnable       = VK_TRUE;
    DepthStencilState->depthWriteEnable      = VK_TRUE;
    DepthStencilState->depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;
    DepthStencilState->depthBoundsTestEnable = VK_FALSE;
    DepthStencilState->stencilTestEnable     = VK_FALSE;
    DepthStencilState->back.failOp           = VK_STENCIL_OP_KEEP;
    DepthStencilState->back.passOp           = VK_STENCIL_OP_KEEP;
    DepthStencilState->back.compareOp        = VK_COMPARE_OP_ALWAYS;
    DepthStencilState->front                 = DepthStencilState->back;

    SampleMask                             = 0xfffffff;
    MultisampleState->pSampleMask          = &SampleMask;
    MultisampleState->rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    Graphics->pStages = reinterpret_cast<VkPipelineShaderStageCreateInfo *>(ShaderStages);
}

bool apemodevk::PipelineStateDescription::IsDynamicStateEnabled(VkDynamicState eDynamicState) const
{
    for (uint32_t Idx = 0; Idx < DynamicState->dynamicStateCount; Idx++)
        if (EnabledDynamicStates[Idx] == eDynamicState)
            return true;

    return false;
}

uint64_t apemodevk::PipelineStateDescription::UpdateHash()
{
    apemode::CityHash64Wrapper HashBuilder;

    if (bIsGraphics)
    {
        //
        // VertexInputState
        //

        HashBuilder.CombineWithArray(InputBindings.data(), InputBindings.size());
        HashBuilder.CombineWithArray(InputAttributes.data(), InputAttributes.size());

        //
        // InputAssemblyState
        //

        HashBuilder.CombineWith(InputAssemblyState);

        //
        // MultisampleState
        //

        {
            VkPipelineMultisampleStateCreateInfo MultisampleStateNoSampleMask = MultisampleState;
            MultisampleStateNoSampleMask.pSampleMask = nullptr;
            HashBuilder.CombineWith(MultisampleStateNoSampleMask);
        }

        if (MultisampleState->pSampleMask)
            HashBuilder.CombineWith(SampleMask);

        //
        // RasterizationState
        //

        HashBuilder.CombineWith(RasterizationState);

        //
        // ViewportState
        //

        if (ViewportState->viewportCount)
        {
            HashBuilder.CombineWith(ViewportState->viewportCount);
            if (!IsDynamicStateEnabled(VK_DYNAMIC_STATE_VIEWPORT))
            {
                HashBuilder.CombineWithArray(Viewports.data(), ViewportState->viewportCount);
            }
        }

        if (ViewportState->scissorCount)
        {
            HashBuilder.CombineWith(ViewportState->scissorCount);
            if (!IsDynamicStateEnabled(VK_DYNAMIC_STATE_SCISSOR))
            {
                HashBuilder.CombineWithArray(ScissorRects.data(), ViewportState->scissorCount);
            }
        }

        //
        // DepthStencilState
        //

        HashBuilder.CombineWith(DepthStencilState);

        //
        // ColorBlendState
        //

        {
            VkPipelineColorBlendStateCreateInfo ColorBlendStateNoAttachments = ColorBlendState;
            ColorBlendStateNoAttachments.pAttachments                        = nullptr;
            HashBuilder.CombineWith (ColorBlendStateNoAttachments);
        }

        HashBuilder.CombineWithArray (ColorBlendAttachmentStates, ColorBlendState->attachmentCount);

        //
        // DynamicState
        //

        if (DynamicState->dynamicStateCount)
            HashBuilder.CombineWithArray (EnabledDynamicStates, DynamicState->dynamicStateCount);

        //
        // Stages
        //

        for (auto const & ShaderBytecode : ShaderBytecodes)
            if (!ShaderBytecode.Id.empty () && ShaderBytecode.Hash)
            {
                switch (ShaderBytecode.eType)
                {
                case kShaderType_Vertex:
                case kShaderType_TesselationControl:
                case kShaderType_TesselationEvaluation:
                case kShaderType_Geometry:
                case kShaderType_Fragment:
                    HashBuilder.CombineWith (ShaderBytecode.Hash);
                    break;
                default:
                    break;
                }
            }

        //TODO Specialization (no documentation)

        //
        // Graphics
        //

        HashBuilder.CombineWith (Graphics->renderPass);
        HashBuilder.CombineWith (Graphics->subpass);
        HashBuilder.CombineWith (Graphics->layout);
        HashBuilder.CombineWith (Graphics->flags);
    }
    else
    {
        //
        // Compute
        //

        bool bFoundComputeShader = false;
        for (auto const & ShaderBytecode : ShaderBytecodes)
            if (!ShaderBytecode.Id.empty () && ShaderBytecode.eType == kShaderType_Compute)
            {
                HashBuilder.CombineWith (ShaderBytecode.Hash);
                bFoundComputeShader = true;
                break;
            }

        apemode_assert (bFoundComputeShader, "No compute shader.");
        HashBuilder.CombineWith (Compute->layout);
        HashBuilder.CombineWith (Compute->flags);
    }

    Hash = HashBuilder.Value;
    return HashBuilder.Value;
}

/// -------------------------------------------------------------------------------------------------------------------
/// PipelineState
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::PipelineState::PipelineState ()
    : pRenderPass (nullptr)
    , pPipelineLayout (nullptr)
    , pNode (nullptr)
    , pDesc (nullptr)
{
}

apemodevk::PipelineState::~PipelineState ()
{
}

void apemodevk::PipelineState::BindTo (apemodevk::CommandBuffer & CmdBuffer)
{
    vkCmdBindPipeline (CmdBuffer, eBindPoint, hPipeline);
}

apemodevk::PipelineState::operator VkPipeline () const
{
    return hPipeline;
}

apemodevk::PipelineState::operator VkPipelineCache () const
{
    return hPipelineCache;
}

/// -------------------------------------------------------------------------------------------------------------------
/// PipelineStateBuilder
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::PipelineStateBuilder::PipelineStateBuilder()
    : InputEditor(*this)
{
    Reset();
}

apemodevk::PipelineStateBuilder::~PipelineStateBuilder()
{
}

void apemodevk::PipelineStateBuilder::Reset()
{
    TemporaryDesc.Reset();
}

void apemodevk::PipelineStateBuilder::SetRenderPass (apemodevk::RenderPass const & RenderPass,
                                                uint32_t                 SubpassId)
{
    TemporaryDesc.pRenderPass          = &RenderPass;

    TemporaryDesc.Graphics->subpass    = SubpassId;
    TemporaryDesc.Graphics->renderPass = RenderPass;
}

void apemodevk::PipelineStateBuilder::SetPipelineLayout (apemodevk::PipelineLayout const & PipelineLayout)
{
    TemporaryDesc.pPipelineLayout = &PipelineLayout;

    TemporaryDesc.Graphics->layout = PipelineLayout;
}

apemodevk::PipelineStateBuilder::InputLayoutBuilder &
apemodevk::PipelineStateBuilder::EditInputLayout(uint32_t InputElementCount,
                                            uint32_t TotalInputAttributeCount)
{
    InputEditor.Reserve(InputElementCount, TotalInputAttributeCount);
    return InputEditor;
}

void apemodevk::PipelineStateBuilder::ClearInputLayout (uint32_t InputElementCount,
                                                   uint32_t TotalInputAttributeCount)
{
    TemporaryDesc.InputBindings.clear ();
    TemporaryDesc.InputAttributes.clear ();
    TemporaryDesc.InputBindings.reserve (InputElementCount);
    TemporaryDesc.InputAttributes.reserve (TotalInputAttributeCount);
}

void apemodevk::PipelineStateBuilder::AddInputElement (
    uint32_t                                  VertexStride,
    VkVertexInputRate                         InputRate,
    VkVertexInputAttributeDescription const * InputAttributes,
    uint32_t                                  InputAttributeCount)
{
    const uint32_t BindingIdx = _Get_collection_length_u (TemporaryDesc.InputBindings);
    auto & Binding = apemodevk::PushBackAndGet(TemporaryDesc.InputBindings);

    Binding           = TInfoStruct<VkVertexInputBindingDescription> ();
    Binding.stride    = VertexStride;
    Binding.inputRate = InputRate;
    Binding.binding   = BindingIdx;

    for (uint32_t AttrIdx = 0; AttrIdx < InputAttributeCount; ++AttrIdx)
    {
        auto & InputAttribute   = apemodevk::PushBackAndGet(TemporaryDesc.InputAttributes);
        InputAttribute.format   = InputAttributes[ AttrIdx ].format;
        InputAttribute.location = InputAttributes[ AttrIdx ].location;
        InputAttribute.offset   = InputAttributes[ AttrIdx ].offset;
        InputAttribute.binding  = BindingIdx;
    }

    apemodevk::AliasStructs (TemporaryDesc.InputBindings,
                       TemporaryDesc.VertexInputState->pVertexBindingDescriptions,
                       TemporaryDesc.VertexInputState->vertexBindingDescriptionCount);

    apemodevk::AliasStructs (TemporaryDesc.InputAttributes,
                       TemporaryDesc.VertexInputState->pVertexAttributeDescriptions,
                       TemporaryDesc.VertexInputState->vertexAttributeDescriptionCount);
}

apemodevk::PipelineStateDescription & apemodevk::PipelineStateBuilder::GetDesc ()
{
    return TemporaryDesc;
}

void apemodevk::PipelineStateBuilder::SetBlendContants (float r, float g, float b, float a)
{
    TemporaryDesc.ColorBlendState->blendConstants[ 0 ] = r;
    TemporaryDesc.ColorBlendState->blendConstants[ 1 ] = g;
    TemporaryDesc.ColorBlendState->blendConstants[ 2 ] = b;
    TemporaryDesc.ColorBlendState->blendConstants[ 3 ] = a;

    DisableDynamicState (VK_DYNAMIC_STATE_BLEND_CONSTANTS);
}

void apemodevk::PipelineStateBuilder::ResetBlendContants ()
{
    EnableDynamicState (VK_DYNAMIC_STATE_BLEND_CONSTANTS);
}

void apemodevk::PipelineStateBuilder::SetLineWidth (float LineWidth)
{
    TemporaryDesc.RasterizationState->lineWidth = LineWidth;
    DisableDynamicState (VK_DYNAMIC_STATE_LINE_WIDTH);
}

void apemodevk::PipelineStateBuilder::ResetLineWidth ()
{
    EnableDynamicState (VK_DYNAMIC_STATE_LINE_WIDTH);
}

void apemodevk::PipelineStateBuilder::SetDepthBias (float DepthBiasConstantFactor,
                                               float DepthBiasClamp,
                                               float DepthBiasSlopeFactor)
{
    TemporaryDesc.RasterizationState->depthBiasClamp          = DepthBiasClamp;
    TemporaryDesc.RasterizationState->depthBiasSlopeFactor    = DepthBiasSlopeFactor;
    TemporaryDesc.RasterizationState->depthBiasConstantFactor = DepthBiasConstantFactor;

    DisableDynamicState(VK_DYNAMIC_STATE_DEPTH_BIAS);
}

void apemodevk::PipelineStateBuilder::ResetDepthBias ()
{
    EnableDynamicState (VK_DYNAMIC_STATE_DEPTH_BIAS);
}

void apemodevk::PipelineStateBuilder::SetDepthBounds (float MinDepthBounds,
                                                 float MaxDepthBounds,
                                                 bool  bEnableDepthBoundsTest)
{
    TemporaryDesc.DepthStencilState->minDepthBounds        = MinDepthBounds;
    TemporaryDesc.DepthStencilState->maxDepthBounds        = MaxDepthBounds;
    TemporaryDesc.DepthStencilState->depthBoundsTestEnable = bEnableDepthBoundsTest;

    DisableDynamicState (VK_DYNAMIC_STATE_DEPTH_BOUNDS);
}

void apemodevk::PipelineStateBuilder::ResetDepthBounds ()
{
    EnableDynamicState (VK_DYNAMIC_STATE_DEPTH_BOUNDS);
}

void apemodevk::PipelineStateBuilder::SetStencilCompareMask (uint32_t FrontFaceStencilCompareMask,
                                                        uint32_t BackFaceStencilCompareMask)
{
    TemporaryDesc.DepthStencilState->front.compareMask = FrontFaceStencilCompareMask;
    TemporaryDesc.DepthStencilState->back.compareMask  = BackFaceStencilCompareMask;

    DisableDynamicState (VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK);
}

void apemodevk::PipelineStateBuilder::ResetStencilCompareMask ()
{
    EnableDynamicState (VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK);
}

void apemodevk::PipelineStateBuilder::SetStencilWriteMask (uint32_t FrontFaceStencilWriteMask,
                                                      uint32_t BackFaceStencilWriteMask)
{
    TemporaryDesc.DepthStencilState->front.writeMask = FrontFaceStencilWriteMask;
    TemporaryDesc.DepthStencilState->back.writeMask  = BackFaceStencilWriteMask;

    DisableDynamicState (VK_DYNAMIC_STATE_STENCIL_WRITE_MASK);
}

void apemodevk::PipelineStateBuilder::ResetStencilWriteMask()
{
    EnableDynamicState (VK_DYNAMIC_STATE_STENCIL_WRITE_MASK);
}

void apemodevk::PipelineStateBuilder::SetStencilReference (uint32_t FrontFaceStencilReference,
                                                      uint32_t BackFaceStencilReference)
{
    TemporaryDesc.DepthStencilState->front.reference = FrontFaceStencilReference;
    TemporaryDesc.DepthStencilState->back.reference  = BackFaceStencilReference;

    DisableDynamicState (VK_DYNAMIC_STATE_STENCIL_REFERENCE);
}

void apemodevk::PipelineStateBuilder::ResetStencilReference()
{
    EnableDynamicState (VK_DYNAMIC_STATE_STENCIL_REFERENCE);
}

void apemodevk::PipelineStateBuilder::SetViewports(VkViewport const * pViewports, size_t ViewportCount)
{
    TemporaryDesc.ViewportState->viewportCount = static_cast<uint32_t>(ViewportCount);

    if (apemode_likely (pViewports != nullptr && ViewportCount != 0))
    {
        TemporaryDesc.Viewports.reserve (ViewportCount);
        std::copy (pViewports, pViewports + ViewportCount,
                     std::back_inserter(TemporaryDesc.Viewports));
        TemporaryDesc.ViewportState->pViewports = TemporaryDesc.Viewports.data();

        DisableDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
    }
    else //if (ViewportCount)
    {
        EnableDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
    }
}

void apemodevk::PipelineStateBuilder::ResetViewports()
{
    SetViewports (nullptr, 0);
}

void apemodevk::PipelineStateBuilder::SetScissorRects(VkRect2D const * pRects, size_t RectCount)
{
    TemporaryDesc.ViewportState->scissorCount = static_cast<uint32_t>(RectCount);

    if (apemode_likely (pRects != nullptr && RectCount != 0))
    {
        TemporaryDesc.ScissorRects.reserve (RectCount);
        std::copy (pRects, pRects + RectCount, std::back_inserter (TemporaryDesc.ScissorRects));
        TemporaryDesc.ViewportState->pViewports = TemporaryDesc.Viewports.data ();

        DisableDynamicState (VK_DYNAMIC_STATE_SCISSOR);
    }
    else //if (RectCount)
    {
        EnableDynamicState (VK_DYNAMIC_STATE_SCISSOR);
    }
}

void apemodevk::PipelineStateBuilder::ResetScissorRects()
{
    SetScissorRects (nullptr, 0);
}

void apemodevk::PipelineStateBuilder::EnableDynamicState(VkDynamicState eDynamicState)
{
    if (TemporaryDesc.IsDynamicStateEnabled (eDynamicState))
        return;

    const uint32_t Counter = TemporaryDesc.DynamicState->dynamicStateCount++;
    TemporaryDesc.EnabledDynamicStates[ Counter ] = eDynamicState;
}

void apemodevk::PipelineStateBuilder::DisableDynamicState (VkDynamicState eDynamicState)
{
    uint32_t       DynamicStateIdx   = 0;
    uint32_t const DynamicStateCount = TemporaryDesc.DynamicState->dynamicStateCount;

    for (; DynamicStateIdx < DynamicStateCount; DynamicStateIdx++)
        if (TemporaryDesc.EnabledDynamicStates[ DynamicStateIdx ] == eDynamicState)
        {
            if (const size_t ShiftCount = DynamicStateCount - DynamicStateIdx - 1)
            {
                std::memcpy (&TemporaryDesc.EnabledDynamicStates[ DynamicStateIdx ],
                             &TemporaryDesc.EnabledDynamicStates[ DynamicStateIdx + 1 ],
                             ShiftCount * sizeof (VkDynamicState));
            }

            --TemporaryDesc.DynamicState->dynamicStateCount;
            return;
        }
}

void apemodevk::PipelineStateBuilder::SetShader (const char * pId,
                                            const char * pMainFn,
                                            const void * pBytecode,
                                            size_t       BytecodeLength)
{
    apemodevk::ShaderBytecode ShaderBytecode;
    ShaderBytecode.SetIdAndType (pId);
    ShaderBytecode.SetMainFn (pMainFn);
    ShaderBytecode.SetBytecode (pBytecode, BytecodeLength);

    TemporaryDesc.ShaderBytecodes[ ShaderBytecode.eType ] = std::move (ShaderBytecode);

    auto & ShaderStage = TemporaryDesc.ShaderStages[ ShaderBytecode.eType ];
    ShaderStage->pName = ShaderBytecode.MainFn.c_str ();
    ShaderStage->stage = (VkShaderStageFlagBits) (ShaderBytecode.eType);

    ++TemporaryDesc.Graphics->stageCount;
}

/// -------------------------------------------------------------------------------------------------------------------
/// PipelineStateBuilder InputLayoutBuilder
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::PipelineStateBuilder::InputLayoutBuilder::InputLayoutBuilder (PipelineStateBuilder & Bilder)
    : Bilder (Bilder)
    , pBinding (nullptr)
    , AttributeLocation (0)
    , AttributeCount (0)
    , AttributeOffset (0)
{
}

apemodevk::PipelineStateBuilder::InputLayoutBuilder &
apemodevk::PipelineStateBuilder::InputLayoutBuilder::Reserve (uint32_t InputElementCount,
                                                         uint32_t TotalInputAttributeCount)
{
    Bilder.ClearInputLayout (InputElementCount, TotalInputAttributeCount);
    return *this;
}

apemodevk::PipelineStateBuilder::InputLayoutBuilder &
apemodevk::PipelineStateBuilder::InputLayoutBuilder::AddInputElement (uint32_t          Stride,
                                                                 VkVertexInputRate InputRate,
                                                                 uint32_t          Count)
{
    const auto BindingIdx = _Get_collection_length_u (Bilder.TemporaryDesc.InputBindings);

    pBinding  = &apemodevk::PushBackAndGet(Bilder.TemporaryDesc.InputBindings);
    *pBinding = TInfoStruct<VkVertexInputBindingDescription> ();

    pBinding->inputRate = InputRate;
    pBinding->binding   = BindingIdx;
    pBinding->stride    = Stride;

    AttributeCount    = Count;
    AttributeOffset   = 0;
    AttributeLocation = 0;

    return *this;
}

apemodevk::PipelineStateBuilder::InputLayoutBuilder &
apemodevk::PipelineStateBuilder::InputLayoutBuilder::AddAttribute (VkFormat Fmt)
{
    auto & Attribute   = apemodevk::PushBackAndGet(Bilder.TemporaryDesc.InputAttributes);
    Attribute.binding  = pBinding->binding;
    Attribute.location = AttributeLocation++;
    Attribute.offset   = AttributeOffset;
    Attribute.format   = Fmt;
    apemode_assert (AttributeLocation < AttributeCount,
                         "Too many attributes for the binding.");

    const auto ElementByteWidth = apemodevk::ResourceReference::GetElementSizeInBytes (Fmt);
    apemode_assert (ElementByteWidth != 0, "Unsupported attribute type.");

    AttributeOffset += ElementByteWidth;
    apemode_assert (AttributeOffset <= pBinding->stride, "Attribute is too large.");

    return *this;
}

void apemodevk::PipelineStateBuilder::InputLayoutBuilder::ShrinkToFit ()
{
    Bilder.TemporaryDesc.InputBindings.shrink_to_fit ();
    Bilder.TemporaryDesc.InputAttributes.shrink_to_fit ();
}

void apemodevk::PipelineStateBuilder::SetPrimitiveTopology (VkPrimitiveTopology Topology,
                                                       bool                bEnableRestart)
{
    TemporaryDesc.InputAssemblyState->topology               = Topology;
    TemporaryDesc.InputAssemblyState->primitiveRestartEnable = bEnableRestart;
}

apemodevk::PipelineState const *
apemodevk::PipelineStateBuilder::RecreatePipelineState (apemodevk::GraphicsDevice & GraphicsNode)
{
    if (apemode_likely (GraphicsNode.IsValid ()))
    {
        auto   Hash    = TemporaryDesc.UpdateHash ();
        auto & Manager = GraphicsNode.GetDefaultPipelineStateManager ();

        if (auto pStoredPipelineState = Manager.TryGetPipelineStateObjectByHash (Hash))
            return pStoredPipelineState;
    }

    return nullptr;
}

/// -------------------------------------------------------------------------------------------------------------------
/// PipelineStateManager PrivateContent
/// -------------------------------------------------------------------------------------------------------------------

struct apemodevk::PipelineStateManager::PrivateContent : public apemodevk::ScalableAllocPolicy,
                                                    public apemodevk::NoCopyAssignPolicy
{
    using PipelineStateHashMap = std::map<apemode::CityHash64Wrapper::ValueType,
                                                   std::unique_ptr<PipelineState>,
                                                   apemode::CityHash64Wrapper::CmpOpLess>;

    std::mutex            Lock;
    PipelineStateHashMap StoredPipelineStates;
};

/// -------------------------------------------------------------------------------------------------------------------
/// PipelineStateManager
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::PipelineStateManager::PipelineStateManager ()
    : pContent (new PrivateContent ())
{
}

apemodevk::PipelineStateManager::~PipelineStateManager ()
{
    apemodevk::TSafeDeleteObj (pContent);
}

void apemodevk::PipelineStateManager::AddNewPipelineStateObject (apemodevk::PipelineState & PipelineState)
{
    std::lock_guard<std::mutex> LockGuard (pContent->Lock);

    apemode_assert (pContent->StoredPipelineStates.find (PipelineState.Hash)
                             == pContent->StoredPipelineStates.end (),
                         "See TryGetPipelineStateObjectByHash(...).");

    pContent->StoredPipelineStates[ PipelineState.Hash ].reset (&PipelineState);
}

apemodevk::PipelineState const *
apemodevk::PipelineStateManager::TryGetPipelineStateObjectByHash (uint64_t Hash)
{
    std::lock_guard<std::mutex> LockGuard (pContent->Lock);

    auto RenderPassCotentIt = pContent->StoredPipelineStates.find (Hash);
    if (RenderPassCotentIt != pContent->StoredPipelineStates.end ())
    {
        return (*RenderPassCotentIt).second.get ();
    }

    return nullptr;
}

/// -------------------------------------------------------------------------------------------------------------------
/// ShaderBytecode
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::ShaderType GetShaderType (const char * pId)
{
    const auto Length = std::strlen (pId);
    auto       pType  = pId + Length - 4;

    if (!std::strcmp (pType, "vert"))
        return apemodevk::kShaderType_Vertex;
    if (!std::strcmp (pType, "frag"))
        return apemodevk::kShaderType_Fragment;
    if (!std::strcmp (pType, "geom"))
        return apemodevk::kShaderType_Geometry;
    if (!std::strcmp (pType, "tesc"))
        return apemodevk::kShaderType_TesselationControl;
    if (!std::strcmp (pType, "tese"))
        return apemodevk::kShaderType_TesselationEvaluation;
    if (!std::strcmp (pType, "comp"))
        return apemodevk::kShaderType_Compute;

    return apemodevk::kShaderType_Unknown;
}

apemodevk::ShaderBytecode::ShaderBytecode () : eType (kShaderType_Unknown), Hash (0)
{

}

apemodevk::ShaderBytecode::ShaderBytecode (ShaderBytecode const & Other)
    : Hash (Other.Hash)
    , Id (Other.Id)
    , eType (Other.eType)
    , Bytecode (Other.Bytecode)
    , MainFn (Other.MainFn)
{
}

apemodevk::ShaderBytecode::ShaderBytecode (ShaderBytecode && Other)
    : Hash (Other.Hash)
    , Id (std::move (Other.Id))
    , eType (Other.eType)
    , Bytecode (std::move (Other.Bytecode))
    , MainFn (std::move (Other.MainFn))
{
}

apemodevk::ShaderBytecode & apemodevk::ShaderBytecode::operator= (apemodevk::ShaderBytecode const & Other)
{
    Hash     = Other.Hash;
    eType    = Other.eType;
    Id       = Other.Id;
    MainFn   = Other.MainFn;
    Bytecode = Other.Bytecode;

    return *this;
}

apemodevk::ShaderBytecode & apemodevk::ShaderBytecode::operator= (apemodevk::ShaderBytecode && Other)
{
    Hash     = Other.Hash;
    eType    = Other.eType;
    Id       = std::move (Other.Id);
    MainFn   = std::move (Other.MainFn);
    Bytecode = std::move (Other.Bytecode);

    return *this;
}

void apemodevk::ShaderBytecode::SetIdAndType (const char * pId)
{
    Id    = pId;
    eType = GetShaderType (pId);
}

void apemodevk::ShaderBytecode::SetMainFn (const char * pMainFn)
{
    MainFn = pMainFn;
}

void apemodevk::ShaderBytecode::SetBytecode (const void * pCode, size_t CodeSize)
{
    Bytecode.resize (CodeSize);
    std::memcpy (Bytecode.data (), pCode, CodeSize);

    Hash = apemode::CityHash64 (reinterpret_cast<const char *> (pCode), CodeSize);
}
