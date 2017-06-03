#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <Resource.Vulkan.h>
//#include <M128CacheAlignedBuffer.h>

#define _Core_BufferPool_UseIntermediateSystemBuffer 1

namespace apemodevk
{
    class CommandBuffer;

    enum BufferPoolType
    {
        kBufferPoolType_CPU = 0x00,
        kBufferPoolType_GPU = 0x01,
        kBufferPoolTypeCount = 2,
    };

    enum BufferPoolStandardPageSize : size_t
    {
        kBufferPoolStandardPageSize_Unknown = 0,
        kBufferPoolStandardPageSize_CPU = 0x00010000,
        kBufferPoolStandardPageSize_GPU = 0x00200000,
        kBufferPoolStandardPageSize_Invalid = SIZE_MAX,
    };

    class BufferPool
        : public apemodevk::ScalableAllocPolicy
        , public apemodevk::NoCopyAssignPolicy
    {
    public:
        struct Page;
        struct Range;

    public:
        struct Range : public apemodevk::ScalableAllocPolicy
        {
            Page *                              PageRef;
            VkDeviceSize                        Offset;
            void *                              Address;
            TInfoStruct<VkDescriptorBufferInfo> DescriptorInfo;

            Range ()
            {
            }
        };

    public:
        struct Page : public apemodevk::ScalableAllocPolicy,
                                                  public apemodevk::NoCopyAssignPolicy
        {
            friend BufferPool;

            BufferPool *                      PoolRef;
            BufferPoolType                    PoolType;
            //std::mutex                         Lock;
            TDispatchableHandle<VkDeviceMemory>          Memory;
            TDispatchableHandle<VkBuffer>                BufferHandle;
            TDispatchableHandle<VkBuffer>                BufferViewHandle;
            //TbbAux::M128CacheAlignedBuffer    SupplyMemory;
            TInfoStruct<VkMemoryRequirements> MemoryReqs;
            TInfoStruct<VkMemoryAllocateInfo> MemoryAllocInfo;
            bool                              bIsHostVisible;
            bool                              bIsCacheCoherent;

        public:
            static Page *                MakeNew ();
            static std::shared_ptr<Page> MakeNewShared ();
            static std::unique_ptr<Page> MakeNewUnique ();

        public:
            Page ();
            ~Page ();
        };

    public:
        explicit BufferPool(BufferPoolType PoolType);
        ~BufferPool();

        bool RecreateResourcesFor (GraphicsDevice & GraphicsNode);
        void OnResourcesEvicted (GraphicsDevice & GraphicsNode);
        void OnCommandListReset (CommandBuffer & CmdBuffer);
        void OnCommandListExecutePreview (CommandBuffer & CmdBuffer);
        bool Suballocate (CommandBuffer & CmdBuffer, uint32_t Size, Range & OutRange);
        Range Suballocate (CommandBuffer & CmdBuffer, uint32_t Size);

    public:
        bool           IsOutOfMemory () const;
        BufferPoolType GetBufferPoolType () const;

    private:
        GraphicsDevice * pNode;
        BufferPoolType   ePoolType;

        std::vector<Page *> Pages;
        std::vector<Page *> StalePages;
        Page **                      ppCurrentPage;
        Page **                      ppLastPage;
        bool                         bIsOutOfMemory;
    };

    template <typename TSrc> class TDynamicConstantBufferView
        //: public TbbAux::TbbScalableAlignedAllocPolicy<TDynamicConstantBufferView<TSrc>>
        //, public apemodevk::NoCopyAssignPolicy
    {
        TSrc              Src;
        bool              bIsSrcStale;
        BufferPool::Range ConstBufferRange;

    public:
        TDynamicConstantBufferView() : bIsSrcStale(false) { }
        inline TSrc &GetSrc() { bIsSrcStale = true; return Src; }
        inline TSrc const &GetSrc() const { return Src; }
        inline TSrc const &GetSrcReadOnly() const { return Src; }

    public:



    };

}