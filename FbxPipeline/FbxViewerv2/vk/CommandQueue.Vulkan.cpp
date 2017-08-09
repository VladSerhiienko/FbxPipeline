//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <CommandQueue.Vulkan.h>

#include <CityHash.h>
#include <TInfoStruct.Vulkan.h>

/// -------------------------------------------------------------------------------------------------------------------
/// CommandBuffer
/// -------------------------------------------------------------------------------------------------------------------

std::unique_ptr<apemodevk::CommandBuffer> apemodevk::CommandBuffer::MakeNewUnique ()
{
    return std::unique_ptr<apemodevk::CommandBuffer> (new CommandBuffer ());
}

std::shared_ptr<apemodevk::CommandBuffer> apemodevk::CommandBuffer::MakeNewLinked ()
{
    return std::shared_ptr<apemodevk::CommandBuffer> (new CommandBuffer ());
}

apemodevk::CommandBuffer::CommandBuffer ()
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

bool apemodevk::CommandBuffer::RecreateResourcesFor (GraphicsDevice & GraphicsNode,
                                              uint32_t         queueFamilyId,
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
    CmdAllocDesc->queueFamilyIndex = queueFamilyId;

    if (!hCmdAlloc.Recreate (GraphicsNode, CmdAllocDesc))
    {
        apemode_halt("Failed to create command allocator.");
        return false;
    }

    TInfoStruct<VkCommandBufferAllocateInfo> CmdListDesc;
    CmdListDesc->level              = CmdListType;
    CmdListDesc->commandPool        = hCmdAlloc;
    CmdListDesc->commandBufferCount = 1;

    if (!hCmdList.Recreate (GraphicsNode, CmdListDesc))
    {
        apemode_halt ("Failed to create command list.");
        return false;
    }

    eType = static_cast<CommandListType> (CmdListType);
    return true;
}

/// -------------------------------------------------------------------------------------------------------------------

bool apemodevk::CommandBuffer::Reset( bool bReleaseResources ) {
    const VkCommandBufferResetFlags ResetFlags = bReleaseResources ? VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT : 0u;
    return VK_SUCCESS == CheckedCall( vkResetCommandBuffer( hCmdList, ResetFlags ) );
}

static uint64_t ComposeKey (VkPipelineStageFlags SrcFlags, VkPipelineStageFlags DstFlags)
{
    return static_cast<uint64_t> (SrcFlags) << 32 | static_cast<uint64_t> (DstFlags);
}

void apemodevk::CommandBuffer::InsertBarrier (VkPipelineStageFlags    SrcFlags,
                                       VkPipelineStageFlags    DstFlags,
                                       VkMemoryBarrier const & Barrier)
{
    ++BarrierCount;

    StagedBarrier const CmdBarrier (SrcFlags, DstFlags, Barrier);
    StagedBarriers.insert (std::make_pair<> (CmdBarrier.StageHash, CmdBarrier));
}

void apemodevk::CommandBuffer::InsertBarrier (VkPipelineStageFlags         SrcFlags,
                                       VkPipelineStageFlags         DstFlags,
                                       VkImageMemoryBarrier const & Barrier)
{
    ++ImgBarrierCount;

    StagedBarrier const CmdBarrier (SrcFlags, DstFlags, Barrier);
    StagedBarriers.insert (std::make_pair<> (CmdBarrier.StageHash, CmdBarrier));
}

void apemodevk::CommandBuffer::InsertBarrier (VkPipelineStageFlags          SrcFlags,
                                       VkPipelineStageFlags          DstFlags,
                                       VkBufferMemoryBarrier const & Barrier)
{
    ++BufferBarrierCount;

    StagedBarrier const CmdBarrier (SrcFlags, DstFlags, Barrier);
    StagedBarriers.insert (std::make_pair<> (CmdBarrier.StageHash, CmdBarrier));
}

void apemodevk::CommandBuffer::FlushStagedBarriers()
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
            apemode_halt ("Memory corruption.");
            break;
        }
    };

    StagedBarrier::ItRange StagedBarrierItRange;
    StagedBarrier::It      StagedBarrierIt = StagedBarriers.begin( );
    for ( ; StagedBarrierIt != StagedBarriers.cend( ); StagedBarrierIt = StagedBarrierItRange.second ) {
        StagedBarrierItRange = StagedBarriers.equal_range( StagedBarrierIt->first );
        std::for_each( StagedBarrierItRange.first, StagedBarrierItRange.second, FillBarriersFn );

        apemode_assert( !Barriers.empty( ) || !ImgBarriers.empty( ) || !BufferBarriers.empty( ),
                        "Nothing to submit to the command list." );

        if ( apemode_likely( !Barriers.empty( ) || !ImgBarriers.empty( ) || !BufferBarriers.empty( ) ) ) {
            vkCmdPipelineBarrier( hCmdList,
                                  StagedBarrierItRange.first->second.SrcStage,
                                  StagedBarrierItRange.first->second.DstStage,
                                  static_cast< VkDependencyFlags >( 0 ),
                                  _Get_collection_length_u( Barriers ),
                                  Barriers.data( ),
                                  _Get_collection_length_u( BufferBarriers ),
                                  BufferBarriers.data( ),
                                  _Get_collection_length_u( ImgBarriers ),
                                  ImgBarriers.data( ) );
        }

        Barriers.clear( );
        ImgBarriers.clear( );
        BufferBarriers.clear( );
    }

    StagedBarriers.clear( );
}

apemodevk::CommandBuffer::operator VkCommandBuffer () const
{
    return hCmdList;
}

bool apemodevk::CommandBuffer::IsDirect () const
{
    return eType == kCommandListType_Direct;
}

/// -------------------------------------------------------------------------------------------------------------------
/// CommandQueue
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::CommandQueue::CommandQueue () : pNode (nullptr), queueFamilyId (0), queueId (0)
{
}

/// -------------------------------------------------------------------------------------------------------------------

apemodevk::CommandQueue::~CommandQueue ()
{
    if (pNode != nullptr)
    {
        CommandQueueReserver::Get ().Unreserve (*pNode, queueFamilyId, queueId);
    }
}

/// -------------------------------------------------------------------------------------------------------------------

bool apemodevk::CommandQueue::RecreateResourcesFor( GraphicsDevice& InGraphicsNode,
                                                    uint32_t        InQueueFamilyId,
                                                    uint32_t        InQueueId ) {
    if ( CommandQueueReserver::Get( ).TryReserve( InGraphicsNode, InQueueFamilyId, InQueueId ) ) {
        pNode         = &InGraphicsNode;
        queueFamilyId = InQueueFamilyId;
        queueId       = InQueueId;

        hCmdQueue.Recreate( InGraphicsNode, InQueueFamilyId, InQueueId );
        return hCmdQueue.IsNotNull( );
    }

    apemode_halt( "Such queue is already resolved: Device = %#p, Family = %u Id = %u",
                  static_cast< VkDevice >( InGraphicsNode ),
                  InQueueFamilyId,
                  InQueueId );
    return false;
}

/// -------------------------------------------------------------------------------------------------------------------

bool apemodevk::CommandQueue::Await( ) {
    apemode_assert( hCmdQueue.IsNotNull( ), "Not initialized." );
    return VK_SUCCESS == CheckedCall( vkQueueWaitIdle( hCmdQueue ) );
}

bool apemodevk::CommandQueue::Execute( CommandBuffer& CmdBuffer,
                                       VkSemaphore*   hWaitSemaphores,
                                       uint32_t       WaitSemaphoreCount,
                                       VkFence        hFence ) {
    apemode_assert (hCmdQueue.IsNotNull (), "Not initialized.");
    if (apemode_likely (hCmdQueue.IsNotNull ()))
    {
        auto hCmdList = static_cast<VkCommandBuffer> (CmdBuffer);

        TInfoStruct<VkSubmitInfo> SubmitDesc;
        SubmitDesc->commandBufferCount = 1;
        SubmitDesc->pCommandBuffers    = &hCmdList;



        /*SubmitDesc->pWaitSemaphores    = hWaitSemaphores;
        SubmitDesc->waitSemaphoreCount = WaitSemaphoreCount;*/

        return VK_SUCCESS == CheckedCall( vkQueueSubmit( hCmdQueue, 1, SubmitDesc, hFence ) );
    }

    return false;
}

bool apemodevk::CommandQueue::Execute (CommandBuffer & CmdBuffer, VkFence Fence)
{
    apemode_assert (hCmdQueue.IsNotNull (), "Not initialized.");
    if (apemode_likely (hCmdQueue.IsNotNull ()))
    {
        auto hCmdList = static_cast<VkCommandBuffer> (CmdBuffer);

        TInfoStruct<VkSubmitInfo> SubmitDesc;
        SubmitDesc->pCommandBuffers    = &hCmdList;
        SubmitDesc->commandBufferCount = 1;

        return VK_SUCCESS == CheckedCall( vkQueueSubmit( hCmdQueue, 1, SubmitDesc, Fence ) );
    }

    return false;
}

/// -------------------------------------------------------------------------------------------------------------------

apemodevk::CommandQueue::operator VkQueue () const
{
    apemode_assert (hCmdQueue.IsNotNull (), "Not initialized.");
    return hCmdQueue;
}

/// -------------------------------------------------------------------------------------------------------------------

bool apemodevk::CommandQueue::Execute( CommandBuffer* CmdLists, uint32_t CmdListCount, VkFence Fence ) {
    apemode_assert( hCmdQueue.IsNotNull( ), "Not initialized." );
    if ( apemode_likely( hCmdQueue.IsNotNull( ) ) ) {
        std::vector< VkCommandBuffer > CmdListHandles;
        CmdListHandles.reserve( CmdListCount );

        std::transform( CmdLists, CmdLists + CmdListCount, std::back_inserter( CmdListHandles ), [&]( CommandBuffer const& CmdBuffer ) {
            return static_cast< VkCommandBuffer >( CmdBuffer );
        } );

        TInfoStruct< VkSubmitInfo > SubmitDesc;
        apemodevk::AliasStructs( CmdListHandles, SubmitDesc->pCommandBuffers, SubmitDesc->commandBufferCount );

        return apemode_likely( VK_SUCCESS == CheckedCall( vkQueueSubmit( hCmdQueue, 1, SubmitDesc, Fence ) ) );
    }

    return false;
}

/// -------------------------------------------------------------------------------------------------------------------
/// CommandQueueReserver Key
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::CommandQueueReserver::Key::Key() : GraphicsNodeHash(0), QueueHash(0)
{
}

/// -------------------------------------------------------------------------------------------------------------------

apemodevk::CommandQueueReserver::Key apemodevk::CommandQueueReserver::Key::NewKeyFor( GraphicsDevice const& GraphicsNode,
                                                                                      uint32_t              InQueueFamilyId,
                                                                                      uint32_t              InQueueId ) {
    Key NewKey;
    NewKey.GraphicsNodeHash = reinterpret_cast< uint64_t >( &GraphicsNode );
    NewKey.queueFamilyId    = InQueueFamilyId;
    NewKey.queueId          = InQueueId;
    return NewKey;
}

/// -------------------------------------------------------------------------------------------------------------------
/// CommandQueueReserver Key Hasher & Equal
/// -------------------------------------------------------------------------------------------------------------------

template < size_t SizeTSize = sizeof( size_t ) >
struct UInt64Hasher {
    static_assert( sizeof( size_t ) == 8 || sizeof( size_t ) == 4, "Unsupported system settings." );
};

template <>
struct UInt64Hasher< 8 > {
    static size_t Hash( uint64_t H ) {
        return H;
    }
};

template <>
struct UInt64Hasher< 4 > {
    static size_t Hash( uint64_t H ) {
        static const uint64_t HBits         = 4;
        static const uint64_t HShift64      = 64 - HBits;
        static const uint64_t HNearestPrime = 11400714819323198549ull;

        return static_cast< size_t >( ( H * HNearestPrime ) >> HShift64 );
    }
};

/// -------------------------------------------------------------------------------------------------------------------

size_t apemodevk::CommandQueueReserver::Key::Hasher::operator( )( Key const& Key ) const {
    const auto KeyHash = apemode::CityHash128to64( Key.QueueHash, Key.GraphicsNodeHash );
    return UInt64Hasher<>::Hash( KeyHash );
}

/// -------------------------------------------------------------------------------------------------------------------

bool apemodevk::CommandQueueReserver::Key::CmpOpEqual::operator( )( Key const& Key0, Key const& Key1 ) const {
    return Key0.QueueHash == Key1.QueueHash && Key0.GraphicsNodeHash == Key1.GraphicsNodeHash;
}

/// -------------------------------------------------------------------------------------------------------------------

bool apemodevk::CommandQueueReserver::Key::CmpOpLess::operator() (Key const & Key0,
                                                             Key const & Key1) const
{
    return Key0.QueueHash < Key1.QueueHash && Key0.GraphicsNodeHash < Key1.GraphicsNodeHash;
}

/// -------------------------------------------------------------------------------------------------------------------
/// CommandQueueReserver Reservation
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::CommandQueueReserver::Reservation::Reservation ()
    : pQueue (nullptr), queueId (0), queueFamilyId (0)
{
}

/// -------------------------------------------------------------------------------------------------------------------

bool apemodevk::CommandQueueReserver::Reservation::IsValid () const
{
    return pQueue != nullptr;
}

/// -------------------------------------------------------------------------------------------------------------------

void apemodevk::CommandQueueReserver::Reservation::Release ()
{
    pQueue = nullptr;
}

/// -------------------------------------------------------------------------------------------------------------------
/// CommandQueueReserver
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::CommandQueueReserver::CommandQueueReserver ()
{
}

/// -------------------------------------------------------------------------------------------------------------------

apemodevk::CommandQueueReserver & apemodevk::CommandQueueReserver::Get ()
{
    static CommandQueueReserver Reserver;
    return Reserver;
}

/// -------------------------------------------------------------------------------------------------------------------

bool apemodevk::CommandQueueReserver::TryReserve( GraphicsDevice const& GraphicsNode,
                                                  uint32_t              queueFamilyId,
                                                  uint32_t              queueId ) {
    const auto  Key = Key::NewKeyFor( GraphicsNode, queueFamilyId, queueId );
    const auto& Reservation = ReservationStorage[ Key ];

    // Valid reservation is active reservation (contains created command queue).
    return Reservation.IsValid( ) == false;
}

/// -------------------------------------------------------------------------------------------------------------------

void apemodevk::CommandQueueReserver::Unreserve( GraphicsDevice const& GraphicsNode,
                                                 uint32_t              queueFamilyId,
                                                 uint32_t              queueId ) {
    const auto Key = Key::NewKeyFor( GraphicsNode, queueFamilyId, queueId );
    ReservationStorage[ Key ].Release( );
}

/// -------------------------------------------------------------------------------------------------------------------
/// CommandBuffer BeginEndScope
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::CommandBuffer::BeginEndScope::BeginEndScope (CommandBuffer & CmdBuffer, bool bOneTimeSubmit)
    : BeginEndScope (CmdBuffer, TInfoStruct<VkCommandBufferInheritanceInfo> (), bOneTimeSubmit)
{
}

apemodevk::CommandBuffer::BeginEndScope::BeginEndScope( CommandBuffer&                        CmdBuffer,
                                                        VkCommandBufferInheritanceInfo const& CmdInherit,
                                                        bool                                  bOneTimeSubmit )
    : AssociatedCmdList( CmdBuffer ) {
    TInfoStruct< VkCommandBufferInheritanceInfo > CmdInheritanceDesc;
    CmdInheritanceDesc = CmdInherit;

    TInfoStruct< VkCommandBufferBeginInfo > CmdBeginDesc;
    CmdBeginDesc->pInheritanceInfo = CmdInheritanceDesc;
    CmdBeginDesc->flags |= bOneTimeSubmit ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;
    CmdBuffer.bIsInBeginEndScope = VK_SUCCESS == CheckedCall( vkBeginCommandBuffer( CmdBuffer, CmdBeginDesc ) );
}

apemodevk::CommandBuffer::BeginEndScope::~BeginEndScope ()
{
    AssociatedCmdList.bIsInBeginEndScope = false;
    vkEndCommandBuffer (AssociatedCmdList);
}

/// -------------------------------------------------------------------------------------------------------------------
/// CommandBuffer StagedBarrier
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::CommandBuffer::StagedBarrier::StagedBarrier (VkPipelineStageFlags    SrcStage,
                                                 VkPipelineStageFlags    DstStage,
                                                 VkMemoryBarrier const & Barrier)
    : SrcStage (SrcStage), DstStage (DstStage), Barrier (Barrier)
{
}

apemodevk::CommandBuffer::StagedBarrier::StagedBarrier (VkPipelineStageFlags         SrcStage,
                                                 VkPipelineStageFlags         DstStage,
                                                 VkImageMemoryBarrier const & Barrier)
    : SrcStage (SrcStage), DstStage (DstStage), ImgBarrier (Barrier)
{
}

apemodevk::CommandBuffer::StagedBarrier::StagedBarrier (VkPipelineStageFlags          SrcStage,
                                                 VkPipelineStageFlags          DstStage,
                                                 VkBufferMemoryBarrier const & Barrier)
    : SrcStage (SrcStage), DstStage (DstStage), BufferBarrier (Barrier)
{
}

/// -------------------------------------------------------------------------------------------------------------------
