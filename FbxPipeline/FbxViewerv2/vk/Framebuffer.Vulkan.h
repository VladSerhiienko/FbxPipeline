#pragma once

#include <GraphicsDevice.Vulkan.h>

#include <TInfoStruct.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>

namespace apemode
{
    class RenderPass;
    class TextureResourceView;
    class ColorResourceView;
    class DepthStencilResourceView;

    class FramebufferDescription;

    class _Graphics_ecosystem_dll_api Framebuffer : public apemode::ScalableAllocPolicy,
                                                    public apemode::NoCopyAssignPolicy
    {
    public:
        uint64_t                                 Hash;
        apemode::TDispatchableHandle<VkFramebuffer> Handle;
        apemode::FramebufferDescription const *     pDesc;

    public:
        operator VkFramebuffer() const { return Handle; }
    };

    class _Graphics_ecosystem_dll_api FramebufferDescription : public apemode::ScalableAllocPolicy,
                                                               public apemode::NoCopyAssignPolicy
    {
    public:
        uint64_t                                          Hash;
        apemode::RenderPass const *                          pRenderPass;
        apemode::TInfoStruct<VkFramebufferCreateInfo>        Desc;
        std::vector<TextureResourceView const *> TextureViews;

    public:
        FramebufferDescription();
        uint64_t GetHash() const { return Hash; }
        VkFramebufferCreateInfo GetDesc() const { return *this; }
        uint64_t UpdateHash();
        void Reset();

    public:
        operator VkFramebufferCreateInfo &() { return Desc; }
        operator VkFramebufferCreateInfo() const { return Desc; }

    public:
        static apemode::FramebufferDescription const *
        MakeNewFromTemporary (apemode::FramebufferDescription const & TemporaryDesc);
    };

    class _Graphics_ecosystem_dll_api FramebufferBuilder : public apemode::ScalableAllocPolicy,
                                                           public apemode::NoCopyAssignPolicy
    {
    public:
        apemode::FramebufferDescription TemporaryDesc;

    public:
        void Attach (TextureResourceView const & TextureView);
        void SetRenderPass (RenderPass const & pRenderPass);

        apemode::Framebuffer const * RecreateFramebuffer (apemode::GraphicsDevice & GraphicsNode);

        void Reset();

    };

    class _Graphics_ecosystem_dll_api FramebufferManager : public apemode::ScalableAllocPolicy,
                                                           public apemode::NoCopyAssignPolicy
    {
        friend apemode::GraphicsDevice;
        friend apemode::FramebufferBuilder;

        struct PrivateContent;
        struct FramebufferContent;

        PrivateContent * Content;

    public:
        FramebufferManager();
        ~FramebufferManager();

        void AddNewFramebuffer (apemode::Framebuffer & Framebuffer);
        apemode::Framebuffer const * TryGetFramebufferObjectByHash (uint64_t Hash);
    };
}