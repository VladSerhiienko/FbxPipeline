//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <DepthStencil.Vulkan.h>
//#include <TDataHandle.h>

/// -------------------------------------------------------------------------------------------------------------------
/// DepthStencilResourceView
/// -------------------------------------------------------------------------------------------------------------------

std::shared_ptr<Core::DepthStencilResourceView> Core::DepthStencilResourceView::MakeNewLinked()
{
    return std::shared_ptr<DepthStencilResourceView> (new DepthStencilResourceView ());
}

std::unique_ptr<Core::DepthStencilResourceView> Core::DepthStencilResourceView::MakeNewUnique()
{
    return std::unique_ptr<DepthStencilResourceView> (new DepthStencilResourceView ());
}

std::unique_ptr<Core::DepthStencilResourceView> Core::DepthStencilResourceView::MakeNullUnique ()
{
    return std::unique_ptr<DepthStencilResourceView> (nullptr);
}

Core::DepthStencilResourceView::DepthStencilResourceView ()
    : DepthFormat (VK_FORMAT_UNDEFINED), StencilFormat (VK_FORMAT_UNDEFINED)
{
    ViewType = kResourceViewType_DepthStencil;

    // Default format is 32-bit depth and 8-bit stencil
    Format = VK_FORMAT_D32_SFLOAT_S8_UINT;
    if (!ResourceReference::GetDepthStencilFormats (Format, DepthFormat, StencilFormat))
    {
        _Game_engine_Halt ("Wrong depth format.");
    }

    ImgType       = VK_IMAGE_TYPE_2D;
    ImgViewType   = VK_IMAGE_VIEW_TYPE_2D;
    DepthAspect   = VK_IMAGE_ASPECT_DEPTH_BIT;
    StencilAspect = VK_IMAGE_ASPECT_STENCIL_BIT;
    ImgUsage      = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    ImgTiling     = VK_IMAGE_TILING_OPTIMAL;
    SampleCount   = VK_SAMPLE_COUNT_1_BIT;

    ImgAspect = 0;
    if (DepthFormat != VK_FORMAT_UNDEFINED)
        ImgAspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
    if (StencilFormat != VK_FORMAT_UNDEFINED)
        ImgAspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
}

Core::DepthStencilResourceView::~DepthStencilResourceView()
{
    _Aux_DebugTraceFunc;
}

bool Core::DepthStencilResourceView::RecreateResourcesFor (GraphicsDevice & InGraphicsNode,
                                                           VkFormat         InFmt,
                                                           uint32_t         InWidth,
                                                           uint32_t         InHeight,
                                                           float            InClearDepth,
                                                           uint32_t         InClearStencil,
                                                           VkImageLayout    InInitialLayout)
{
    _Game_engine_Assert (InWidth != 0 && InHeight != 0, "Invalid dimensions.");

    if (InFmt != VK_FORMAT_UNDEFINED)
    {
        VkFormat OutDepthFmt, OutStencilFmt;
        if (!ResourceReference::GetDepthStencilFormats (InFmt, OutDepthFmt, OutStencilFmt))
        {
            _Game_engine_Halt ("Wrong depth format.");
        }
        else
        {
            ImgAspect     = 0;
            Format        = InFmt;
            DepthFormat   = OutDepthFmt;
            StencilFormat = OutStencilFmt;

            if (DepthFormat != VK_FORMAT_UNDEFINED)
                ImgAspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
            if (StencilFormat != VK_FORMAT_UNDEFINED)
                ImgAspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }

    TInfoStruct<VkImageCreateInfo> ImgDesc;
    ImgDesc->initialLayout = InInitialLayout;
    ImgDesc->imageType     = ImgType;
    ImgDesc->format        = Format;
    ImgDesc->extent.width  = InWidth;
    ImgDesc->extent.height = InHeight;
    ImgDesc->extent.depth  = 1;
    ImgDesc->mipLevels     = 1;
    ImgDesc->arrayLayers   = 1;
    ImgDesc->samples       = static_cast<VkSampleCountFlagBits> (SampleCount);
    ImgDesc->tiling        = ImgTiling;
    ImgDesc->usage         = ImgUsage;
    ImgDesc->flags         = 0;

    if (ImgHandle.Recreate(InGraphicsNode, ImgDesc))
    {
        Width           = InWidth;
        Height          = InHeight;
        Depth           = ImgDesc->extent.depth;
        ArrayLayerCount = ImgDesc->arrayLayers;
        MipLevelCount   = ImgDesc->mipLevels;

        // TODO 
        //      Should be moved to ResourceAssembler

        if (!HasResource())
            ResourceRef = ResourceReference::MakeNewLinked(InGraphicsNode);

        // Fill allocation info

        vkGetImageMemoryRequirements(InGraphicsNode, ImgHandle, ResourceRef->MemoryReqs);
        ResourceRef->MemoryAlloc->allocationSize = ResourceRef->MemoryReqs->size;

        if (!ResourceReference::GetMemoryTypeFromProperties (
                InGraphicsNode,
                ResourceRef->MemoryReqs->memoryTypeBits,
                0,
                ResourceRef->MemoryAlloc->memoryTypeIndex))
        {
            _Game_engine_Halt("GetMemoryTypeFromProperties failed. "
                              "No suitable memory available.");
            ResourceRef.reset();
            return false;
        }

        Width = InWidth;
        Height = InHeight;

        // Try to allocate memory and bind it to the resource

        if (!ResourceRef->MemoryHandle.Recreate (InGraphicsNode,
                                                 ResourceRef->MemoryAlloc))
        {
            _Game_engine_Halt ("Failed to allocate memory.");
            ResourceRef.reset ();
            return false;
        }

        VirtualAddressOffset = 0;
        if (!ResultHandle::Succeeded (vkBindImageMemory (InGraphicsNode,
                                                         ImgHandle,
                                                         ResourceRef->MemoryHandle,
                                                         VirtualAddressOffset)))
        {
            _Game_engine_Halt ("vkBindImageMemory failed.");
            ResourceRef.reset ();
            return false;
        }

        if (Format != VK_FORMAT_UNDEFINED)
        {
            TInfoStruct<VkImageViewCreateInfo> DepthStencilViewDesc;
            DepthStencilViewDesc->format                          = Format;
            DepthStencilViewDesc->subresourceRange.aspectMask     = ImgAspect;
            DepthStencilViewDesc->subresourceRange.baseMipLevel   = 0;
            DepthStencilViewDesc->subresourceRange.baseArrayLayer = 0;
            DepthStencilViewDesc->subresourceRange.levelCount     = MipLevelCount;
            DepthStencilViewDesc->subresourceRange.layerCount     = ArrayLayerCount;
            DepthStencilViewDesc->viewType                        = ImgViewType;
            DepthStencilViewDesc->image                           = ImgHandle;

            if (!ImgViewHandle.Recreate (InGraphicsNode, DepthStencilViewDesc))
            {
                _Game_engine_Halt("Invalid image view desc.");
                return false;
            }

            MemoryStates[ nullptr ] = MemoryState (VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                                   VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                                   VK_QUEUE_FAMILY_IGNORED,
                                                   InInitialLayout,
                                                   DepthStencilViewDesc->subresourceRange);
        }

        //// Not needed for 2 same views: ImgViewHandle, DepthImgViewHandle
        //if (DepthFormat != Format && DepthFormat != VK_FORMAT_UNDEFINED)
        //{
        //    TInfoStruct<VkImageViewCreateInfo> DepthViewDesc;
        //    DepthViewDesc->format                          = Format;
        //    DepthViewDesc->subresourceRange.aspectMask     = DepthAspect;
        //    DepthViewDesc->subresourceRange.baseMipLevel   = 0;
        //    DepthViewDesc->subresourceRange.levelCount     = 1;
        //    DepthViewDesc->subresourceRange.baseArrayLayer = 0;
        //    DepthViewDesc->subresourceRange.layerCount     = 1;
        //    DepthViewDesc->viewType                        = ImgViewType;
        //    DepthViewDesc->image                           = ImgHandle;

        //    if (!DepthImgViewHandle.Recreate(InGraphicsNode, DepthViewDesc))
        //    {
        //        _Game_engine_Halt("Invalid image view desc.");
        //        return false;
        //    }

        //    ImgSubresRange = DepthViewDesc->subresourceRange;
        //}

        //if (StencilFormat != VK_FORMAT_UNDEFINED)
        //{
        //    TInfoStruct<VkImageViewCreateInfo> StencilViewDesc;
        //    StencilViewDesc->format                          = Format;
        //    StencilViewDesc->subresourceRange.aspectMask     = StencilAspect;
        //    StencilViewDesc->subresourceRange.baseMipLevel   = 0;
        //    StencilViewDesc->subresourceRange.levelCount     = 1;
        //    StencilViewDesc->subresourceRange.baseArrayLayer = 0;
        //    StencilViewDesc->subresourceRange.layerCount     = 1;
        //    StencilViewDesc->viewType                        = ImgViewType;
        //    StencilViewDesc->image                           = ImgHandle;

        //    if (!StencilImgViewHandle.Recreate (InGraphicsNode, StencilViewDesc))
        //    {
        //        _Game_engine_Halt ("Invalid image view desc.");
        //        return false;
        //    }

        //    ImgSubresRange = StencilViewDesc->subresourceRange;
        //}
    }

    return true;
}
