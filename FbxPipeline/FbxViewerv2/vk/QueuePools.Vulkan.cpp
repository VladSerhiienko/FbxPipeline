#include "QueuePools.Vulkan.h"

apemodevk::QueuePool::QueuePool( VkDevice                       pInDevice,
                                 VkPhysicalDevice               pInPhysicalDevice,
                                 const VkQueueFamilyProperties* pQueuePropsIt,
                                 const VkQueueFamilyProperties* pQueuePropsEnd,
                                 const float*                   pQueuePrioritiesIt,
                                 const float*                   pQueuePrioritiesItEnd )
    : pDevice( pInDevice )
    , pPhysicalDevice( pInPhysicalDevice ) {
    (void) pQueuePrioritiesIt;
    (void) pQueuePrioritiesItEnd;

    Pools.reserve( std::distance( pQueuePropsIt, pQueuePropsEnd ) );

    uint32_t familyIndex = 0;
    std::transform( pQueuePropsIt, pQueuePropsEnd, std::back_inserter( Pools ), [&]( const VkQueueFamilyProperties& InProps ) {
        return QueueFamilyPool( pInDevice, pInPhysicalDevice, familyIndex++, InProps );
    } );
}

apemodevk::QueuePool::~QueuePool( ) {
}

apemodevk::QueueFamilyPool* apemodevk::QueuePool::GetPool( uint32_t poolIndex ) {
    if ( poolIndex < Pools.size( ) )
        return &Pools[ poolIndex ];
    return nullptr;
}

const apemodevk::QueueFamilyPool* apemodevk::QueuePool::GetPool( uint32_t poolIndex ) const {
    if ( poolIndex < Pools.size( ) )
        return &Pools[ poolIndex ];
    return nullptr;
}

uint32_t apemodevk::QueuePool::GetPoolCount( ) const {
    return Pools.size( );
}

apemodevk::AcquiredQueue apemodevk::QueuePool::Acquire( bool         bIgnoreFenceStatus,
                                                        VkQueueFlags queueFlags,
                                                        bool         bExactMatchByFlags ) {
    /* First, try to find the exact match. */
    /* This can be crucial for only trasfer or only compute queues. */
    for ( auto& pool : Pools )
        if (pool.QueueFamilyProps.queueFlags == queueFlags) {
            auto acquiredQueue = pool.Acquire( bIgnoreFenceStatus );
            if ( VK_NULL_HANDLE != acquiredQueue.pQueue )
                return acquiredQueue;
        }

    /* Try to find something usable. */
    if ( false == bExactMatchByFlags )
        for ( auto& pool : Pools )
            if ( queueFlags == ( pool.QueueFamilyProps.queueFlags & queueFlags ) ) {
                auto acquiredQueue = pool.Acquire( bIgnoreFenceStatus );
                if ( VK_NULL_HANDLE != acquiredQueue.pQueue )
                    return acquiredQueue;
            }

    /* Nothing available for immediate usage */
    return {};
}

void apemodevk::QueuePool::Release( const apemodevk::AcquiredQueue& acquiredQueue ) {
    Pools[ acquiredQueue.QueueFamilyId ].Release( acquiredQueue );
}

apemodevk::QueueFamilyPool::QueueFamilyPool( VkDevice                       pInDevice,
                                             VkPhysicalDevice               pInPhysicalDevice,
                                             uint32_t                       InQueueFamilyIndex,
                                             VkQueueFamilyProperties const& InQueueFamilyProps )
    : QueueFamilyProps( InQueueFamilyProps )
    , QueueFamilyId( InQueueFamilyIndex )
    , pDevice( pInDevice )
    , pPhysicalDevice( pInPhysicalDevice ) {
    Queues.resize( InQueueFamilyProps.queueCount );
}

apemodevk::QueueFamilyPool::~QueueFamilyPool( ) {
    if ( false == Queues.empty( ) ) {
        uint32_t queueIndex = QueueFamilyProps.queueCount;
        while ( queueIndex-- ) {
            if ( VK_NULL_HANDLE != Queues[ queueIndex ].hFence ) {
                vkWaitForFences( pDevice, 1, &Queues[ queueIndex ].hFence, true, UINT64_MAX );
                vkDestroyFence( pDevice, Queues[ queueIndex ].hFence, nullptr );
            }
        }
    }
}

const VkQueueFamilyProperties& apemodevk::QueueFamilyPool::GetQueueFamilyProps( ) const {
    return QueueFamilyProps;
}

bool apemodevk::QueueFamilyPool::SupportsPresenting( VkSurfaceKHR pSurface ) const {
    VkBool32 supported = false;

    switch ( vkGetPhysicalDeviceSurfaceSupportKHR( pPhysicalDevice, QueueFamilyId, pSurface, &supported ) ) {
        case VK_SUCCESS:
            /* Function succeeded, see the assigned pSucceeded value. */
            return supported;

        case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        case VK_ERROR_SURFACE_LOST_KHR:
            /* TODO: Cases we expect to happen, need to handle */
            DebugBreak( );
            break;
    }

    return false;
}

bool apemodevk::QueueFamilyPool::SupportsGraphics( ) const {
    return apemodevk::HasFlagEql( QueueFamilyProps.queueFlags, VK_QUEUE_GRAPHICS_BIT );
}

bool apemodevk::QueueFamilyPool::SupportsCompute( ) const {
    return apemodevk::HasFlagEql( QueueFamilyProps.queueFlags, VK_QUEUE_COMPUTE_BIT );
}

bool apemodevk::QueueFamilyPool::SupportsSparseBinding( ) const {
    return apemodevk::HasFlagEql( QueueFamilyProps.queueFlags, VK_QUEUE_SPARSE_BINDING_BIT );
}

bool apemodevk::QueueFamilyPool::SupportsTransfer( ) const {
    return apemodevk::HasFlagEql( QueueFamilyProps.queueFlags, VK_QUEUE_TRANSFER_BIT );
}

apemodevk::AcquiredQueue apemodevk::QueueFamilyPool::Acquire( bool bIgnoreFence ) {
    uint32_t queueIndex = 0;

    /* Loop through queues */
    for (auto& queue : Queues) {
        /* If the queue is not used by other thread and it is not executing cmd lists, it will be returned */
        /* Note that queue can suspend the execution of the commands, or discard "one time" buffers. */
        if ( false == queue.bInUse.exchange( true, std::memory_order_acquire ) ) {
            if ( bIgnoreFence || VK_NULL_HANDLE == queue.hQueue || VK_SUCCESS == vkGetFenceStatus( pDevice, queue.hFence ) ) {
                if ( VK_NULL_HANDLE == queue.hQueue ) {
                    VkFenceCreateInfo fenceCreateInfo;
                    InitializeStruct( fenceCreateInfo );
                    /* Since the queue is free to use after creation. */
                    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

                    vkGetDeviceQueue( pDevice, QueueFamilyId, queueIndex, &queue.hQueue );
                    vkCreateFence( pDevice, &fenceCreateInfo, nullptr, &queue.hFence );

                    assert( queue.hQueue );
                    assert( queue.hFence );
                }

                AcquiredQueue acquiredQueue;
                acquiredQueue.pQueue        = queue.hQueue;
                acquiredQueue.pFence        = queue.hFence;
                acquiredQueue.QueueFamilyId = QueueFamilyId;
                acquiredQueue.QueueId       = queueIndex;
                return acquiredQueue;
            }

            /* Move back to unused state */
            queue.bInUse.exchange( false, std::memory_order_release );
        }

        ++queueIndex;
    }

    /* If no free queue found, null is returned */
    return {};
}

bool apemodevk::QueueFamilyPool::Release( const apemodevk::AcquiredQueue& acquiredQueue ) {
    /* Check if the queue was acquired */
    if ( VK_NULL_HANDLE != acquiredQueue.pQueue ) {
        /* No longer used, ok */
        const bool previouslyUsed = Queues[ acquiredQueue.QueueId ].bInUse.exchange( false, std::memory_order_release );
        /* Try to track incorrect usage or atomic mess. */
        if ( false == previouslyUsed )
            DebugBreak( );

        return true;
    }

    DebugBreak( );
    return false;
}

apemodevk::QueueInPool::QueueInPool( ) {
}

apemodevk::QueueInPool::QueueInPool( const QueueInPool& other )
    : hQueue( other.hQueue )
    , hFence( other.hFence )
    , bInUse( other.bInUse.load( std::memory_order_relaxed ) ) {
}
