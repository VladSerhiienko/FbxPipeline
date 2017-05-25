#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>

namespace apemode
{
    class CommandList;
    class CommandQueue;
    class RenderPass;
    class Framebuffer;
    class RootSignature;
    class PipelineState;

    class _Graphics_ecosystem_dll_api CommandList : public apemode::ScalableAllocPolicy,
                                                    public apemode::NoCopyAssignPolicy
    {
    public:
        enum CommandListType
        {
            kCommandListType_Direct = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            kCommandListType_Bundle = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
            kCommandListType_Invalid = -1
        };

    public:
        struct BeginEndScope;
        friend BeginEndScope;

    public:
        using BarrierVector       = std::vector<VkMemoryBarrier>;
        using ImageBarrierVector  = std::vector<VkImageMemoryBarrier>;
        using BufferBarrierVector = std::vector<VkBufferMemoryBarrier>;
        using PtrVector           = std::vector<CommandList *>;

    public:
        static std::unique_ptr<CommandList> MakeNewUnique ();
        static std::shared_ptr<CommandList> MakeNewLinked ();

    public:
        struct _Graphics_ecosystem_dll_api BeginEndScope
        {
            CommandList & AssociatedCmdList;
            BeginEndScope (CommandList & CmdList, bool bOneTimeSubmit);
            BeginEndScope (CommandList &                          CmdList,
                           VkCommandBufferInheritanceInfo const & Inheritance,
                           bool                                   bOneTimeSubmit);
            ~BeginEndScope ();
        };

    public:
        CommandList();

        /**
         * Creates command buffer and command pool objects.
         * @returns True if succeeded, false otherwise.
         */
        bool RecreateResourcesFor(GraphicsDevice & GraphicsNode,
                                  uint32_t         QueueFamilyId,
                                  bool             bIsDirect,
                                  bool             bIsTransient);

        bool Reset(bool bReleaseResources);

        void InsertBarrier(VkPipelineStageFlags    SrcPipelineStageFlags,
                           VkPipelineStageFlags    DstPipelineStageFlags,
                           VkMemoryBarrier const & Barrier);

        void InsertBarrier(VkPipelineStageFlags         SrcPipelineStageFlags,
                           VkPipelineStageFlags         DstPipelineStageFlags,
                           VkImageMemoryBarrier const & Barrier);

        void InsertBarrier(VkPipelineStageFlags          SrcPipelineStageFlags,
                           VkPipelineStageFlags          DstPipelineStageFlags,
                           VkBufferMemoryBarrier const & Barrier);

        void FlushStagedBarriers();

    public:
        bool IsDirect () const;

    public:
        operator VkCommandBuffer() const;

    protected:
        struct StagedBarrier : public apemode::ScalableAllocPolicy
        {
            enum EType
            {
                kType_Global,
                kType_Img,
                kType_Buffer,
            };

            union
            {
                struct
                {
                    VkPipelineStageFlags SrcStage;
                    VkPipelineStageFlags DstStage;
                };

                static_assert (sizeof (VkPipelineStageFlags) == sizeof (uint32_t),
                               "Reconsider hashing.");

                uint64_t StageHash;
            };

            union
            {
                VkStructureType       BarrierType;
                VkMemoryBarrier       Barrier;
                VkImageMemoryBarrier  ImgBarrier;
                VkBufferMemoryBarrier BufferBarrier;
            };

            StagedBarrier (VkPipelineStageFlags    SrcStage,
                           VkPipelineStageFlags    DstStage,
                           VkMemoryBarrier const & Barrier);

            StagedBarrier (VkPipelineStageFlags         SrcStage,
                           VkPipelineStageFlags         DstStage,
                           VkImageMemoryBarrier const & Barrier);

            StagedBarrier (VkPipelineStageFlags          SrcStage,
                           VkPipelineStageFlags          DstStage,
                           VkBufferMemoryBarrier const & Barrier);

            using Key       = uint64_t;
            using KeyLessOp = std::less<uint64_t>;
            using Multimap  = std::multimap<const Key, const StagedBarrier, KeyLessOp>;
            using It        = Multimap::iterator;
            using ItRange   = std::pair<It, It>;
            using Pair      = std::pair<Key, StagedBarrier>;
        };

    public:
        CommandListType eType;
        bool            bIsInBeginEndScope;

        StagedBarrier::Multimap StagedBarriers;
        BarrierVector           Barriers;
        ImageBarrierVector      ImgBarriers;
        BufferBarrierVector     BufferBarriers;
        uint32_t                BarrierCount;
        uint32_t                ImgBarrierCount;
        uint32_t                BufferBarrierCount;

        apemode::RenderPass const *                   pRenderPass;
        apemode::Framebuffer const *                  pFramebuffer;
        apemode::RootSignature const *                pRootSignature;
        apemode::PipelineState const *                pPipelineState;
        apemode::TDispatchableHandle<VkCommandBuffer> hCmdList;
        apemode::TDispatchableHandle<VkCommandPool>   hCmdAlloc;
    };

    /**
     * Stores reserved command queues of devices. This class is used by queues,
     * but can also be potentially used by the graphics devices.
     */
    class _Graphics_ecosystem_dll_api CommandQueueReserver : public apemode::ScalableAllocPolicy,
                                                             public apemode::NoCopyAssignPolicy
    {
        friend CommandQueue;
        friend GraphicsDevice;

        struct Key
        {
            uint64_t GraphicsNodeHash;

            union 
            {
                struct
                {
                    uint32_t QueueId;
                    uint32_t QueueFamilyId;
                };

                uint64_t QueueHash;
            };

            Key();
            struct Hasher { size_t operator () (Key const &) const; };
            struct CmpOpLess { bool operator () (Key const &, Key const &) const; };
            struct CmpOpEqual { bool operator () (Key const &, Key const &) const; };
            static Key NewKeyFor (GraphicsDevice const & GraphicsNode, uint32_t QueueFamilyId, uint32_t QueueId);
        };

        struct Reservation
        {
            CommandQueue * pQueue;
            uint32_t       QueueId;
            uint32_t       QueueFamilyId;

            Reservation();
            bool IsValid() const;
            void Release();

            using CmpOpLess       = Key::CmpOpLess;
            using LookupContainer = std::map<Key, Reservation, CmpOpLess>;
        };

        Reservation::LookupContainer ReservationStorage;

        CommandQueueReserver ();
        /** Returns True if this queue was not created previously, otherwise false. */
        bool TryReserve (GraphicsDevice const & GraphicsNode, uint32_t QueueFamilyId, uint32_t QueueId);
        /** Must be called when the queue gets destructed to avoid resources leaks. */
        void Unreserve (GraphicsDevice const & GraphicsNode, uint32_t QueueFamilyId, uint32_t QueueId);
        /** Returns queue reserver instance. */
        static CommandQueueReserver & Get ();
    };

    class _Graphics_ecosystem_dll_api CommandQueue : public apemode::ScalableAllocPolicy,
                                                     public apemode::NoCopyAssignPolicy
    {
    public:
        CommandQueue ();
        ~CommandQueue ();

    public:
        bool RecreateResourcesFor (GraphicsDevice & GraphicsNode,
                                   uint32_t         QueueFamilyId,
                                   uint32_t         QueueId);

        bool Execute (CommandList & CmdList,
                      VkSemaphore * hWaitSemaphore,
                      uint32_t      WaitSemaphoreCount,
                      VkFence       hFence);

        bool Execute (CommandList & CmdList, VkFence hFence);
        bool Execute (CommandList * pCmdLists, uint32_t CmdListCount, VkFence Fence);

        /** 
         * Waits on the completion of all work within a single queue.
         * It will block until all command buffers and sparse binding 
         * operations in the queue have completed.
         */
        bool Await ();

    public:
        operator VkQueue () const;

    public:
        GraphicsDevice *             pGraphicsNode;
        TDispatchableHandle<VkQueue> hCmdQueue;
        uint32_t                     QueueFamilyId;
        uint32_t                     QueueId;
    };

}