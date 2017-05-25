#pragma once

#include <GraphicsDevice.Vulkan.h>

#include <TInfoStruct.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>

namespace Core
{
    class RenderPass;
    class TextureResourceView;
    class ColorResourceView;
    class DepthStencilResourceView;

    class FramebufferDescription;

    class _Graphics_ecosystem_dll_api Framebuffer : public Aux::ScalableAllocPolicy,
                                                    public Aux::NoCopyAssignPolicy
    {
    public:
        uint64_t                                 Hash;
        Core::TDispatchableHandle<VkFramebuffer> Handle;
        Core::FramebufferDescription const *     pDesc;

    public:
        operator VkFramebuffer() const { return Handle; }
    };

    class _Graphics_ecosystem_dll_api FramebufferDescription : public Aux::ScalableAllocPolicy,
                                                               public Aux::NoCopyAssignPolicy
    {
    public:
        uint64_t                                          Hash;
        Core::RenderPass const *                          pRenderPass;
        Core::TInfoStruct<VkFramebufferCreateInfo>        Desc;
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
        static Core::FramebufferDescription const *
        MakeNewFromTemporary (Core::FramebufferDescription const & TemporaryDesc);
    };

    class _Graphics_ecosystem_dll_api FramebufferBuilder : public Aux::ScalableAllocPolicy,
                                                           public Aux::NoCopyAssignPolicy
    {
    public:
        Core::FramebufferDescription TemporaryDesc;

    public:
        void Attach (TextureResourceView const & TextureView);
        void SetRenderPass (RenderPass const & pRenderPass);

        Core::Framebuffer const * RecreateFramebuffer (Core::GraphicsDevice & GraphicsNode);

        void Reset();

    };

    class _Graphics_ecosystem_dll_api FramebufferManager : public Aux::ScalableAllocPolicy,
                                                           public Aux::NoCopyAssignPolicy
    {
        friend Core::GraphicsDevice;
        friend Core::FramebufferBuilder;

        struct PrivateContent;
        struct FramebufferContent;

        PrivateContent * Content;

    public:
        FramebufferManager();
        ~FramebufferManager();

        void AddNewFramebuffer (Core::Framebuffer & Framebuffer);
        Core::Framebuffer const * TryGetFramebufferObjectByHash (uint64_t Hash);
    };
}