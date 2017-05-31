//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <RenderPass.Vulkan.h>

#include <DepthStencil.Vulkan.h>
#include <RenderTarget.Vulkan.h>

/// -------------------------------------------------------------------------------------------------------------------

union ExposedSwapchainAttachment {
    struct
    {
        uint32_t AttachmentId;
        uint32_t SwapchainId;
    };

    uint64_t AttachmentHash;
};

/// -------------------------------------------------------------------------------------------------------------------
/// RenderPassBuilder
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::RenderPassBuilder::RenderPassBuilder()
{
}

void apemodevk::RenderPassBuilder::Reset (uint32_t MaxAttachments,
                                     uint32_t MaxDependencies,
                                     uint32_t MaxSwaphchainAttachments)
{
    TemporaryDesc.Reset();
    TemporaryDesc.Attachments.reserve(MaxAttachments);
    TemporaryDesc.SubpassDependencies.reserve(MaxDependencies);
    TemporaryDesc.SwapchainAttachmentHashes.reserve (MaxSwaphchainAttachments);
}

/// -------------------------------------------------------------------------------------------------------------------

apemodevk::RenderPassDescription::SubpassDescription &
apemodevk::RenderPassBuilder::GetOrCreateSubpass(uint32_t SubpassId)
{
    auto const SubpassIt    = TemporaryDesc.SubpassDescriptions.find(SubpassId);
    auto const SubpassItEnd = TemporaryDesc.SubpassDescriptions.end();

    if (SubpassIt != SubpassItEnd)
    {
        return *(*SubpassIt).second;
    }

    auto NewSubpassIt = TemporaryDesc.SubpassDescriptions.insert(std::make_pair(
        SubpassId, RenderPassDescription::SubpassDescription::MakeNewLinked(SubpassId)));
    NewSubpassIt.first->second->Id = SubpassId;
    return *NewSubpassIt.first->second;
}

/// -------------------------------------------------------------------------------------------------------------------

void apemodevk::RenderPassBuilder::ResetSubpass (uint32_t SubpassId,
                                            uint32_t MaxColors,
                                            uint32_t MaxInputs,
                                            uint32_t MaxPreserves)
{
    auto & Subpass = GetOrCreateSubpass (SubpassId);

    Subpass.ColorRefs.reserve (MaxColors);
    Subpass.ResolveRefs.reserve (MaxColors);
    Subpass.InputRefs.reserve (MaxInputs);
    Subpass.PreserveIndices.reserve (MaxPreserves);
    Subpass.DepthStencilRef.layout = VK_IMAGE_LAYOUT_MAX_ENUM;
}

uint32_t apemodevk::RenderPassBuilder::AddAttachment (VkFormat            ImgFmt,
                                                 VkSampleCountFlags  SampleCount,
                                                 VkImageLayout       ImgInitialLayout,
                                                 VkImageLayout       ImgFinalLayout,
                                                 VkAttachmentLoadOp  ImgLoadOp,
                                                 VkAttachmentStoreOp ImgStoreOp,
                                                 bool                bImgMayAlias,
                                                 uint32_t            SwapchainId)
{
    const auto OutId      = _Get_collection_length_u (TemporaryDesc.Attachments);

    TemporaryDesc.Attachments.push_back( VkAttachmentDescription{} );
    auto& Attachment = TemporaryDesc.Attachments.back( );

    Attachment.format         = ImgFmt;
    Attachment.samples        = static_cast<VkSampleCountFlagBits> (SampleCount);
    Attachment.initialLayout  = ImgInitialLayout;
    Attachment.finalLayout    = ImgFinalLayout;
    Attachment.loadOp         = ImgLoadOp;
    Attachment.storeOp        = ImgStoreOp;
    Attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    Attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    if (bImgMayAlias)
        Attachment.flags |= VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;

    if (SwapchainId != sInvalidSwapchainId)
    {
        ExposedSwapchainAttachment ExposedAttachment;
        ExposedAttachment.AttachmentId      = OutId;
        ExposedAttachment.SwapchainId       = SwapchainId;
        TemporaryDesc.SwapchainAttachmentHashes.push_back (ExposedAttachment.AttachmentHash);
    }

    return OutId;
}

uint32_t apemodevk::RenderPassBuilder::AddAttachment (VkFormat            ImgFmt,
                                                 VkSampleCountFlags  ImgSampleCount,
                                                 VkImageLayout       ImgInitialLayout,
                                                 VkImageLayout       ImgFinalLayout,
                                                 VkAttachmentLoadOp  DepthLoadOp,
                                                 VkAttachmentStoreOp DepthStoreOp,
                                                 VkAttachmentLoadOp  StencilLoadOp,
                                                 VkAttachmentStoreOp StencilStoreOp,
                                                 bool                bImgMayAlias)
{
    uint32_t const OutId = AddAttachment (ImgFmt,
                                          ImgSampleCount,
                                          ImgInitialLayout,
                                          ImgFinalLayout,
                                          DepthLoadOp,
                                          DepthStoreOp,
                                          bImgMayAlias,
                                          false);

    auto & Attachment         = TemporaryDesc.Attachments[ OutId ];
    Attachment.stencilLoadOp  = StencilLoadOp;
    Attachment.stencilStoreOp = StencilStoreOp;

    return OutId;
}

void apemodevk::RenderPassBuilder::AddColorToSubpass (uint32_t      SubpassId,
                                                 uint32_t      AttachmentId,
                                                 VkImageLayout ImgSubpassLayout)
{
    auto & Subpass    = GetOrCreateSubpass (SubpassId);
    auto & ColorRef   = apemodevk::PushBackAndGet( Subpass.ColorRefs);
    auto & ResolveRef = apemodevk::PushBackAndGet(Subpass.ResolveRefs);

    ColorRef.attachment = AttachmentId;
    ColorRef.layout     = ImgSubpassLayout;

    ResolveRef.attachment = VK_ATTACHMENT_UNUSED;
    ResolveRef.layout     = VK_IMAGE_LAYOUT_UNDEFINED;
}

void apemodevk::RenderPassBuilder::AddColorToSubpass (uint32_t      SubpassId,
                                                 uint32_t      AttachmentId,
                                                 VkImageLayout ImgSubpassLayout,
                                                 uint32_t      ResolveAttachmentId,
                                                 VkImageLayout ResolveImgSubpassLayout)
{
    auto & Subpass    = GetOrCreateSubpass (SubpassId);
    auto & ColorRef   = apemodevk::PushBackAndGet(Subpass.ColorRefs);
    auto & ResolveRef = apemodevk::PushBackAndGet(Subpass.ResolveRefs);

    ColorRef.attachment = AttachmentId;
    ColorRef.layout     = ImgSubpassLayout;

    ResolveRef.attachment = ResolveAttachmentId;
    ResolveRef.layout     = ResolveImgSubpassLayout;
}

void apemodevk::RenderPassBuilder::AddInputToSubpass (uint32_t      SubpassId,
                                                 uint32_t      AttachmentId,
                                                 VkImageLayout ImgSubpassLayout)
{
    auto & Subpass  = GetOrCreateSubpass (SubpassId);
    auto & InputRef = apemodevk::PushBackAndGet(Subpass.InputRefs);

    InputRef.attachment = AttachmentId;
    InputRef.layout     = ImgSubpassLayout;
}

void apemodevk::RenderPassBuilder::SetDepthToSubpass (uint32_t      SubpassId,
                                                 uint32_t      AttachmentId,
                                                 VkImageLayout ImgSubpassLayout)
{
    auto & Subpass = GetOrCreateSubpass (SubpassId);

    Subpass.DepthStencilRef.attachment = AttachmentId;
    Subpass.DepthStencilRef.layout     = ImgSubpassLayout;
}

void apemodevk::RenderPassBuilder::PreserveInSubpass (uint32_t SubpassId, uint32_t AttachmentId)
{
    auto & Subpass = GetOrCreateSubpass (SubpassId);

    Subpass.PreserveIndices.push_back (AttachmentId);
}

void apemodevk::RenderPassBuilder::SetSubpassDependency(uint32_t             SrcSubpassId,
                                                   VkPipelineStageFlags SrcSubpassStage,
                                                   VkAccessFlags        SrcSubpassAccess,
                                                   uint32_t             DstSubpassId,
                                                   VkPipelineStageFlags DstSubpassStage,
                                                   VkAccessFlags        DstSubpassAccess,
                                                   bool                 bDependentByRegion)
{
    auto & SubpassDependency = apemodevk::PushBackAndGet(TemporaryDesc.SubpassDependencies);

    SubpassDependency               = TInfoStruct<VkSubpassDependency> ();
    SubpassDependency.srcSubpass    = SrcSubpassId;
    SubpassDependency.srcStageMask  = SrcSubpassStage;
    SubpassDependency.srcAccessMask = SrcSubpassAccess;
    SubpassDependency.dstSubpass    = DstSubpassId;
    SubpassDependency.dstStageMask  = DstSubpassStage;
    SubpassDependency.dstAccessMask = DstSubpassAccess;

    if (bDependentByRegion)
        SubpassDependency.dependencyFlags |= VK_DEPENDENCY_BY_REGION_BIT;
}

apemodevk::RenderPass const *
apemodevk::RenderPassBuilder::RecreateRenderPass(apemodevk::GraphicsDevice & GraphicsNode)
{
    using SubpassIdPair = std::pair<uint32_t, apemodevk::RenderPassDescription::SubpassDescription::LkPtr>;

    if (GraphicsNode.IsValid() && VerifySubpasses())
    {
        // This function transforms my custom subpass description
        // to a native Vulkan type.
        auto SubpassDescNativeTransformerFn = [&](SubpassIdPair SubpassPair) {
            auto &     Desc           = *SubpassPair.second;
            const auto InputCount     = _Get_collection_length_u (Desc.InputRefs);
            const auto ColorCount     = _Get_collection_length_u (Desc.ColorRefs);
            const auto PreservedCount = _Get_collection_length_u (Desc.PreserveIndices);

            TInfoStruct<VkSubpassDescription> OutDesc;

            OutDesc->inputAttachmentCount    = InputCount;
            OutDesc->pInputAttachments       = Desc.InputRefs.data();
            OutDesc->colorAttachmentCount    = ColorCount;
            OutDesc->pColorAttachments       = Desc.ColorRefs.data();
            OutDesc->pResolveAttachments     = Desc.ResolveRefs.data();
            OutDesc->pDepthStencilAttachment = &Desc.DepthStencilRef;
            OutDesc->preserveAttachmentCount = PreservedCount;
            OutDesc->pPreserveAttachments    = Desc.PreserveIndices.data();

            return OutDesc;
        };

        // Allocate enough memory and fill out native Vulkan subpasses.
        std::vector<VkSubpassDescription> Subpasses;
        Subpasses.reserve(TemporaryDesc.SubpassDescriptions.size());
        std::transform(TemporaryDesc.SubpassDescriptions.begin(),
                         TemporaryDesc.SubpassDescriptions.end(),
                         std::back_inserter(Subpasses),
                         SubpassDescNativeTransformerFn);

        //TODO Fill render pass create info...

        apemodevk::AliasStructs (TemporaryDesc.Attachments,
                           TemporaryDesc.Desc->pAttachments,
                           TemporaryDesc.Desc->attachmentCount);

        apemodevk::AliasStructs(Subpasses,
                          TemporaryDesc.Desc->pSubpasses,
                          TemporaryDesc.Desc->subpassCount);

        auto   Hash    = TemporaryDesc.UpdateHash();
        auto & Manager = GraphicsNode.GetDefaultRenderPassManager();

        if (auto pExistingRenderPass = Manager.TryGetRenderPassObjectByHash(Hash))
            return pExistingRenderPass;

        TDispatchableHandle<VkRenderPass> NewHandle;
        if (NewHandle.Recreate(GraphicsNode, TemporaryDesc.Desc))
        {
            auto pRenderPass = new apemodevk::RenderPass();
            pRenderPass->Handle.Swap(NewHandle);
            pRenderPass->Hash          = Hash;
            pRenderPass->pNode = &GraphicsNode;
            pRenderPass->pDesc         = RenderPassDescription::MakeNewFromTemporary(TemporaryDesc);

            // Stores compiled render passes, so we could reuse them later
            Manager.AddNewRenderPassObject(*pRenderPass);
            return pRenderPass;
        }
    }

    return nullptr;
}

/// -------------------------------------------------------------------------------------------------------------------

bool apemodevk::RenderPassBuilder::VerifySubpasses() const
{
    if (!TemporaryDesc.SubpassDescriptions.empty())
    {
        size_t MaxSubpassIndex = 0;
        for (auto const & SubpassPair : TemporaryDesc.SubpassDescriptions)
        {
            auto const CurrSubpassIndex = static_cast<size_t>(SubpassPair.second->Id);
            MaxSubpassIndex = std::max(CurrSubpassIndex, MaxSubpassIndex);
        }

        const size_t LastExpectedSubpassIndex = TemporaryDesc.SubpassDescriptions.size() - 1;
        return LastExpectedSubpassIndex == MaxSubpassIndex;
    }

    _Game_engine_Error("No subpasses.");
    return false;
}

/// -------------------------------------------------------------------------------------------------------------------
/// RenderPassDescription SubpassDescription
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::RenderPassDescription::SubpassDescription::SubpassDescription ( uint32_t Id ) : Id ( Id )
{
    DepthStencilRef.layout = VK_IMAGE_LAYOUT_MAX_ENUM;
}

bool apemodevk::RenderPassDescription::SubpassDescription::HasDepthStencilRef () const
{
    return DepthStencilRef.layout == VK_IMAGE_LAYOUT_MAX_ENUM;
}

apemodevk::RenderPassDescription::SubpassDescription::UqPtr
apemodevk::RenderPassDescription::SubpassDescription::MakeNewUnique (uint32_t SubpassId)
{
    using Deleter = apemodevk::RenderPassDescription::SubpassDescription::UqPtr::deleter_type;
    return std::make_unique<SubpassDescription> (SubpassId);
}

apemodevk::RenderPassDescription::SubpassDescription::LkPtr
apemodevk::RenderPassDescription::SubpassDescription::MakeNewLinked(uint32_t SubpassId)
{
    using Deleter = apemodevk::RenderPassDescription::SubpassDescription::UqPtr::deleter_type;
    return std::make_shared<SubpassDescription> (SubpassId);
}

/// -------------------------------------------------------------------------------------------------------------------
/// RenderPassManager PrivateContent
/// -------------------------------------------------------------------------------------------------------------------

struct apemodevk::RenderPassManager::PrivateContent : public apemodevk::ScalableAllocPolicy,
                                                 public apemodevk::NoCopyAssignPolicy
{
    using HashType         = apemode::CityHash64Wrapper::ValueType;
    using RenderPassLookup = std::map<HashType, std::unique_ptr<RenderPass> >;

    std::mutex        Lock;
    RenderPassLookup StoredRenderPasses;
};

/// -------------------------------------------------------------------------------------------------------------------
/// RenderPassDescription
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::RenderPassDescription::RenderPassDescription () : Hash (0)
{
    Reset ();
}

void apemodevk::RenderPassDescription::Reset ()
{
    Hash = 0;
    Desc.ZeroMemory ();
    Attachments.clear ();
    SubpassDescriptions.clear ();
    SubpassDependencies.clear();
    SwapchainAttachmentHashes.clear ();
}

uint64_t apemodevk::RenderPassDescription::UpdateHash()
{
    VkRenderPassCreateInfo PartialDesc = Desc;

    auto pSubpasses    = PartialDesc.pSubpasses;
    auto pAttachments  = PartialDesc.pAttachments;
    auto pDependencies = PartialDesc.pDependencies;

    PartialDesc.pSubpasses    = nullptr;
    PartialDesc.pAttachments  = nullptr;
    PartialDesc.pDependencies = nullptr;

    auto HashWrapper = apemode::CityHash64Wrapper ();
    HashWrapper.CombineWith (PartialDesc);

    if (apemode_likely (PartialDesc.attachmentCount != 0))
    {
        auto const pAttachmentIt    = pAttachments;
        auto const pAttachmentItEnd = pAttachments + PartialDesc.attachmentCount;
        HashWrapper.CombineWithArray(pAttachmentIt, pAttachmentItEnd);
    }

    if (apemode_likely (PartialDesc.dependencyCount != 0))
    {
        auto const pDependencyIt    = pDependencies;
        auto const pDependencyItEnd = pDependencies + PartialDesc.dependencyCount;
        HashWrapper.CombineWithArray(pDependencyIt, pDependencyItEnd);
    }

    if (PartialDesc.subpassCount)
    {
        auto const pSubpassIt    = pSubpasses;
        auto const pSubpassItEnd = pSubpasses + PartialDesc.subpassCount;
        std::for_each (
            pSubpassIt, pSubpassItEnd, [this, &HashWrapper](VkSubpassDescription const & Desc)
            {
                VkSubpassDescription PartialDesc = Desc;

                auto pInputAttachments    = Desc.pInputAttachments;
                auto pColorAttachments    = Desc.pColorAttachments;
                auto pResolveAttachments  = Desc.pResolveAttachments;
                auto pPreserveAttachments = Desc.pPreserveAttachments;

                PartialDesc.pInputAttachments    = nullptr;
                PartialDesc.pColorAttachments    = nullptr;
                PartialDesc.pResolveAttachments  = nullptr;
                PartialDesc.pPreserveAttachments = nullptr;

                HashWrapper.CombineWith(PartialDesc);

                if (Desc.pDepthStencilAttachment)
                    HashWrapper.CombineWith(*Desc.pDepthStencilAttachment);

                if (pInputAttachments)
                {
                    auto const It    = pInputAttachments;
                    auto const ItEnd = pInputAttachments + PartialDesc.inputAttachmentCount;
                    HashWrapper.CombineWithArray(It, ItEnd);
                }

                if (pColorAttachments)
                {
                    auto const It    = pColorAttachments;
                    auto const ItEnd = pColorAttachments + PartialDesc.colorAttachmentCount;
                    HashWrapper.CombineWithArray(It, ItEnd);
                }

                if (pResolveAttachments)
                {
                    auto const It    = pResolveAttachments;
                    auto const ItEnd = pResolveAttachments + PartialDesc.colorAttachmentCount;
                    HashWrapper.CombineWithArray(It, ItEnd);
                }

                if (pPreserveAttachments)
                {
                    auto const It    = pPreserveAttachments;
                    auto const ItEnd = pPreserveAttachments + PartialDesc.preserveAttachmentCount;
                    HashWrapper.CombineWithArray(It, ItEnd);
                }
            });
    }

    Hash = HashWrapper;
    return HashWrapper;
}

bool apemodevk::RenderPassDescription::GetSwapchainAttachmentInfo (uint32_t   AttachmentId,
                                                              uint32_t & OutSwapchainId) const
{
    if (!SwapchainAttachmentHashes.empty ())
        for (auto & SwapchainAttachmentHash : SwapchainAttachmentHashes)
        {
            ExposedSwapchainAttachment SwapchainAttachment;
            SwapchainAttachment.AttachmentHash = SwapchainAttachmentHash;
            if (SwapchainAttachment.AttachmentId == AttachmentId)
            {
                OutSwapchainId = SwapchainAttachment.SwapchainId;
                return true;
            }
        }

    return false;
}

/// -------------------------------------------------------------------------------------------------------------------

apemodevk::RenderPassDescription const *
apemodevk::RenderPassDescription::MakeNewFromTemporary(RenderPassDescription const & TemporaryDesc)
{
    if (auto pNewDesc = new apemodevk::RenderPassDescription())
    {
        pNewDesc->Hash = TemporaryDesc.Hash;

        // Description remains the same, basically.
        // The attachement & subpass must be copied
        // The attachement & subpass pointers should be updated.
        pNewDesc->Desc = TemporaryDesc.Desc;

        if (!TemporaryDesc.Attachments.empty ())
        {
            pNewDesc->Attachments.reserve (TemporaryDesc.Attachments.size ());
            std::copy (TemporaryDesc.Attachments.begin (),
                         TemporaryDesc.Attachments.end (),
                         std::back_inserter (pNewDesc->Attachments));

            apemodevk::AliasStructs (pNewDesc->Attachments,
                               pNewDesc->Desc->pAttachments,
                               pNewDesc->Desc->attachmentCount);
        }

        if (!TemporaryDesc.SubpassDescriptions.empty ())
        {
            //pNewDesc->SubpassDescriptions.reserve (TemporaryDesc.SubpassDescriptions.size ());
            /*std::copy (TemporaryDesc.SubpassDescriptions.begin (),
                         TemporaryDesc.SubpassDescriptions.end (),
                         std::back_inserter (pNewDesc->SubpassDescriptions));*/
            pNewDesc->SubpassDescriptions = TemporaryDesc.SubpassDescriptions;

            // Note: Here we set pSubpasses to nullptr, as we maintain separate custom type
            //       describing subpasses within render pass (cannot be aliased).
            pNewDesc->Desc->pSubpasses   = nullptr;
            pNewDesc->Desc->subpassCount = _Get_collection_length_u (pNewDesc->SubpassDescriptions);
        }

        if (!TemporaryDesc.SubpassDependencies.empty ())
        {
            pNewDesc->SubpassDependencies.reserve (TemporaryDesc.SubpassDependencies.size ());
            std::copy (TemporaryDesc.SubpassDependencies.begin (),
                         TemporaryDesc.SubpassDependencies.end (),
                         std::back_inserter (pNewDesc->SubpassDependencies));

            apemodevk::AliasStructs (pNewDesc->SubpassDependencies,
                               pNewDesc->Desc->pDependencies,
                               pNewDesc->Desc->dependencyCount);
        }

        if (!TemporaryDesc.SwapchainAttachmentHashes.empty ())
        {
            pNewDesc->SwapchainAttachmentHashes.reserve (TemporaryDesc.SwapchainAttachmentHashes.size ());
            std::copy (TemporaryDesc.SwapchainAttachmentHashes.begin (),
                         TemporaryDesc.SwapchainAttachmentHashes.end (),
                         std::back_inserter (pNewDesc->SwapchainAttachmentHashes));
        }

        return pNewDesc;
    }

    _Game_engine_Error("Out of system memory.");
    return nullptr;
}

/// -------------------------------------------------------------------------------------------------------------------
/// RenderPassManager
/// -------------------------------------------------------------------------------------------------------------------

void apemodevk::RenderPassManager::AddNewRenderPassObject(apemodevk::RenderPass & NewRenderPass)
{
    std::lock_guard<std::mutex> LockGuard (pContent->Lock);

    _Game_engine_Assert (pContent->StoredRenderPasses.find (NewRenderPass.Hash)
                             == pContent->StoredRenderPasses.end (),
                         "Already exists, please see TryGetRenderPassObjectByHash function.");

    pContent->StoredRenderPasses[NewRenderPass.Hash].reset (&NewRenderPass);
}

/// -------------------------------------------------------------------------------------------------------------------

apemodevk::RenderPassManager::RenderPassManager()
    : pContent(new PrivateContent())
{
}

apemodevk::RenderPassManager::~RenderPassManager()
{
    apemodevk::TSafeDeleteObj (pContent);
}

apemodevk::RenderPass const *
apemodevk::RenderPassManager::TryGetRenderPassObjectByHash(uint64_t Hash)
{
    std::lock_guard<std::mutex> LockGuard (pContent->Lock);

    auto RenderPassCotentIt = pContent->StoredRenderPasses.find(Hash);
    if (RenderPassCotentIt != pContent->StoredRenderPasses.end ())
        return (*RenderPassCotentIt).second.get();

    return nullptr;
}

apemodevk::RenderPass::RenderPass () : pNode (nullptr), pDesc (nullptr), Hash (0)
{
}

apemodevk::RenderPass::operator VkRenderPass () const
{
    return Handle;
}
