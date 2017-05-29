//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <RenderTargets.Vulkan.h>

#include <CommandQueue.Vulkan.h>
#include <Swapchain.Vulkan.h>
#include <RenderPass.Vulkan.h>
#include <Framebuffer.Vulkan.h>

/// -------------------------------------------------------------------------------------------------------------------
/// RenderTargets
/// -------------------------------------------------------------------------------------------------------------------

apemode::RenderTargets::RenderTargets()
{
    WriteFrame = 0;
    ReadFrame  = kFrameMaxCount - 1;
    FrameCount = kFrameMaxCount;

    AttachmentCount  = 0;
    DepthStencilFormat = VK_FORMAT_UNDEFINED;
    for (auto & ColorFormat : pColorFormats)
        ColorFormat = VK_FORMAT_UNDEFINED;
}

/// -------------------------------------------------------------------------------------------------------------------

apemode::RenderTargets::~RenderTargets()
{
    _Aux_DebugTraceFunc;
}

/// -------------------------------------------------------------------------------------------------------------------

bool apemode::RenderTargets::RecreateResourcesFor (GraphicsDevice & InGraphicsNode,
                                                VkSwapchainKHR   InSwapchainHandle,
                                                VkFormat         InColorFmt,
                                                uint32_t         InColorWidth,
                                                uint32_t         InColorHeight,
                                                VkFormat         InDepthStencilFmt,
                                                uint32_t         InDepthStencilWidth,
                                                uint32_t         InDepthStencilHeight,
                                                bool             InDepthStencilReversed)
{
    _Game_engine_Assert (InSwapchainHandle != nullptr, "Swapchain is not initialized.");

    uint32_t OutSwapchainBufferCount = 0;
    if (ResultHandle::Succeeded (vkGetSwapchainImagesKHR (
            InGraphicsNode, InSwapchainHandle, &OutSwapchainBufferCount, nullptr)))
    {
        _Game_engine_Assert (OutSwapchainBufferCount != 1,
                             "Frame count must be at least 2,"
                             "otherwise this class is redundand");
        _Game_engine_Assert (OutSwapchainBufferCount <= kFrameMaxCount,
                             "Frame count overlow (%u vs %u)",
                             OutSwapchainBufferCount,
                             kFrameMaxCount);

        std::vector<VkImage> SwapchainBufferImgs (OutSwapchainBufferCount);
        if (_Game_engine_Unlikely (
                ResultHandle::Failed (vkGetSwapchainImagesKHR (InGraphicsNode,
                                                               InSwapchainHandle,
                                                               &OutSwapchainBufferCount,
                                                               SwapchainBufferImgs.data ()))))
        {
            _Game_engine_Halt("vkGetSwapchainImagesKHR failed.");
            return false;
        }

        WriteFrame = 0;
        ReadFrame  = OutSwapchainBufferCount - 1;

        AttachmentCount = 1;
        FrameCount        = OutSwapchainBufferCount;

        for (uint32_t FrameIdx = 0; FrameIdx < FrameCount; FrameIdx++)
        {
            for (uint32_t AttIdx = 0; AttIdx < AttachmentCount; AttIdx++)
            {
                ppColorViews[ FrameIdx ][ AttIdx ] = ColorResourceView::MakeNewLinked ();
                pColorFormats[ AttIdx ]            = InColorFmt;

                if (_Game_engine_Unlikely (
                        !ppColorViews[ FrameIdx ][ AttIdx ]->RecreateResourcesFor (
                            InGraphicsNode, InColorFmt, SwapchainBufferImgs[ FrameIdx ])))
                {
                    _Game_engine_Error("Failed to create RTV.");
                    return false;
                }

                // TODO Refactor
                ppColorViews[ FrameIdx ][ AttIdx ]->Width  = InColorWidth;
                ppColorViews[ FrameIdx ][ AttIdx ]->Height = InColorHeight;
            }
        }

        RenderArea.offset.x      = 0;
        RenderArea.offset.y      = 0;
        RenderArea.extent.width  = InColorWidth;
        RenderArea.extent.height = InColorHeight;

        for (uint32_t AttIdx = 0; AttIdx < AttachmentCount; AttIdx++)
        {
            pScissors[ AttIdx ].offset.x      = 0;
            pScissors[ AttIdx ].offset.y      = 0;
            pScissors[ AttIdx ].extent.width  = InColorWidth;
            pScissors[ AttIdx ].extent.height = InColorHeight;

            pViewports[ AttIdx ].x        = 0.0f;
            pViewports[ AttIdx ].y        = 0.0f;
            pViewports[ AttIdx ].width    = static_cast<float> (InColorWidth);
            pViewports[ AttIdx ].height   = static_cast<float> (InColorHeight);
            pViewports[ AttIdx ].minDepth = 0.0f;
            pViewports[ AttIdx ].maxDepth = 1.0f;
        }

        if (apemode_likely (InDepthStencilFmt != VK_FORMAT_UNDEFINED))
        {
            _Game_engine_Assert (InDepthStencilWidth != 0 && InDepthStencilHeight != 0,
                                 "DSV dimensions.");

            DepthStencilFormat = InDepthStencilFmt;
            for (uint32_t FrameIdx = 0; FrameIdx < FrameCount; FrameIdx++)
            {
                pDepthStencilViews[ FrameIdx ] = DepthStencilResourceView::MakeNewLinked ();

                if (_Game_engine_Unlikely (!pDepthStencilViews[ FrameIdx ]->RecreateResourcesFor (
                        InGraphicsNode,
                        InDepthStencilFmt,
                        InDepthStencilWidth,
                        InDepthStencilHeight,
                        InDepthStencilReversed ? 0.0f : 1.0f,
                        0,
                        VK_IMAGE_LAYOUT_UNDEFINED)))
                {
                    _Game_engine_Error ("Failed to create DSV.");
                    return false;
                }
            }
        }
    }

    if (apemode_likely (AttachmentCount))
    {
        static const uint32_t kDefaultSubpassId = 0;

        apemode::RenderPassBuilder RenderPassBuilder;
        RenderPassBuilder.Reset (2, 0, 1);
        auto ColorId = RenderPassBuilder.AddAttachment (InColorFmt,
                                                        VK_SAMPLE_COUNT_1_BIT,
                                                        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                        VK_ATTACHMENT_LOAD_OP_LOAD,
                                                        VK_ATTACHMENT_STORE_OP_STORE,
                                                        false,
                                                        true);
        auto DepthId
            = RenderPassBuilder.AddAttachment (InDepthStencilFmt,
                                               VK_SAMPLE_COUNT_1_BIT,
                                               VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                               VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                               VK_ATTACHMENT_LOAD_OP_LOAD,
                                               VK_ATTACHMENT_STORE_OP_STORE,
                                               VK_ATTACHMENT_LOAD_OP_LOAD,
                                               VK_ATTACHMENT_STORE_OP_STORE,
                                               false);

        RenderPassBuilder.ResetSubpass (0, 1, 0, 0);
        RenderPassBuilder.AddColorToSubpass (0, ColorId, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        RenderPassBuilder.SetDepthToSubpass (0, DepthId, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

        pRenderPass = RenderPassBuilder.RecreateRenderPass (InGraphicsNode);

        apemode::FramebufferBuilder FramebufferBuilder;
        for (uint32_t FrameIdx = 0; FrameIdx < FrameCount; FrameIdx++)
        {
            for (uint32_t AttIdx = 0; AttIdx < AttachmentCount; AttIdx++)
            {
                FramebufferBuilder.Attach (*ppColorViews[ FrameIdx ][ AttIdx ]);
            }

            if (apemode_likely (pDepthStencilViews[ FrameIdx ]))
            {
                FramebufferBuilder.Attach (*pDepthStencilViews[ FrameIdx ]);
            }

            FramebufferBuilder.SetRenderPass (*pRenderPass);
            ppFramebuffers[ FrameIdx ] = FramebufferBuilder.RecreateFramebuffer (InGraphicsNode);
            _Game_engine_Assert (ppFramebuffers[ FrameIdx ] != nullptr,
                                 "Failed to create framebuffer for frame %u",
                                 FrameIdx);

            FramebufferBuilder.Reset ();
        }
    }

    return true;
}

/// -------------------------------------------------------------------------------------------------------------------

void apemode::RenderTargets::SetRenderTargetClearValues (uint32_t                  InOffset,
                                                      uint32_t                  InClearValueCount,
                                                      VkClearColorValue const * InClearValues)
{
    const bool bIsInputLikelyOk = InClearValues != nullptr && InClearValueCount != 0;
    const bool bIsRangeOk = (InOffset < (AttachmentCount - 1))
                            && (InClearValueCount <= (AttachmentCount - InOffset));

    _Game_engine_Assert(bIsInputLikelyOk && bIsInputLikelyOk, "Invalid input.");

    for (uint32_t CurrentFrame = 0; CurrentFrame < FrameCount; CurrentFrame++)
    {
        for (uint32_t i = InOffset, j = 0; j < InClearValueCount; ++i, ++j)
        {
            //TODO Simd instruction
            // VkClearColorValue is union, that is why member-size assigning with uint32`s...
            ClearValues[CurrentFrame][i].color.uint32[0] = InClearValues[j].uint32[0];
            ClearValues[CurrentFrame][i].color.uint32[1] = InClearValues[j].uint32[1];
            ClearValues[CurrentFrame][i].color.uint32[2] = InClearValues[j].uint32[2];
            ClearValues[CurrentFrame][i].color.uint32[3] = InClearValues[j].uint32[3];
        }
    }
}

/// -------------------------------------------------------------------------------------------------------------------

void apemode::RenderTargets::SetDepthStencilClearValue (
    VkClearDepthStencilValue DepthStencilClearValue)
{
    for (uint32_t CurrentFrame = 0; CurrentFrame < FrameCount; CurrentFrame++)
    {
        ClearValues[CurrentFrame][AttachmentCount].depthStencil = DepthStencilClearValue;
    }
}

/// -------------------------------------------------------------------------------------------------------------------

void apemode::RenderTargets::AdvanceFrameCounters ()
{
    ReadFrame  = WriteFrame;
    WriteFrame = (WriteFrame + 1) % FrameCount;
}

uint32_t apemode::RenderTargets::GetWriteFrame () const
{
    return WriteFrame;
}

uint32_t apemode::RenderTargets::GetReadFrame () const
{
    return ReadFrame;
}

uint32_t apemode::RenderTargets::GetAttachmentCount () const
{
    return AttachmentCount;
}

apemode::RenderPass const * apemode::RenderTargets::GetRenderPass () const
{
    return pRenderPass;
}

apemode::Framebuffer const * apemode::RenderTargets::GetWriteFramebuffer() const
{
    return ppFramebuffers[WriteFrame];
}

/// -------------------------------------------------------------------------------------------------------------------
/// RenderTargets BeginEndScope
/// -------------------------------------------------------------------------------------------------------------------

apemode::RenderTargets::BeginEndScope::BeginEndScope (CommandList &   CmdList,
                                                   RenderTargets & RenderTargets)
    : AssociatedCmdList (CmdList)
    , AssociatedRenderTargets (RenderTargets)
{
    _Game_engine_Assert (CmdList.bIsInBeginEndScope,
                         "Command recording was not started.");
    _Game_engine_Assert (!!RenderTargets.pRenderPass
                             && !!RenderTargets.ppFramebuffers[ RenderTargets.WriteFrame ],
                         "Render targets was not successfully initialized.");

    TInfoStruct<VkRenderPassBeginInfo> RenderPassBeginDesc;
    RenderPassBeginDesc->renderArea      = RenderTargets.RenderArea;
    RenderPassBeginDesc->pClearValues    = RenderTargets.ClearValues[ RenderTargets.WriteFrame ];
    RenderPassBeginDesc->clearValueCount = RenderTargets.AttachmentCount + 1;
    RenderPassBeginDesc->framebuffer     = *RenderTargets.ppFramebuffers[ RenderTargets.WriteFrame ];
    RenderPassBeginDesc->renderPass      = *RenderTargets.pRenderPass;

    //TODO Is it correct? I`m not quite sure, see docs.
    //TODO CmdList.Type == CommandList::kCommandListType_Direct should be moved to a function
    //TODO Branching can be removed with static_cast and additional assertion (for CmdList.Type {0, 1} range).
    auto SubpassContents = CmdList.eType == CommandList::kCommandListType_Direct
        ? VK_SUBPASS_CONTENTS_INLINE
        : VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;

    vkCmdBeginRenderPass (CmdList, RenderPassBeginDesc, SubpassContents);
    vkCmdSetViewport (CmdList, 0, RenderTargets.AttachmentCount, RenderTargets.pViewports);
    vkCmdSetScissor (CmdList, 0, RenderTargets.AttachmentCount, RenderTargets.pScissors);

    CmdList.pRenderPass  = RenderTargets.pRenderPass;
    CmdList.pFramebuffer = RenderTargets.ppFramebuffers[RenderTargets.WriteFrame];
}

/// -------------------------------------------------------------------------------------------------------------------

apemode::RenderTargets::BeginEndScope::~BeginEndScope ()
{
    AssociatedCmdList.pRenderPass = nullptr;
    AssociatedCmdList.pFramebuffer = nullptr;

    vkCmdEndRenderPass(AssociatedCmdList);
}

/// -------------------------------------------------------------------------------------------------------------------
