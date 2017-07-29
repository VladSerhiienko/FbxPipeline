//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <GraphicsDevice.Vulkan.h>
#include <GraphicsManager.Vulkan.h>

#include <QueuePools.Vulkan.h>

#include <GraphicsManager.KnownExtensions.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>
#include <SystemAllocationCallbacks.Vulkan.h>
#include <TInfoStruct.Vulkan.h>

/// -------------------------------------------------------------------------------------------------------------------
/// GraphicsDevice PrivateContent
/// -------------------------------------------------------------------------------------------------------------------

bool apemodevk::GraphicsManager::NativeLayerWrapper::IsValidDeviceLayer( ) const {
    if ( IsUnnamedLayer( ) ) {
        auto const KnownExtensionBeginIt = Vulkan::KnownDeviceExtensions;
        auto const KnownExtensionEndIt   = Vulkan::KnownDeviceExtensions + Vulkan::KnownDeviceExtensionCount;

        auto Counter = [this]( char const* KnownExtension ) {
            auto const ExtensionBeginIt = Extensions.begin( );
            auto const ExtensionEndIt   = Extensions.end( );

            auto Finder = [&]( VkExtensionProperties const& Extension ) {
                static const int eStrCmp_EqualStrings = 0;
                return strcmp( KnownExtension, Extension.extensionName ) == eStrCmp_EqualStrings;
            };

            auto const FoundExtensionIt  = std::find_if( ExtensionBeginIt, ExtensionEndIt, Finder );
            auto const bIsExtensionFound = FoundExtensionIt != ExtensionEndIt;
            apemode_assert( bIsExtensionFound, "Extension '%s' was not found.", KnownExtension );

            return bIsExtensionFound;
        };

        size_t KnownExtensionsFound = std::count_if( KnownExtensionBeginIt, KnownExtensionEndIt, Counter );
        return KnownExtensionsFound == Vulkan::KnownDeviceExtensionCount;
    }

    return true;
}

bool apemodevk::GraphicsDevice::ScanDeviceQueues( VkQueueFamilyPropertiesVector& QueueProps,
                                                  VkDeviceQueueCreateInfoVector& QueueReqs,
                                                  FloatVector&                   QueuePriorities ) {
    uint32_t QueuesFound = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( pPhysicalDevice, &QueuesFound, NULL );

    if ( QueuesFound != 0 ) {
        QueueProps.resize( QueuesFound );
        QueueReqs.reserve( QueuesFound );

        vkGetPhysicalDeviceQueueFamilyProperties( pPhysicalDevice, &QueuesFound, QueueProps.data( ) );
        platform::DebugTrace( "Queue Families: %u", QueuesFound );

        uint32_t TotalQueuePrioritiesCount = 0;
        std::for_each( QueueProps.begin( ), QueueProps.end( ), [&]( VkQueueFamilyProperties& QueueProp ) {
            platform::DebugTrace( "> Flags: %x, Count: %u", QueueProp.queueFlags, QueueProp.queueCount );
            TotalQueuePrioritiesCount += QueueProp.queueCount;
        } );

        uint32_t QueuePrioritiesAssigned = 0;
        QueuePriorities.resize( TotalQueuePrioritiesCount );
        std::for_each( QueueProps.begin( ), QueueProps.end( ), [&]( VkQueueFamilyProperties& QueueProp ) {
            auto& QueueReq             = apemodevk::PushBackAndGet( QueueReqs );
            QueueReq->pNext            = NULL;
            QueueReq->queueFamilyIndex = static_cast< uint32_t >( std::distance( QueueProps.data( ), &QueueProp ) );
            QueueReq->queueCount       = QueueProp.queueCount;
            QueueReq->pQueuePriorities = QueuePriorities.data( ) + QueuePrioritiesAssigned;

            QueuePrioritiesAssigned += QueueProp.queueCount;
        } );
    }

    return true;
}

bool apemodevk::GraphicsDevice::ScanDeviceLayerProperties( uint32_t flags ) {

    struct SpecialLayerOrExtension {
        uint32_t    eFlag;
        const char* pName;
    };

    DeviceLayers.clear( );
    DeviceLayerProps.clear( );
    DeviceExtensions.clear( );
    DeviceExtensionProps.clear( );

    VkResult eResult;

    uint32_t LayerCount = 0;
    eResult = vkEnumerateDeviceLayerProperties( pPhysicalDevice, &LayerCount, NULL );
    if (  VK_SUCCESS == eResult && LayerCount ) {
        DeviceLayerProps.resize( LayerCount );
        eResult = vkEnumerateDeviceLayerProperties( pPhysicalDevice, &LayerCount, DeviceLayerProps.data( ) );
    }

    if ( VK_SUCCESS != eResult )
        return false;

    DeviceLayers.reserve( LayerCount + 1 );
    DeviceLayers.push_back( nullptr );
    std::transform( DeviceLayerProps.begin( ),
                    DeviceLayerProps.end( ),
                    std::back_inserter( DeviceLayers ),
                    [&]( VkLayerProperties const& ExtProp ) { return ExtProp.layerName; } );

    for ( auto layerName : DeviceLayers ) {
        uint32_t ExtPropCount = 0;
        eResult = vkEnumerateDeviceExtensionProperties( pPhysicalDevice, layerName, &ExtPropCount, NULL );
        if ( VK_SUCCESS == eResult && ExtPropCount ) {
            uint32_t firstExt = DeviceExtensionProps.size( );
            DeviceExtensionProps.resize( DeviceExtensionProps.size( ) + ExtPropCount );
            eResult = vkEnumerateDeviceExtensionProperties( pPhysicalDevice, layerName, &ExtPropCount, DeviceExtensionProps.data( ) + firstExt );
        }
    }

    std::sort( DeviceExtensionProps.begin( ),
               DeviceExtensionProps.end( ),
               [&]( VkExtensionProperties const& a, VkExtensionProperties const& b ) {
                   return 0 > strcmp( a.extensionName, b.extensionName );
               } );
    DeviceExtensionProps.erase( std::unique( DeviceExtensionProps.begin( ),
                                             DeviceExtensionProps.end( ),
                                             [&]( VkExtensionProperties const& a, VkExtensionProperties const& b ) {
                                                 return 0 == strcmp( a.extensionName, b.extensionName );
                                             } ),
                                DeviceExtensionProps.end( ) );

    DeviceLayers.erase( DeviceLayers.begin( ) );

    for ( SpecialLayerOrExtension specialLayer : {
              SpecialLayerOrExtension{GraphicsManager::kEnable_LUNARG_vktrace, "VK_LAYER_LUNARG_vktrace"},
              SpecialLayerOrExtension{GraphicsManager::kEnable_LUNARG_api_dump, "VK_LAYER_LUNARG_api_dump"},
              SpecialLayerOrExtension{GraphicsManager::kEnable_RENDERDOC_Capture, "VK_LAYER_RENDERDOC_Capture"},
              /*
              SpecialLayerOrExtension{kEnable_RENDERDOC_Capture, "VK_LAYER_LUNARG_core_validation"},
              SpecialLayerOrExtension{kEnable_RENDERDOC_Capture, "VK_LAYER_LUNARG_monitor"},
              SpecialLayerOrExtension{kEnable_RENDERDOC_Capture, "VK_LAYER_LUNARG_object_tracker"},
              SpecialLayerOrExtension{kEnable_RENDERDOC_Capture, "VK_LAYER_LUNARG_parameter_validation"},
              SpecialLayerOrExtension{kEnable_RENDERDOC_Capture, "VK_LAYER_LUNARG_screenshot"},
              SpecialLayerOrExtension{kEnable_RENDERDOC_Capture, "VK_LAYER_LUNARG_standard_validation"},
              */
              SpecialLayerOrExtension{8, "VK_LAYER_GOOGLE_threading"},
              SpecialLayerOrExtension{8, "VK_LAYER_GOOGLE_unique_objects"},
              SpecialLayerOrExtension{8, "VK_LAYER_NV_optimus"},
              SpecialLayerOrExtension{8, "VK_LAYER_NV_nsight"},
          } ) {
        if ( false == HasFlagEq( flags, specialLayer.eFlag ) ) {
            auto specialLayerIt = std::find_if( DeviceLayers.begin( ), DeviceLayers.end( ), [&]( const char* pName ) {
                return 0 == strcmp( pName, specialLayer.pName );
            } );

            if ( specialLayerIt != DeviceLayers.end( ) )
                DeviceLayers.erase( specialLayerIt );
        }
    }

    platform::DebugTrace( "Device Layers (%u):", DeviceLayers.size( ) );
    std::for_each( DeviceLayers.begin( ), DeviceLayers.end( ), [&]( const char* pName ) { platform::DebugTrace( "> %s", pName ); } );

    DeviceExtensions.reserve( DeviceExtensionProps.size( ) );
    std::transform( DeviceExtensionProps.begin( ),
                    DeviceExtensionProps.end( ),
                    std::back_inserter( DeviceExtensions ),
                    [&]( VkExtensionProperties const& ExtProp ) { return ExtProp.extensionName; } );

    platform::DebugTrace( "Device Extensions (%u):", DeviceExtensions.size( ) );
    std::for_each( DeviceExtensions.begin( ), DeviceExtensions.end( ), [&]( const char* pName ) { platform::DebugTrace( "> %s", pName ); } );

    return VK_SUCCESS == eResult;
}

bool apemodevk::GraphicsDevice::ScanFormatProperties( ) {
    VkFormat NativeFormatIt = VK_FORMAT_UNDEFINED;
    for ( ; NativeFormatIt < VK_FORMAT_RANGE_SIZE; ) {
        const VkFormat NativeFormat = NativeFormatIt;
        vkGetPhysicalDeviceFormatProperties( pPhysicalDevice, NativeFormat, &FormatProperties[ NativeFormat ] );
        NativeFormatIt = static_cast< VkFormat >( NativeFormatIt + 1 );
    }

    return true;
}

bool apemodevk::GraphicsDevice::RecreateResourcesFor( VkPhysicalDevice InAdapterHandle, GraphicsManager& GraphicsEcosystem, uint32_t flags ) {
    pManager = &GraphicsEcosystem;
    apemode_assert( Args != nullptr, "Ecosystem is required in case of Vulkan." );

    pPhysicalDevice = InAdapterHandle;

    // Physical device is required to create a related logical device.
    // Likewise, vulkan instance is required for physical device.
    if ( VK_NULL_HANDLE != pPhysicalDevice ) {
        vkGetPhysicalDeviceProperties( pPhysicalDevice, &AdapterProps );
        vkGetPhysicalDeviceMemoryProperties( pPhysicalDevice, &MemoryProps );
        vkGetPhysicalDeviceFeatures( pPhysicalDevice, &Features );

        platform::DebugTrace( "Device Name: %s", AdapterProps.deviceName );
        platform::DebugTrace( "Device Type: %u", AdapterProps.deviceType );

        VkQueueFamilyPropertiesVector QueueProps;
        VkDeviceQueueCreateInfoVector QueueReqs;
        FloatVector                   QueuePriorities;

        if ( ScanDeviceQueues( QueueProps, QueueReqs, QueuePriorities ) && ScanFormatProperties( ) && ScanDeviceLayerProperties( flags ) ) {
            std::vector< const char* > DeviceExtNames;
            DeviceExtNames.reserve( DeviceExtensionProps.size( ) );
            std::transform( DeviceExtensionProps.begin( ),
                            DeviceExtensionProps.end( ),
                            std::back_inserter( DeviceExtNames ),
                            [&]( VkExtensionProperties const& ExtProp ) { return ExtProp.extensionName; } );

            VkPhysicalDeviceFeatures Features;
            vkGetPhysicalDeviceFeatures( pPhysicalDevice, &Features );

            TInfoStruct< VkDeviceCreateInfo > DeviceDesc;
            DeviceDesc->pEnabledFeatures        = &Features;
            DeviceDesc->queueCreateInfoCount    = _Get_collection_length_u( QueueReqs );
            DeviceDesc->pQueueCreateInfos       = QueueReqs.data( );
            DeviceDesc->enabledLayerCount       = (uint32_t)DeviceLayers.size();
            DeviceDesc->ppEnabledLayerNames     = DeviceLayers.data( );
            DeviceDesc->enabledExtensionCount   = (uint32_t)DeviceExtensions.size();
            DeviceDesc->ppEnabledExtensionNames = DeviceExtensions.data( );

            const auto bOk = hLogicalDevice.Recreate( pPhysicalDevice, DeviceDesc );
            apemode_assert( bOk, "vkCreateDevice failed." );

            pQueuePool.reset( new QueuePool( hLogicalDevice,
                                             pPhysicalDevice,
                                             QueueProps.data( ),
                                             QueueProps.data( ) + QueueProps.size( ),
                                             QueuePriorities.data( ),
                                             QueuePriorities.data( ) + QueuePriorities.size( ) ) );

            pCmdBufferPool.reset( new CommandBufferPool( hLogicalDevice,
                                                         pPhysicalDevice,
                                                         QueueProps.data( ),
                                                         QueueProps.data( ) + QueueProps.size( ) ) );

            return bOk;
        }
    }

    return false;
}

/// -------------------------------------------------------------------------------------------------------------------
/// GraphicsDevice
/// -------------------------------------------------------------------------------------------------------------------

std::unique_ptr< apemodevk::GraphicsDevice > apemodevk::GraphicsDevice::MakeNewUnique( ) {
    return std::unique_ptr< GraphicsDevice >( new GraphicsDevice( ) );
}

std::unique_ptr< apemodevk::GraphicsDevice > apemodevk::GraphicsDevice::MakeNullUnique( ) {
    return std::unique_ptr< GraphicsDevice >( nullptr );
}

apemodevk::GraphicsDevice::GraphicsDevice( ) : pManager( nullptr ) {
}

apemodevk::GraphicsDevice::~GraphicsDevice( ) {
}

apemodevk::GraphicsDevice::operator VkDevice( ) const {
    return hLogicalDevice;
}

apemodevk::GraphicsDevice::operator VkPhysicalDevice( ) const {
    return pPhysicalDevice;
}

apemodevk::GraphicsDevice::operator VkInstance( ) const {
    return pManager->hInstance;
}

bool apemodevk::GraphicsDevice::IsValid( ) const {
    return hLogicalDevice.IsNotNull( );
}

bool apemodevk::GraphicsDevice::Await( ) {
    apemode_assert( IsValid( ), "Must be valid." );
    return VK_SUCCESS == CheckedCall( vkDeviceWaitIdle( *this ) );
}

apemodevk::QueuePool* apemodevk::GraphicsDevice::GetQueuePool( ) {
    return pQueuePool.get( );
}

const apemodevk::QueuePool* apemodevk::GraphicsDevice::GetQueuePool( ) const {
    return pQueuePool.get( );
}

apemodevk::CommandBufferPool * apemodevk::GraphicsDevice::GetCommandBufferPool()
{
    return pCmdBufferPool.get( );
}

const apemodevk::CommandBufferPool* apemodevk::GraphicsDevice::GetCommandBufferPool( ) const {
    return pCmdBufferPool.get( );
}

apemodevk::GraphicsManager& apemodevk::GraphicsDevice::GetGraphicsManager( ) {
    return *pManager;
}

const apemodevk::GraphicsManager& apemodevk::GraphicsDevice::GetGraphicsManager( ) const {
    return *pManager;
}
