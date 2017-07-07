#pragma once

#include <SceneRendererBase.h>
#include <GraphicsDevice.Vulkan.h>

namespace apemodevk {

    class GraphicsDevice;

    class QueuePool;
    class QueueFamilyPool;

    struct QueueInPool {
        apemodevk::TDispatchableHandle< VkQueue > hQueue;         /* Handle */
        apemodevk::TDispatchableHandle< VkFence > hFence;         /* Indicates the execution is in progress */
        std::atomic_bool                          bInUse = false; /* Indicates it is used by other thread */
        uint32_t                                  Id     = 0;     /* Index in the list */
    };

    class QueueFamilyPool {
        friend class QueuePool;

        VkDevice                   pDevice         = VK_NULL_HANDLE;
        VkPhysicalDevice           pPhysicalDevice = VK_NULL_HANDLE;
        uint32_t                   AvailableCount  = 0;
        uint32_t                   FamilyId        = 0;
        VkQueueFamilyProperties    FamilyProps;
        std::vector< QueueInPool > Queues;

        QueueFamilyPool( VkDevice                       pInDevice,
                         VkPhysicalDevice               pInPhysicalDevice,
                         uint32_t                       InQueueFamilyIndex,
                         VkQueueFamilyProperties const& InQueueFamilyProps );

    public:
        ~QueueFamilyPool( );

        VkQueueFamilyProperties const& GetQueueFamilyProps( );
        bool SupportsPresenting( VkSurfaceKHR pSurface ) const;

        /**
         * @param bIgnoreFenceStatus If any command buffer submitted to this queue is in the executable state,
         *                           it is moved to the pending state. Note, that VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
         *                           command buffers will be invalidated (unless explicit vkWaitForFences).
         * @return Unused queue, or null if none was found.
         * @note Release for reusing, @see Release().
         */
        QueueInPool* Acquire( bool bIgnoreFenceStatus );

        /**
         * Allows the queue to be reused.
         */
        bool Release( VkQueue pUnneededQueue );
    };

    struct AcquiredQueue {
        VkQueue  pQueue        = VK_NULL_HANDLE;
        VkFence  pFence        = VK_NULL_HANDLE;
        uint32_t QueueFamilyId = 0;
        uint32_t QueueId       = 0;
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

        /**
         * @return nullptr if poolIndex is out of range.
         */
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

        void Release( AcquiredQueue acquiredQueue );
    };

}