//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <BufferPools.Vulkan.h>

#include <CommandQueue.Vulkan.h>

/// -------------------------------------------------------------------------------------------------------------------
/// BufferPool
/// -------------------------------------------------------------------------------------------------------------------

apemode::BufferPool::BufferPool(BufferPoolType PoolType)
    : ePoolType(PoolType)
{
    _Aux_DebugTraceFunc;
}

apemode::BufferPool::~BufferPool()
{
    _Aux_DebugTraceFunc;
}

bool apemode::BufferPool::RecreateResourcesFor(GraphicsDevice & InGraphicsNode)
{
    Pages.clear();
    pNode = &InGraphicsNode;
    return true;
}

void apemode::BufferPool::OnResourcesEvicted(GraphicsDevice & InGraphicsNode)
{
}

void apemode::BufferPool::OnCommandListReset(CommandBuffer & CmdBuffer)
{
}

apemode::BufferPool::Range apemode::BufferPool::Suballocate(CommandBuffer & CmdBuffer, uint32_t Size)
{
    apemode::BufferPool::Range RetRange;
    const bool bSuballocated = Suballocate(CmdBuffer, Size, RetRange);
    _Game_engine_Assert(bSuballocated, "Out of memory");
    return RetRange;
}

bool apemode::BufferPool::Suballocate(CommandBuffer & CmdBuffer, uint32_t Size, Range & OutRange)
{
    return false;
}

void apemode::BufferPool::OnCommandListExecutePreview(CommandBuffer & CmdBuffer)
{
}

bool apemode::BufferPool::IsOutOfMemory() const
{
    return bIsOutOfMemory;
}

apemode::BufferPoolType apemode::BufferPool::GetBufferPoolType() const
{
    return ePoolType;
}

apemode::BufferPool::Page * apemode::BufferPool::Page::MakeNew ()
{
    return new Page ();
}

std::shared_ptr<apemode::BufferPool::Page> apemode::BufferPool::Page::MakeNewShared ()
{
    return std::shared_ptr<apemode::BufferPool::Page> (MakeNew ());
}

std::unique_ptr<apemode::BufferPool::Page> apemode::BufferPool::Page::MakeNewUnique ()
{
    return std::unique_ptr<apemode::BufferPool::Page> (MakeNew ());
}

apemode::BufferPool::Page::Page ()
{
    _Aux_DebugTraceFunc;
}

apemode::BufferPool::Page::~Page ()
{
    _Aux_DebugTraceFunc;
}