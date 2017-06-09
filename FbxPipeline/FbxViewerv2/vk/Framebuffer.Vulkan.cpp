//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <Framebuffer.Vulkan.h>

#include <RenderTarget.Vulkan.h>
#include <RenderPass.Vulkan.h>
#include <CityHash.h>

/// -------------------------------------------------------------------------------------------------------------------
/// FramebufferManager PrivateContent
/// -------------------------------------------------------------------------------------------------------------------

struct apemodevk::FramebufferManager::PrivateContent
    : public apemodevk::ScalableAllocPolicy
    , public apemodevk::NoCopyAssignPolicy
{
    typedef apemodevk::TSafeDeleteObjOp<Framebuffer > FBObjDeleter;
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

apemodevk::FramebufferManager::FramebufferManager()
    : Content(new PrivateContent())
{
}

apemodevk::FramebufferManager::~FramebufferManager()
{
    apemodevk::TSafeDeleteObj(Content);
}

/// -------------------------------------------------------------------------------------------------------------------

void apemodevk::FramebufferManager::AddNewFramebuffer(apemodevk::Framebuffer & Framebuffer)
{
    //std::lock_guard<std::mutex> LockGuard(Content->Lock);
    std::lock_guard<std::mutex> LockGuard(Content->Lock);

    apemode_assert(Content->StoredFramebuffers.find(Framebuffer.Hash) == Content->StoredFramebuffers.end(),
                        "See TryGetFramebufferObjectByHash(...).");

    Content->StoredFramebuffers[Framebuffer.Hash].reset(&Framebuffer);
}

/// -------------------------------------------------------------------------------------------------------------------

apemodevk::Framebuffer const * apemodevk::FramebufferManager::TryGetFramebufferObjectByHash(uint64_t Hash)
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

void apemodevk::FramebufferBuilder::Attach (TextureResourceView const & TextureView)
{
    apemode_assert ((TextureView.CanGet<apemodevk::ColorResourceView> ()
                          || TextureView.CanGet<apemodevk::DepthStencilResourceView> ())
                             && (TextureView.Width != 0 && TextureView.Height != 0),
                         "Invalid texture view.");

    if (TextureView.CanGet<apemodevk::ColorResourceView> ()
        || TextureView.CanGet<apemodevk::DepthStencilResourceView> ())
    {
        if (TemporaryDesc.Desc->width == 0 &&
            TemporaryDesc.Desc->height == 0)
        {
            TemporaryDesc.Desc->width  = TextureView.Width;
            TemporaryDesc.Desc->height = TextureView.Height;
        }

        apemode_assert (TemporaryDesc.Desc->width == TextureView.Width
                                 && TemporaryDesc.Desc->height == TextureView.Height,
                             "Dimension mismatch.");
    }

    TemporaryDesc.TextureViews.push_back (&TextureView);
    ++TemporaryDesc.Desc->attachmentCount;
}

/// -------------------------------------------------------------------------------------------------------------------

void apemodevk::FramebufferBuilder::SetRenderPass(RenderPass const & pRenderPass)
{
    TemporaryDesc.pRenderPass = &pRenderPass;
}

/// -------------------------------------------------------------------------------------------------------------------

apemodevk::Framebuffer const * apemodevk::FramebufferBuilder::RecreateFramebuffer(apemodevk::GraphicsDevice & GraphicsNode)
{
    if (GraphicsNode.IsValid())
    {
        if (!TemporaryDesc.pRenderPass)
        {
            apemode_halt("Render pass was not set.");
            return nullptr;
        }

        auto ExtractTextureViewFn = [&](TextureResourceView const * pTextureView)
        {
            apemode_assert (pTextureView != nullptr, "Cannot be null.");
            return pTextureView ? pTextureView->ImgViewHandle.Handle : nullptr;
        };

        std::vector<VkImageView> ImgViews;
        ImgViews.reserve(TemporaryDesc.TextureViews.size());

        std::transform (TemporaryDesc.TextureViews.begin (),
                          TemporaryDesc.TextureViews.end (),
                          std::back_inserter (ImgViews),
                          ExtractTextureViewFn);

        apemodevk::AliasStructs (ImgViews,
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
            auto pFramebuffer   = new apemodevk::Framebuffer ();
            pFramebuffer->Hash  = Hash;
            pFramebuffer->pDesc = FramebufferDescription::MakeNewFromTemporary (TemporaryDesc);
            pFramebuffer->Handle.Swap (NewHandle);

            // Stores compiled framebuffers, so we could reuse them later
            Manager.AddNewFramebuffer (*pFramebuffer);
            return pFramebuffer;
        }
    }

    apemode_error ("Out of GPU or system memory.");
    return nullptr;
}

/// -------------------------------------------------------------------------------------------------------------------

void apemodevk::FramebufferBuilder::Reset()
{
    TemporaryDesc.Reset();
}

/// -------------------------------------------------------------------------------------------------------------------
/// FramebufferDescription
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::FramebufferDescription::FramebufferDescription()
{
    Reset();
}

/// -------------------------------------------------------------------------------------------------------------------

void apemodevk::FramebufferDescription::Reset()
{
    Hash        = 0;
    pRenderPass = nullptr;

    TextureViews.clear();
    TextureViews.reserve(8 + 1);

    Desc.InitializeStruct( );
}

/// -------------------------------------------------------------------------------------------------------------------

uint64_t apemodevk::FramebufferDescription::UpdateHash()
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

apemodevk::FramebufferDescription const *
apemodevk::FramebufferDescription::MakeNewFromTemporary(FramebufferDescription const & TemporaryDesc)
{
    if (auto pFramebuffer = new apemodevk::FramebufferDescription())
    {
        pFramebuffer->Hash        = TemporaryDesc.Hash;
        pFramebuffer->Desc        = TemporaryDesc.Desc;
        pFramebuffer->pRenderPass = TemporaryDesc.pRenderPass;

        std::copy (TemporaryDesc.TextureViews.begin (),
                     TemporaryDesc.TextureViews.end (),
                     std::back_inserter (pFramebuffer->TextureViews));

        return pFramebuffer;
    }

    apemode_error("Out of system memory.");
    return nullptr;
}
