//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <PipelineState.Vulkan.h>

#include <RenderPass.Vulkan.h>
#include <CommandQueue.Vulkan.h>
#include <RootSignature.Vulkan.h>
#include <Resource.Vulkan.h>

#include <CityHash.h>

/// -------------------------------------------------------------------------------------------------------------------
/// PipelineStateDescription
/// -------------------------------------------------------------------------------------------------------------------

void Core::PipelineStateDescription::Reset()
{
    DynamicState.ZeroMemory();
    InputAssemblyState.ZeroMemory();
    RasterizationState.ZeroMemory();
    ColorBlendState.ZeroMemory();
    DepthStencilState.ZeroMemory();
    MultisampleState.ZeroMemory();
    VertexInputState.ZeroMemory();
    ViewportState.ZeroMemory();
    Cache.ZeroMemory();

    for (auto & ShaderStage : ShaderStages)
        ShaderStage.ZeroMemory();

    for (auto & DynamicState : EnabledDynamicStates)
        DynamicState = VK_DYNAMIC_STATE_MAX_ENUM;

    SampleMask = 0;

    for (auto & ShaderSpecialization : ShaderSpecializations)
        Aux::ZeroMemory(ShaderSpecialization);

    for (auto & ColorBlendAttachmentState : ColorBlendAttachmentStates)
        Aux::ZeroMemory(ColorBlendAttachmentState);

    Compute.ZeroMemory();
    Graphics.ZeroMemory();

    Hash           = 0;
    bIsGraphics    = true;
    pRenderPass    = nullptr;
    pRootSignature = nullptr;

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

bool Core::PipelineStateDescription::IsDynamicStateEnabled(VkDynamicState eDynamicState) const
{
    for (uint32_t Idx = 0; Idx < DynamicState->dynamicStateCount; Idx++)
        if (EnabledDynamicStates[Idx] == eDynamicState)
            return true;

    return false;
}

uint64_t Core::PipelineStateDescription::UpdateHash()
{
    Aux::CityHash64Wrapper HashBuilder;

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

        _Game_engine_Assert (bFoundComputeShader, "No compute shader.");
        HashBuilder.CombineWith (Compute->layout);
        HashBuilder.CombineWith (Compute->flags);
    }

    Hash = HashBuilder.Value;
    return HashBuilder.Value;
}

/// -------------------------------------------------------------------------------------------------------------------
/// PipelineState
/// -------------------------------------------------------------------------------------------------------------------

Core::PipelineState::PipelineState ()
    : pRenderPass (nullptr)
    , pRootSignature (nullptr)
    , pGraphicsNode (nullptr)
    , pDesc (nullptr)
{
}

Core::PipelineState::~PipelineState ()
{
}

void Core::PipelineState::BindTo (Core::CommandList & CmdList)
{
    vkCmdBindPipeline (CmdList, eBindPoint, hPipeline);
}

Core::PipelineState::operator VkPipeline () const
{
    return hPipeline;
}

Core::PipelineState::operator VkPipelineCache () const
{
    return hPipelineCache;
}

/// -------------------------------------------------------------------------------------------------------------------
/// PipelineStateBuilder
/// -------------------------------------------------------------------------------------------------------------------

Core::PipelineStateBuilder::PipelineStateBuilder()
    : InputEditor(*this)
{
    Reset();
}

Core::PipelineStateBuilder::~PipelineStateBuilder()
{
}

void Core::PipelineStateBuilder::Reset()
{
    TemporaryDesc.Reset();
}

void Core::PipelineStateBuilder::SetRenderPass (Core::RenderPass const & RenderPass,
                                                uint32_t                 SubpassId)
{
    TemporaryDesc.pRenderPass          = &RenderPass;

    TemporaryDesc.Graphics->subpass    = SubpassId;
    TemporaryDesc.Graphics->renderPass = RenderPass;
}

void Core::PipelineStateBuilder::SetRootSignature (Core::RootSignature const & RootSignature)
{
    TemporaryDesc.pRootSignature = &RootSignature;

    TemporaryDesc.Graphics->layout = RootSignature;
}

Core::PipelineStateBuilder::InputLayoutBuilder &
Core::PipelineStateBuilder::EditInputLayout(uint32_t InputElementCount,
                                            uint32_t TotalInputAttributeCount)
{
    InputEditor.Reserve(InputElementCount, TotalInputAttributeCount);
    return InputEditor;
}

void Core::PipelineStateBuilder::ClearInputLayout (uint32_t InputElementCount,
                                                   uint32_t TotalInputAttributeCount)
{
    TemporaryDesc.InputBindings.clear ();
    TemporaryDesc.InputAttributes.clear ();
    TemporaryDesc.InputBindings.reserve (InputElementCount);
    TemporaryDesc.InputAttributes.reserve (TotalInputAttributeCount);
}

void Core::PipelineStateBuilder::AddInputElement (
    uint32_t                                  VertexStride,
    VkVertexInputRate                         InputRate,
    VkVertexInputAttributeDescription const * InputAttributes,
    uint32_t                                  InputAttributeCount)
{
    const uint32_t BindingIdx = _Get_collection_length_u (TemporaryDesc.InputBindings);
    auto & Binding = Aux::PushBackAndGet(TemporaryDesc.InputBindings);

    Binding           = TInfoStruct<VkVertexInputBindingDescription> ();
    Binding.stride    = VertexStride;
    Binding.inputRate = InputRate;
    Binding.binding   = BindingIdx;

    for (uint32_t AttrIdx = 0; AttrIdx < InputAttributeCount; ++AttrIdx)
    {
        auto & InputAttribute   = Aux::PushBackAndGet(TemporaryDesc.InputAttributes);
        InputAttribute.format   = InputAttributes[ AttrIdx ].format;
        InputAttribute.location = InputAttributes[ AttrIdx ].location;
        InputAttribute.offset   = InputAttributes[ AttrIdx ].offset;
        InputAttribute.binding  = BindingIdx;
    }

    Aux::AliasStructs (TemporaryDesc.InputBindings,
                       TemporaryDesc.VertexInputState->pVertexBindingDescriptions,
                       TemporaryDesc.VertexInputState->vertexBindingDescriptionCount);

    Aux::AliasStructs (TemporaryDesc.InputAttributes,
                       TemporaryDesc.VertexInputState->pVertexAttributeDescriptions,
                       TemporaryDesc.VertexInputState->vertexAttributeDescriptionCount);
}

Core::PipelineStateDescription & Core::PipelineStateBuilder::GetDesc ()
{
    return TemporaryDesc;
}

void Core::PipelineStateBuilder::SetBlendContants (float r, float g, float b, float a)
{
    TemporaryDesc.ColorBlendState->blendConstants[ 0 ] = r;
    TemporaryDesc.ColorBlendState->blendConstants[ 1 ] = g;
    TemporaryDesc.ColorBlendState->blendConstants[ 2 ] = b;
    TemporaryDesc.ColorBlendState->blendConstants[ 3 ] = a;

    DisableDynamicState (VK_DYNAMIC_STATE_BLEND_CONSTANTS);
}

void Core::PipelineStateBuilder::ResetBlendContants ()
{
    EnableDynamicState (VK_DYNAMIC_STATE_BLEND_CONSTANTS);
}

void Core::PipelineStateBuilder::SetLineWidth (float LineWidth)
{
    TemporaryDesc.RasterizationState->lineWidth = LineWidth;
    DisableDynamicState (VK_DYNAMIC_STATE_LINE_WIDTH);
}

void Core::PipelineStateBuilder::ResetLineWidth ()
{
    EnableDynamicState (VK_DYNAMIC_STATE_LINE_WIDTH);
}

void Core::PipelineStateBuilder::SetDepthBias (float DepthBiasConstantFactor,
                                               float DepthBiasClamp,
                                               float DepthBiasSlopeFactor)
{
    TemporaryDesc.RasterizationState->depthBiasClamp          = DepthBiasClamp;
    TemporaryDesc.RasterizationState->depthBiasSlopeFactor    = DepthBiasSlopeFactor;
    TemporaryDesc.RasterizationState->depthBiasConstantFactor = DepthBiasConstantFactor;

    DisableDynamicState(VK_DYNAMIC_STATE_DEPTH_BIAS);
}

void Core::PipelineStateBuilder::ResetDepthBias ()
{
    EnableDynamicState (VK_DYNAMIC_STATE_DEPTH_BIAS);
}

void Core::PipelineStateBuilder::SetDepthBounds (float MinDepthBounds,
                                                 float MaxDepthBounds,
                                                 bool  bEnableDepthBoundsTest)
{
    TemporaryDesc.DepthStencilState->minDepthBounds        = MinDepthBounds;
    TemporaryDesc.DepthStencilState->maxDepthBounds        = MaxDepthBounds;
    TemporaryDesc.DepthStencilState->depthBoundsTestEnable = bEnableDepthBoundsTest;

    DisableDynamicState (VK_DYNAMIC_STATE_DEPTH_BOUNDS);
}

void Core::PipelineStateBuilder::ResetDepthBounds ()
{
    EnableDynamicState (VK_DYNAMIC_STATE_DEPTH_BOUNDS);
}

void Core::PipelineStateBuilder::SetStencilCompareMask (uint32_t FrontFaceStencilCompareMask,
                                                        uint32_t BackFaceStencilCompareMask)
{
    TemporaryDesc.DepthStencilState->front.compareMask = FrontFaceStencilCompareMask;
    TemporaryDesc.DepthStencilState->back.compareMask  = BackFaceStencilCompareMask;

    DisableDynamicState (VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK);
}

void Core::PipelineStateBuilder::ResetStencilCompareMask ()
{
    EnableDynamicState (VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK);
}

void Core::PipelineStateBuilder::SetStencilWriteMask (uint32_t FrontFaceStencilWriteMask,
                                                      uint32_t BackFaceStencilWriteMask)
{
    TemporaryDesc.DepthStencilState->front.writeMask = FrontFaceStencilWriteMask;
    TemporaryDesc.DepthStencilState->back.writeMask  = BackFaceStencilWriteMask;

    DisableDynamicState (VK_DYNAMIC_STATE_STENCIL_WRITE_MASK);
}

void Core::PipelineStateBuilder::ResetStencilWriteMask()
{
    EnableDynamicState (VK_DYNAMIC_STATE_STENCIL_WRITE_MASK);
}

void Core::PipelineStateBuilder::SetStencilReference (uint32_t FrontFaceStencilReference,
                                                      uint32_t BackFaceStencilReference)
{
    TemporaryDesc.DepthStencilState->front.reference = FrontFaceStencilReference;
    TemporaryDesc.DepthStencilState->back.reference  = BackFaceStencilReference;

    DisableDynamicState (VK_DYNAMIC_STATE_STENCIL_REFERENCE);
}

void Core::PipelineStateBuilder::ResetStencilReference()
{
    EnableDynamicState (VK_DYNAMIC_STATE_STENCIL_REFERENCE);
}

void Core::PipelineStateBuilder::SetViewports(VkViewport const * pViewports, size_t ViewportCount)
{
    TemporaryDesc.ViewportState->viewportCount = static_cast<uint32_t>(ViewportCount);

    if (_Game_engine_Likely (pViewports != nullptr && ViewportCount != 0))
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

void Core::PipelineStateBuilder::ResetViewports()
{
    SetViewports (nullptr, 0);
}

void Core::PipelineStateBuilder::SetScissorRects(VkRect2D const * pRects, size_t RectCount)
{
    TemporaryDesc.ViewportState->scissorCount = static_cast<uint32_t>(RectCount);

    if (_Game_engine_Likely (pRects != nullptr && RectCount != 0))
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

void Core::PipelineStateBuilder::ResetScissorRects()
{
    SetScissorRects (nullptr, 0);
}

void Core::PipelineStateBuilder::EnableDynamicState(VkDynamicState eDynamicState)
{
    if (TemporaryDesc.IsDynamicStateEnabled (eDynamicState))
        return;

    const uint32_t Counter = TemporaryDesc.DynamicState->dynamicStateCount++;
    TemporaryDesc.EnabledDynamicStates[ Counter ] = eDynamicState;
}

void Core::PipelineStateBuilder::DisableDynamicState (VkDynamicState eDynamicState)
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

void Core::PipelineStateBuilder::SetShader (const char * pId,
                                            const char * pMainFn,
                                            const void * pBytecode,
                                            size_t       BytecodeLength)
{
    Core::ShaderBytecode ShaderBytecode;
    ShaderBytecode.SetIdAndType (pId);
    ShaderBytecode.SetMainFn (pMainFn);
    ShaderBytecode.SetBytecode (pBytecode, BytecodeLength);

    TemporaryDesc.ShaderBytecodes[ ShaderBytecode.eType ] = std::move (ShaderBytecode);

    auto & ShaderStage = TemporaryDesc.ShaderStages[ ShaderBytecode.eType ];
    ShaderStage->pName = ShaderBytecode.MainFn.c_str ();
    ShaderStage->stage = static_cast<VkShaderStageFlagBits> (ShaderBytecode.eType);

    ++TemporaryDesc.Graphics->stageCount;
}

/// -------------------------------------------------------------------------------------------------------------------
/// PipelineStateBuilder InputLayoutBuilder
/// -------------------------------------------------------------------------------------------------------------------

Core::PipelineStateBuilder::InputLayoutBuilder::InputLayoutBuilder (PipelineStateBuilder & Bilder)
    : Bilder (Bilder)
    , pBinding (nullptr)
    , AttributeLocation (0)
    , AttributeCount (0)
    , AttributeOffset (0)
{
}

Core::PipelineStateBuilder::InputLayoutBuilder &
Core::PipelineStateBuilder::InputLayoutBuilder::Reserve (uint32_t InputElementCount,
                                                         uint32_t TotalInputAttributeCount)
{
    Bilder.ClearInputLayout (InputElementCount, TotalInputAttributeCount);
    return *this;
}

Core::PipelineStateBuilder::InputLayoutBuilder &
Core::PipelineStateBuilder::InputLayoutBuilder::AddInputElement (uint32_t          Stride,
                                                                 VkVertexInputRate InputRate,
                                                                 uint32_t          Count)
{
    const auto BindingIdx = _Get_collection_length_u (Bilder.TemporaryDesc.InputBindings);

    pBinding  = &Aux::PushBackAndGet(Bilder.TemporaryDesc.InputBindings);
    *pBinding = TInfoStruct<VkVertexInputBindingDescription> ();

    pBinding->inputRate = InputRate;
    pBinding->binding   = BindingIdx;
    pBinding->stride    = Stride;

    AttributeCount    = Count;
    AttributeOffset   = 0;
    AttributeLocation = 0;

    return *this;
}

Core::PipelineStateBuilder::InputLayoutBuilder &
Core::PipelineStateBuilder::InputLayoutBuilder::AddAttribute (VkFormat Fmt)
{
    auto & Attribute   = Aux::PushBackAndGet(Bilder.TemporaryDesc.InputAttributes);
    Attribute.binding  = pBinding->binding;
    Attribute.location = AttributeLocation++;
    Attribute.offset   = AttributeOffset;
    Attribute.format   = Fmt;
    _Game_engine_Assert (AttributeLocation < AttributeCount,
                         "Too many attributes for the binding.");

    const auto ElementByteWidth = Core::ResourceReference::GetElementSizeInBytes (Fmt);
    _Game_engine_Assert (ElementByteWidth != 0, "Unsupported attribute type.");

    AttributeOffset += ElementByteWidth;
    _Game_engine_Assert (AttributeOffset <= pBinding->stride, "Attribute is too large.");

    return *this;
}

void Core::PipelineStateBuilder::InputLayoutBuilder::ShrinkToFit ()
{
    Bilder.TemporaryDesc.InputBindings.shrink_to_fit ();
    Bilder.TemporaryDesc.InputAttributes.shrink_to_fit ();
}

void Core::PipelineStateBuilder::SetPrimitiveTopology (VkPrimitiveTopology Topology,
                                                       bool                bEnableRestart)
{
    TemporaryDesc.InputAssemblyState->topology               = Topology;
    TemporaryDesc.InputAssemblyState->primitiveRestartEnable = bEnableRestart;
}

Core::PipelineState const *
Core::PipelineStateBuilder::RecreatePipelineState (Core::GraphicsDevice & GraphicsNode)
{
    if (_Game_engine_Likely (GraphicsNode.IsValid ()))
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

struct Core::PipelineStateManager::PrivateContent : public Aux::ScalableAllocPolicy,
                                                    public Aux::NoCopyAssignPolicy
{
    using PipelineStateHashMap = std::map<Aux::CityHash64Wrapper::ValueType,
                                                   std::unique_ptr<PipelineState>,
                                                   Aux::CityHash64Wrapper::CmpOpLess>;

    std::mutex            Lock;
    PipelineStateHashMap StoredPipelineStates;
};

/// -------------------------------------------------------------------------------------------------------------------
/// PipelineStateManager
/// -------------------------------------------------------------------------------------------------------------------

Core::PipelineStateManager::PipelineStateManager ()
    : pContent (new PrivateContent ())
{
}

Core::PipelineStateManager::~PipelineStateManager ()
{
    Aux::TSafeDeleteObj (pContent);
}

void Core::PipelineStateManager::AddNewPipelineStateObject (Core::PipelineState & PipelineState)
{
    std::lock_guard<std::mutex> LockGuard (pContent->Lock);

    _Game_engine_Assert (pContent->StoredPipelineStates.find (PipelineState.Hash)
                             == pContent->StoredPipelineStates.end (),
                         "See TryGetPipelineStateObjectByHash(...).");

    pContent->StoredPipelineStates[ PipelineState.Hash ].reset (&PipelineState);
}

Core::PipelineState const *
Core::PipelineStateManager::TryGetPipelineStateObjectByHash (uint64_t Hash)
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

Core::ShaderType GetShaderType (const char * pId)
{
    const auto Length = std::strlen (pId);
    auto       pType  = pId + Length - 4;

    if (!std::strcmp (pType, "vert"))
        return Core::kShaderType_Vertex;
    if (!std::strcmp (pType, "frag"))
        return Core::kShaderType_Fragment;
    if (!std::strcmp (pType, "geom"))
        return Core::kShaderType_Geometry;
    if (!std::strcmp (pType, "tesc"))
        return Core::kShaderType_TesselationControl;
    if (!std::strcmp (pType, "tese"))
        return Core::kShaderType_TesselationEvaluation;
    if (!std::strcmp (pType, "comp"))
        return Core::kShaderType_Compute;

    return Core::kShaderType_Unknown;
}

Core::ShaderBytecode::ShaderBytecode () : eType (kShaderType_Unknown), Hash (0)
{

}

Core::ShaderBytecode::ShaderBytecode (ShaderBytecode const & Other)
    : Hash (Other.Hash)
    , Id (Other.Id)
    , eType (Other.eType)
    , Bytecode (Other.Bytecode)
    , MainFn (Other.MainFn)
{
}

Core::ShaderBytecode::ShaderBytecode (ShaderBytecode && Other)
    : Hash (Other.Hash)
    , Id (std::move (Other.Id))
    , eType (Other.eType)
    , Bytecode (std::move (Other.Bytecode))
    , MainFn (std::move (Other.MainFn))
{
}

Core::ShaderBytecode & Core::ShaderBytecode::operator= (Core::ShaderBytecode const & Other)
{
    Hash     = Other.Hash;
    eType    = Other.eType;
    Id       = Other.Id;
    MainFn   = Other.MainFn;
    Bytecode = Other.Bytecode;

    return *this;
}

Core::ShaderBytecode & Core::ShaderBytecode::operator= (Core::ShaderBytecode && Other)
{
    Hash     = Other.Hash;
    eType    = Other.eType;
    Id       = std::move (Other.Id);
    MainFn   = std::move (Other.MainFn);
    Bytecode = std::move (Other.Bytecode);

    return *this;
}

void Core::ShaderBytecode::SetIdAndType (const char * pId)
{
    Id    = pId;
    eType = GetShaderType (pId);
}

void Core::ShaderBytecode::SetMainFn (const char * pMainFn)
{
    MainFn = pMainFn;
}

void Core::ShaderBytecode::SetBytecode (const void * pCode, size_t CodeSize)
{
    Bytecode.resize (CodeSize);
    std::memcpy (Bytecode.data (), pCode, CodeSize);

    Hash = Aux::CityHash64 (reinterpret_cast<const char *> (pCode), CodeSize);
}
