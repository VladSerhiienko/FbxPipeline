#pragma once

#include <Resource.Vulkan.h>

namespace apemode
{
    class TextureResourceView : public ResourceView
    {
    public:
        template <typename N = decltype(ElementCount)>  N GetElementCount() const { return static_cast<N> (ElementCount); }
        template <typename N = decltype(ElementStride)> N GetElementSizeInBytes() const { return static_cast<N> (ElementStride); }
        template <typename N = decltype(ElementStride)> N GetTotalSizeInBytes() const { return static_cast<N> (ElementStride * ElementCount); }

    public:
        static std::shared_ptr<TextureResourceView> MakeNewLinked ();
        static std::unique_ptr<TextureResourceView> MakeNewUnique ();

    public:
        TextureResourceView();
        ~TextureResourceView();

        void SetState(apemode::CommandBuffer &  CmdBuffer,
                      VkPipelineStageFlags PipelineStageFlags,
                      VkAccessFlags        AccessMask,
                      VkImageLayout        ImgLayout,
                      uint32_t             BaseMipLevel    = 0,
                      uint32_t             MipLevelCount   = 1,
                      uint32_t             BaseArrayLayer  = 0,
                      uint32_t             ArrayLayerCount = 1,
                      uint32_t             QueueFamily     = VK_QUEUE_FAMILY_IGNORED);

    public:
        bool                                   bIsCube;
        uint32_t                               Width;
        uint32_t                               Height;
        uint32_t                               Depth;
        uint16_t                               ArrayLayerCount;
        uint16_t                               MipLevelCount;
        VkFormat                               Format;
        VkImageType                            ImgType;
        VkImageViewType                        ImgViewType;
        VkClearValue                           ClearColor;
        VkSampleCountFlags                     SampleCount;
        VkImageTiling                          ImgTiling;
        VkImageUsageFlags                      ImgUsage;
        VkImageAspectFlags                     ImgAspect;
        apemode::TDispatchableHandle<VkImage>     ImgHandle;
        apemode::TDispatchableHandle<VkImageView> ImgViewHandle;
    };

}