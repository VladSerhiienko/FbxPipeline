//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <Framebuffer.Vulkan.h>

#include <RenderTarget.Vulkan.h>
#include <RenderPass.Vulkan.h>
#include <CityHash.h>

/// -------------------------------------------------------------------------------------------------------------------
/// FramebufferManager PrivateContent
/// -------------------------------------------------------------------------------------------------------------------

struct Core::FramebufferManager::PrivateContent
    : public Aux::ScalableAllocPolicy
    , public Aux::NoCopyAssignPolicy
{
    typedef Aux::TSafeDeleteObjOp<Framebuffer > FBObjDeleter;
    typedef std::unique_ptr<Framebuffer, FBObjDeleter> FBObjUniquePtr;
    typedef Aux::CityHash64Wrapper::ValueType  HashType;
    typedef Aux::CityHash64Wrapper::CmpOpLess  HashOpLess;
    typedef std::map<HashType, FBObjUniquePtr, HashOpLess> FramebufferMap;

    //std::mutex      Lock;
    std::mutex Lock;
    FramebufferMap StoredFramebuffers;
};

/// -------------------------------------------------------------------------------------------------------------------
/// FramebufferManager
/// -------------------------------------------------------------------------------------------------------------------

Core::FramebufferManager::FramebufferManager()
    : Content(new PrivateContent())
{
}

Core::FramebufferManager::~FramebufferManager()
{
    Aux::TSafeDeleteObj(Content);
}

/// -------------------------------------------------------------------------------------------------------------------

void Core::FramebufferManager::AddNewFramebuffer(Core::Framebuffer & Framebuffer)
{
    //std::lock_guard<std::mutex> LockGuard(Content->Lock);
    std::lock_guard<std::mutex> LockGuard(Content->Lock);

    _Game_engine_Assert(Content->StoredFramebuffers.find(Framebuffer.Hash) == Content->StoredFramebuffers.end(),
                        "See TryGetFramebufferObjectByHash(...).");

    Content->StoredFramebuffers[Framebuffer.Hash].reset(&Framebuffer);
}

/// -------------------------------------------------------------------------------------------------------------------

Core::Framebuffer const * Core::FramebufferManager::TryGetFramebufferObjectByHash(uint64_t Hash)
{
    //std::lock_guard<std::mutex> LockGuard (Content->Lock);
    std::lock_guard<std::mutex> LockGuard(Content->Lock);

    auto RenderPassCotentIt = Content->StoredFramebuffers.find(Hash);
    if (RenderPassCotentIt != Content->StoredFramebuffers.end())
    {
        return (*RenderPassCotentIt).second.get();
    }

    return nullptr;
}

/// -------------------------------------------------------------------------------------------------------------------
/// FramebufferBuilder
/// -------------------------------------------------------------------------------------------------------------------

void Core::FramebufferBuilder::Attach (TextureResourceView const & TextureView)
{
    _Game_engine_Assert ((TextureView.CanGet<Core::ColorResourceView> ()
                          || TextureView.CanGet<Core::DepthStencilResourceView> ())
                             && (TextureView.Width != 0 && TextureView.Height != 0),
                         "Invalid texture view.");

    if (TextureView.CanGet<Core::ColorResourceView> ()
        || TextureView.CanGet<Core::DepthStencilResourceView> ())
    {
        if (TemporaryDesc.Desc->width == 0 &&
            TemporaryDesc.Desc->height == 0)
        {
            TemporaryDesc.Desc->width  = TextureView.Width;
            TemporaryDesc.Desc->height = TextureView.Height;
        }

        _Game_engine_Assert (TemporaryDesc.Desc->width == TextureView.Width
                                 && TemporaryDesc.Desc->height == TextureView.Height,
                             "Dimension mismatch.");
    }

    TemporaryDesc.TextureViews.push_back (&TextureView);
    ++TemporaryDesc.Desc->attachmentCount;
}

/// -------------------------------------------------------------------------------------------------------------------

void Core::FramebufferBuilder::SetRenderPass(RenderPass const & pRenderPass)
{
    TemporaryDesc.pRenderPass = &pRenderPass;
}

/// -------------------------------------------------------------------------------------------------------------------

Core::Framebuffer const * Core::FramebufferBuilder::RecreateFramebuffer(Core::GraphicsDevice & GraphicsNode)
{
    if (GraphicsNode.IsValid())
    {
        if (!TemporaryDesc.pRenderPass)
        {
            _Game_engine_Halt("Render pass was not set.");
            return nullptr;
        }

        auto ExtractTextureViewFn = [&](TextureResourceView const * pTextureView)
        {
            _Game_engine_Assert (pTextureView != nullptr, "Cannot be null.");
            return pTextureView ? pTextureView->ImgViewHandle.Handle : nullptr;
        };

        std::vector<VkImageView> ImgViews;
        ImgViews.reserve(TemporaryDesc.TextureViews.size());

        std::transform (TemporaryDesc.TextureViews.begin (),
                          TemporaryDesc.TextureViews.end (),
                          std::back_inserter (ImgViews),
                          ExtractTextureViewFn);

        Aux::AliasStructs (ImgViews,
                           TemporaryDesc.Desc->pAttachments,
                           TemporaryDesc.Desc->attachmentCount);

        TemporaryDesc.Desc->renderPass = *TemporaryDesc.pRenderPass;

        uint64_t Hash    = TemporaryDesc.UpdateHash ();
        auto &   Manager = GraphicsNode.GetDefaultFramebufferManager ();

        if (auto pStoredRenderPass = Manager.TryGetFramebufferObjectByHash (Hash))
            return pStoredRenderPass;

        TDispatchableHandle<VkFramebuffer> NewHandle;
        if (NewHandle.Recreate (GraphicsNode, TemporaryDesc.Desc))
        {
            auto pFramebuffer   = new Core::Framebuffer ();
            pFramebuffer->Hash  = Hash;
            pFramebuffer->pDesc = FramebufferDescription::MakeNewFromTemporary (TemporaryDesc);
            pFramebuffer->Handle.Swap (NewHandle);

            // Stores compiled framebuffers, so we could reuse them later
            Manager.AddNewFramebuffer (*pFramebuffer);
            return pFramebuffer;
        }
    }

    _Game_engine_Error ("Out of GPU or system memory.");
    return nullptr;
}

/// -------------------------------------------------------------------------------------------------------------------

void Core::FramebufferBuilder::Reset()
{
    TemporaryDesc.Reset();
}

/// -------------------------------------------------------------------------------------------------------------------
/// FramebufferDescription
/// -------------------------------------------------------------------------------------------------------------------

Core::FramebufferDescription::FramebufferDescription()
{
    Reset();
}

/// -------------------------------------------------------------------------------------------------------------------

void Core::FramebufferDescription::Reset()
{
    Hash        = 0;
    pRenderPass = nullptr;

    TextureViews.clear();
    TextureViews.reserve(8 + 1);

    Desc.ZeroMemory();
}

/// -------------------------------------------------------------------------------------------------------------------

uint64_t Core::FramebufferDescription::UpdateHash()
{
    VkFramebufferCreateInfo PartialDesc = Desc;

    auto pAttachments        = PartialDesc.pAttachments;
    PartialDesc.pAttachments = nullptr;

    auto HashWrapper = Aux::CityHash64Wrapper ();
    HashWrapper.CombineWith (PartialDesc);

    auto const pAttachmentIt    = pAttachments;
    auto const pAttachmentItEnd = pAttachments + PartialDesc.attachmentCount;
    HashWrapper.CombineWithArray (pAttachmentIt, pAttachmentItEnd);

    Hash = HashWrapper;
    return HashWrapper;
}

/// -------------------------------------------------------------------------------------------------------------------

Core::FramebufferDescription const *
Core::FramebufferDescription::MakeNewFromTemporary(FramebufferDescription const & TemporaryDesc)
{
    if (auto pFramebuffer = new Core::FramebufferDescription())
    {
        pFramebuffer->Hash        = TemporaryDesc.Hash;
        pFramebuffer->Desc        = TemporaryDesc.Desc;
        pFramebuffer->pRenderPass = TemporaryDesc.pRenderPass;

        std::copy (TemporaryDesc.TextureViews.begin (),
                     TemporaryDesc.TextureViews.end (),
                     std::back_inserter (pFramebuffer->TextureViews));

        return pFramebuffer;
    }

    _Game_engine_Error("Out of system memory.");
    return nullptr;
}
