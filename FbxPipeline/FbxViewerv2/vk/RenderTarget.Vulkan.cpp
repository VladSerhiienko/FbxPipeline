//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <RenderTarget.Vulkan.h>

/// -------------------------------------------------------------------------------------------------------------------
/// ColorResourceView
/// -------------------------------------------------------------------------------------------------------------------

std::shared_ptr<apemodevk::ColorResourceView> apemodevk::ColorResourceView::MakeNewLinked ()
{
    return std::shared_ptr<apemodevk::ColorResourceView> (new ColorResourceView ());
}

std::unique_ptr<apemodevk::ColorResourceView> apemodevk::ColorResourceView::MakeNewUnique ()
{
    return std::unique_ptr<apemodevk::ColorResourceView>(new ColorResourceView());
}

std::unique_ptr<apemodevk::ColorResourceView> apemodevk::ColorResourceView::MakeNullUnique ()
{
    return std::unique_ptr<apemodevk::ColorResourceView> (nullptr);
}

apemodevk::ColorResourceView::ColorResourceView () : bIsOwnedBySwapchain (false)
{
    Format   = VK_FORMAT_R8G8B8A8_UNORM;
    ViewType = kResourceViewType_RenderTarget;
}

apemodevk::ColorResourceView::~ColorResourceView()
{
    if (bIsOwnedBySwapchain)
    {
        ImgHandle.Release ();
        ImgViewHandle.Release ();
    }

    _Aux_DebugTraceFunc;
}

bool apemodevk::ColorResourceView::RecreateResourcesFor (GraphicsDevice & InGraphicsNode,
                                                    VkFormat         InFmt,
                                                    VkImage          InImg)
{
    ArrayLayerCount = 1;
    MipLevelCount   = 1;
    Depth           = 1;

    TInfoStruct<VkComponentMapping> CmpMapping;
    CmpMapping->r = VK_COMPONENT_SWIZZLE_R;
    CmpMapping->g = VK_COMPONENT_SWIZZLE_G;
    CmpMapping->b = VK_COMPONENT_SWIZZLE_B;
    CmpMapping->a = VK_COMPONENT_SWIZZLE_A;

    TInfoStruct<VkImageSubresourceRange> ImgSubresRange;
    ImgSubresRange->aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    ImgSubresRange->levelCount     = MipLevelCount;
    ImgSubresRange->layerCount     = ArrayLayerCount;
    ImgSubresRange->baseMipLevel   = 0;
    ImgSubresRange->baseArrayLayer = 0;


    TInfoStruct<VkImageViewCreateInfo> ImgViewDesc;
    ImgViewDesc->viewType         = VK_IMAGE_VIEW_TYPE_2D;
    ImgViewDesc->subresourceRange = ImgSubresRange;
    ImgViewDesc->components       = CmpMapping;
    ImgViewDesc->format           = InFmt;
    ImgViewDesc->image            = InImg;

    MemoryStates[ nullptr ] = MemoryState (VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                           VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                           VK_QUEUE_FAMILY_IGNORED,
                                           VK_IMAGE_LAYOUT_UNDEFINED,
                                           ImgSubresRange);

    // Swapchain owns images, no ownership will be acquired.
    if (!ImgHandle.Assign (InGraphicsNode, InGraphicsNode, InImg, false)
        || !ImgViewHandle.Recreate (InGraphicsNode, ImgViewDesc))
    {
        _Game_engine_Error("Failed to assign image / create image view.");
        return false;
    }

    Format              = InFmt;
    bIsOwnedBySwapchain = true;
    ResourceRef         = ResourceReference::MakeNewLinked (InGraphicsNode);

    return true;
}
