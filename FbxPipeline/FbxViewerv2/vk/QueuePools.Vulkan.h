#pragma once

#include <SceneRendererBase.h>
#include <GraphicsDevice.Vulkan.h>

namespace apemodevk {

    class GraphicsDevice;

    class QueuePool;
    class QueueFamilyPool;

    struct QueueInPool {
        VkQueue          hQueue = VK_NULL_HANDLE; /* Handle */
        VkFence          hFence = VK_NULL_HANDLE; /* Indicates the execution is in progress */
        std::atomic_bool bInUse = false;          /* Indicates it is used by other thread */

        QueueInPool( );
        QueueInPool( const QueueInPool& other );
    };

    struct AcquiredQueue {
        VkQueue  pQueue        = VK_NULL_HANDLE; /* Free to use queue. */
        VkFence  pFence        = VK_NULL_HANDLE; /* Acquire() can optionally ignore fence status, so the user can await. */
        uint32_t QueueFamilyId = 0; /* Queue family index */
        uint32_t QueueId       = 0; /* Queue index in family pool */
    };

    class QueueFamilyPool {
        friend class QueuePool;

        VkDevice                   pDevice         = VK_NULL_HANDLE;
        VkPhysicalDevice           pPhysicalDevice = VK_NULL_HANDLE;
        uint32_t                   QueueFamilyId   = 0;
        VkQueueFamilyProperties    QueueFamilyProps;
        std::vector< QueueInPool > Queues;

        QueueFamilyPool( VkDevice                       pInDevice,
                         VkPhysicalDevice               pInPhysicalDevice,
                         uint32_t                       InQueueFamilyIndex,
                         VkQueueFamilyProperties const& InQueueFamilyProps );
        QueueFamilyPool( const QueueFamilyPool& other ) = default;

    public:
        QueueFamilyPool( QueueFamilyPool&& other ) = default;
        ~QueueFamilyPool( );

        const VkQueueFamilyProperties& GetQueueFamilyProps( ) const;

        bool SupportsPresenting( VkSurfaceKHR pSurface ) const;
        bool SupportsGraphics( ) const;
        bool SupportsCompute( ) const;
        bool SupportsSparseBinding( ) const;
        bool SupportsTransfer( ) const;

        /**
         * @param bIgnoreFenceStatus If any command buffer submitted to this queue is in the executable state,
         *                           it is moved to the pending state. Note, that VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
         *                           command buffers will be invalidated (unless explicit vkWaitForFences).
         * @return Unused queue, or null if none was found.
         * @note Release for reusing, @see Release().
         */
        AcquiredQueue Acquire( bool bIgnoreFenceStatus );

        /**
         * Allows the queue to be reused.
         */
        bool Release( const AcquiredQueue& pUnneededQueue );
    };

    /**
     * QueuePool is created by device.
     */
    class QueuePool {
        VkDevice                       pDevice         = VK_NULL_HANDLE;
        VkPhysicalDevice               pPhysicalDevice = VK_NULL_HANDLE;
        std::vector< QueueFamilyPool > Pools;

        friend class GraphicsDevice;
        QueuePool( VkDevice                       pInDevice,
                   VkPhysicalDevice               pInPhysicalDevice,
                   const VkQueueFamilyProperties* pQueuePropsIt,
                   const VkQueueFamilyProperties* pQueuePropsEnd,
                   const float*                   pQueuePrioritiesIt,
                   const float*                   pQueuePrioritiesItEnd );

    public:
        ~QueuePool( );

        QueueFamilyPool*       GetPool( uint32_t poolIndex );
        const QueueFamilyPool* GetPool( uint32_t poolIndex ) const;
        uint32_t               GetPoolCount( ) const;

        static const VkQueueFlags sQueueAllBits = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT;

        /**
         * @param bIgnoreFenceStatus If any command buffer submitted to this queue is in the executable state,
         *                           it is moved to the pending state. Note, that VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
         *                           command buffers will be invalidated (unless explicit vkWaitForFences).
         * @param RequiredQueueFlags Flags that must be available enabled (try to pick only the needed bits).
         * @param bExactMatchByFlags Only the flags in RequiredQueueFlags must be present (for copy queues is important).
         * @return Unused queue, or null if none was found.
         * @note Release for reusing, @see Release().
         */
        AcquiredQueue Acquire( bool         bIgnoreFenceStatus,
                               VkQueueFlags RequiredQueueFlags = sQueueAllBits,
                               bool         bExactMatchByFlags = false );
        
        /**
         * Allows the queue to be reused.
         */
        void Release( const AcquiredQueue& acquiredQueue );
    };

}