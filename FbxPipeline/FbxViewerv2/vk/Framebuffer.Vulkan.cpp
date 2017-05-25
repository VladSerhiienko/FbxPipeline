//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <Framebuffer.Vulkan.h>

#include <RenderTarget.Vulkan.h>
#include <RenderPass.Vulkan.h>
#include <CityHash.h>

/// -------------------------------------------------------------------------------------------------------------------
/// FramebufferManager PrivateContent
/// -------------------------------------------------------------------------------------------------------------------

struct apemode::FramebufferManager::PrivateContent
    : public apemode::ScalableAllocPolicy
    , public apemode::NoCopyAssignPolicy
{
    typedef apemode::TSafeDeleteObjOp<Framebuffer > FBObjDeleter;
    typedef std::unique_ptr<Framebuffer, FBObjDeleter> FBObjUniquePtr;
    typedef apemode::CityHash64Wrapper::ValueType  HashType;
    typedef apemode::CityHash64Wrapper::CmpOpLess  HashOpLess;
    typedef std::map<HashType, FBObjUniquePtr, HashOpLess> FramebufferMap;

    //std::mutex      Lock;
    std::mutex Lock;
    FramebufferMap StoredFramebuffers;
};

/// -------------------------------------------------------------------------------------------------------------------
/// FramebufferManager
/// -------------------------------------------------------------------------------------------------------------------

apemode::FramebufferManager::FramebufferManager()
    : Content(new PrivateContent())
{
}

apemode::FramebufferManager::~FramebufferManager()
{
    apemode::TSafeDeleteObj(Content);
}

/// -------------------------------------------------------------------------------------------------------------------

void apemode::FramebufferManager::AddNewFramebuffer(apemode::Framebuffer & Framebuffer)
{
    //std::lock_guard<std::mutex> LockGuard(Content->Lock);
    std::lock_guard<std::mutex> LockGuard(Content->Lock);

    _Game_engine_Assert(Content->StoredFramebuffers.find(Framebuffer.Hash) == Content->StoredFramebuffers.end(),
                        "See TryGetFramebufferObjectByHash(...).");

    Content->StoredFramebuffers[Framebuffer.Hash].reset(&Framebuffer);
}

/// -------------------------------------------------------------------------------------------------------------------

apemode::Framebuffer const * apemode::FramebufferManager::TryGetFramebufferObjectByHash(uint64_t Hash)
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

void apemode::FramebufferBuilder::Attach (TextureResourceView const & TextureView)
{
    _Game_engine_Assert ((TextureView.CanGet<apemode::ColorResourceView> ()
                          || TextureView.CanGet<apemode::DepthStencilResourceView> ())
                             && (TextureView.Width != 0 && TextureView.Height != 0),
                         "Invalid texture view.");

    if (TextureView.CanGet<apemode::ColorResourceView> ()
        || TextureView.CanGet<apemode::DepthStencilResourceView> ())
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

void apemode::FramebufferBuilder::SetRenderPass(RenderPass const & pRenderPass)
{
    TemporaryDesc.pRenderPass = &pRenderPass;
}

/// -------------------------------------------------------------------------------------------------------------------

apemode::Framebuffer const * apemode::FramebufferBuilder::RecreateFramebuffer(apemode::GraphicsDevice & GraphicsNode)
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

        apemode::AliasStructs (ImgViews,
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
            auto pFramebuffer   = new apemode::Framebuffer ();
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

void apemode::FramebufferBuilder::Reset()
{
    TemporaryDesc.Reset();
}

/// -------------------------------------------------------------------------------------------------------------------
/// FramebufferDescription
/// -------------------------------------------------------------------------------------------------------------------

apemode::FramebufferDescription::FramebufferDescription()
{
    Reset();
}

/// -------------------------------------------------------------------------------------------------------------------

void apemode::FramebufferDescription::Reset()
{
    Hash        = 0;
    pRenderPass = nullptr;

    TextureViews.clear();
    TextureViews.reserve(8 + 1);

    Desc.ZeroMemory();
}

/// -------------------------------------------------------------------------------------------------------------------

uint64_t apemode::FramebufferDescription::UpdateHash()
{
    VkFramebufferCreateInfo PartialDesc = Desc;

    auto pAttachments        = PartialDesc.pAttachments;
    PartialDesc.pAttachments = nullptr;

    auto HashWrapper = apemode::CityHash64Wrapper ();
    HashWrapper.CombineWith (PartialDesc);

    auto const pAttachmentIt    = pAttachments;
    auto const pAttachmentItEnd = pAttachments + PartialDesc.attachmentCount;
    HashWrapper.CombineWithArray (pAttachmentIt, pAttachmentItEnd);

    Hash = HashWrapper;
    return HashWrapper;
}

/// -------------------------------------------------------------------------------------------------------------------

apemode::FramebufferDescription const *
apemode::FramebufferDescription::MakeNewFromTemporary(FramebufferDescription const & TemporaryDesc)
{
    if (auto pFramebuffer = new apemode::FramebufferDescription())
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
