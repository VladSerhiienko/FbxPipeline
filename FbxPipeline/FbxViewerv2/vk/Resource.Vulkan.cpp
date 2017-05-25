//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <Resource.Vulkan.h>

#include <GraphicsEcosystems.Vulkan.h>

//#include <TDataHandle.h>
#include <TInfoStruct.Vulkan.h>

#include <GraphicsEcosystem.KnownExtensions.Vulkan.h>
#include <GraphicsEcosystem.PrivateContent.Vulkan.h>
#include <GraphicsDevice.PrivateContent.Vulkan.h>

/// -------------------------------------------------------------------------------------------------------------------
/// ResourceReference
/// -------------------------------------------------------------------------------------------------------------------

Core::ResourceReference::LkPtr Core::ResourceReference::MakeNewLinked(GraphicsDevice & GraphicsNode)
{
    return std::make_shared<ResourceReference>(GraphicsNode);
}

Core::ResourceReference::ResourceReference(GraphicsDevice & GraphicsNode)
    : GraphicsNode(GraphicsNode)
{
}

uint32_t Core::ResourceReference::GetElementSizeInBytes(VkFormat Format)
{
    switch (Format)
    {
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8_SNORM:
    case VK_FORMAT_R8_USCALED:
    case VK_FORMAT_R8_SSCALED:
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R8_SRGB:
    case VK_FORMAT_S8_UINT:
        return 1;

    case VK_FORMAT_R4G4_UNORM_PACK8:
    case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
    case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
    case VK_FORMAT_R5G6B5_UNORM_PACK16:
    case VK_FORMAT_B5G6R5_UNORM_PACK16:
    case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
    case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
    case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
    case VK_FORMAT_R8G8_UNORM:
    case VK_FORMAT_R8G8_SNORM:
    case VK_FORMAT_R8G8_SSCALED:
    case VK_FORMAT_R8G8_USCALED:
    case VK_FORMAT_R8G8_UINT:
    case VK_FORMAT_R8G8_SINT:
    case VK_FORMAT_R8G8_SRGB:
    case VK_FORMAT_R16_UNORM:
    case VK_FORMAT_R16_SNORM:
    case VK_FORMAT_R16_USCALED:
    case VK_FORMAT_R16_SSCALED:
    case VK_FORMAT_R16_UINT:
    case VK_FORMAT_R16_SINT:
    case VK_FORMAT_R16_SFLOAT:
        return 2;

    case VK_FORMAT_R8G8B8_UNORM:
    case VK_FORMAT_R8G8B8_SNORM:
    case VK_FORMAT_R8G8B8_USCALED:
    case VK_FORMAT_R8G8B8_SSCALED:
    case VK_FORMAT_R8G8B8_UINT:
    case VK_FORMAT_R8G8B8_SINT:
    case VK_FORMAT_R8G8B8_SRGB:
    case VK_FORMAT_B8G8R8_UNORM:
    case VK_FORMAT_B8G8R8_SNORM:
    case VK_FORMAT_B8G8R8_USCALED:
    case VK_FORMAT_B8G8R8_SSCALED:
    case VK_FORMAT_B8G8R8_UINT:
    case VK_FORMAT_B8G8R8_SINT:
    case VK_FORMAT_B8G8R8_SRGB:
    case VK_FORMAT_D16_UNORM_S8_UINT:
        return 3;

    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SNORM:
    case VK_FORMAT_R8G8B8A8_USCALED:
    case VK_FORMAT_R8G8B8A8_SSCALED:
    case VK_FORMAT_R8G8B8A8_UINT:
    case VK_FORMAT_R8G8B8A8_SINT:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_SNORM:
    case VK_FORMAT_B8G8R8A8_USCALED:
    case VK_FORMAT_B8G8R8A8_SSCALED:
    case VK_FORMAT_B8G8R8A8_UINT:
    case VK_FORMAT_B8G8R8A8_SINT:
    case VK_FORMAT_B8G8R8A8_SRGB:
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
    case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
    case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
    case VK_FORMAT_A8B8G8R8_UINT_PACK32:
    case VK_FORMAT_A8B8G8R8_SINT_PACK32:
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
    case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
    case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
    case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
    case VK_FORMAT_A2R10G10B10_UINT_PACK32:
    case VK_FORMAT_A2R10G10B10_SINT_PACK32:
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
    case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
    case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
    case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:
    case VK_FORMAT_A2B10G10R10_SINT_PACK32:
    case VK_FORMAT_R16G16_UNORM:
    case VK_FORMAT_R16G16_SNORM:
    case VK_FORMAT_R16G16_USCALED:
    case VK_FORMAT_R16G16_SSCALED:
    case VK_FORMAT_R16G16_UINT:
    case VK_FORMAT_R16G16_SINT:
    case VK_FORMAT_R16G16_SFLOAT:
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32_SINT:
    case VK_FORMAT_R32_SFLOAT:
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
        return 4;

    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return 5;

    case VK_FORMAT_R16G16B16_UNORM:
    case VK_FORMAT_R16G16B16_SNORM:
    case VK_FORMAT_R16G16B16_USCALED:
    case VK_FORMAT_R16G16B16_SSCALED:
    case VK_FORMAT_R16G16B16_UINT:
    case VK_FORMAT_R16G16B16_SINT:
    case VK_FORMAT_R16G16B16_SFLOAT:
        return 6;

    case VK_FORMAT_R16G16B16A16_UNORM:
    case VK_FORMAT_R16G16B16A16_SNORM:
    case VK_FORMAT_R16G16B16A16_USCALED:
    case VK_FORMAT_R16G16B16A16_SSCALED:
    case VK_FORMAT_R16G16B16A16_UINT:
    case VK_FORMAT_R16G16B16A16_SINT:
    case VK_FORMAT_R16G16B16A16_SFLOAT:
    case VK_FORMAT_R32G32_UINT:
    case VK_FORMAT_R32G32_SINT:
    case VK_FORMAT_R32G32_SFLOAT:
    case VK_FORMAT_R64_UINT:
    case VK_FORMAT_R64_SINT:
    case VK_FORMAT_R64_SFLOAT:
        return 8;

    case VK_FORMAT_R32G32B32_UINT:
    case VK_FORMAT_R32G32B32_SINT:
    case VK_FORMAT_R32G32B32_SFLOAT:
        return 12;

    case VK_FORMAT_R32G32B32A32_UINT:
    case VK_FORMAT_R32G32B32A32_SINT:
    case VK_FORMAT_R32G32B32A32_SFLOAT:
    case VK_FORMAT_R64G64_UINT:
    case VK_FORMAT_R64G64_SINT:
    case VK_FORMAT_R64G64_SFLOAT:
        return 16;

    case VK_FORMAT_R64G64B64_UINT:
    case VK_FORMAT_R64G64B64_SINT:
    case VK_FORMAT_R64G64B64_SFLOAT:
        return 24;

    case VK_FORMAT_R64G64B64A64_UINT:
    case VK_FORMAT_R64G64B64A64_SINT:
    case VK_FORMAT_R64G64B64A64_SFLOAT:
        return 36;

    default:
        return 0;
    }
}

bool Core::ResourceReference::IsSRGBFormat(VkFormat Format)
{
    switch (Format)
    {
    case VK_FORMAT_R8_SRGB:
    case VK_FORMAT_R8G8_SRGB:
    case VK_FORMAT_R8G8B8_SRGB:
    case VK_FORMAT_B8G8R8_SRGB:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_B8G8R8A8_SRGB:
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
    case VK_FORMAT_BC2_SRGB_BLOCK:
    case VK_FORMAT_BC3_SRGB_BLOCK:
    case VK_FORMAT_BC7_SRGB_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
        return true;
    }

    return false;
}

bool Core::ResourceReference::IsDepthStencilFormat (VkFormat Format)
{
    switch (Format)
    {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_S8_UINT:
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return true;
    }

    return false;
}

bool Core::ResourceReference::GetDepthStencilFormats(VkFormat DSVFormat, VkFormat & DepthFormat, VkFormat & StencilFormat)
{
    bool bOk = false;

    switch (DSVFormat)
    {
    case VK_FORMAT_D16_UNORM:
        DepthFormat = VK_FORMAT_D16_UNORM;
        StencilFormat = VK_FORMAT_UNDEFINED;
        bOk = true;
        break;
    case VK_FORMAT_D32_SFLOAT:
        DepthFormat = VK_FORMAT_D16_UNORM;
        StencilFormat = VK_FORMAT_UNDEFINED;
        bOk = true;
        break;
    case VK_FORMAT_S8_UINT:
        DepthFormat = VK_FORMAT_UNDEFINED;
        StencilFormat = VK_FORMAT_S8_UINT;
        bOk = true;
        break;
    case VK_FORMAT_D16_UNORM_S8_UINT:
        DepthFormat = VK_FORMAT_D16_UNORM;
        StencilFormat = VK_FORMAT_S8_UINT;
        bOk = true;
        break;
    case VK_FORMAT_D24_UNORM_S8_UINT:
        DepthFormat = VK_FORMAT_X8_D24_UNORM_PACK32;
        StencilFormat = VK_FORMAT_S8_UINT;
        bOk = true;
        break;
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        DepthFormat = VK_FORMAT_D32_SFLOAT;
        StencilFormat = VK_FORMAT_S8_UINT;
        bOk = true;
        break;
    }

    return bOk;
}

bool Core::ResourceReference::GetMemoryTypeFromProperties(GraphicsDevice & InGraphicsNode,
                                                          uint32_t         InTypeBits,
                                                          VkFlags          InRequirementsMask,
                                                          uint32_t &       OutTypeIndex)
{
    auto &     NodeContent     = static_cast<GraphicsDevice::PrivateContent &>(InGraphicsNode);
    auto const NodeMemoryTypeCount = _Get_array_length(NodeContent.DeviceHandle.MemoryProps.memoryTypes);

    // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < NodeMemoryTypeCount; i++)
    {
        if ((InTypeBits & 1) == 1)
        {
            // Type is available, does it match user properties?
            auto & TypeProps = NodeContent.DeviceHandle.MemoryProps.memoryTypes[i];
            if ((TypeProps.propertyFlags & InRequirementsMask) == InRequirementsMask)
            {
                OutTypeIndex = i;
                return true;
            }
        }

        InTypeBits >>= 1;
    }

    // No memory types matched, return failure
    return false;
}

/// -------------------------------------------------------------------------------------------------------------------
/// ResourceView
/// -------------------------------------------------------------------------------------------------------------------

Core::ResourceView::ResourceView()
{
    //MemoryStates.reserve(16);
}

Core::ResourceView::MemoryState Core::ResourceView::GetState(Core::CommandList & CmdList) const
{
    const auto StateIt = MemoryStates.find (&CmdList);
    const bool bHasCmdListAssociation = StateIt != MemoryStates.end ();
    return bHasCmdListAssociation ? StateIt->second : MemoryState ();
}

void Core::ResourceView::SetState (Core::CommandList & CmdList, MemoryState const & State)
{
    MemoryStates[&CmdList ] = State;
}

Core::ResourceView::ResourceViewType
Core::ResourceView::GetBaseResourceViewType (ResourceViewType eViewType)
{
    return (eViewType & kResourceViewTypeBaseMask);
}

Core::ResourceView::MemoryState::MemoryState ()
    : eType (kType_Undefined)
    , AccessMask (VK_ACCESS_FLAG_BITS_MAX_ENUM)
    , ImgLayout (VK_IMAGE_LAYOUT_MAX_ENUM)
    , QueueFamily (VK_QUEUE_FAMILY_IGNORED)
    , PipelineStageFlags (VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM)
{
    ImgSubresRange.aspectMask     = VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;
    ImgSubresRange.baseArrayLayer = -1;
    ImgSubresRange.baseMipLevel   = -1;
    ImgSubresRange.layerCount     = -1;
    ImgSubresRange.levelCount     = -1;
}

Core::ResourceView::MemoryState::MemoryState (VkPipelineStageFlags PipelineStageFlags,
                                              VkAccessFlags        AccessMask,
                                              uint32_t             QueueFamily,
                                              uint32_t             BufferSize,
                                              uint32_t             BufferOffset)
    : eType (kType_Buffer)
    , AccessMask (AccessMask)
    , QueueFamily (QueueFamily)
    , PipelineStageFlags (PipelineStageFlags)
    , BufferSize (BufferSize)
    , BufferOffset (BufferOffset)
{
}

Core::ResourceView::MemoryState::MemoryState (VkPipelineStageFlags    PipelineStageFlags,
                                              VkAccessFlags           AccessMask,
                                              uint32_t                QueueFamily,
                                              VkImageLayout           ImgLayout,
                                              VkImageSubresourceRange ImgSubresRange)
    : eType (kType_Image)
    , AccessMask (AccessMask)
    , QueueFamily (QueueFamily)
    , PipelineStageFlags (PipelineStageFlags)
    , ImgLayout (ImgLayout)
    , ImgSubresRange (ImgSubresRange)
{
}

bool Core::ResourceView::MemoryState::IsValid () const
{
    return eType != kType_Undefined;
}

bool Core::ResourceView::MemoryState::IsImgBarrier () const
{
    return eType == kType_Image;
}
