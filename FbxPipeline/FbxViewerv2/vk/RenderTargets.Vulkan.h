#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <RenderTarget.Vulkan.h>
#include <DepthStencil.Vulkan.h>

namespace apemode
{
    class CommandBuffer;
    class RenderPass;
    class Framebuffer;

    class RenderTargets : public apemode::ScalableAllocPolicy,
                                                      public apemode::NoCopyAssignPolicy
    {
    public:
        enum
        {
            kColorMaxCount = 8,
            kFrameMaxCount = 3,
        };

    public:
        struct BeginEndScope;
        friend BeginEndScope;

        struct BeginEndScope : public apemode::ScalableAllocPolicy,
                                                           public apemode::NoCopyAssignPolicy
        {
            CommandBuffer &   AssociatedCmdList;
            RenderTargets & AssociatedRenderTargets;

            BeginEndScope (CommandBuffer & CmdBuffer, RenderTargets & RenderTargets);
            ~BeginEndScope();
        };

    public:
        RenderTargets ();
        ~RenderTargets ();

        bool RecreateResourcesFor (GraphicsDevice & InGraphicsNode,
                                   VkSwapchainKHR   InSwapchain,
                                   VkFormat         InColorFormat,
                                   uint32_t         InColorWidth,
                                   uint32_t         InColorHeight,
                                   VkFormat         InDepthStencilFormat,
                                   uint32_t         InDepthStencilWidth,
                                   uint32_t         InDepthStencilHeight,
                                   bool             InDepthStencilReversedZ = true);

        void SetRenderTargetClearValues (uint32_t                  FirstClearValue,
                                         uint32_t                  ClearValueCount,
                                         VkClearColorValue const * pClearValues);

        void SetDepthStencilClearValue (VkClearDepthStencilValue ClearValue);

        void AdvanceFrameCounters ();

        uint32_t                  GetReadFrame () const;
        uint32_t                  GetWriteFrame () const;
        uint32_t                  GetAttachmentCount () const;
        apemode::RenderPass const *  GetRenderPass () const;
        apemode::Framebuffer const * GetWriteFramebuffer () const;

    public:
        uint32_t ReadFrame;
        uint32_t WriteFrame;
        uint32_t FrameCount;
        uint32_t AttachmentCount;

        /** Those fields are always 'fullscreen' by default. */
        VkRect2D   RenderArea;
        VkRect2D   pScissors[ kColorMaxCount ];
        VkViewport pViewports[ kColorMaxCount ];

        /** Contains clear values for both color textures and depth-stencil texture. */
        VkClearValue ClearValues[kFrameMaxCount][kColorMaxCount + 1];

        VkFormat                                         DepthStencilFormat;
        std::shared_ptr<apemode::DepthStencilResourceView>  pDepthStencilViews[ kFrameMaxCount ];
        VkFormat                                         pColorFormats[ kColorMaxCount ];
        std::shared_ptr<apemode::ColorResourceView> ppColorViews[ kFrameMaxCount ][ kColorMaxCount ];

        apemode::RenderPass const *  pRenderPass;
        apemode::Framebuffer const * ppFramebuffers[ kFrameMaxCount ];
    };
}