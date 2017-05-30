//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <CommandQueue.Vulkan.h>

#include <CityHash.h>
#include <TInfoStruct.Vulkan.h>

/// -------------------------------------------------------------------------------------------------------------------
/// CommandBuffer
/// -------------------------------------------------------------------------------------------------------------------

std::unique_ptr<apemode::CommandBuffer> apemode::CommandBuffer::MakeNewUnique ()
{
    return std::unique_ptr<apemode::CommandBuffer> (new CommandBuffer ());
}

std::shared_ptr<apemode::CommandBuffer> apemode::CommandBuffer::MakeNewLinked ()
{
    return std::shared_ptr<apemode::CommandBuffer> (new CommandBuffer ());
}

apemode::CommandBuffer::CommandBuffer ()
    : eType (kCommandListType_Invalid)
    , pRenderPass (nullptr)
    , pFramebuffer (nullptr)
    , pPipelineLayout (nullptr)
    , pPipelineState (nullptr)
    , BarrierCount (0)
    , ImgBarrierCount (0)
    , BufferBarrierCount (0)
{
}

/// -------------------------------------------------------------------------------------------------------------------

bool apemode::CommandBuffer::RecreateResourcesFor (GraphicsDevice & GraphicsNode,
                                              uint32_t         QueueFamilyId,
                                              bool             bIsDirect,
                                              bool             bIsTransient)
{
    static const VkCommandPoolCreateFlags CmdPoolTransientWithIndividualReset
        = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT
        | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    static const VkCommandPoolCreateFlags CmdPoolWithIndividualReset
        = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    const VkCommandPoolCreateFlags CmdAllocType = bIsTransient
        ? CmdPoolTransientWithIndividualReset
        : CmdPoolWithIndividualReset;

    const VkCommandBufferLevel CmdListType = bIsDirect 
        ? VK_COMMAND_BUFFER_LEVEL_PRIMARY 
        : VK_COMMAND_BUFFER_LEVEL_SECONDARY;

    TInfoStruct<VkCommandPoolCreateInfo> CmdAllocDesc;
    CmdAllocDesc->flags            = CmdAllocType;
    CmdAllocDesc->queueFamilyIndex = QueueFamilyId;

    if (!hCmdAlloc.Recreate (GraphicsNode, CmdAllocDesc))
    {
        _Game_engine_Halt("Failed to create command allocator.");
        return false;
    }

    TInfoStruct<VkCommandBufferAllocateInfo> CmdListDesc;
    CmdListDesc->level              = CmdListType;
    CmdListDesc->commandPool        = hCmdAlloc;
    CmdListDesc->commandBufferCount = 1;

    if (!hCmdList.Allocate (GraphicsNode, CmdListDesc))
    {
        _Game_engine_Halt ("Failed to create command list.");
        return false;
    }

    eType = static_cast<CommandListType> (CmdListType);
    return true;
}

/// -------------------------------------------------------------------------------------------------------------------

bool apemode::CommandBuffer::Reset (bool bReleaseResources)
{
    const VkCommandBufferResetFlags ResetFlags = bReleaseResources 
        ? VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT 
        : 0u;

    const ResultHandle ErrorHandle = vkResetCommandBuffer (hCmdList, ResetFlags);
    return ErrorHandle.Succeeded ();
}

static uint64_t ComposeKey (VkPipelineStageFlags SrcFlags, VkPipelineStageFlags DstFlags)
{
    return static_cast<uint64_t> (SrcFlags) << 32 | static_cast<uint64_t> (DstFlags);
}

void apemode::CommandBuffer::InsertBarrier (VkPipelineStageFlags    SrcFlags,
                                       VkPipelineStageFlags    DstFlags,
                                       VkMemoryBarrier const & Barrier)
{
    ++BarrierCount;

    StagedBarrier const CmdBarrier (SrcFlags, DstFlags, Barrier);
    StagedBarriers.insert (std::make_pair<> (CmdBarrier.StageHash, CmdBarrier));
}

void apemode::CommandBuffer::InsertBarrier (VkPipelineStageFlags         SrcFlags,
                                       VkPipelineStageFlags         DstFlags,
                                       VkImageMemoryBarrier const & Barrier)
{
    ++ImgBarrierCount;

    StagedBarrier const CmdBarrier (SrcFlags, DstFlags, Barrier);
    StagedBarriers.insert (std::make_pair<> (CmdBarrier.StageHash, CmdBarrier));
}

void apemode::CommandBuffer::InsertBarrier (VkPipelineStageFlags          SrcFlags,
                                       VkPipelineStageFlags          DstFlags,
                                       VkBufferMemoryBarrier const & Barrier)
{
    ++BufferBarrierCount;

    StagedBarrier const CmdBarrier (SrcFlags, DstFlags, Barrier);
    StagedBarriers.insert (std::make_pair<> (CmdBarrier.StageHash, CmdBarrier));
}

void apemode::CommandBuffer::FlushStagedBarriers()
{
    Barriers.reserve (BarrierCount);
    ImgBarriers.reserve (ImgBarrierCount);
    BufferBarriers.reserve (BufferBarrierCount);

    BarrierCount       = 0;
    ImgBarrierCount    = 0;
    BufferBarrierCount = 0;

    auto FillBarriersFn = [this](StagedBarrier::Pair const & KeydStagedBarrier)
    {
        switch (KeydStagedBarrier.second.BarrierType)
        {
        case VK_STRUCTURE_TYPE_MEMORY_BARRIER:
            Barriers.push_back (KeydStagedBarrier.second.Barrier);
            break;
        case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
            ImgBarriers.push_back (KeydStagedBarrier.second.ImgBarrier);
            break;
        case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
            BufferBarriers.push_back (KeydStagedBarrier.second.BufferBarrier);
            break;
        default:
            _Game_engine_Halt ("Memory corruption.");
            break;
        }
    };

    StagedBarrier::ItRange StagedBarrierItRange;
    StagedBarrier::It      StagedBarrierIt = StagedBarriers.begin ();
    for (; StagedBarrierIt != StagedBarriers.cend (); StagedBarrierIt = StagedBarrierItRange.second)
    {
        StagedBarrierItRange = StagedBarriers.equal_range(StagedBarrierIt->first);
        std::for_each(StagedBarrierItRange.first, StagedBarrierItRange.second, FillBarriersFn);

        _Game_engine_Assert (!Barriers.empty ()
                             || !ImgBarriers.empty ()
                             || !BufferBarriers.empty (),
                             "Nothing to submit to the command list.");

        if (apemode_likely (!Barriers.empty ()
                                 || !ImgBarriers.empty ()
                                 || !BufferBarriers.empty ()))
        {
            vkCmdPipelineBarrier (hCmdList,
                                  StagedBarrierItRange.first->second.SrcStage,
                                  StagedBarrierItRange.first->second.DstStage,
                                  static_cast<VkDependencyFlags> (0),
                                  _Get_collection_length_u (Barriers),
                                  Barriers.data (),
                                  _Get_collection_length_u (BufferBarriers),
                                  BufferBarriers.data (),
                                  _Get_collection_length_u (ImgBarriers),
                                  ImgBarriers.data ());
        }

        Barriers.clear ();
        ImgBarriers.clear ();
        BufferBarriers.clear ();
    }

    StagedBarriers.clear ();
}

apemode::CommandBuffer::operator VkCommandBuffer () const
{
    return hCmdList;
}

bool apemode::CommandBuffer::IsDirect () const
{
    return eType == kCommandListType_Direct;
}

/// -------------------------------------------------------------------------------------------------------------------
/// CommandQueue
/// -------------------------------------------------------------------------------------------------------------------

apemode::CommandQueue::CommandQueue () : pNode (nullptr), QueueFamilyId (0), QueueId (0)
{
}

/// -------------------------------------------------------------------------------------------------------------------

apemode::CommandQueue::~CommandQueue ()
{
    if (pNode != nullptr)
    {
        CommandQueueReserver::Get ().Unreserve (*pNode, QueueFamilyId, QueueId);
    }
}

/// -------------------------------------------------------------------------------------------------------------------

bool apemode::CommandQueue::RecreateResourcesFor (GraphicsDevice & InGraphicsNode,
                                               uint32_t         InQueueFamilyId,
                                               uint32_t         InQueueId)
{
    if (CommandQueueReserver::Get ().TryReserve (InGraphicsNode, InQueueFamilyId, InQueueId))
    {
        pNode = &InGraphicsNode;
        QueueFamilyId = InQueueFamilyId;
        QueueId       = InQueueId;

        hCmdQueue.Recreate (InGraphicsNode, InQueueFamilyId, InQueueId);
        return hCmdQueue.IsNotNull ();
    }

    _Game_engine_Halt ("Such queue is already resolved: Device = %#p, Family = %u Id = %u",
                       static_cast<VkDevice> (InGraphicsNode),
                       InQueueFamilyId,
                       InQueueId);
    return false;
}

/// -------------------------------------------------------------------------------------------------------------------

bool apemode::CommandQueue::Await ()
{
    _Game_engine_Assert(hCmdQueue.IsNotNull(), "Not initialized.");

    ResultHandle eQueueWaitIdleOk = vkQueueWaitIdle (hCmdQueue);
    _Game_engine_Assert (eQueueWaitIdleOk.Succeeded (), "Failed to wait for queue.");

    return eQueueWaitIdleOk.Succeeded ();
}

bool apemode::CommandQueue::Execute (CommandBuffer & CmdBuffer,
                                  VkSemaphore * hWaitSemaphores,
                                  uint32_t      WaitSemaphoreCount,
                                  VkFence       hFence)
{
    _Game_engine_Assert (hCmdQueue.IsNotNull (), "Not initialized.");
    if (apemode_likely (hCmdQueue.IsNotNull ()))
    {
        auto hCmdList = static_cast<VkCommandBuffer> (CmdBuffer);

        TInfoStruct<VkSubmitInfo> SubmitDesc;
        SubmitDesc->commandBufferCount = 1;
        SubmitDesc->pCommandBuffers    = &hCmdList;
        /*SubmitDesc->pWaitSemaphores    = hWaitSemaphores;
        SubmitDesc->waitSemaphoreCount = WaitSemaphoreCount;*/

        return ResultHandle::Succeeded (vkQueueSubmit (hCmdQueue, 1, SubmitDesc, hFence));
    }

    return false;
}

bool apemode::CommandQueue::Execute (CommandBuffer & CmdBuffer, VkFence Fence)
{
    _Game_engine_Assert (hCmdQueue.IsNotNull (), "Not initialized.");
    if (apemode_likely (hCmdQueue.IsNotNull ()))
    {
        auto hCmdList = static_cast<VkCommandBuffer> (CmdBuffer);

        TInfoStruct<VkSubmitInfo> SubmitDesc;
        SubmitDesc->pCommandBuffers    = &hCmdList;
        SubmitDesc->commandBufferCount = 1;

        return ResultHandle::Succeeded (vkQueueSubmit (hCmdQueue, 1, SubmitDesc, Fence));
    }

    return false;
}

/// -------------------------------------------------------------------------------------------------------------------

apemode::CommandQueue::operator VkQueue () const
{
    _Game_engine_Assert (hCmdQueue.IsNotNull (), "Not initialized.");
    return hCmdQueue;
}

/// -------------------------------------------------------------------------------------------------------------------

bool apemode::CommandQueue::Execute (CommandBuffer * CmdLists, uint32_t CmdListCount, VkFence Fence)
{
    _Game_engine_Assert (hCmdQueue.IsNotNull (), "Not initialized.");
    if (apemode_likely (hCmdQueue.IsNotNull ()))
    {
        std::vector<VkCommandBuffer> CmdListHandles;
        CmdListHandles.reserve (CmdListCount);

        std::transform (
            CmdLists,
            CmdLists + CmdListCount,
            std::back_inserter (CmdListHandles),
            [&](CommandBuffer const & CmdBuffer) { return static_cast<VkCommandBuffer> (CmdBuffer); });

        TInfoStruct<VkSubmitInfo> SubmitDesc;
        apemode::AliasStructs (CmdListHandles,
                           SubmitDesc->pCommandBuffers,
                           SubmitDesc->commandBufferCount);

        return apemode_likely(ResultHandle::Succeeded(vkQueueSubmit(
            hCmdQueue, 1, SubmitDesc, Fence
            )));
    }

    return false;
}

/// -------------------------------------------------------------------------------------------------------------------
/// CommandQueueReserver Key
/// -------------------------------------------------------------------------------------------------------------------

apemode::CommandQueueReserver::Key::Key() : GraphicsNodeHash(0), QueueHash(0)
{
}

/// -------------------------------------------------------------------------------------------------------------------

apemode::CommandQueueReserver::Key apemode::CommandQueueReserver::Key::NewKeyFor(
    GraphicsDevice const & GraphicsNode, uint32_t InQueueFamilyId, uint32_t InQueueId)
{
    Key NewKey;
    NewKey.GraphicsNodeHash = reinterpret_cast<uint64_t>(&GraphicsNode);
    NewKey.QueueFamilyId    = InQueueFamilyId;
    NewKey.QueueId          = InQueueId;
    return NewKey;
}

/// -------------------------------------------------------------------------------------------------------------------
/// CommandQueueReserver Key Hasher & Equal
/// -------------------------------------------------------------------------------------------------------------------

template <size_t SizeTSize = sizeof(size_t)> struct UInt64Hasher
{
    static_assert (sizeof (size_t) == 8 || sizeof (size_t) == 4,
                   "Unsupported system settings.");
};

template <>
struct UInt64Hasher<8>
{
    static size_t Hash (uint64_t H)
    {
        return H;
    }
};

template <>
struct UInt64Hasher<4>
{
    static size_t Hash (uint64_t H)
    {
        static const uint64_t HBits         = 4;
        static const uint64_t HShift64      = 64 - HBits;
        static const uint64_t HNearestPrime = 11400714819323198549ull;

        return static_cast<size_t> ((H * HNearestPrime) >> HShift64);
    }
};

/// -------------------------------------------------------------------------------------------------------------------

size_t apemode::CommandQueueReserver::Key::Hasher::operator() (Key const & Key) const
{
    const auto KeyHash = apemode::CityHash128to64 (Key.QueueHash, Key.GraphicsNodeHash);
    return UInt64Hasher<>::Hash (KeyHash);
}

/// -------------------------------------------------------------------------------------------------------------------

bool apemode::CommandQueueReserver::Key::CmpOpEqual::operator() (Key const & Key0,
                                                              Key const & Key1) const
{
    return Key0.QueueHash == Key1.QueueHash && Key0.GraphicsNodeHash == Key1.GraphicsNodeHash;
}

/// -------------------------------------------------------------------------------------------------------------------

bool apemode::CommandQueueReserver::Key::CmpOpLess::operator() (Key const & Key0,
                                                             Key const & Key1) const
{
    return Key0.QueueHash < Key1.QueueHash && Key0.GraphicsNodeHash < Key1.GraphicsNodeHash;
}

/// -------------------------------------------------------------------------------------------------------------------
/// CommandQueueReserver Reservation
/// -------------------------------------------------------------------------------------------------------------------

apemode::CommandQueueReserver::Reservation::Reservation ()
    : pQueue (nullptr), QueueId (0), QueueFamilyId (0)
{
}

/// -------------------------------------------------------------------------------------------------------------------

bool apemode::CommandQueueReserver::Reservation::IsValid () const
{
    return pQueue != nullptr;
}

/// -------------------------------------------------------------------------------------------------------------------

void apemode::CommandQueueReserver::Reservation::Release ()
{
    pQueue = nullptr;
}

/// -------------------------------------------------------------------------------------------------------------------
/// CommandQueueReserver
/// -------------------------------------------------------------------------------------------------------------------

apemode::CommandQueueReserver::CommandQueueReserver ()
{
}

/// -------------------------------------------------------------------------------------------------------------------

apemode::CommandQueueReserver & apemode::CommandQueueReserver::Get ()
{
    static CommandQueueReserver Reserver;
    return Reserver;
}

/// -------------------------------------------------------------------------------------------------------------------

bool apemode::CommandQueueReserver::TryReserve (GraphicsDevice const & GraphicsNode,
                                             uint32_t               QueueFamilyId,
                                             uint32_t               QueueId)
{
    const auto   Key         = Key::NewKeyFor (GraphicsNode, QueueFamilyId, QueueId);
    const auto & Reservation = ReservationStorage[Key];

    // Valid reservation is active reservation (contains created command queue).
    return Reservation.IsValid () == false;
}

/// -------------------------------------------------------------------------------------------------------------------

void apemode::CommandQueueReserver::Unreserve (GraphicsDevice const & GraphicsNode,
                                            uint32_t               QueueFamilyId,
                                            uint32_t               QueueId)
{
    const auto Key = Key::NewKeyFor (GraphicsNode, QueueFamilyId, QueueId);
    ReservationStorage[Key].Release ();
}

/// -------------------------------------------------------------------------------------------------------------------
/// CommandBuffer BeginEndScope
/// -------------------------------------------------------------------------------------------------------------------

apemode::CommandBuffer::BeginEndScope::BeginEndScope (CommandBuffer & CmdBuffer, bool bOneTimeSubmit)
    : BeginEndScope (CmdBuffer, TInfoStruct<VkCommandBufferInheritanceInfo> (), bOneTimeSubmit)
{
}

apemode::CommandBuffer::BeginEndScope::BeginEndScope (CommandBuffer &                          CmdBuffer,
                                                 VkCommandBufferInheritanceInfo const & CmdInherit,
                                                 bool bOneTimeSubmit)
    : AssociatedCmdList (CmdBuffer)
{
    TInfoStruct<VkCommandBufferInheritanceInfo> CmdInheritanceDesc;
    CmdInheritanceDesc = CmdInherit;

    TInfoStruct<VkCommandBufferBeginInfo> CmdBeginDesc;
    CmdBeginDesc->pInheritanceInfo = CmdInheritanceDesc;
    CmdBeginDesc->flags |= bOneTimeSubmit ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;

    const ResultHandle eOk = vkBeginCommandBuffer (CmdBuffer, CmdBeginDesc);
    CmdBuffer.bIsInBeginEndScope = eOk.Succeeded ();
}


apemode::CommandBuffer::BeginEndScope::~BeginEndScope ()
{
    AssociatedCmdList.bIsInBeginEndScope = false;
    vkEndCommandBuffer (AssociatedCmdList);
}

/// -------------------------------------------------------------------------------------------------------------------
/// CommandBuffer StagedBarrier
/// -------------------------------------------------------------------------------------------------------------------

apemode::CommandBuffer::StagedBarrier::StagedBarrier (VkPipelineStageFlags    SrcStage,
                                                 VkPipelineStageFlags    DstStage,
                                                 VkMemoryBarrier const & Barrier)
    : SrcStage (SrcStage), DstStage (DstStage), Barrier (Barrier)
{
}

apemode::CommandBuffer::StagedBarrier::StagedBarrier (VkPipelineStageFlags         SrcStage,
                                                 VkPipelineStageFlags         DstStage,
                                                 VkImageMemoryBarrier const & Barrier)
    : SrcStage (SrcStage), DstStage (DstStage), ImgBarrier (Barrier)
{
}

apemode::CommandBuffer::StagedBarrier::StagedBarrier (VkPipelineStageFlags          SrcStage,
                                                 VkPipelineStageFlags          DstStage,
                                                 VkBufferMemoryBarrier const & Barrier)
    : SrcStage (SrcStage), DstStage (DstStage), BufferBarrier (Barrier)
{
}

/// -------------------------------------------------------------------------------------------------------------------
