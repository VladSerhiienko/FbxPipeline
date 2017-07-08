#pragma once

#include <GraphicsManager.Vulkan.h>

namespace apemodevk {
    class Swapchain;
    class CommandQueue;
    class GraphicsManager;

    class QueuePool;

    class GraphicsDevice : public apemodevk::ScalableAllocPolicy, public apemodevk::NoCopyAssignPolicy {
    public:
        friend Swapchain;
        friend GraphicsManager;

        typedef std::vector< float >                           FloatVector;
        typedef TInfoStruct< VkLayerProperties >::Vector       VkLayerPropertiesVector;
        typedef TInfoStruct< VkQueueFamilyProperties >::Vector VkQueueFamilyPropertiesVector;
        typedef TInfoStruct< VkDeviceQueueCreateInfo >::Vector VkDeviceQueueCreateInfoVector;
        typedef GraphicsManager::NativeLayerWrapper            NativeLayerWrapper;
        typedef VkFormatProperties                             VkFormatPropertiesArray[ VK_FORMAT_RANGE_SIZE ];

        static std::unique_ptr< GraphicsDevice > MakeNewUnique( );
        static std::unique_ptr< GraphicsDevice > MakeNullUnique( );

        GraphicsDevice( );
        ~GraphicsDevice( );

        bool RecreateResourcesFor( VkPhysicalDevice pPhysicalDevice, GraphicsManager &GraphicsEcosystem );

        bool IsValid( ) const;
        bool Await( );

        QueuePool *      GetQueuePool( );
        const QueuePool *GetQueuePool( ) const;

        GraphicsManager &      GetGraphicsEcosystem( );
        NativeLayerWrapper &   GetUnnamedLayer( );

        bool ScanDeviceQueues( VkQueueFamilyPropertiesVector &QueueProps,
                               VkDeviceQueueCreateInfoVector &QueueReqs,
                               FloatVector &                  QueuePriorities );

        bool ScanDeviceLayerProperties( );
        bool ScanFormatProperties( );

        operator VkDevice( ) const;
        operator VkPhysicalDevice( ) const;
        operator VkInstance( ) const;

        GraphicsManager *                    pGraphicsEcosystem;
        TDispatchableHandle< VkDevice >      hLogicalDevice;
        VkPhysicalDevice                     pPhysicalDevice;
        VkPhysicalDeviceProperties           AdapterProps;
        VkPhysicalDeviceMemoryProperties     MemoryProps;
        VkPhysicalDeviceFeatures             Features;
        VkFormatPropertiesArray              FormatProperties;
        std::vector< NativeLayerWrapper >    LayerWrappers;
        std::vector< const char * >          PresentLayers;
        std::vector< VkExtensionProperties > DeviceExtensionProps;
        std::unique_ptr< QueuePool >         pQueuePool;
    };
}