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

apemodevk::GraphicsDevice::NativeLayerWrapper& apemodevk::GraphicsDevice::GetUnnamedLayer( ) {
    apemode_assert( LayerWrappers.front( ).IsUnnamedLayer( ), "vkEnumerateInstanceExtensionProperties failed." );
    return LayerWrappers.front( );
}

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

        uint32_t TotalQueuePrioritiesCount = 0;
        std::for_each( QueueProps.begin( ), QueueProps.end( ), [&]( VkQueueFamilyProperties& QueueProp ) {
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

bool apemodevk::GraphicsDevice::ScanDeviceLayerProperties( ) {
    uint32_t ExtPropCount = 0;
    ResultHandle ErrorHandle = vkEnumerateDeviceExtensionProperties( pPhysicalDevice, NULL, &ExtPropCount, NULL );
    if ( ErrorHandle && ExtPropCount ) {
        DeviceExtensionProps.resize( ExtPropCount );
        ErrorHandle = vkEnumerateDeviceExtensionProperties( pPhysicalDevice, NULL, &ExtPropCount, DeviceExtensionProps.data( ) );

        return ErrorHandle;
    }

    return false;
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

bool apemodevk::GraphicsDevice::RecreateResourcesFor( VkPhysicalDevice InAdapterHandle, GraphicsManager& GraphicsEcosystem ) {
    pGraphicsEcosystem = &GraphicsEcosystem;
    apemode_assert( Args != nullptr, "Ecosystem is required in case of Vulkan." );

    pPhysicalDevice = InAdapterHandle;

    // Physical device is required to create a related logical device.
    // Likewise, vulkan instance is required for physical device.
    if ( VK_NULL_HANDLE != pPhysicalDevice ) {
        vkGetPhysicalDeviceProperties( pPhysicalDevice, &AdapterProps );
        vkGetPhysicalDeviceMemoryProperties( pPhysicalDevice, &MemoryProps );
        vkGetPhysicalDeviceFeatures( pPhysicalDevice, &Features );

        VkQueueFamilyPropertiesVector QueueProps;
        VkDeviceQueueCreateInfoVector QueueReqs;
        FloatVector                   QueuePriorities;

        if ( ScanDeviceQueues( QueueProps, QueueReqs, QueuePriorities ) && ScanFormatProperties( ) && ScanDeviceLayerProperties( ) ) {
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
            DeviceDesc->enabledLayerCount       = _Get_collection_length_u( PresentLayers );
            DeviceDesc->ppEnabledLayerNames     = PresentLayers.data( );
            DeviceDesc->enabledExtensionCount   = _Get_collection_length_u( DeviceExtNames );
            DeviceDesc->ppEnabledExtensionNames = DeviceExtNames.data( );

            const auto bOk = hLogicalDevice.Recreate( pPhysicalDevice, DeviceDesc );
            apemode_assert( bOk, "vkCreateDevice failed." );

            pQueuePool.reset( new QueuePool( hLogicalDevice,
                                             pPhysicalDevice,
                                             QueueProps.data( ),
                                             QueueProps.data( ) + QueueProps.size( ),
                                             QueuePriorities.data( ),
                                             QueuePriorities.data( ) + QueuePriorities.size( ) ) );

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

apemodevk::GraphicsDevice::GraphicsDevice( ) : pGraphicsEcosystem( nullptr ) {
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
    return pGraphicsEcosystem->hInstance;
}

bool apemodevk::GraphicsDevice::IsValid( ) const {
    return hLogicalDevice.IsNotNull( );
}

bool apemodevk::GraphicsDevice::Await( ) {
    apemode_assert( IsValid( ), "Must be valid." );

    ResultHandle eDeviceWaitIdleOk = vkDeviceWaitIdle( *this );
    apemode_assert( eDeviceWaitIdleOk.Succeeded( ), "Failed to wait for device." );

    return eDeviceWaitIdleOk.Succeeded( );
}

apemodevk::QueuePool* apemodevk::GraphicsDevice::GetQueuePool( ) {
    return pQueuePool.get( );
}

const apemodevk::QueuePool* apemodevk::GraphicsDevice::GetQueuePool( ) const {
    return pQueuePool.get( );
}

apemodevk::GraphicsManager& apemodevk::GraphicsDevice::GetGraphicsEcosystem( ) {
    return *pGraphicsEcosystem;
}