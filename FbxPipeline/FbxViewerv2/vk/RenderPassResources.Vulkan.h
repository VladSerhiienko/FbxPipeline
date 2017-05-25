#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <RenderTarget.Vulkan.h>
#include <DepthStencil.Vulkan.h>

namespace Core
{
    class CommandList;
    class RenderPass;
    class Framebuffer;

    class _Graphics_ecosystem_dll_api RenderPassResources : public Aux::ScalableAllocPolicy,
                                                            public Aux::NoCopyAssignPolicy
    {
    public:
        enum
        {
            kFrameMaxCount           = 3,
            kColorMaxCountPerSubpass = 8,
        };

    public:
        using TextureViewPtr      = std::shared_ptr<Core::TextureResourceView>;
        using ColorViewPtr        = std::shared_ptr<Core::ColorResourceView>;
        using DepthStencilViewPtr = std::shared_ptr<Core::DepthStencilResourceView>;

    public:
        struct BeginEndScope;
        friend BeginEndScope;
        struct _Graphics_ecosystem_dll_api BeginEndScope : public Aux::ScalableAllocPolicy,
                                                           public Aux::NoCopyAssignPolicy
        {
            CommandList &         AssocCmdList;
            RenderPassResources & AssocResources;

            BeginEndScope (CommandList & InCmdList, RenderPassResources & InResources);
            ~BeginEndScope ();
        };

    public:
        bool RecreateResourcesFor (Core::GraphicsDevice &   InGraphicsNode,
                                   Core::RenderPass const & InRenderPass,
                                   Core::Swapchain **       ppInSwapchains,
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
        Core::RenderPass const *  GetRenderPass () const;
        Core::Framebuffer const * GetWriteFramebuffer() const;

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
        Core::RenderPass const *             pRenderPass;
        Core::Framebuffer const *            ppFramebuffers[ kFrameMaxCount ];
    };
}