#pragma once

#include <GraphicsDevice.Vulkan.h>

#include <TInfoStruct.Vulkan.h>
#include <CityHash.h>
#include <NativeDispatchableHandles.Vulkan.h>

namespace apemode
{
    class ResourceView;
    class ColorResourceView;
    class DepthStencilResourceView;

    class RenderPass;
    class RenderPassDescription;
    class RenderPassBuilder;

    class _Graphics_ecosystem_dll_api RenderPass : public apemode::ScalableAllocPolicy,
                                                   public apemode::NoCopyAssignPolicy
    {
    public:
        uint64_t                                Hash;
        apemode::TDispatchableHandle<VkRenderPass> Handle;
        apemode::RenderPassDescription const *     pDesc;
        apemode::GraphicsDevice const *            pGraphicsNode;

    public:
        RenderPass();

    public:
        operator VkRenderPass() const;
    };

    class _Graphics_ecosystem_dll_api RenderPassDescription : public apemode::ScalableAllocPolicy,
                                                              public apemode::NoCopyAssignPolicy
    {
    public:
        struct _Graphics_ecosystem_dll_api SubpassDescription : public apemode::ScalableAllocPolicy,
                                                                public apemode::NoCopyAssignPolicy
        {
            using NativeSubpassDescription  = TInfoStruct<VkSubpassDescription>;
            using NativeAttachmentReference = TInfoStruct<VkAttachmentReference>;
            static_assert(sizeof(VkSubpassDescription) == sizeof(NativeSubpassDescription), "Size mismatch.");
            static_assert(sizeof(VkAttachmentReference) == sizeof(NativeAttachmentReference), "Size mismatch.");

            using Key             = uint32_t;
            using UqPtr           = std::unique_ptr<SubpassDescription>;
            using LkPtr           = std::shared_ptr<SubpassDescription>;
            using LookupContainer = std::map<Key, LkPtr>;

            uint32_t                                    Id;
            VkPipelineBindPoint                         BindPoint;
            std::vector<VkAttachmentReference> InputRefs;
            std::vector<VkAttachmentReference> ColorRefs;
            VkAttachmentReference                       DepthStencilRef;
            std::vector<VkAttachmentReference> ResolveRefs;
            std::vector<uint32_t>              PreserveIndices;

            SubpassDescription(uint32_t Id);

            bool HasDepthStencilRef () const;

        public:
            static std::unique_ptr<SubpassDescription> MakeNewUnique(uint32_t Id);
            static std::shared_ptr<SubpassDescription> MakeNewLinked(uint32_t Id);
        };

    public:
        uint64_t                                      Hash;
        apemode::TInfoStruct<VkRenderPassCreateInfo>     Desc;
        std::vector<VkAttachmentDescription> Attachments;
        std::vector<VkSubpassDependency>     SubpassDependencies;
        std::vector<uint64_t>                SwapchainAttachmentHashes;
        SubpassDescription::LookupContainer           SubpassDescriptions;

    public:
        RenderPassDescription();

        void Reset();
        uint64_t UpdateHash();

        bool GetSwapchainAttachmentInfo (uint32_t AttachmentId, uint32_t & OutSwapchainId) const;

    public:
        static RenderPassDescription const *
        MakeNewFromTemporary (RenderPassDescription const & TemporaryDesc);
    };

    class _Graphics_ecosystem_dll_api RenderPassBuilder : public apemode::ScalableAllocPolicy,
                                                          public apemode::NoCopyAssignPolicy
    {
    public:
        static const uint32_t sInvalidSwapchainId = 0xffffffff;

    public:
        RenderPassDescription TemporaryDesc;

    public:
        RenderPassBuilder();

        void Reset (uint32_t MaxAttachments,
                    uint32_t MaxDependencies,
                    uint32_t MaxSwaphchainAttachments);

        uint32_t AddAttachment (VkFormat            eImgFmt,
                                VkSampleCountFlags  eImgSampleCount,
                                VkImageLayout       eImgInitialLayout,
                                VkImageLayout       eImgFinalLayout,
                                VkAttachmentLoadOp  eImgLoadOp,
                                VkAttachmentStoreOp eImgStoreOp,
                                bool                bImgMayAlias = false,
                                uint32_t            SwapchainId  = sInvalidSwapchainId);

        uint32_t AddAttachment (VkFormat            eDepthStencilFmt,
                                VkSampleCountFlags  eDepthStencilSampleCount,
                                VkImageLayout       eDepthStencilInitialLayout,
                                VkImageLayout       eDepthStencilFinalLayout,
                                VkAttachmentLoadOp  eDepthLoadOp,
                                VkAttachmentStoreOp eDepthStoreOp,
                                VkAttachmentLoadOp  eStencilLoadOp,
                                VkAttachmentStoreOp eStencilStoreOp,
                                bool                bDepthStencilMayAlias = false);

        void ResetSubpass (uint32_t SubpassId,
                           uint32_t MaxColors,
                           uint32_t MaxInputs,
                           uint32_t MaxPreserves);

        void AddColorToSubpass (uint32_t      SubpassId,
                                uint32_t      ImgAttachmentId,
                                VkImageLayout eImgSubpassLayout);

        void AddColorToSubpass (uint32_t      SubpassId,
                                uint32_t      ImgAttachmentId,
                                VkImageLayout eImgSubpassLayout,
                                uint32_t      ResolveImgAttachmentId,
                                VkImageLayout eResolveImgSubpassLayout);

        void AddInputToSubpass (uint32_t      SubpassId,
                                uint32_t      ImgAttachmentId,
                                VkImageLayout eImgSubpassLayout);

        void SetDepthToSubpass (uint32_t      SubpassId,
                                uint32_t      ImgAttachmentId,
                                VkImageLayout eImgSubpassLayout);

        void PreserveInSubpass (uint32_t SubpassId,
                                uint32_t AttachmentId);

        void SetSubpassDependency (uint32_t             SrcSubpassId,
                                   VkPipelineStageFlags eSrcSubpassStage,
                                   VkAccessFlags        eSrcSubpassAccess,
                                   uint32_t             DstSubpassId,
                                   VkPipelineStageFlags eDstSubpassStage,
                                   VkAccessFlags        eDstSubpassAccess,
                                   bool                 bDependentByRegion);

        apemode::RenderPass const * RecreateRenderPass (apemode::GraphicsDevice & GraphicsNode);

        /**
         * Checks whether subpasses are complete.
         * @returns True if subpasses are complete, false otherwise.
         */
        bool VerifySubpasses () const;

    private:
        RenderPassDescription::SubpassDescription & GetOrCreateSubpass(uint32_t SubpassId);
    };

    class _Graphics_ecosystem_dll_api RenderPassManager
    {
        friend apemode::GraphicsDevice;
        friend apemode::RenderPassBuilder;
        struct RenderPassContent;
        struct PrivateContent;

        PrivateContent * pContent;

    public:
        RenderPassManager ();
        ~RenderPassManager ();

        void AddNewRenderPassObject (apemode::RenderPass & RenderPass);
        apemode::RenderPass const * TryGetRenderPassObjectByHash (uint64_t Hash);
    };
}
