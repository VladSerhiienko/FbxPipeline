//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <GraphicsDevice.Vulkan.h>
#include <GraphicsManager.Vulkan.h>

#include <NativeDispatchableHandles.Vulkan.h>
#include <SystemAllocationCallbacks.Vulkan.h>
#include <TInfoStruct.Vulkan.h>

#include <GraphicsManager.KnownExtensions.Vulkan.h>
#include <GraphicsManager.KnownLayers.Vulkan.h>

/// -------------------------------------------------------------------------------------------------------------------
/// GraphicsEcosystem PrivateContent
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::GraphicsManager::APIVersion::APIVersion( bool bDump )
    : Major( VK_API_VERSION_1_0 >> 22 ), Minor( ( VK_API_VERSION_1_0 >> 12 ) & 0x3ff ), Patch( VK_API_VERSION_1_0 & 0xfff ) {
}

bool apemodevk::GraphicsManager::ScanInstanceLayerProperties( uint32_t flags ) {
    struct SpecialLayerOrExtension {
        uint32_t    eFlag;
        const char* pName;
    };

    InstanceLayers.clear( );
    InstanceLayerProps.clear( );
    InstanceExtensions.clear( );
    InstanceExtensionProps.clear( );

    uint32_t PropCount = 0;
    uint32_t LayerCount = 0;

    switch ( CheckedCall( vkEnumerateInstanceLayerProperties( &LayerCount, NULL ) ) ) {
        case VK_SUCCESS:
            InstanceLayerProps.resize( LayerCount );
            CheckedCall( vkEnumerateInstanceLayerProperties( &LayerCount, InstanceLayerProps.data( ) ) );
            break;

        default:
            return false;
    }

    InstanceLayers.reserve( LayerCount + 1 );
    InstanceLayers.push_back( nullptr );
    std::transform( InstanceLayerProps.begin( ),
                    InstanceLayerProps.end( ),
                    std::back_inserter( InstanceLayers ),
                    [&]( VkLayerProperties const& ExtProp ) { return ExtProp.layerName; } );

    for ( SpecialLayerOrExtension specialLayer : {
              SpecialLayerOrExtension{kEnable_LUNARG_vktrace, "VK_LAYER_LUNARG_vktrace"},
              SpecialLayerOrExtension{kEnable_LUNARG_api_dump, "VK_LAYER_LUNARG_api_dump"},
              SpecialLayerOrExtension{kEnable_RENDERDOC_Capture, "VK_LAYER_RENDERDOC_Capture"},
              /*SpecialLayerOrExtension{kEnable_RENDERDOC_Capture, "VK_LAYER_LUNARG_core_validation"},
              SpecialLayerOrExtension{kEnable_RENDERDOC_Capture, "VK_LAYER_LUNARG_monitor"},
              SpecialLayerOrExtension{kEnable_RENDERDOC_Capture, "VK_LAYER_LUNARG_object_tracker"},
              SpecialLayerOrExtension{kEnable_RENDERDOC_Capture, "VK_LAYER_LUNARG_parameter_validation"},
              SpecialLayerOrExtension{kEnable_RENDERDOC_Capture, "VK_LAYER_LUNARG_screenshot"},
              SpecialLayerOrExtension{kEnable_RENDERDOC_Capture, "VK_LAYER_LUNARG_standard_validation"},*/
              SpecialLayerOrExtension{8, "VK_LAYER_GOOGLE_threading"},
              SpecialLayerOrExtension{8, "VK_LAYER_GOOGLE_unique_objects"},
              SpecialLayerOrExtension{8, "VK_LAYER_NV_optimus"},
              SpecialLayerOrExtension{8, "VK_LAYER_NV_nsight"},
          } ) {
        if ( false == HasFlagEq( flags, specialLayer.eFlag ) ) {
            auto specialLayerIt = std::find_if( ++InstanceLayers.begin( ), InstanceLayers.end( ), [&]( const char* pName ) {
                return 0 == strcmp( pName, specialLayer.pName );
            } );

            if ( specialLayerIt != InstanceLayers.end( ) )
                InstanceLayers.erase( specialLayerIt );
        }
    }

    for ( auto pInstanceLayer : InstanceLayers ) {
        switch ( CheckedCall( vkEnumerateInstanceExtensionProperties( pInstanceLayer, &PropCount, NULL ) ) ) {
            case VK_SUCCESS: {
                uint32_t FirstInstanceExtension = InstanceExtensionProps.size( );
                InstanceExtensionProps.resize( InstanceExtensionProps.size( ) + PropCount );
                CheckedCall( vkEnumerateInstanceExtensionProperties( pInstanceLayer, &PropCount, InstanceExtensionProps.data( ) + FirstInstanceExtension ) );
            } break;

            default:
                return false;
        }
    }

    std::sort( InstanceExtensionProps.begin( ),
               InstanceExtensionProps.end( ),
               [&]( VkExtensionProperties const& a, VkExtensionProperties const& b ) {
                   return 0 > strcmp( a.extensionName, b.extensionName );
               } );
    InstanceExtensionProps.erase( std::unique( InstanceExtensionProps.begin( ),
                                               InstanceExtensionProps.end( ),
                                               [&]( VkExtensionProperties const& a, VkExtensionProperties const& b ) {
                                                   return 0 == strcmp( a.extensionName, b.extensionName );
                                               } ),
                                  InstanceExtensionProps.end( ) );

    InstanceLayers.erase( InstanceLayers.begin( ) );

    platform::DebugTrace( "Instance Layers (%u):", InstanceLayers.size( ) );
    std::for_each( InstanceLayers.begin( ), InstanceLayers.end( ), [&]( const char* pName ) {
        platform::DebugTrace( "> %s", pName );
    } );

    InstanceExtensions.reserve( InstanceExtensionProps.size( ) );
    std::transform( InstanceExtensionProps.begin( ),
                    InstanceExtensionProps.end( ),
                    std::back_inserter( InstanceExtensions ),
                    [&]( VkExtensionProperties const& ExtProp ) { return ExtProp.extensionName; } );

    /*
        APEMODE VK: > VK_EXT_debug_report
        APEMODE VK: > VK_EXT_display_surface_counter
        APEMODE VK: > VK_KHR_get_physical_device_properties2
        APEMODE VK: > VK_KHR_get_surface_capabilities2
        APEMODE VK: > VK_KHR_surface
        APEMODE VK: > VK_KHR_win32_surface
        APEMODE VK: > VK_KHX_device_group_creation
        APEMODE VK: > VK_NV_external_memory_capabilities
    */

    for ( SpecialLayerOrExtension specialLayer : {SpecialLayerOrExtension{8, "VK_NV_external_memory_capabilities"},
                                                  SpecialLayerOrExtension{8, "VK_KHX_device_group_creation"},
                                                  //SpecialLayerOrExtension{8, "VK_KHR_win32_surface"},
                                                  //SpecialLayerOrExtension{8, "VK_KHR_surface"},
                                                  SpecialLayerOrExtension{8, "VK_KHR_get_surface_capabilities2"},
                                                  SpecialLayerOrExtension{8, "VK_KHR_get_physical_device_properties2"},
                                                  SpecialLayerOrExtension{8, "VK_EXT_display_surface_counter"}} ) {
        if ( false == HasFlagEq( flags, specialLayer.eFlag ) ) {
            auto specialLayerIt = std::find_if( InstanceExtensions.begin( ),
                                                InstanceExtensions.end( ),
                                                [&]( const char* pName ) { return 0 == strcmp( pName, specialLayer.pName ); } );

            if ( specialLayerIt != InstanceExtensions.end( ) )
                InstanceExtensions.erase( specialLayerIt );
        }
    }

    platform::DebugTrace( "Instance Extensions (%u):", InstanceExtensions.size( ) );
    std::for_each( InstanceExtensions.begin( ), InstanceExtensions.end( ), [&]( const char* pName ) {
        platform::DebugTrace( "> %s", pName );
    } );

    return true;
}

bool apemodevk::GraphicsManager::ScanAdapters( uint32_t flags) {
    std::vector<VkPhysicalDevice> Adapters;

    uint32_t AdaptersFound = 0;
    CheckedCall( vkEnumeratePhysicalDevices( hInstance, &AdaptersFound, NULL ) );

    Adapters.resize( AdaptersFound );
    CheckedCall( vkEnumeratePhysicalDevices( hInstance, &AdaptersFound, Adapters.data( ) ) );

    // TODO:
    //      Choose the best 2 nodes here.
    //      Ensure the integrate GPU is always the secondary one.

    std::for_each( Adapters.begin( ), Adapters.end( ), [&]( VkPhysicalDevice const &Adapter ) {
        GraphicsDevice *CurrentNode = nullptr;

        if ( GetPrimaryGraphicsNode( ) == nullptr ) {
            PrimaryNode.swap( GraphicsDevice::MakeNewUnique( ) );
            CurrentNode = GetPrimaryGraphicsNode( );
        } else if ( GetSecondaryGraphicsNode( ) == nullptr ) {
            SecondaryNode.swap( GraphicsDevice::MakeNewUnique( ) );
            CurrentNode = GetSecondaryGraphicsNode( );
        }

        if ( CurrentNode != nullptr ) {
            CurrentNode->RecreateResourcesFor( Adapter, *this, flags );
        }
    } );

    return AdaptersFound != 0;
}

apemodevk::GraphicsManager::NativeLayerWrapper &apemodevk::GraphicsManager::GetUnnamedLayer( ) {
    apemode_assert( LayerWrappers.front( ).IsUnnamedLayer( ), "vkEnumerateInstanceExtensionProperties failed." );
    return LayerWrappers.front( );
}

bool apemodevk::GraphicsManager::InitializeInstance( uint32_t flags ) {
    if ( !ScanInstanceLayerProperties( flags ) )
        return false;

    TInfoStruct< VkApplicationInfo > AppDesc;
    AppDesc->apiVersion         = VK_API_VERSION_1_0;
    AppDesc->pApplicationName   = AppName.c_str( );
    AppDesc->pEngineName        = EngineName.c_str( );
    AppDesc->applicationVersion = 1;
    AppDesc->engineVersion      = 1;
    AppDesc->pNext              = VK_NULL_HANDLE;

    // clang-format off
    auto DebugFlags 
        = VK_DEBUG_REPORT_ERROR_BIT_EXT
        | VK_DEBUG_REPORT_DEBUG_BIT_EXT
        | VK_DEBUG_REPORT_WARNING_BIT_EXT
        | VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
    // clang-format on

    TInfoStruct< VkDebugReportCallbackCreateInfoEXT > DebugDesc;
    DebugDesc->pfnCallback = DebugCallback;
    DebugDesc->pUserData   = this;
    DebugDesc->sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    DebugDesc->flags       = DebugFlags;

    TInfoStruct< VkInstanceCreateInfo > InstanceDesc;
    InstanceDesc->pNext                   = DebugDesc;
    InstanceDesc->pApplicationInfo        = AppDesc;
    InstanceDesc->enabledLayerCount       = _Get_collection_length_u( InstanceLayers );
    InstanceDesc->ppEnabledLayerNames     = InstanceLayers.data( );
    InstanceDesc->enabledExtensionCount   = _Get_collection_length_u( InstanceExtensions );
    InstanceDesc->ppEnabledExtensionNames = InstanceExtensions.data( );

    /*
    Those layers cannot be used as standalone layers (please, see vktrace, renderdoc docs)
    -----------------------------------------------------------------
        Layer VK_LAYER_LUNARG_vktrace (impl 0x00000001, spec 0x00400003):
                Extension VK_KHR_surface (spec 0x00000019)
                Extension VK_KHR_win32_surface (spec 0x00000005)
                Extension VK_EXT_debug_report (spec 0x00000001)
    -----------------------------------------------------------------
        Layer VK_LAYER_RENDERDOC_Capture (impl 0x0000001a, spec 0x000d2001):
                Extension VK_KHR_surface (spec 0x00000019)
                Extension VK_KHR_win32_surface (spec 0x00000005)
                Extension VK_EXT_debug_report (spec 0x00000001)
    -----------------------------------------------------------------
    */

    bool bIsOk = hInstance.Recreate( InstanceDesc );
    apemode_assert( bIsOk, "vkCreateInstance failed." );

    if ( !ScanAdapters( flags ) )
        return false;

    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL apemodevk::GraphicsManager::BreakCallback( VkFlags                    msgFlags,
                                                                          VkDebugReportObjectTypeEXT objType,
                                                                          uint64_t                   srcObject,
                                                                          size_t                     location,
                                                                          int32_t                    msgCode,
                                                                          const char *               pLayerPrefix,
                                                                          const char *               pMsg,
                                                                          void *                     pUserData ) {
    if ( msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT ) {
        platform::DebugBreak( );
    }

    /*
    * false indicates that layer should not bail-out of an
    * API call that had validation failures. This may mean that the
    * app dies inside the driver due to invalid parameter(s).
    * That's what would happen without validation layers, so we'll
    * keep that behavior here.
    */

    return false;
}

VKAPI_ATTR VkBool32 VKAPI_CALL apemodevk::GraphicsManager::DebugCallback( VkFlags                    msgFlags,
                                                                          VkDebugReportObjectTypeEXT objType,
                                                                          uint64_t                   srcObject,
                                                                          size_t                     location,
                                                                          int32_t                    msgCode,
                                                                          const char *               pLayerPrefix,
                                                                          const char *               pMsg,
                                                                          void *                     pUserData ) {
    if ( msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT ) {

        if ( nullptr == strstr( pMsg, "Nvda.Graphics.Interception" ) )
            platform::DebugBreak( );

    } else if ( msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT ) {
        platform::DebugBreak( );
    } else if ( msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT ) {
    } else if ( msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT ) {
    }

    /* False indicates that layer should not bail-out of an
     * API call that had validation failures. This may mean that the
     * app dies inside the driver due to invalid parameter(s).
     * That's what would happen without validation layers, so we'll
     * keep that behavior here.
     */

    return false;
}

bool apemodevk::GraphicsManager::NativeLayerWrapper::IsUnnamedLayer( ) const {
    return bIsUnnamed;
}

bool apemodevk::GraphicsManager::NativeLayerWrapper::IsValidInstanceLayer( ) const {
    if ( IsUnnamedLayer( ) ) {
        auto const KnownExtensionBeginIt = Vulkan::KnownInstanceExtensions;
        auto const KnownExtensionEndIt   = Vulkan::KnownInstanceExtensions + Vulkan::KnownInstanceExtensionCount;
        size_t     KnownExtensionsFound =
            std::count_if( KnownExtensionBeginIt, KnownExtensionEndIt, [this]( char const *KnownExtension ) {
                auto const ExtensionBeginIt = Extensions.begin( );
                auto const ExtensionEndIt   = Extensions.end( );
                auto const FoundExtensionIt =
                    std::find_if( ExtensionBeginIt, ExtensionEndIt, [&]( VkExtensionProperties const &Extension ) {
                        static const errno_t eStrCmp_EqualStrings = 0;
                        return strcmp( KnownExtension, Extension.extensionName ) == eStrCmp_EqualStrings;
                    } );

                auto const bIsExtensionFound = FoundExtensionIt != ExtensionEndIt;
                apemode_assert( bIsExtensionFound, "Extension '%s' was not found.", KnownExtension );

                return bIsExtensionFound;
            } );

        return KnownExtensionsFound == Vulkan::KnownInstanceExtensionCount;
    }

    return true;
}

/// -------------------------------------------------------------------------------------------------------------------
/// GraphicsEcosystem
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::GraphicsManager::GraphicsManager( ) {
}

apemodevk::GraphicsManager::~GraphicsManager( ) {
}

apemodevk::GraphicsDevice *apemodevk::GraphicsManager::GetPrimaryGraphicsNode( ) {
    return PrimaryNode.get( );
}

apemodevk::GraphicsDevice *apemodevk::GraphicsManager::GetSecondaryGraphicsNode( ) {
    return SecondaryNode.get( );
}

bool apemodevk::GraphicsManager::RecreateGraphicsNodes( uint32_t flags ) {
    auto const bIsInstInitialized = InitializeInstance( flags );
    apemode_assert( bIsInstInitialized, "Vulkan Instance initialization failed." );
    return bIsInstInitialized;
}
