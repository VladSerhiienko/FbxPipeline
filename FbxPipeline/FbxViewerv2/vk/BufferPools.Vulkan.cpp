#include "BufferPools.Vulkan.h"

bool apemodevk::HostBufferPool::Page::Recreate( VkDevice              pInLogicalDevice,
                                                VkPhysicalDevice      pInPhysicalDevice,
                                                uint32_t              alignment,
                                                uint32_t              bufferRange,
                                                VkBufferUsageFlags    bufferUsageFlags,
                                                VkMemoryPropertyFlags memoryPropertyFlags ) {
    pMapped              = nullptr;
    pDevice              = pInLogicalDevice;
    TotalOffsetCount     = bufferRange / alignment;
    eMemoryPropertyFlags = memoryPropertyFlags;

    Ranges.reserve( std::min< uint32_t >( 64, TotalOffsetCount ) );

    TotalSize = bufferRange;
    Alignment = alignment;

    VkBufferCreateInfo bufferCreateInfo;
    InitializeStruct( bufferCreateInfo );
    bufferCreateInfo.size  = bufferRange;
    bufferCreateInfo.usage = bufferUsageFlags;

    if ( false == hBuffer.Recreate( pInLogicalDevice, pInPhysicalDevice, bufferCreateInfo ) ) {
        apemodevk::platform::DebugBreak( );
        return false;
    }

    auto memoryAllocateInfo = hBuffer.GetMemoryAllocateInfo( memoryPropertyFlags );
    if ( false == hMemory.Recreate( pInLogicalDevice, memoryAllocateInfo ) ) {
        apemodevk::platform::DebugBreak( );
        return false;
    }

    if ( false == hBuffer.BindMemory( hMemory, 0 ) ) {
        apemodevk::platform::DebugBreak( );
        return false;
    }

    return Reset( );
}

bool apemodevk::HostBufferPool::Page::Reset( ) {
    if ( nullptr == pMapped ) {
        if ( VK_SUCCESS != vkInvalidateMappedMemoryRanges( pDevice, (uint32_t) Ranges.size( ), Ranges.data( ) ) ) {
            apemodevk::platform::DebugBreak( );
        }

        Ranges.clear( );
        CurrentOffsetIndex = 0;

        pMapped = hMemory.Map( 0, TotalSize, 0 );
        if ( nullptr == pMapped ) {
            apemodevk::platform::DebugBreak( );
            return false;
        }
    }

    /* TODO: Create descriptor set. */
    return true;
}

bool apemodevk::HostBufferPool::Page::Flush( ) {
    if ( pMapped ) {
        if ( false == HasFlagEq( eMemoryPropertyFlags, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) ) {
            if ( VK_SUCCESS != vkFlushMappedMemoryRanges( pDevice, (uint32_t) Ranges.size( ), Ranges.data( ) ) ) {
                apemodevk::platform::DebugBreak( );
                return false;
            }
        }

        hMemory.Unmap( );
        pMapped = nullptr;
    }

    return true;
}

void apemodevk::HostBufferPool::Recreate( VkDevice                      pInLogicalDevice,
                                          VkPhysicalDevice              pInPhysicalDevice,
                                          const VkPhysicalDeviceLimits *pInLimits,
                                          VkBufferUsageFlags            bufferUsageFlags,
                                          bool                          bInHostCoherent ) {
    Destroy( );

    pLogicalDevice  = pInLogicalDevice;
    pPhysicalDevice = pInPhysicalDevice;
    eMemoryPropertyFlags |= bInHostCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0;

    Pages.reserve( 16 );

    if ( 0 != bufferUsageFlags )
        eBufferUsageFlags = bufferUsageFlags;

    /* Use defaults otherwise. */
    if ( nullptr != pInLimits ) {
        if ( HasFlagEq( eBufferUsageFlags, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT ) ) {
            MaxPageRange = pInLimits->maxUniformBufferRange;
            MinAlignment = (uint32_t) pInLimits->minUniformBufferOffsetAlignment;
        } else {
            MaxPageRange = pInLimits->maxStorageBufferRange;
            MinAlignment = (uint32_t) pInLimits->minStorageBufferOffsetAlignment;
        }

        /* Catch incorrectly initialized device limits. */
        assert( 0 != MaxPageRange );
        assert( 0 != MinAlignment );
    }
}

apemodevk::HostBufferPool::Page *apemodevk::HostBufferPool::FindPage( uint32_t dataStructureSize ) {
    /* Ensure it is possible to allocate that space. */
    if ( dataStructureSize > MaxPageRange ) {
        apemodevk::platform::DebugBreak( );
        return nullptr;
    }

    /* Calculate how many chunks the data covers. */
    const uint32_t coveredOffsetCount = dataStructureSize / MinAlignment + 1;

    /* Try to find an existing free page. */
    auto pageIt = std::find_if( Pages.begin( ), Pages.end( ), [&]( Page *pPage ) {
        assert( nullptr != pPage );
        const uint32_t availableOffsets = pPage->TotalOffsetCount - pPage->CurrentOffsetIndex + 1;
        return availableOffsets >= coveredOffsetCount;
    } );

    /* Return free page. */
    if ( pageIt != Pages.end( ) ) {
        return *pageIt;
    }

    /* Allocate new page and return. */
    Pages.push_back( new Page( ) );

    Pages.back( )->Recreate( pLogicalDevice,
                             pPhysicalDevice,
                             MinAlignment,
                             HasFlagEq( eBufferUsageFlags, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT )
                                 ? MaxPageRange
                                 : std::min( MaxPageRange, dataStructureSize ),
                             eBufferUsageFlags,
                             eMemoryPropertyFlags );

    return Pages.back( );
}

void apemodevk::HostBufferPool::Flush( ) {
    std::for_each( Pages.begin( ), Pages.end( ), [&]( Page *pPage ) {
        assert( nullptr != pPage );
        pPage->Flush( );
    } );
}

void apemodevk::HostBufferPool::Reset( ) {
    std::for_each( Pages.begin( ), Pages.end( ), [&]( Page *pPage ) {
        assert( nullptr != pPage );
        pPage->Reset( );
    } );
}

void apemodevk::HostBufferPool::Destroy( ) {
    std::for_each( Pages.begin( ), Pages.end( ), [&]( Page *pPage ) {
        assert( nullptr != pPage );
        delete pPage;
    } );

    Pages.clear( );
}

uint32_t apemodevk::HostBufferPool::Page::Push( const void *pDataStructure, uint32_t ByteSize ) {
    const uint32_t coveredOffsetCount   = ByteSize / Alignment + 1;
    const uint32_t availableOffsetCount = TotalOffsetCount - CurrentOffsetIndex + 1;

    assert( nullptr != pMapped );
    assert( coveredOffsetCount <= availableOffsetCount );

    const uint32_t currentMappedOffset = CurrentOffsetIndex * Alignment;

    if ( false == HasFlagEq( eMemoryPropertyFlags, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) ) {
        /* Fill current range for flush. */
        Ranges.emplace_back( );

        auto &range = Ranges.back( );
        InitializeStruct( range );
        range.offset = currentMappedOffset;
        range.size   = ByteSize;
        range.memory = hMemory;
    }

    /* Get current memory pointer and copy there. */
    auto mappedData = pMapped + currentMappedOffset;
    memcpy( mappedData, pDataStructure, ByteSize );
    CurrentOffsetIndex += coveredOffsetCount;

    return currentMappedOffset;
}

apemodevk::HostBufferPool::SuballocResult apemodevk::HostBufferPool::Suballocate( const void *pDataStructure,
                                                                                  uint32_t    ByteSize ) {
    SuballocResult suballocResult;
    InitializeStruct( suballocResult.descBufferInfo );

    if ( auto pPage = FindPage( ByteSize ) ) {
        suballocResult.descBufferInfo.buffer = pPage->hBuffer;
        suballocResult.descBufferInfo.range  = pPage->TotalSize;
        suballocResult.dynamicOffset         = pPage->Push( pDataStructure, ByteSize );
    }

    return suballocResult;
}
