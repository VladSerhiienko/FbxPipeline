//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <RootSignature.Vulkan.h>

#include <CityHash.h>

/// -------------------------------------------------------------------------------------------------------------------
/// RootParameter
/// -------------------------------------------------------------------------------------------------------------------

Core::RootParameter::RootParameter()
{
    Clear();
}

Core::RootParameter::~RootParameter()
{
}

void Core::RootParameter::Clear()
{
    Binding.ZeroMemory();
}

void Core::RootParameter::InitAsUniformBuffer(uint32_t         Register,
                                              uint32_t         Count,
                                              ShaderVisibility StageFlags,
                                              uint32_t         Set)
{
    DescSet                  = Set;
    Binding->binding         = Register;
    Binding->descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    Binding->descriptorCount = Count;
    Binding->stageFlags      = static_cast<VkShaderStageFlagBits>(StageFlags);
}

void Core::RootParameter::InitAsUniformBufferDynamic(uint32_t         Register,
                                                     uint32_t         Count,
                                                     ShaderVisibility StageFlags,
                                                     uint32_t         Set)
{
    DescSet                  = Set;
    Binding->binding         = Register;
    Binding->descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    Binding->descriptorCount = Count;
    Binding->stageFlags      = static_cast<VkShaderStageFlagBits>(StageFlags);
}

void Core::RootParameter::InitAsSampler(uint32_t         Register,
                                        uint32_t         Count,
                                        ShaderVisibility StageFlags,
                                        uint32_t         Set)
{
    DescSet                  = Set;
    Binding->binding         = Register;
    Binding->descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
    Binding->descriptorCount = Count;
    Binding->stageFlags      = static_cast<VkShaderStageFlagBits>(StageFlags);
}

void Core::RootParameter::InitAsCombinedImageSampler(uint32_t         Register,
                                                     uint32_t         Count,
                                                     VkSampler *      pImmutableSamplers,
                                                     ShaderVisibility StageFlags,
                                                     uint32_t         Set)
{
    DescSet                     = Set;
    Binding->binding            = Register;
    Binding->pImmutableSamplers = pImmutableSamplers;
    Binding->descriptorCount    = Count;
    Binding->descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    Binding->stageFlags         = static_cast<VkShaderStageFlagBits>(StageFlags);
}

Core::RootParameter::operator VkDescriptorSetLayoutBinding() const
{
    return Binding;
}

/// -------------------------------------------------------------------------------------------------------------------
/// RootSignature
/// -------------------------------------------------------------------------------------------------------------------

Core::RootSignature::RootSignature()
    : pDesc(nullptr)
    , pGraphicsNode(nullptr)
{
}

Core::RootSignature::~RootSignature()
{
    if (_Game_engine_Likely (pGraphicsNode != nullptr && !DescSetLayouts.empty ()))
    {
        std::for_each (DescSetLayouts.begin (),
                         DescSetLayouts.end (),
                         [&](VkDescriptorSetLayout SetLayoutHandle) 
                         {
                             Core::TDispatchableHandle<VkDescriptorSetLayout> DestroySetLayout (
                                 *pGraphicsNode, SetLayoutHandle);
                         });
    }
}

Core::RootSignature::operator VkPipelineLayout() const
{
    return PipelineLayoutHandle;
}

/// -------------------------------------------------------------------------------------------------------------------
/// RootSignatureDescription
/// -------------------------------------------------------------------------------------------------------------------

Core::RootSignatureDescription::RootSignatureDescription()
{
    Reset();
}

Core::RootSignatureDescription::~RootSignatureDescription()
{
}

void Core::RootSignatureDescription::Reset()
{
    Params.clear();
}

uint32_t Core::RootSignatureDescription::GetParamCount() const
{
    return _Get_collection_length_u (Params);
}

Core::RootParameter const &
Core::RootSignatureDescription::GetParameter(uint32_t ParameterIndex) const
{
    _Game_engine_Assert (Params.size () < ParameterIndex, 
                         "Index is out of range.");

    return Params[ParameterIndex];
}

uint64_t Core::RootSignatureDescription::UpdateHash()
{
    Aux::CityHash64Wrapper HashBuilder;
    HashBuilder.CombineWithArray (Params.data (), Params.size ());
    HashBuilder.CombineWithArray (PushConstRanges.data (), PushConstRanges.size ());

    Hash = HashBuilder.Value;
    return HashBuilder.Value;
}

Core::RootSignatureDescription const *
Core::RootSignatureDescription::MakeNewFromTemporary(RootSignatureDescription const & TemporaryDesc)
{
    auto pOutDesc = new Core::RootSignatureDescription();

    if (!TemporaryDesc.Params.empty())
    {
        pOutDesc->Params.reserve(TemporaryDesc.Params.size());

        std::copy(TemporaryDesc.Params.begin(),
                    TemporaryDesc.Params.end(),
                    std::back_inserter(pOutDesc->Params));

        pOutDesc->PushConstRanges.reserve(TemporaryDesc.PushConstRanges.size());

        std::copy(TemporaryDesc.PushConstRanges.begin(),
                    TemporaryDesc.PushConstRanges.end(),
                    std::back_inserter(pOutDesc->PushConstRanges));
    }

    pOutDesc->Hash = TemporaryDesc.Hash;
    return pOutDesc;
}

/// -------------------------------------------------------------------------------------------------------------------
/// RootSignatureBuilder
/// -------------------------------------------------------------------------------------------------------------------

Core::RootSignatureBuilder::RootSignatureBuilder()
{
}

void Core::RootSignatureBuilder::Reset (uint32_t BindingCount, uint32_t PushConstRangeCount)
{
    TemporaryDesc.Params.clear ();
    TemporaryDesc.Params.reserve (BindingCount);
    TemporaryDesc.PushConstRanges.clear ();
    TemporaryDesc.PushConstRanges.reserve (PushConstRangeCount);
}

Core::RootParameter & Core::RootSignatureBuilder::AddParameter()
{
    return Aux::PushBackAndGet( TemporaryDesc.Params);
}

VkPushConstantRange & Core::RootSignatureBuilder::AddPushConstRange()
{
    return Aux::PushBackAndGet(TemporaryDesc.PushConstRanges);
}

Core::RootSignature const *
Core::RootSignatureBuilder::RecreateRootSignature(Core::GraphicsDevice & GraphicsNode)
{
    using SetKey                = uint32_t;
    using Binding               = VkDescriptorSetLayoutBinding;
    using KeyLessOp             = std::less<SetKey>;
    using SetToBindingMultimap = std::multimap<SetKey, Binding, KeyLessOp>;
    using BindingVector        = std::vector<VkDescriptorSetLayoutBinding>;
    using SetToBindingIt       = SetToBindingMultimap::iterator;
    using SetToBindingItRange  = std::pair<SetToBindingIt, SetToBindingIt>;
    using SetToBindingPair     = std::pair<SetKey, VkDescriptorSetLayoutBinding>;

    if (_Game_engine_Likely (GraphicsNode.IsValid ()))
    {
        SetToBindingMultimap Bindings;
        BindingVector        SetLayoutBindings;

        auto   Hash    = TemporaryDesc.UpdateHash ();
        auto & Manager = GraphicsNode.GetDefaultRootSignatureManager ();

        if (auto pStoredRootSign = Manager.TryGetRootSignatureObjectByHash (Hash))
            return pStoredRootSign;

        std::vector<VkDescriptorSetLayout> DescSetLayouts;
        if (_Game_engine_Likely (!TemporaryDesc.Params.empty ()))
        {
            std::for_each (TemporaryDesc.Params.begin (),
                             TemporaryDesc.Params.end (),
                             [&](Core::RootParameter const & Param)
                             {
                                 Bindings.insert (std::make_pair (
                                     Param.DescSet, Param.Binding.Desc));
                             });

            SetToBindingItRange ItRange;
            SetToBindingIt      It = Bindings.begin ();
            for (; It != Bindings.cend (); It = ItRange.second)
            {
                ItRange = Bindings.equal_range (It->first);

                const auto BindingCount = std::distance (ItRange.first, ItRange.second);
                SetLayoutBindings.reserve (static_cast<size_t> (BindingCount));

                std::for_each (ItRange.first, ItRange.second, [&](SetToBindingPair const & Pair) {
                    SetLayoutBindings.push_back (Pair.second);
                });

                Aux::CityHash64Wrapper DescSetLayoutHashBuilder;
                DescSetLayoutHashBuilder.CombineWithArray (SetLayoutBindings.data (),
                                                           SetLayoutBindings.size ());

                if (auto StoredDescSetLayout = Manager.GetDescSetLayout(DescSetLayoutHashBuilder.Value))
                {
                    DescSetLayouts.push_back (StoredDescSetLayout);
                }
                else
                {
                    Core::TInfoStruct<VkDescriptorSetLayoutCreateInfo> DescSetLayoutDesc;

                    Aux::AliasStructs (SetLayoutBindings,
                                       DescSetLayoutDesc->pBindings,
                                       DescSetLayoutDesc->bindingCount);

                    Core::TDispatchableHandle<VkDescriptorSetLayout> TempDescSetLayout;
                    if (!TempDescSetLayout.Recreate (GraphicsNode, DescSetLayoutDesc))
                    {
                        _Game_engine_Error ("Failed to create descriptor set layout.");
                        return false;
                    }

                    auto DestSetLayoutHandle = TempDescSetLayout.Release ();
                    Manager.SetDescSetLayout (DescSetLayoutHashBuilder.Value, DestSetLayoutHandle);

                    DescSetLayouts.push_back (DestSetLayoutHandle);
                }
            }
        }

        TInfoStruct<VkPipelineLayoutCreateInfo> PipelineLayoutDesc;
        Aux::AliasStructs (DescSetLayouts,
                           PipelineLayoutDesc->pSetLayouts,
                           PipelineLayoutDesc->setLayoutCount);

        if (!TemporaryDesc.PushConstRanges.empty())
        {
            Aux::AliasStructs (TemporaryDesc.PushConstRanges,
                               PipelineLayoutDesc->pPushConstantRanges,
                               PipelineLayoutDesc->pushConstantRangeCount);
        }

        Core::TDispatchableHandle<VkPipelineLayout> PipelineLayoutHandle;
        if (!PipelineLayoutHandle.Recreate(GraphicsNode, PipelineLayoutDesc))
        {
            return false;
        }

        if (_Game_engine_Likely (PipelineLayoutHandle.IsNotNull ()))
        {
            auto pRootSign = new Core::RootSignature();

            pRootSign->DescSetLayouts.swap(DescSetLayouts);
            pRootSign->PipelineLayoutHandle.Swap(PipelineLayoutHandle);
            pRootSign->Hash          = Hash;
            pRootSign->pGraphicsNode = &GraphicsNode;
            pRootSign->pDesc = RootSignatureDescription::MakeNewFromTemporary(TemporaryDesc);

            Manager.AddNewRootSignatureObject(*pRootSign);

            return pRootSign;
        }
    }

    return nullptr;
}

/// -------------------------------------------------------------------------------------------------------------------
/// RootSignatureManager PrivateContent
/// -------------------------------------------------------------------------------------------------------------------

struct Core::RootSignatureManager::PrivateContent : public Aux::ScalableAllocPolicy,
                                                    public Aux::NoCopyAssignPolicy
{
    using HashType            = Aux::CityHash64Wrapper::ValueType;
    using HashOpLess          = Aux::CityHash64Wrapper::CmpOpLess;
    using HashOp              = Aux::CityHash64Wrapper::HashOp<>;
    using HashOpEqual         = Aux::CityHash64Wrapper::CmpOpEqual;
    using RootSignatureLookup = std::map<HashType, std::unique_ptr<Core::RootSignature> >;
    using DescriptorSetLayoutLookup = std::map<HashType, VkDescriptorSetLayout>;

    std::mutex                 Lock;
    RootSignatureLookup       StoredRootSigns;
    DescriptorSetLayoutLookup StoredDescSetLayouts;
};

/// -------------------------------------------------------------------------------------------------------------------
/// RootSignatureManager
/// -------------------------------------------------------------------------------------------------------------------

Core::RootSignatureManager::RootSignatureManager () : pContent (new PrivateContent ())
{
}

Core::RootSignatureManager::~RootSignatureManager ()
{
    Aux::TSafeDeleteObj (pContent);
}

VkDescriptorSetLayout Core::RootSignatureManager::GetDescSetLayout (uint64_t Hash)
{
    auto DescSetLayoutIt = pContent->StoredDescSetLayouts.find (Hash);
    if (DescSetLayoutIt != pContent->StoredDescSetLayouts.end ())
        return DescSetLayoutIt->second;

    return VK_NULL_HANDLE;
}

void Core::RootSignatureManager::SetDescSetLayout (uint64_t Hash, VkDescriptorSetLayout SetLayout)
{
    _Game_engine_Assert (pContent->StoredDescSetLayouts.find (Hash)
                             == pContent->StoredDescSetLayouts.end (),
                         "Already stored, try to look-up for it first.");

    pContent->StoredDescSetLayouts[ Hash ] = SetLayout;
}

void Core::RootSignatureManager::AddNewRootSignatureObject (Core::RootSignature & RootSign)
{
    std::lock_guard<std::mutex> LockGuard (pContent->Lock);

    _Game_engine_Assert (pContent->StoredRootSigns.find (RootSign.Hash)
                             == pContent->StoredRootSigns.end (),
                         "See TryGetPipelineStateObjectByHash(...).");

    pContent->StoredRootSigns[ RootSign.Hash ].reset (&RootSign);
}

Core::RootSignature const *
Core::RootSignatureManager::TryGetRootSignatureObjectByHash (uint64_t Hash)
{
    std::lock_guard<std::mutex> LockGuard (pContent->Lock);

    auto RenderPassContentIt = pContent->StoredRootSigns.find (Hash);
    if (RenderPassContentIt != pContent->StoredRootSigns.end ())
    {
        return (*RenderPassContentIt).second.get ();
    }

    return nullptr;
}
