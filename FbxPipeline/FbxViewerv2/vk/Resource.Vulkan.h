#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>
#include <TInfoStruct.Vulkan.h>
//#include <TDataHandle.h>

namespace apemode
{
    class CommandList;
    class ResourceView;
    class BufferResourceView;
    class IntermediateResourceView;
    class IndexBufferResourceView;
    class VertexBufferResourceView;
    class ConstantBufferResource;
    class TextureResourceView;
    class ColorResourceView;
    class DepthStencilResourceView;

    class ResourceReference : public apemode::ScalableAllocPolicy,
                                                          public apemode::NoCopyAssignPolicy
    {
    public:

    public:
        static uint32_t GetElementSizeInBytes(VkFormat Format);
        static bool IsSRGBFormat(VkFormat Format);
        static bool IsDepthStencilFormat(VkFormat Format);
        static bool GetDepthStencilFormats(VkFormat DSVFormat, VkFormat & DepthFormat, VkFormat & StencilFormat);

    public:
        static bool GetMemoryTypeFromProperties(GraphicsDevice & InGraphicsNode,
                                                uint32_t         InTypeBits,
                                                VkFlags          InRequirementsMask,
                                                uint32_t &       OutTypeIndex);

    public:
        typedef apemode::TSafeDeleteObjOp<ResourceReference> ResourceDeleter;
        typedef std::shared_ptr<ResourceReference> LkPtr;
        typedef std::unique_ptr<ResourceReference> UqPtr;

    public:
        static LkPtr MakeNewLinked(GraphicsDevice & GraphicsNode);

    public:
        ResourceReference(GraphicsDevice & GraphicsNode);

    public:
        GraphicsDevice &                  GraphicsNode;
        TDispatchableHandle<VkDeviceMemory>          MemoryHandle;
        TInfoStruct<VkMemoryAllocateInfo> MemoryAlloc;
        TInfoStruct<VkMemoryRequirements> MemoryReqs;
    };

    class ResourceView : public apemode::ScalableAllocPolicy,
                                                     public apemode::NoCopyAssignPolicy
    {
    public:
        class MemoryState : public apemode::ScalableAllocPolicy
        {
        public:
            enum EType
            {
                kType_Undefined = 0,
                kType_Buffer,
                kType_Image,
            };

            EType                eType;
            VkAccessFlags        AccessMask;
            uint32_t             QueueFamily;
            VkPipelineStageFlags PipelineStageFlags;

            union
            {
                struct
                {
                    uint32_t BufferSize;
                    uint32_t BufferOffset;
                };
                struct
                {
                    VkImageLayout           ImgLayout;
                    VkImageSubresourceRange ImgSubresRange;
                };
            };

        public:
            /** Creates invalid state. */
            MemoryState ();

            /** Creates buffer state. */
            MemoryState (VkPipelineStageFlags PipelineStageFlags,
                         VkAccessFlags        AccessMask,
                         uint32_t             QueueFamily,
                         uint32_t             BufferSize,
                         uint32_t             BufferOffset);

            /** Creates image state. */
            MemoryState (VkPipelineStageFlags    PipelineStageFlags,
                         VkAccessFlags           AccessMask,
                         uint32_t                QueueFamily,
                         VkImageLayout           ImgLayout,
                         VkImageSubresourceRange ImgSubresRange);

            bool IsValid () const;
            bool IsImgBarrier () const;
        };

        enum ResourceViewType
        {
            kResourceViewType_Generic            = 0x000,
            kResourceViewType_Buffer             = 0x001,
            kResourceViewType_Texture            = 0x002,
            kResourceViewType_IntermediateBuffer = 0x010 | kResourceViewType_Buffer,
            kResourceViewType_ConstantBuffer     = 0x020 | kResourceViewType_Buffer,
            kResourceViewType_IndexBuffer        = 0x030 | kResourceViewType_Buffer,
            kResourceViewType_VertexBuffer       = 0x040 | kResourceViewType_Buffer,
            kResourceViewType_RenderTarget       = 0x010 | kResourceViewType_Texture,
            kResourceViewType_DepthStencil       = 0x020 | kResourceViewType_Texture,
            kResourceViewTypeBaseMask            = 0x00f,
        };

        static ResourceViewType GetBaseResourceViewType (ResourceViewType eType);

    public:
        ResourceView ();
        inline bool HasResource () { return ResourceRef.get() != nullptr; }

        template <typename U> inline bool CanGet() const { return false; }
        template <> inline bool CanGet<ResourceView>() const { return true; }
        template <> inline bool CanGet<IntermediateResourceView>() const { return CanGet<BufferResourceView>(); }
        template <> inline bool CanGet<BufferResourceView>() const { return apemode::HasFlagEql(ViewType, kResourceViewType_Buffer); }
        template <> inline bool CanGet<TextureResourceView>() const { return apemode::HasFlagEql(ViewType, kResourceViewType_Texture); }
        template <> inline bool CanGet<ConstantBufferResource>() const { return ViewType == kResourceViewType_ConstantBuffer; }
        template <> inline bool CanGet<IndexBufferResourceView>() const { return ViewType == kResourceViewType_IndexBuffer; }
        template <> inline bool CanGet<VertexBufferResourceView>() const { return ViewType == kResourceViewType_VertexBuffer; }
        template <> inline bool CanGet<ColorResourceView>() const { return ViewType == kResourceViewType_RenderTarget; }
        template <> inline bool CanGet<DepthStencilResourceView>() const { return ViewType == kResourceViewType_DepthStencil; }

        inline bool IsPlainResource() { return CanGet<BufferResourceView>(); }
        inline bool IsOpaqueResource() { return CanGet<TextureResourceView>(); }

        template <typename U> inline U *TryGet() { if (CanGet<U>()) return static_cast<U*>(this); return nullptr; }
        template <typename U> inline U const *TryGet() const { if (CanGet<U>()) return static_cast<U const*>(this); return nullptr; }

        MemoryState GetState(apemode::CommandList & CmdList) const;
        void SetState (apemode::CommandList & CmdList, MemoryState const & State);

    public:
        ResourceViewType                              ViewType;
        VkDeviceSize                                  VirtualAddressOffset;
        std::shared_ptr<ResourceReference>            ResourceRef;
        std::map<CommandList *, MemoryState> MemoryStates;
    };

}

_Game_engine_Define_enum_flag_operators(apemode::ResourceView::ResourceViewType);
//_Game_engine_Define_enum_flag_operators(apemode::ResourceView::ResourceAccessFlags);

