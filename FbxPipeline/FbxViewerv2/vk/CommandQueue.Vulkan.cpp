//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <CommandQueue.Vulkan.h>

#include <CityHash.h>
#include <TInfoStruct.Vulkan.h>

/// -------------------------------------------------------------------------------------------------------------------
/// CommandList
/// -------------------------------------------------------------------------------------------------------------------

std::unique_ptr<Core::CommandList> Core::CommandList::MakeNewUnique ()
{
    return std::unique_ptr<Core::CommandList> (new CommandList ());
}

std::shared_ptr<Core::CommandList> Core::CommandList::MakeNewLinked ()
{
    return std::shared_ptr<Core::CommandList> (new CommandList ());
}

Core::CommandList::CommandList ()
    : eType (kCommandListType_Invalid)
    , pRenderPass (nullptr)
    , pFramebuffer (nullptr)
    , pRootSignature (nullptr)
    , pPipelineState (nullptr)
    , BarrierCount (0)
    , ImgBarrierCount (0)
    , BufferBarrierCount (0)
{
}

/// -------------------------------------------------------------------------------------------------------------------

bool Core::CommandList::RecreateResourcesFor (GraphicsDevice & GraphicsNode,
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

bool Core::CommandList::Reset (bool bReleaseResources)
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

void Core::CommandList::InsertBarrier (VkPipelineStageFlags    SrcFlags,
                                       VkPipelineStageFlags    DstFlags,
                                       VkMemoryBarrier const & Barrier)
{
    ++BarrierCount;

    StagedBarrier const CmdBarrier (SrcFlags, DstFlags, Barrier);
    StagedBarriers.insert (std::make_pair<> (CmdBarrier.StageHash, CmdBarrier));
}

void Core::CommandList::InsertBarrier (VkPipelineStageFlags         SrcFlags,
                                       VkPipelineStageFlags         DstFlags,
                                       VkImageMemoryBarrier const & Barrier)
{
    ++ImgBarrierCount;

    StagedBarrier const CmdBarrier (SrcFlags, DstFlags, Barrier);
    StagedBarriers.insert (std::make_pair<> (CmdBarrier.StageHash, CmdBarrier));
}

void Core::CommandList::InsertBarrier (VkPipelineStageFlags          SrcFlags,
                                       VkPipelineStageFlags          DstFlags,
                                       VkBufferMemoryBarrier const & Barrier)
{
    ++BufferBarrierCount;

    StagedBarrier const CmdBarrier (SrcFlags, DstFlags, Barrier);
    StagedBarriers.insert (std::make_pair<> (CmdBarrier.StageHash, CmdBarrier));
}

void Core::CommandList::FlushStagedBarriers()
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

        if (_Game_engine_Likely (!Barriers.empty ()
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

Core::CommandList::operator VkCommandBuffer () const
{
    return hCmdList;
}

bool Core::CommandList::IsDirect () const
{
    return eType == kCommandListType_Direct;
}

/// -------------------------------------------------------------------------------------------------------------------
/// CommandQueue
/// -------------------------------------------------------------------------------------------------------------------

Core::CommandQueue::CommandQueue () : pGraphicsNode (nullptr), QueueFamilyId (0), QueueId (0)
{
}

/// -------------------------------------------------------------------------------------------------------------------

Core::CommandQueue::~CommandQueue ()
{
    if (pGraphicsNode != nullptr)
    {
        CommandQueueReserver::Get ().Unreserve (*pGraphicsNode, QueueFamilyId, QueueId);
    }
}

/// -------------------------------------------------------------------------------------------------------------------

bool Core::CommandQueue::RecreateResourcesFor (GraphicsDevice & InGraphicsNode,
                                               uint32_t         InQueueFamilyId,
                                               uint32_t         InQueueId)
{
    if (CommandQueueReserver::Get ().TryReserve (InGraphicsNode, InQueueFamilyId, InQueueId))
    {
        pGraphicsNode = &InGraphicsNode;
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

bool Core::CommandQueue::Await ()
{
    _Game_engine_Assert(hCmdQueue.IsNotNull(), "Not initialized.");

    ResultHandle eQueueWaitIdleOk = vkQueueWaitIdle (hCmdQueue);
    _Game_engine_Assert (eQueueWaitIdleOk.Succeeded (), "Failed to wait for queue.");

    return eQueueWaitIdleOk.Succeeded ();
}

bool Core::CommandQueue::Execute (CommandList & CmdList,
                                  VkSemaphore * hWaitSemaphores,
                                  uint32_t      WaitSemaphoreCount,
                                  VkFence       hFence)
{
    _Game_engine_Assert (hCmdQueue.IsNotNull (), "Not initialized.");
    if (_Game_engine_Likely (hCmdQueue.IsNotNull ()))
    {
        auto hCmdList = static_cast<VkCommandBuffer> (CmdList);

        TInfoStruct<VkSubmitInfo> SubmitDesc;
        SubmitDesc->commandBufferCount = 1;
        SubmitDesc->pCommandBuffers    = &hCmdList;
        /*SubmitDesc->pWaitSemaphores    = hWaitSemaphores;
        SubmitDesc->waitSemaphoreCount = WaitSemaphoreCount;*/

        return ResultHandle::Succeeded (vkQueueSubmit (hCmdQueue, 1, SubmitDesc, hFence));
    }

    return false;
}

bool Core::CommandQueue::Execute (CommandList & CmdList, VkFence Fence)
{
    _Game_engine_Assert (hCmdQueue.IsNotNull (), "Not initialized.");
    if (_Game_engine_Likely (hCmdQueue.IsNotNull ()))
    {
        auto hCmdList = static_cast<VkCommandBuffer> (CmdList);

        TInfoStruct<VkSubmitInfo> SubmitDesc;
        SubmitDesc->pCommandBuffers    = &hCmdList;
        SubmitDesc->commandBufferCount = 1;

        return ResultHandle::Succeeded (vkQueueSubmit (hCmdQueue, 1, SubmitDesc, Fence));
    }

    return false;
}

/// -------------------------------------------------------------------------------------------------------------------

Core::CommandQueue::operator VkQueue () const
{
    _Game_engine_Assert (hCmdQueue.IsNotNull (), "Not initialized.");
    return hCmdQueue;
}

/// -------------------------------------------------------------------------------------------------------------------

bool Core::CommandQueue::Execute (CommandList * CmdLists, uint32_t CmdListCount, VkFence Fence)
{
    _Game_engine_Assert (hCmdQueue.IsNotNull (), "Not initialized.");
    if (_Game_engine_Likely (hCmdQueue.IsNotNull ()))
    {
        std::vector<VkCommandBuffer> CmdListHandles;
        CmdListHandles.reserve (CmdListCount);

        std::transform (
            CmdLists,
            CmdLists + CmdListCount,
            std::back_inserter (CmdListHandles),
            [&](CommandList const & CmdList) { return static_cast<VkCommandBuffer> (CmdList); });

        TInfoStruct<VkSubmitInfo> SubmitDesc;
        Aux::AliasStructs (CmdListHandles,
                           SubmitDesc->pCommandBuffers,
                           SubmitDesc->commandBufferCount);

        return _Game_engine_Likely(ResultHandle::Succeeded(vkQueueSubmit(
            hCmdQueue, 1, SubmitDesc, Fence
            )));
    }

    return false;
}

/// -------------------------------------------------------------------------------------------------------------------
/// CommandQueueReserver Key
/// -------------------------------------------------------------------------------------------------------------------

Core::CommandQueueReserver::Key::Key() : GraphicsNodeHash(0), QueueHash(0)
{
}

/// -------------------------------------------------------------------------------------------------------------------

Core::CommandQueueReserver::Key Core::CommandQueueReserver::Key::NewKeyFor(
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

size_t Core::CommandQueueReserver::Key::Hasher::operator() (Key const & Key) const
{
    const auto KeyHash = Aux::CityHash128to64 (Key.QueueHash, Key.GraphicsNodeHash);
    return UInt64Hasher<>::Hash (KeyHash);
}

/// -------------------------------------------------------------------------------------------------------------------

bool Core::CommandQueueReserver::Key::CmpOpEqual::operator() (Key const & Key0,
                                                              Key const & Key1) const
{
    return Key0.QueueHash == Key1.QueueHash && Key0.GraphicsNodeHash == Key1.GraphicsNodeHash;
}

/// -------------------------------------------------------------------------------------------------------------------

bool Core::CommandQueueReserver::Key::CmpOpLess::operator() (Key const & Key0,
                                                             Key const & Key1) const
{
    return Key0.QueueHash < Key1.QueueHash && Key0.GraphicsNodeHash < Key1.GraphicsNodeHash;
}

/// -------------------------------------------------------------------------------------------------------------------
/// CommandQueueReserver Reservation
/// -------------------------------------------------------------------------------------------------------------------

Core::CommandQueueReserver::Reservation::Reservation ()
    : pQueue (nullptr), QueueId (0), QueueFamilyId (0)
{
}

/// -------------------------------------------------------------------------------------------------------------------

bool Core::CommandQueueReserver::Reservation::IsValid () const
{
    return pQueue != nullptr;
}

/// -------------------------------------------------------------------------------------------------------------------

void Core::CommandQueueReserver::Reservation::Release ()
{
    pQueue = nullptr;
}

/// -------------------------------------------------------------------------------------------------------------------
/// CommandQueueReserver
/// -------------------------------------------------------------------------------------------------------------------

Core::CommandQueueReserver::CommandQueueReserver ()
{
}

/// -------------------------------------------------------------------------------------------------------------------

Core::CommandQueueReserver & Core::CommandQueueReserver::Get ()
{
    static CommandQueueReserver Reserver;
    return Reserver;
}

/// -------------------------------------------------------------------------------------------------------------------

bool Core::CommandQueueReserver::TryReserve (GraphicsDevice const & GraphicsNode,
                                             uint32_t               QueueFamilyId,
                                             uint32_t               QueueId)
{
    const auto   Key         = Key::NewKeyFor (GraphicsNode, QueueFamilyId, QueueId);
    const auto & Reservation = ReservationStorage[Key];

    // Valid reservation is active reservation (contains created command queue).
    return Reservation.IsValid () == false;
}

/// -------------------------------------------------------------------------------------------------------------------

void Core::CommandQueueReserver::Unreserve (GraphicsDevice const & GraphicsNode,
                                            uint32_t               QueueFamilyId,
                                            uint32_t               QueueId)
{
    const auto Key = Key::NewKeyFor (GraphicsNode, QueueFamilyId, QueueId);
    ReservationStorage[Key].Release ();
}

/// -------------------------------------------------------------------------------------------------------------------
/// CommandList BeginEndScope
/// -------------------------------------------------------------------------------------------------------------------

Core::CommandList::BeginEndScope::BeginEndScope (CommandList & CmdList, bool bOneTimeSubmit)
    : BeginEndScope (CmdList, TInfoStruct<VkCommandBufferInheritanceInfo> (), bOneTimeSubmit)
{
}

Core::CommandList::BeginEndScope::BeginEndScope (CommandList &                          CmdList,
                                                 VkCommandBufferInheritanceInfo const & CmdInherit,
                                                 bool bOneTimeSubmit)
    : AssociatedCmdList (CmdList)
{
    TInfoStruct<VkCommandBufferInheritanceInfo> CmdInheritanceDesc;
    CmdInheritanceDesc = CmdInherit;

    TInfoStruct<VkCommandBufferBeginInfo> CmdBeginDesc;
    CmdBeginDesc->pInheritanceInfo = CmdInheritanceDesc;
    CmdBeginDesc->flags |= bOneTimeSubmit ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;

    const ResultHandle eOk = vkBeginCommandBuffer (CmdList, CmdBeginDesc);
    CmdList.bIsInBeginEndScope = eOk.Succeeded ();
}


Core::CommandList::BeginEndScope::~BeginEndScope ()
{
    AssociatedCmdList.bIsInBeginEndScope = false;
    vkEndCommandBuffer (AssociatedCmdList);
}

/// -------------------------------------------------------------------------------------------------------------------
/// CommandList StagedBarrier
/// -------------------------------------------------------------------------------------------------------------------

Core::CommandList::StagedBarrier::StagedBarrier (VkPipelineStageFlags    SrcStage,
                                                 VkPipelineStageFlags    DstStage,
                                                 VkMemoryBarrier const & Barrier)
    : SrcStage (SrcStage), DstStage (DstStage), Barrier (Barrier)
{
}

Core::CommandList::StagedBarrier::StagedBarrier (VkPipelineStageFlags         SrcStage,
                                                 VkPipelineStageFlags         DstStage,
                                                 VkImageMemoryBarrier const & Barrier)
    : SrcStage (SrcStage), DstStage (DstStage), ImgBarrier (Barrier)
{
}

Core::CommandList::StagedBarrier::StagedBarrier (VkPipelineStageFlags          SrcStage,
                                                 VkPipelineStageFlags          DstStage,
                                                 VkBufferMemoryBarrier const & Barrier)
    : SrcStage (SrcStage), DstStage (DstStage), BufferBarrier (Barrier)
{
}

/// -------------------------------------------------------------------------------------------------------------------
