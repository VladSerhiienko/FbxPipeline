#pragma once

#include <GraphicsDevice.Vulkan.h>

#include <TInfoStruct.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>

namespace apemodevk
{
    class RenderPass;
    class TextureResourceView;
    class ColorResourceView;
    class DepthStencilResourceView;

    class FramebufferDescription;

    class Framebuffer : public apemodevk::ScalableAllocPolicy,
                                                    public apemodevk::NoCopyAssignPolicy
    {
    public:
        uint64_t                                 Hash;
        apemodevk::TDispatchableHandle<VkFramebuffer> Handle;
        apemodevk::FramebufferDescription const *     pDesc;

    public:
        operator VkFramebuffer() const { return Handle; }
    };

    class FramebufferDescription : public apemodevk::ScalableAllocPolicy,
                                                               public apemodevk::NoCopyAssignPolicy
    {
    public:
        uint64_t                                          Hash;
        apemodevk::RenderPass const *                          pRenderPass;
        apemodevk::TInfoStruct<VkFramebufferCreateInfo>        Desc;
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
        static apemodevk::FramebufferDescription const *
        MakeNewFromTemporary (apemodevk::FramebufferDescription const & TemporaryDesc);
    };

    class FramebufferBuilder : public apemodevk::ScalableAllocPolicy,
                                                           public apemodevk::NoCopyAssignPolicy
    {
    public:
        apemodevk::FramebufferDescription TemporaryDesc;

    public:
        void Attach (TextureResourceView const & TextureView);
        void SetRenderPass (RenderPass const & pRenderPass);

        apemodevk::Framebuffer const * RecreateFramebuffer (apemodevk::GraphicsDevice & GraphicsNode);

        void Reset();

    };

    class FramebufferManager : public apemodevk::ScalableAllocPolicy,
                                                           public apemodevk::NoCopyAssignPolicy
    {
        friend apemodevk::GraphicsDevice;
        friend apemodevk::FramebufferBuilder;

        struct PrivateContent;
        struct FramebufferContent;

        PrivateContent * Content;

    public:
        FramebufferManager();
        ~FramebufferManager();

        void AddNewFramebuffer (apemodevk::Framebuffer & Framebuffer);
        apemodevk::Framebuffer const * TryGetFramebufferObjectByHash (uint64_t Hash);
    };
}