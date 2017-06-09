//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <BufferPools.Vulkan.h>

#include <CommandQueue.Vulkan.h>

/// -------------------------------------------------------------------------------------------------------------------
/// BufferPool
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::BufferPool::BufferPool(BufferPoolType PoolType)
    : ePoolType(PoolType)
{
}

apemodevk::BufferPool::~BufferPool()
{
}

bool apemodevk::BufferPool::RecreateResourcesFor(GraphicsDevice & InGraphicsNode)
{
    Pages.clear();
    pNode = &InGraphicsNode;
    return true;
}

void apemodevk::BufferPool::OnResourcesEvicted(GraphicsDevice & InGraphicsNode)
{
}

void apemodevk::BufferPool::OnCommandListReset(CommandBuffer & CmdBuffer)
{
}

apemodevk::BufferPool::Range apemodevk::BufferPool::Suballocate(CommandBuffer & CmdBuffer, uint32_t Size)
{
    apemodevk::BufferPool::Range RetRange;
    const bool bSuballocated = Suballocate(CmdBuffer, Size, RetRange);
    apemode_assert(bSuballocated, "Out of memory");
    return RetRange;
}

bool apemodevk::BufferPool::Suballocate(CommandBuffer & CmdBuffer, uint32_t Size, Range & OutRange)
{
    return false;
}

void apemodevk::BufferPool::OnCommandListExecutePreview(CommandBuffer & CmdBuffer)
{
}

bool apemodevk::BufferPool::IsOutOfMemory() const
{
    return bIsOutOfMemory;
}

apemodevk::BufferPoolType apemodevk::BufferPool::GetBufferPoolType() const
{
    return ePoolType;
}

apemodevk::BufferPool::Page * apemodevk::BufferPool::Page::MakeNew ()
{
    return new Page ();
}

std::shared_ptr<apemodevk::BufferPool::Page> apemodevk::BufferPool::Page::MakeNewShared ()
{
    return std::shared_ptr<apemodevk::BufferPool::Page> (MakeNew ());
}

std::unique_ptr<apemodevk::BufferPool::Page> apemodevk::BufferPool::Page::MakeNewUnique ()
{
    return std::unique_ptr<apemodevk::BufferPool::Page> (MakeNew ());
}

apemodevk::BufferPool::Page::Page ()
{
}

apemodevk::BufferPool::Page::~Page ()
{
}