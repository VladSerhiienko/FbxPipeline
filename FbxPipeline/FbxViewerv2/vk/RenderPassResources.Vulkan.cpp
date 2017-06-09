//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <Swapchain.Vulkan.h>
#include <RenderPass.Vulkan.h>
#include <Framebuffer.Vulkan.h>
#include <CommandQueue.Vulkan.h>

#include <RenderPassResources.Vulkan.h>

/// -------------------------------------------------------------------------------------------------------------------

union ExposedAttachmentHash {
    struct
    {
        uint32_t AttachmentId;
        uint32_t FrameIdx;
    };

    uint64_t AttachmentHash;
};

/// -------------------------------------------------------------------------------------------------------------------
/// RenderPassResources
/// -------------------------------------------------------------------------------------------------------------------

static bool ExtractSwapchainBuffers (apemodevk::GraphicsDevice &          InGraphicsNode,
                                     apemodevk::Swapchain &               InSwapchain,
                                     uint32_t                        InFrameCount,
                                     std::vector<VkImage> & OutSwapchainBufferImgs)
{
    apemode_assert (InGraphicsNode.IsValid () && InSwapchain.hSwapchain.IsNotNull (),
                         "Not initialized.");

    uint32_t OutSwapchainBufferCount = 0;
    if (apemode_likely (apemodevk::ResultHandle::Succeeded (vkGetSwapchainImagesKHR (
            InGraphicsNode, InSwapchain.hSwapchain, &OutSwapchainBufferCount, nullptr))))
    {
        apemode_assert (OutSwapchainBufferCount != InFrameCount,
                             "Frame count does not match buffer img count (%u, %u)",
                             OutSwapchainBufferCount,
                             InFrameCount);

        OutSwapchainBufferImgs.resize (OutSwapchainBufferCount, VkImage (nullptr));
        if (apemode_likely (apemodevk::ResultHandle::Succeeded (
                vkGetSwapchainImagesKHR (InGraphicsNode,
                                         InSwapchain.hSwapchain,
                                         &OutSwapchainBufferCount,
                                         OutSwapchainBufferImgs.data ()))))
        {
            return true;
        }

        OutSwapchainBufferImgs.resize(0);
    }

    apemode_halt ("vkGetSwapchainImagesKHR failed.");
    return false;
}

void apemodevk::RenderPassResources::SetWriteFrame (uint32_t InWriteFrame)
{
    apemode_assert (FrameCount != 1 && InWriteFrame < FrameCount,
                         "Index is out of range.");

    if (FrameCount != 1 && InWriteFrame < FrameCount)
    {
        WriteFrame = InWriteFrame;
        ReadFrame  = (InWriteFrame + FrameCount + 1) % FrameCount;
    }
    else if (FrameCount == 1)
    {
        WriteFrame = 0;
        ReadFrame  = 0;
    }
}

bool apemodevk::RenderPassResources::RecreateResourcesFor (apemodevk::GraphicsDevice &   InGraphicsNode,
                                                      apemodevk::RenderPass const & InRenderPass,
                                                      apemodevk::Swapchain **       ppInSwapchains,
                                                      uint32_t                 InSwapchainCount,
                                                      uint32_t                 InFrameCount,
                                                      uint32_t                 InColorWidth,
                                                      uint32_t                 InColorHeight,
                                                      VkClearValue             InColorClear,
                                                      uint32_t                 InDepthStencilWidth,
                                                      uint32_t                 InDepthStencilHeight,
                                                      VkClearValue             InDepthStencilClear,
                                                      bool                     bInReversedZ)
{
    if (apemode_unlikely (InRenderPass.pNode != &InGraphicsNode
                               || InRenderPass.pDesc == nullptr))
    {
        apemode_halt ("Render pass is not initialized, or logical device mismatch.");
        return false;
    }

    apemode_assert (InColorWidth > 0 && InColorHeight > 0, "Cannot be zero.");

    pRenderPass = &InRenderPass;
    RenderArea.offset = { 0, 0 };
    RenderArea.extent = { InColorWidth, InColorHeight };

    auto & RenderPassDesc = *InRenderPass.pDesc;

    apemode_assert (InFrameCount <= kFrameMaxCount,
                         "Frame count overlow (%u vs %u)",
                         InFrameCount,
                         kFrameMaxCount);

    FrameCount = InFrameCount;
    SetWriteFrame (0);

    for (uint32_t FrameIdx = 0; FrameIdx < FrameCount; FrameIdx++)
        TextureViews[ FrameIdx ].reserve (RenderPassDesc.Attachments.size ());

    uint32_t AttachmentId = 0;
    for (auto & Attachment : RenderPassDesc.Attachments)
    {
        if (apemode_unlikely (ResourceReference::IsDepthStencilFormat (Attachment.format)))
        {
            apemode_assert (InDepthStencilWidth > 0 && InDepthStencilHeight > 0,
                                 "Cannot be zero.");

            ClearValues.push_back (InDepthStencilClear);

            for (uint32_t FrameIdx = 0; FrameIdx < FrameCount; FrameIdx++)
            {
                DepthStencilAttachments[ FrameIdx ].push_back (
                    DepthStencilResourceView::MakeNewLinked ());
                apemode_assert (DepthStencilAttachments[ FrameIdx ].back (),
                                     "Out of system memory.");

                if (!DepthStencilAttachments[ FrameIdx ].back ()->RecreateResourcesFor (
                        InGraphicsNode,
                        Attachment.format,
                        InDepthStencilWidth,
                        InDepthStencilHeight,
                        bInReversedZ ? 0.0f : 1.0f,
                        0,
                        VK_IMAGE_LAYOUT_UNDEFINED))
                {
                    apemode_halt ("Failed to create depth-stencil view.");
                    return false;
                }

                DepthStencilAttachments[ FrameIdx ].back ()->ClearColor = InDepthStencilClear;

                TextureViews[ FrameIdx ].push_back (
                    DepthStencilAttachments[ FrameIdx ].back ().get ());
            }
        }
        else
        {
            uint32_t SwapchainId;
            if (RenderPassDesc.GetSwapchainAttachmentInfo (AttachmentId, SwapchainId))
            {
                apemode_assert (SwapchainId < InSwapchainCount, "Index is out of range.");

                auto & SwapchainBuffers
                    = ppInSwapchains[ SwapchainId ]->hImgs;

                apemode_assert (SwapchainBuffers.size () == FrameCount,
                                     "Swapchain configuration does not match the provided one.");

                ClearValues.push_back (InColorClear);
                Scissors.push_back (VkRect2D{ { 0, 0 }, { InColorWidth, InColorHeight } });
                Viewports.push_back (VkViewport{ 0.0f,
                                                 0.0f,
                                                 static_cast<float> (InColorWidth),
                                                 static_cast<float> (InColorHeight),
                                                 0.0f,
                                                 1.0f });

                for (uint32_t FrameIdx = 0; FrameIdx < FrameCount; FrameIdx++)
                {
                    ColorAttachments[ FrameIdx ].push_back (ColorResourceView::MakeNewLinked ());
                    apemode_assert (ColorAttachments[ FrameIdx ].back (),
                                         "Out of system memory.");

                    if (!ColorAttachments[ FrameIdx ].back ()->RecreateResourcesFor (
                            InGraphicsNode, Attachment.format, SwapchainBuffers[ FrameIdx ]))
                    {
                        apemode_halt ("Failed to create color view.");
                        return false;
                    }

                    ColorAttachments[ FrameIdx ].back ()->Width      = InColorWidth;
                    ColorAttachments[ FrameIdx ].back ()->Height     = InColorHeight;
                    ColorAttachments[ FrameIdx ].back ()->ClearColor = InColorClear;

                    TextureViews[ FrameIdx ].push_back (
                        ColorAttachments[ FrameIdx ].back ().get ());
                }
            }
            else
            {
                apemode_halt ("Not implemented.");

                ClearValues.push_back (InColorClear);
                for (uint32_t FrameIdx = 0; FrameIdx < FrameCount; FrameIdx++)
                    TextureViews[ FrameIdx ].push_back (nullptr);
            }

        }

        ++AttachmentId;
    }

    FramebufferBuilder FramebufferBuilder;
    for (uint32_t FrameIdx = 0; FrameIdx < FrameCount; FrameIdx++)
    {
        for (auto pTextureView : TextureViews[ FrameIdx ])
        {
            apemode_assert (pTextureView != nullptr, "Not initialized.");
            FramebufferBuilder.Attach (*pTextureView);
        }

        FramebufferBuilder.SetRenderPass (*GetRenderPass ());
        ppFramebuffers[ FrameIdx ] = FramebufferBuilder.RecreateFramebuffer (InGraphicsNode);

        if (ppFramebuffers[ FrameIdx ] == nullptr)
        {
            apemode_error ("Failed to create framebuffer.");
            return false;
        }

        FramebufferBuilder.Reset ();
    }

    apemode_assert (ClearValues.size () == RenderPassDesc.Attachments.size (),
                         "Should be equal.");

    return true;
}

void apemodevk::RenderPassResources::AdvanceFrame ()
{
    ReadFrame  = WriteFrame;
    WriteFrame = (WriteFrame + 1) % FrameCount;
}

uint32_t apemodevk::RenderPassResources::GetWriteFrame () const
{
    return WriteFrame;
}

uint32_t apemodevk::RenderPassResources::GetFrameCount () const
{
    return FrameCount;
}

uint32_t apemodevk::RenderPassResources::GetReadFrame () const
{
    return ReadFrame;
}

uint32_t apemodevk::RenderPassResources::GetAttachmentCount () const
{
    return _Get_collection_length_u (ClearValues);
}

apemodevk::RenderPass const * apemodevk::RenderPassResources::GetRenderPass () const
{
    return pRenderPass;
}

apemodevk::Framebuffer const * apemodevk::RenderPassResources::GetWriteFramebuffer () const
{
    return ppFramebuffers[ GetWriteFrame () ];
}

/// -------------------------------------------------------------------------------------------------------------------
/// RenderPassResources BeginEndScope
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::RenderPassResources::BeginEndScope::BeginEndScope (CommandBuffer &         CmdBuffer,
                                                         RenderPassResources & Resources)
    : AssocCmdList (CmdBuffer), AssocResources (Resources)
{
    apemode_assert (CmdBuffer.bIsInBeginEndScope,
                         "Not started.");
    apemode_assert (Resources.GetRenderPass () != nullptr && Resources.GetWriteFramebuffer () != nullptr,
                         "Not initialized.");

    TInfoStruct<VkRenderPassBeginInfo> RenderPassBeginDesc;
    RenderPassBeginDesc->renderArea      = Resources.RenderArea;
    RenderPassBeginDesc->pClearValues    = Resources.ClearValues.data ();
    RenderPassBeginDesc->clearValueCount = _Get_collection_length_u (Resources.ClearValues);
    RenderPassBeginDesc->framebuffer     = *Resources.GetWriteFramebuffer ();
    RenderPassBeginDesc->renderPass      = *Resources.GetRenderPass ();

    vkCmdBeginRenderPass (CmdBuffer,
                          RenderPassBeginDesc,
                          CmdBuffer.IsDirect () ? VK_SUBPASS_CONTENTS_INLINE
                                              : VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    //vkCmdSetViewport (CmdBuffer, 0, _Get_collection_length_u (Resources.Viewports), Resources.Viewports.data ());
    //vkCmdSetScissor (CmdBuffer, 0, _Get_collection_length_u (Resources.Scissors), Resources.Scissors.data ());

    CmdBuffer.pRenderPass  = Resources.GetRenderPass ();
    CmdBuffer.pFramebuffer = Resources.GetWriteFramebuffer ();
}

apemodevk::RenderPassResources::BeginEndScope::~BeginEndScope ()
{
    AssocCmdList.pRenderPass  = nullptr;
    AssocCmdList.pFramebuffer = nullptr;

    vkCmdEndRenderPass (AssocCmdList);
}
