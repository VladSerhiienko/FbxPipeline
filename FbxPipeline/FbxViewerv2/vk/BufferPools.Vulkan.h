#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <Resource.Vulkan.h>
#include <M128CacheAlignedBuffer.h>

#define _Core_BufferPool_UseIntermediateSystemBuffer 1

namespace Core
{
    class CommandList;

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

    class _Graphics_ecosystem_dll_api BufferPool
        : public Aux::ScalableAllocPolicy
        , public Aux::NoCopyAssignPolicy
    {
    public:
        struct Page;
        struct Range;

    public:
        struct _Graphics_ecosystem_dll_api Range : public Aux::ScalableAllocPolicy
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
        struct _Graphics_ecosystem_dll_api Page : public Aux::ScalableAllocPolicy,
                                                  public Aux::NoCopyAssignPolicy
        {
            friend BufferPool;

            BufferPool *                      PoolRef;
            BufferPoolType                    PoolType;
            Aux::Lock                         Lock;
            TDispatchableHandle<VkDeviceMemory>          Memory;
            TDispatchableHandle<VkBuffer>                BufferHandle;
            TDispatchableHandle<VkBuffer>                BufferViewHandle;
            TbbAux::M128CacheAlignedBuffer    SupplyMemory;
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
        void OnCommandListReset (CommandList & CmdList);
        void OnCommandListExecutePreview (CommandList & CmdList);
        bool Suballocate (CommandList & CmdList, uint32_t Size, Range & OutRange);
        Range Suballocate (CommandList & CmdList, uint32_t Size);

    public:
        bool           IsOutOfMemory () const;
        BufferPoolType GetBufferPoolType () const;

    private:
        GraphicsDevice * pGraphicsNode;
        BufferPoolType   ePoolType;

        std::vector<Page *> Pages;
        std::vector<Page *> StalePages;
        Page **                      ppCurrentPage;
        Page **                      ppLastPage;
        bool                         bIsOutOfMemory;
    };

    template <typename TSrc> class TDynamicConstantBufferView
        : public TbbAux::TbbScalableAlignedAllocPolicy<TDynamicConstantBufferView<TSrc>>
        , public Aux::NoCopyAssignPolicy
    {
        TSrc              Src;
        bool              bIsSrcStale;
        BufferPool::Range ConstBufferRange;

    public:
        TDynamicConstantBufferView() : bIsSrcStale(false) { }
        _Force_inline_function TSrc &GetSrc() { bIsSrcStale = true; return Src; }
        _Force_inline_function TSrc const &GetSrc() const { return Src; }
        _Force_inline_function TSrc const &GetSrcReadOnly() const { return Src; }

    public:



    };

}