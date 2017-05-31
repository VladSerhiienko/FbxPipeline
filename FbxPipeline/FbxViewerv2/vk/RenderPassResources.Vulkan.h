#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <RenderTarget.Vulkan.h>
#include <DepthStencil.Vulkan.h>

namespace apemodevk
{
    class CommandBuffer;
    class RenderPass;
    class Framebuffer;

    class RenderPassResources : public apemodevk::ScalableAllocPolicy,
                                                            public apemodevk::NoCopyAssignPolicy
    {
    public:
        enum
        {
            kFrameMaxCount           = 3,
            kColorMaxCountPerSubpass = 8,
        };

    public:
        using TextureViewPtr      = std::shared_ptr<apemodevk::TextureResourceView>;
        using ColorViewPtr        = std::shared_ptr<apemodevk::ColorResourceView>;
        using DepthStencilViewPtr = std::shared_ptr<apemodevk::DepthStencilResourceView>;

    public:
        struct BeginEndScope;
        friend BeginEndScope;
        struct BeginEndScope : public apemodevk::ScalableAllocPolicy,
                                                           public apemodevk::NoCopyAssignPolicy
        {
            CommandBuffer &         AssocCmdList;
            RenderPassResources & AssocResources;

            BeginEndScope (CommandBuffer & InCmdList, RenderPassResources & InResources);
            ~BeginEndScope ();
        };

    public:
        bool RecreateResourcesFor (apemodevk::GraphicsDevice &   InGraphicsNode,
                                   apemodevk::RenderPass const & InRenderPass,
                                   apemodevk::Swapchain **       ppInSwapchains,
                                   uint32_t                 InSwapchainCount,
                                   uint32_t                 InFrameCount,
                                   uint32_t                 InColorWidth,
                                   uint32_t                 InColorHeight,
                                   VkClearValue             InColorClearValue,
                                   uint32_t                 InDepthStencilWidth,
                                   uint32_t                 InDepthStencilHeight,
                                   VkClearValue             InDepthStencilClearValue,
                                   bool                     bInReversedZ = true);

    public:
        uint32_t                  GetReadFrame() const;
        uint32_t                  GetWriteFrame() const;
        uint32_t                  GetFrameCount () const;
        uint32_t                  GetAttachmentCount() const;
        apemodevk::RenderPass const *  GetRenderPass () const;
        apemodevk::Framebuffer const * GetWriteFramebuffer() const;

        /**
         * Sets WriteFrame value to the provided one (once all the conditions are satisfied),
         * ReadFrame gets the value, that is previous one of provided WriteFrame value.
         */
        void SetWriteFrame (uint32_t InWriteFrame);

        /**
         * ReadFrame gets WriteFrame value, WriteFrame gets next value according to FrameCount.
         */
        void AdvanceFrame ();

    public:
        uint32_t                             ReadFrame;
        uint32_t                             WriteFrame;
        uint32_t                             FrameCount;
        VkRect2D                             RenderArea;
        std::vector< VkViewport >            Viewports;
        std::vector< VkRect2D >              Scissors;
        std::vector< VkClearValue >          ClearValues;
        std::vector< ColorViewPtr >          ColorAttachments[ kFrameMaxCount ];
        std::vector< DepthStencilViewPtr >   DepthStencilAttachments[ kFrameMaxCount ];
        std::vector< TextureResourceView * > TextureViews[ kFrameMaxCount ];
        apemodevk::RenderPass const *             pRenderPass;
        apemodevk::Framebuffer const *            ppFramebuffers[ kFrameMaxCount ];
    };
}