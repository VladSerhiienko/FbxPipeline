//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <Buffer.Vulkan.h>

#include <CommandQueue.Vulkan.h>

/// -------------------------------------------------------------------------------------------------------------------
/// BufferResourceView
/// -------------------------------------------------------------------------------------------------------------------

uint32_t apemodevk::BufferResourceView::GetElementCount () const
{
    return ElementCount;
}

uint32_t apemodevk::BufferResourceView::GetElementSizeInBytes () const
{
    return ElementStride;
}

uint32_t apemodevk::BufferResourceView::GetTotalSizeInBytes () const
{
    return ElementStride * ElementCount;
}

std::shared_ptr<apemodevk::BufferResourceView> apemodevk::BufferResourceView::MakeNewLinked ()
{
    return std::shared_ptr<BufferResourceView> (new BufferResourceView ());
}

std::unique_ptr<apemodevk::BufferResourceView> apemodevk::BufferResourceView::MakeNewUnique ()
{
    return std::unique_ptr<BufferResourceView> (new BufferResourceView ());
}

apemodevk::BufferResourceView::BufferResourceView ()
{
    ViewType = kResourceViewType_Buffer;
}

void apemodevk::BufferResourceView::SetState (apemodevk::CommandBuffer &  CmdBuffer,
                                         VkPipelineStageFlags PipelineStageFlags,
                                         VkAccessFlags        AccessMask,
                                         uint32_t             BufferSize,
                                         uint32_t             BufferOffset,
                                         uint32_t             QueueFamily)
{
    const auto StateIt = MemoryStates.find (&CmdBuffer);
    const MemoryState & State = StateIt != MemoryStates.end()
                              ? StateIt->second
                              : MemoryStates[nullptr];

    if (apemode_likely (State.AccessMask != AccessMask
                             || State.QueueFamily != QueueFamily
                             || State.PipelineStageFlags != PipelineStageFlags
                             || State.BufferSize != BufferSize
                             || State.BufferOffset != BufferOffset))
    {
        TInfoStruct<VkBufferMemoryBarrier> Barrier;
        Barrier->size                = BufferSize;
        Barrier->offset              = BufferOffset;
        Barrier->buffer              = BufferHandle;
        Barrier->srcAccessMask       = State.AccessMask;
        Barrier->dstAccessMask       = AccessMask;
        Barrier->srcQueueFamilyIndex = State.QueueFamily;
        Barrier->dstQueueFamilyIndex = QueueFamily;

        CmdBuffer.InsertBarrier(State.PipelineStageFlags, 
                              PipelineStageFlags, 
                              Barrier);

        ResourceView::SetState (CmdBuffer,
                                MemoryState (PipelineStageFlags,
                                             AccessMask,
                                             QueueFamily,
                                             BufferSize,
                                             BufferOffset));
    }
}

apemodevk::BufferResourceView::~BufferResourceView ()
{
}
