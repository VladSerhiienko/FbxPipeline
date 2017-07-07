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
        if ( pool.FamilyProps.queueFlags == queueFlags )
            if ( QueueInPool* queueInPool = pool.Acquire( bIgnoreFenceStatus ) )
                return {queueInPool->hQueue, queueInPool->hFence, pool.FamilyId};

    /* Try to find something usable. */
    if ( false == bExactMatchByFlags )
        for ( auto& pool : Pools )
            if ( queueFlags == ( pool.FamilyProps.queueFlags & queueFlags ) )
                if ( QueueInPool* queueInPool = pool.Acquire( bIgnoreFenceStatus ) )
                    return {queueInPool->hQueue, queueInPool->hFence, pool.FamilyId};

    /* Nothing available for immediate usage */
    return {};
}

void apemodevk::QueuePool::Release( AcquiredQueue acquiredQueue ) {
    Pools[ acquiredQueue.QueueFamilyId ].Release( acquiredQueue.pQueue );
}

apemodevk::QueueFamilyPool::QueueFamilyPool( VkDevice                       pInDevice,
                                             VkPhysicalDevice               pInPhysicalDevice,
                                             uint32_t                       InQueueFamilyIndex,
                                             VkQueueFamilyProperties const& InQueueFamilyProps )
    : FamilyProps( InQueueFamilyProps )
    , FamilyId( InQueueFamilyIndex )
    , AvailableCount( InQueueFamilyProps.queueCount )
    , pDevice( pInDevice )
    , pPhysicalDevice( pInPhysicalDevice ) {
    Queues.resize( InQueueFamilyProps.queueCount );

    VkFenceCreateInfo fenceCreateInfo;
    InitializeStruct( fenceCreateInfo );
    /* Since the queue is free to use after creation. */
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    uint32_t queueIndex = FamilyProps.queueCount;
    while ( queueIndex-- ) {
        /* Queues are created with device, no need for lazy initialization. */
        Queues[ queueIndex ].hQueue.Recreate( pInDevice, InQueueFamilyIndex, queueIndex );
        Queues[ queueIndex ].hFence.Recreate( pInDevice, fenceCreateInfo );
    }
}

apemodevk::QueueFamilyPool::~QueueFamilyPool( ) {
    uint32_t queueIndex = FamilyProps.queueCount;
    while ( queueIndex-- ) {
        Queues[ queueIndex ].hQueue.WaitIdle( );
        Queues[ queueIndex ].hQueue.Destroy( );
        Queues[ queueIndex ].hFence.Wait( );
        Queues[ queueIndex ].hFence.Destroy( );
    }

    Queues.clear( );
}

VkQueueFamilyProperties const& apemodevk::QueueFamilyPool::GetQueueFamilyProps( ) {
    return FamilyProps;
}

bool apemodevk::QueueFamilyPool::SupportsPresenting( VkSurfaceKHR pSurface ) const {
    VkBool32 supported = false;

    switch ( vkGetPhysicalDeviceSurfaceSupportKHR( pPhysicalDevice, FamilyId, pSurface, &supported ) ) {
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

apemodevk::QueueInPool* apemodevk::QueueFamilyPool::Acquire( bool bIgnoreFence ) {
    /* Loop through queues */
    for ( auto& queue : Queues )
        /* If the queue is not used by other thread and it is not executing cmd lists, it will be returned */
        /* Note that the used can suspend the execution of the commands, or discard for "one time" buffers. */
        if ( false == queue.bInUse.exchange( true, std::memory_order_acquire ) ) {
            if ( bIgnoreFence || queue.hFence.IsSignalled( ) )
                return &queue;

            /* Move back to unused state */
            queue.bInUse.exchange( false, std::memory_order_release );
        }

    /* If no free queue found, null is returned */
    return nullptr;
}

bool apemodevk::QueueFamilyPool::Release( VkQueue pUnneededQueue ) {
    /* Loop through queues */
    for ( auto& queue : Queues )
        /* Check if the queue is in the pool */
        if ( pUnneededQueue == queue.hQueue.Handle ) {
            /* No longer used, ok */
            const bool previouslyUsed = queue.bInUse.exchange( false, std::memory_order_release );

            /* Try to track ncorrect usage or atomic mess. */
            if ( false == previouslyUsed )
                DebugBreak( );

            return true;
        }

    DebugBreak( );
    return false;
}
