//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <PipelineLayout.Vulkan.h>

#include <CityHash.h>

/// -------------------------------------------------------------------------------------------------------------------
/// PipelineLayoutParameter
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::PipelineLayoutParameter::PipelineLayoutParameter()
{
    Clear();
}

apemodevk::PipelineLayoutParameter::~PipelineLayoutParameter()
{
}

void apemodevk::PipelineLayoutParameter::Clear()
{
    Binding.ZeroMemory();
}

void apemodevk::PipelineLayoutParameter::InitAsUniformBuffer(uint32_t         Register,
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

void apemodevk::PipelineLayoutParameter::InitAsUniformBufferDynamic(uint32_t         Register,
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

void apemodevk::PipelineLayoutParameter::InitAsSampler(uint32_t         Register,
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

void apemodevk::PipelineLayoutParameter::InitAsCombinedImageSampler(
    uint32_t Register, uint32_t Count, VkSampler* pImmutableSamplers, VkShaderStageFlagBits StageFlags, uint32_t Set ) {
    DescSet                     = Set;
    Binding->binding            = Register;
    Binding->pImmutableSamplers = pImmutableSamplers;
    Binding->descriptorCount    = Count;
    Binding->descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    Binding->stageFlags         = static_cast<VkShaderStageFlagBits>(StageFlags);
}

apemodevk::PipelineLayoutParameter::operator VkDescriptorSetLayoutBinding() const
{
    return Binding;
}

/// -------------------------------------------------------------------------------------------------------------------
/// PipelineLayout
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::PipelineLayout::PipelineLayout()
    : pDesc(nullptr)
    , pNode(nullptr)
{
}

apemodevk::PipelineLayout::~PipelineLayout()
{
    if (apemode_likely (pNode != nullptr && !DescSetLayouts.empty ()))
    {
        std::for_each( DescSetLayouts.begin( ), DescSetLayouts.end( ), [&]( VkDescriptorSetLayout SetLayoutHandle ) {
            apemodevk::TDispatchableHandle< VkDescriptorSetLayout > DestroySetLayout( *pNode, SetLayoutHandle );
        } );
    }
}

apemodevk::PipelineLayout::operator VkPipelineLayout() const
{
    return PipelineLayoutHandle;
}

/// -------------------------------------------------------------------------------------------------------------------
/// PipelineLayoutDescription
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::PipelineLayoutDescription::PipelineLayoutDescription()
{
    Reset();
}

apemodevk::PipelineLayoutDescription::~PipelineLayoutDescription()
{
}

void apemodevk::PipelineLayoutDescription::Reset()
{
    Params.clear();
}

uint32_t apemodevk::PipelineLayoutDescription::GetParamCount() const
{
    return _Get_collection_length_u (Params);
}

apemodevk::PipelineLayoutParameter const &
apemodevk::PipelineLayoutDescription::GetParameter(uint32_t ParameterIndex) const
{
    _Game_engine_Assert (Params.size () < ParameterIndex, 
                         "Index is out of range.");

    return Params[ParameterIndex];
}

uint64_t apemodevk::PipelineLayoutDescription::UpdateHash()
{
    apemode::CityHash64Wrapper HashBuilder;
    HashBuilder.CombineWithArray (Params.data (), Params.size ());
    HashBuilder.CombineWithArray (PushConstRanges.data (), PushConstRanges.size ());

    Hash = HashBuilder.Value;
    return HashBuilder.Value;
}

apemodevk::PipelineLayoutDescription const *
apemodevk::PipelineLayoutDescription::MakeNewFromTemporary(PipelineLayoutDescription const & TemporaryDesc)
{
    auto pOutDesc = new apemodevk::PipelineLayoutDescription();

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

apemodevk::PipelineLayoutBuilder::PipelineLayoutBuilder()
{
}

void apemodevk::PipelineLayoutBuilder::Reset (uint32_t BindingCount, uint32_t PushConstRangeCount)
{
    TemporaryDesc.Params.clear ();
    TemporaryDesc.Params.reserve (BindingCount);
    TemporaryDesc.PushConstRanges.clear ();
    TemporaryDesc.PushConstRanges.reserve (PushConstRangeCount);
}

apemodevk::PipelineLayoutParameter & apemodevk::PipelineLayoutBuilder::AddParameter()
{
    return apemodevk::PushBackAndGet( TemporaryDesc.Params);
}

VkPushConstantRange & apemodevk::PipelineLayoutBuilder::AddPushConstRange()
{
    return apemodevk::PushBackAndGet(TemporaryDesc.PushConstRanges);
}

apemodevk::PipelineLayout const *
apemodevk::PipelineLayoutBuilder::RecreatePipelineLayout(apemodevk::GraphicsDevice & GraphicsNode)
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
                           [&]( apemodevk::PipelineLayoutParameter const& Param ) {
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
                    apemodevk::TInfoStruct< VkDescriptorSetLayoutCreateInfo > DescSetLayoutDesc;
                    apemodevk::AliasStructs( SetLayoutBindings, DescSetLayoutDesc->pBindings, DescSetLayoutDesc->bindingCount );

                    apemodevk::TDispatchableHandle< VkDescriptorSetLayout > TempDescSetLayout;
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
        apemodevk::AliasStructs( DescSetLayouts, PipelineLayoutDesc->pSetLayouts, PipelineLayoutDesc->setLayoutCount );

        if ( !TemporaryDesc.PushConstRanges.empty( ) ) {
            apemodevk::AliasStructs( TemporaryDesc.PushConstRanges,
                                   PipelineLayoutDesc->pPushConstantRanges,
                                   PipelineLayoutDesc->pushConstantRangeCount );
        }

        apemodevk::TDispatchableHandle< VkPipelineLayout > PipelineLayoutHandle;
        if ( !PipelineLayoutHandle.Recreate( GraphicsNode, PipelineLayoutDesc ) ) {
            return false;
        }

        if ( apemode_likely( PipelineLayoutHandle.IsNotNull( ) ) ) {
            auto pRootSign = new apemodevk::PipelineLayout( );

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

struct apemodevk::PipelineLayoutManager::PrivateContent : public apemodevk::ScalableAllocPolicy,
                                                    public apemodevk::NoCopyAssignPolicy
{
    using HashType            = apemode::CityHash64Wrapper::ValueType;
    using HashOpLess          = apemode::CityHash64Wrapper::CmpOpLess;
    using HashOp              = apemode::CityHash64Wrapper::HashOp<>;
    using HashOpEqual         = apemode::CityHash64Wrapper::CmpOpEqual;
    using PipelineLayoutLookup = std::map<HashType, std::unique_ptr<apemodevk::PipelineLayout> >;
    using DescriptorSetLayoutLookup = std::map<HashType, VkDescriptorSetLayout>;

    std::mutex                 Lock;
    PipelineLayoutLookup       StoredRootSigns;
    DescriptorSetLayoutLookup StoredDescSetLayouts;
};

/// -------------------------------------------------------------------------------------------------------------------
/// PipelineLayoutManager
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::PipelineLayoutManager::PipelineLayoutManager () : pContent (new PrivateContent ())
{
}

apemodevk::PipelineLayoutManager::~PipelineLayoutManager ()
{
    apemodevk::TSafeDeleteObj (pContent);
}

VkDescriptorSetLayout apemodevk::PipelineLayoutManager::GetDescSetLayout (uint64_t Hash)
{
    auto DescSetLayoutIt = pContent->StoredDescSetLayouts.find (Hash);
    if (DescSetLayoutIt != pContent->StoredDescSetLayouts.end ())
        return DescSetLayoutIt->second;

    return VK_NULL_HANDLE;
}

void apemodevk::PipelineLayoutManager::SetDescSetLayout (uint64_t Hash, VkDescriptorSetLayout SetLayout)
{
    _Game_engine_Assert (pContent->StoredDescSetLayouts.find (Hash)
                             == pContent->StoredDescSetLayouts.end (),
                         "Already stored, try to look-up for it first.");

    pContent->StoredDescSetLayouts[ Hash ] = SetLayout;
}

void apemodevk::PipelineLayoutManager::AddNewPipelineLayoutObject (apemodevk::PipelineLayout & RootSign)
{
    std::lock_guard<std::mutex> LockGuard (pContent->Lock);

    _Game_engine_Assert (pContent->StoredRootSigns.find (RootSign.Hash)
                             == pContent->StoredRootSigns.end (),
                         "See TryGetPipelineStateObjectByHash(...).");

    pContent->StoredRootSigns[ RootSign.Hash ].reset (&RootSign);
}

apemodevk::PipelineLayout const *
apemodevk::PipelineLayoutManager::TryGetPipelineLayoutObjectByHash (uint64_t Hash)
{
    std::lock_guard<std::mutex> LockGuard (pContent->Lock);

    auto RenderPassContentIt = pContent->StoredRootSigns.find (Hash);
    if (RenderPassContentIt != pContent->StoredRootSigns.end ())
    {
        return (*RenderPassContentIt).second.get ();
    }

    return nullptr;
}
