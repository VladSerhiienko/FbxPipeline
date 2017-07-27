#pragma once

#include <math.h>
#include <mathfu/matrix.h>
#include <mathfu/vector.h>
#include <mathfu/glsl_mappings.h>

#include <GraphicsDevice.Vulkan.h>
#include <DescriptorPool.Vulkan.h>

#define _apemodevk_HostBufferPool_Page_InvalidateOrFlushAllRanges 0

namespace apemodevk {

    struct HostBufferPool {
        struct Page {
            TDispatchableHandle< VkBuffer >       hBuffer;
            TDispatchableHandle< VkDeviceMemory > hMemory;
            VkDevice                              pDevice              = VK_NULL_HANDLE;
            uint8_t *                             pMapped              = nullptr;
            uint32_t                              Alignment            = 0;
            uint32_t                              TotalSize            = 0;
            uint32_t                              CurrentOffsetIndex   = 0;
            uint32_t                              TotalOffsetCount     = 0;
            VkMemoryPropertyFlags                 eMemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

#if _apemodevk_HostBufferPool_Page_InvalidateOrFlushAllRanges
            std::vector< VkMappedMemoryRange > Ranges;
#else
            VkMappedMemoryRange Range;
#endif

            bool Recreate( VkDevice              pInLogicalDevice,
                           VkPhysicalDevice      pInPhysicalDevice,
                           uint32_t              alignment,
                           uint32_t              bufferRange,
                           VkBufferUsageFlags    bufferUsageFlags,
                           VkMemoryPropertyFlags eMemoryPropertyFlags );

            bool Reset( );

            /* NOTE: Called from pool. */
            bool Flush( );

            /* NOTE: Does not handle space requirements (aborts in debug mode only). */
            uint32_t Push( const void *pDataStructure, uint32_t ByteSize );

            /* NOTE: Does not handle space requirements (aborts in debug mode only). */
            template < typename TDataStructure >
            uint32_t TPush( const TDataStructure &dataStructure ) {
                return Push( &dataStructure, sizeof( TDataStructure ) );
            }
        };

        struct SuballocResult {
            VkDescriptorBufferInfo descBufferInfo;
            uint32_t               dynamicOffset;
        };

        VkDevice              pLogicalDevice       = VK_NULL_HANDLE;
        VkPhysicalDevice      pPhysicalDevice      = VK_NULL_HANDLE;
        VkBufferUsageFlags    eBufferUsageFlags    = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        VkMemoryPropertyFlags eMemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        uint32_t              MinAlignment         = 256;
        uint32_t              MaxPageRange         = 65536; /* 256 * 256 */
        std::vector< Page * > Pages;

        /**
         * @param pInLogicalDevice Logical device.
         * @param pInPhysicalDevice Physical device.
         * @param pInDescPool Descriptor pool.
         * @param pInLimits Device limits (null is ok).
         * @param usageFlags Usually UNIFORM.
         **/
        void Recreate( VkDevice                      pInLogicalDevice,
                       VkPhysicalDevice              pInPhysicalDevice,
                       const VkPhysicalDeviceLimits *pInLimits,
                       VkBufferUsageFlags            bufferUsageFlags,
                       bool                          bInHostCoherent );

        Page *FindPage( uint32_t dataStructureSize );

        /* On command buffer submission */
        void Flush( );

        void Reset( );

        void Destroy( );

        SuballocResult Suballocate(const void *pDataStructure, uint32_t ByteSize);

        template < typename TDataStructure >
        SuballocResult TSuballocate( const TDataStructure &dataStructure ) {
            return Suballocate( &dataStructure, sizeof( TDataStructure ) );
        }
    };
}