//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <PipelineLayout.Vulkan.h>

#include <CityHash.h>

/// -------------------------------------------------------------------------------------------------------------------
/// PipelineLayoutParameter
/// -------------------------------------------------------------------------------------------------------------------

apemode::PipelineLayoutParameter::PipelineLayoutParameter()
{
    Clear();
}

apemode::PipelineLayoutParameter::~PipelineLayoutParameter()
{
}

void apemode::PipelineLayoutParameter::Clear()
{
    Binding.ZeroMemory();
}

void apemode::PipelineLayoutParameter::InitAsUniformBuffer(uint32_t         Register,
                                              uint32_t         Count,
                                              VkShaderStageFlagBits StageFlags,
                                              uint32_t         Set)
{
    DescSet                  = Set;
    Binding->binding         = Register;
    Binding->descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    Binding->descriptorCount = Count;
    Binding->stageFlags      = static_cast<VkShaderStageFlagBits>(StageFlags);
}

void apemode::PipelineLayoutParameter::InitAsUniformBufferDynamic(uint32_t         Register,
                                                     uint32_t         Count,
                                                     VkShaderStageFlagBits StageFlags,
                                                     uint32_t         Set)
{
    DescSet                  = Set;
    Binding->binding         = Register;
    Binding->descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    Binding->descriptorCount = Count;
    Binding->stageFlags      = static_cast<VkShaderStageFlagBits>(StageFlags);
}

void apemode::PipelineLayoutParameter::InitAsSampler(uint32_t         Register,
                                        uint32_t         Count,
                                        VkShaderStageFlagBits StageFlags,
                                        uint32_t         Set)
{
    DescSet                  = Set;
    Binding->binding         = Register;
    Binding->descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
    Binding->descriptorCount = Count;
    Binding->stageFlags      = static_cast<VkShaderStageFlagBits>(StageFlags);
}

void apemode::PipelineLayoutParameter::InitAsCombinedImageSampler(
    uint32_t Register, uint32_t Count, VkSampler* pImmutableSamplers, VkShaderStageFlagBits StageFlags, uint32_t Set ) {
    DescSet                     = Set;
    Binding->binding            = Register;
    Binding->pImmutableSamplers = pImmutableSamplers;
    Binding->descriptorCount    = Count;
    Binding->descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    Binding->stageFlags         = static_cast<VkShaderStageFlagBits>(StageFlags);
}

apemode::PipelineLayoutParameter::operator VkDescriptorSetLayoutBinding() const
{
    return Binding;
}

/// -------------------------------------------------------------------------------------------------------------------
/// PipelineLayout
/// -------------------------------------------------------------------------------------------------------------------

apemode::PipelineLayout::PipelineLayout()
    : pDesc(nullptr)
    , pNode(nullptr)
{
}

apemode::PipelineLayout::~PipelineLayout()
{
    if (apemode_likely (pNode != nullptr && !DescSetLayouts.empty ()))
    {
        std::for_each( DescSetLayouts.begin( ), DescSetLayouts.end( ), [&]( VkDescriptorSetLayout SetLayoutHandle ) {
            apemode::TDispatchableHandle< VkDescriptorSetLayout > DestroySetLayout( *pNode, SetLayoutHandle );
        } );
    }
}

apemode::PipelineLayout::operator VkPipelineLayout() const
{
    return PipelineLayoutHandle;
}

/// -------------------------------------------------------------------------------------------------------------------
/// PipelineLayoutDescription
/// -------------------------------------------------------------------------------------------------------------------

apemode::PipelineLayoutDescription::PipelineLayoutDescription()
{
    Reset();
}

apemode::PipelineLayoutDescription::~PipelineLayoutDescription()
{
}

void apemode::PipelineLayoutDescription::Reset()
{
    Params.clear();
}

uint32_t apemode::PipelineLayoutDescription::GetParamCount() const
{
    return _Get_collection_length_u (Params);
}

apemode::PipelineLayoutParameter const &
apemode::PipelineLayoutDescription::GetParameter(uint32_t ParameterIndex) const
{
    _Game_engine_Assert (Params.size () < ParameterIndex, 
                         "Index is out of range.");

    return Params[ParameterIndex];
}

uint64_t apemode::PipelineLayoutDescription::UpdateHash()
{
    apemode::CityHash64Wrapper HashBuilder;
    HashBuilder.CombineWithArray (Params.data (), Params.size ());
    HashBuilder.CombineWithArray (PushConstRanges.data (), PushConstRanges.size ());

    Hash = HashBuilder.Value;
    return HashBuilder.Value;
}

apemode::PipelineLayoutDescription const *
apemode::PipelineLayoutDescription::MakeNewFromTemporary(PipelineLayoutDescription const & TemporaryDesc)
{
    auto pOutDesc = new apemode::PipelineLayoutDescription();

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
/// PipelineLayoutBuilder
/// -------------------------------------------------------------------------------------------------------------------

apemode::PipelineLayoutBuilder::PipelineLayoutBuilder()
{
}

void apemode::PipelineLayoutBuilder::Reset (uint32_t BindingCount, uint32_t PushConstRangeCount)
{
    TemporaryDesc.Params.clear ();
    TemporaryDesc.Params.reserve (BindingCount);
    TemporaryDesc.PushConstRanges.clear ();
    TemporaryDesc.PushConstRanges.reserve (PushConstRangeCount);
}

apemode::PipelineLayoutParameter & apemode::PipelineLayoutBuilder::AddParameter()
{
    return apemode::PushBackAndGet( TemporaryDesc.Params);
}

VkPushConstantRange & apemode::PipelineLayoutBuilder::AddPushConstRange()
{
    return apemode::PushBackAndGet(TemporaryDesc.PushConstRanges);
}

apemode::PipelineLayout const *
apemode::PipelineLayoutBuilder::RecreatePipelineLayout(apemode::GraphicsDevice & GraphicsNode)
{
    using SetKey                = uint32_t;
    using Binding               = VkDescriptorSetLayoutBinding;
    using KeyLessOp             = std::less<SetKey>;
    using SetToBindingMultimap = std::multimap<SetKey, Binding, KeyLessOp>;
    using BindingVector        = std::vector<VkDescriptorSetLayoutBinding>;
    using SetToBindingIt       = SetToBindingMultimap::iterator;
    using SetToBindingItRange  = std::pair<SetToBindingIt, SetToBindingIt>;
    using SetToBindingPair     = std::pair<SetKey, VkDescriptorSetLayoutBinding>;

    if (apemode_likely (GraphicsNode.IsValid ()))
    {
        SetToBindingMultimap Bindings;
        BindingVector        SetLayoutBindings;

        auto   Hash    = TemporaryDesc.UpdateHash ();
        auto & Manager = GraphicsNode.GetDefaultPipelineLayoutManager ();

        if (auto pStoredRootSign = Manager.TryGetPipelineLayoutObjectByHash (Hash))
            return pStoredRootSign;

        std::vector<VkDescriptorSetLayout> DescSetLayouts;
        if (apemode_likely (!TemporaryDesc.Params.empty ()))
        {
            std::for_each( TemporaryDesc.Params.begin( ),
                           TemporaryDesc.Params.end( ),
                           [&]( apemode::PipelineLayoutParameter const& Param ) {
                               Bindings.insert( std::make_pair( Param.DescSet, Param.Binding.Desc ) );
                           } );

            SetToBindingItRange ItRange;
            SetToBindingIt      It = Bindings.begin ();
            for (; It != Bindings.cend (); It = ItRange.second)
            {
                ItRange = Bindings.equal_range (It->first);

                const auto BindingCount = std::distance( ItRange.first, ItRange.second );
                SetLayoutBindings.reserve( static_cast< size_t >( BindingCount ) );

                std::for_each( ItRange.first, ItRange.second, [&]( SetToBindingPair const& Pair ) {
                    SetLayoutBindings.push_back( Pair.second );
                } );

                apemode::CityHash64Wrapper DescSetLayoutHashBuilder;
                DescSetLayoutHashBuilder.CombineWithArray( SetLayoutBindings.data( ), SetLayoutBindings.size( ) );

                if ( auto StoredDescSetLayout = Manager.GetDescSetLayout( DescSetLayoutHashBuilder.Value ) ) {
                    DescSetLayouts.push_back (StoredDescSetLayout);
                } else {
                    apemode::TInfoStruct< VkDescriptorSetLayoutCreateInfo > DescSetLayoutDesc;
                    apemode::AliasStructs( SetLayoutBindings, DescSetLayoutDesc->pBindings, DescSetLayoutDesc->bindingCount );

                    apemode::TDispatchableHandle< VkDescriptorSetLayout > TempDescSetLayout;
                    if ( !TempDescSetLayout.Recreate( GraphicsNode, DescSetLayoutDesc ) ) {
                        _Game_engine_Error( "Failed to create descriptor set layout." );
                        return false;
                    }

                    auto DestSetLayoutHandle = TempDescSetLayout.Release( );
                    Manager.SetDescSetLayout( DescSetLayoutHashBuilder.Value, DestSetLayoutHandle );
                    DescSetLayouts.push_back( DestSetLayoutHandle );
                }
            }
        }

        TInfoStruct< VkPipelineLayoutCreateInfo > PipelineLayoutDesc;
        apemode::AliasStructs( DescSetLayouts, PipelineLayoutDesc->pSetLayouts, PipelineLayoutDesc->setLayoutCount );

        if ( !TemporaryDesc.PushConstRanges.empty( ) ) {
            apemode::AliasStructs( TemporaryDesc.PushConstRanges,
                                   PipelineLayoutDesc->pPushConstantRanges,
                                   PipelineLayoutDesc->pushConstantRangeCount );
        }

        apemode::TDispatchableHandle< VkPipelineLayout > PipelineLayoutHandle;
        if ( !PipelineLayoutHandle.Recreate( GraphicsNode, PipelineLayoutDesc ) ) {
            return false;
        }

        if ( apemode_likely( PipelineLayoutHandle.IsNotNull( ) ) ) {
            auto pRootSign = new apemode::PipelineLayout( );

            pRootSign->DescSetLayouts.swap( DescSetLayouts );
            pRootSign->PipelineLayoutHandle.Swap( PipelineLayoutHandle );
            pRootSign->Hash  = Hash;
            pRootSign->pNode = &GraphicsNode;
            pRootSign->pDesc = PipelineLayoutDescription::MakeNewFromTemporary( TemporaryDesc );

            Manager.AddNewPipelineLayoutObject( *pRootSign );
            return pRootSign;
        }
    }

    return nullptr;
}

/// -------------------------------------------------------------------------------------------------------------------
/// PipelineLayoutManager PrivateContent
/// -------------------------------------------------------------------------------------------------------------------

struct apemode::PipelineLayoutManager::PrivateContent : public apemode::ScalableAllocPolicy,
                                                    public apemode::NoCopyAssignPolicy
{
    using HashType            = apemode::CityHash64Wrapper::ValueType;
    using HashOpLess          = apemode::CityHash64Wrapper::CmpOpLess;
    using HashOp              = apemode::CityHash64Wrapper::HashOp<>;
    using HashOpEqual         = apemode::CityHash64Wrapper::CmpOpEqual;
    using PipelineLayoutLookup = std::map<HashType, std::unique_ptr<apemode::PipelineLayout> >;
    using DescriptorSetLayoutLookup = std::map<HashType, VkDescriptorSetLayout>;

    std::mutex                 Lock;
    PipelineLayoutLookup       StoredRootSigns;
    DescriptorSetLayoutLookup StoredDescSetLayouts;
};

/// -------------------------------------------------------------------------------------------------------------------
/// PipelineLayoutManager
/// -------------------------------------------------------------------------------------------------------------------

apemode::PipelineLayoutManager::PipelineLayoutManager () : pContent (new PrivateContent ())
{
}

apemode::PipelineLayoutManager::~PipelineLayoutManager ()
{
    apemode::TSafeDeleteObj (pContent);
}

VkDescriptorSetLayout apemode::PipelineLayoutManager::GetDescSetLayout (uint64_t Hash)
{
    auto DescSetLayoutIt = pContent->StoredDescSetLayouts.find (Hash);
    if (DescSetLayoutIt != pContent->StoredDescSetLayouts.end ())
        return DescSetLayoutIt->second;

    return VK_NULL_HANDLE;
}

void apemode::PipelineLayoutManager::SetDescSetLayout (uint64_t Hash, VkDescriptorSetLayout SetLayout)
{
    _Game_engine_Assert (pContent->StoredDescSetLayouts.find (Hash)
                             == pContent->StoredDescSetLayouts.end (),
                         "Already stored, try to look-up for it first.");

    pContent->StoredDescSetLayouts[ Hash ] = SetLayout;
}

void apemode::PipelineLayoutManager::AddNewPipelineLayoutObject (apemode::PipelineLayout & RootSign)
{
    std::lock_guard<std::mutex> LockGuard (pContent->Lock);

    _Game_engine_Assert (pContent->StoredRootSigns.find (RootSign.Hash)
                             == pContent->StoredRootSigns.end (),
                         "See TryGetPipelineStateObjectByHash(...).");

    pContent->StoredRootSigns[ RootSign.Hash ].reset (&RootSign);
}

apemode::PipelineLayout const *
apemode::PipelineLayoutManager::TryGetPipelineLayoutObjectByHash (uint64_t Hash)
{
    std::lock_guard<std::mutex> LockGuard (pContent->Lock);

    auto RenderPassContentIt = pContent->StoredRootSigns.find (Hash);
    if (RenderPassContentIt != pContent->StoredRootSigns.end ())
    {
        return (*RenderPassContentIt).second.get ();
    }

    return nullptr;
}
