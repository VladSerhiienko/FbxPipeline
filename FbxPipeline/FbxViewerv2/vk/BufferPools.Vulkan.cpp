//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <BufferPools.Vulkan.h>

#include <CommandQueue.Vulkan.h>

/// -------------------------------------------------------------------------------------------------------------------
/// BufferPool
/// -------------------------------------------------------------------------------------------------------------------

Core::BufferPool::BufferPool(BufferPoolType PoolType)
    : ePoolType(PoolType)
{
    _Aux_DebugTraceFunc;
}

Core::BufferPool::~BufferPool()
{
    _Aux_DebugTraceFunc;
}

bool Core::BufferPool::RecreateResourcesFor(GraphicsDevice & InGraphicsNode)
{
    Pages.clear();
    pGraphicsNode = &InGraphicsNode;
    return true;
}

void Core::BufferPool::OnResourcesEvicted(GraphicsDevice & InGraphicsNode)
{
}

void Core::BufferPool::OnCommandListReset(CommandList & CmdList)
{
}

Core::BufferPool::Range Core::BufferPool::Suballocate(CommandList & CmdList, uint32_t Size)
{
    Core::BufferPool::Range RetRange;
    const bool bSuballocated = Suballocate(CmdList, Size, RetRange);
    _Game_engine_Assert(bSuballocated, "Out of memory");
    return RetRange;
}

bool Core::BufferPool::Suballocate(CommandList & CmdList, uint32_t Size, Range & OutRange)
{
    return false;
}

void Core::BufferPool::OnCommandListExecutePreview(CommandList & CmdList)
{
}

bool Core::BufferPool::IsOutOfMemory() const
{
    return bIsOutOfMemory;
}

Core::BufferPoolType Core::BufferPool::GetBufferPoolType() const
{
    return ePoolType;
}

Core::BufferPool::Page * Core::BufferPool::Page::MakeNew ()
{
    return new Page ();
}

std::shared_ptr<Core::BufferPool::Page> Core::BufferPool::Page::MakeNewShared ()
{
    return std::shared_ptr<Core::BufferPool::Page> (MakeNew ());
}

std::unique_ptr<Core::BufferPool::Page> Core::BufferPool::Page::MakeNewUnique ()
{
    return std::unique_ptr<Core::BufferPool::Page> (MakeNew ());
}

Core::BufferPool::Page::Page ()
{
    _Aux_DebugTraceFunc;
}

Core::BufferPool::Page::~Page ()
{
    _Aux_DebugTraceFunc;
}