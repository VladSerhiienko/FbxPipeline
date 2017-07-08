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
    uint32_t                LayerPropertyCount = 0;
    ResultHandle            ErrorHandle;
    std::vector<VkLayerProperties> GlobalLayers;

    do {
        ErrorHandle = vkEnumerateInstanceLayerProperties( &LayerPropertyCount, NULL );
        apemode_assert( ErrorHandle, "vkEnumerateInstanceLayerProperties failed." );

        if ( ErrorHandle.Succeeded( ) && LayerPropertyCount ) {
            GlobalLayers.resize( LayerPropertyCount );
            ErrorHandle = vkEnumerateInstanceLayerProperties( &LayerPropertyCount, GlobalLayers.data( ) );
        }

    } while ( ErrorHandle == ResultHandle::Incomplete );

    apemode_assert( ErrorHandle, "vkEnumerateInstanceLayerProperties failed." );
    LayerWrappers.reserve( LayerPropertyCount + 1 );

    auto ResolveLayerExtensions = [this]( VkLayerProperties const *Layer ) {
        ResultHandle ErrorHandle;
        uint32_t     LayerExtensionCount = 0;

        const bool  bIsUnnamed   = !Layer;
        auto &      LayerWrapper = apemodevk::PushBackAndGet( LayerWrappers );
        char const *LayerName    = bIsUnnamed ? LayerWrapper.Layer->layerName : nullptr;

        LayerWrapper.Layer      = !bIsUnnamed ? *Layer : VkLayerProperties( );
        LayerWrapper.bIsUnnamed = bIsUnnamed;

        do {
            ErrorHandle = vkEnumerateInstanceExtensionProperties( LayerName, &LayerExtensionCount, NULL );
            apemode_assert( ErrorHandle, "vkEnumerateInstanceExtensionProperties failed." );

            if ( ErrorHandle.Succeeded( ) && LayerExtensionCount ) {
                LayerWrapper.Extensions.resize( LayerExtensionCount );
                ErrorHandle =
                    vkEnumerateInstanceExtensionProperties( LayerName, &LayerExtensionCount, LayerWrapper.Extensions.data( ) );
            }

        } while ( ErrorHandle == ResultHandle::Incomplete );

        apemode_assert( ErrorHandle, "vkEnumerateInstanceExtensionProperties failed." );
        apemode_assert( LayerWrapper.IsValidInstanceLayer( ), "Layer is invalid" );
    };

    auto ResolveLayerExtensionsRef = [this, &ResolveLayerExtensions]( VkLayerProperties const &Layer ) {
        ResolveLayerExtensions( &Layer );
    };

    ResolveLayerExtensions( nullptr );
    std::for_each( GlobalLayers.begin( ), GlobalLayers.end( ), ResolveLayerExtensionsRef );

    PresentLayers.reserve( GlobalLayers.size( ) );

    auto       LayerWrapperIt    = LayerWrappers.begin( );
    auto const LayerWrapperEndIt = LayerWrappers.end( );
    std::advance( LayerWrapperIt, 1 );

    auto ResolveLayerName = [&]( NativeLayerWrapper const &LayerWrapper ) {

        if ( !strcmp( LayerWrapper.Layer->layerName, "VK_LAYER_LUNARG_vktrace" ) && ( flags & kEnableVkTraceLayer ) ) {
            PresentLayers.push_back( LayerWrapper.Layer->layerName );
        } else if ( !strcmp( LayerWrapper.Layer->layerName, "VK_LAYER_LUNARG_api_dump" ) && ( flags & kEnableVkApiDumpLayer ) ) {
            PresentLayers.push_back( LayerWrapper.Layer->layerName );
        } else if ( !strcmp( LayerWrapper.Layer->layerName, "VK_LAYER_RENDERDOC_Capture" ) && ( flags & kEnableRenderDocLayer ) ) {
            PresentLayers.push_back( LayerWrapper.Layer->layerName );
        } else if ( strcmp( LayerWrapper.Layer->layerName, "VK_LAYER_LUNARG_vktrace" ) &&
                    strcmp( LayerWrapper.Layer->layerName, "VK_LAYER_LUNARG_api_dump" ) &&
                    strcmp( LayerWrapper.Layer->layerName, "VK_LAYER_RENDERDOC_Capture" ) && ( flags & kEnableLayers ) ) {
            PresentLayers.push_back( LayerWrapper.Layer->layerName );
        } else {
        }
    };

    std::for_each( LayerWrapperIt, LayerWrapperEndIt, ResolveLayerName );

    PresentExtensions.reserve( GetUnnamedLayer( ).Extensions.size( ) );

    auto const ExtensionIt    = GetUnnamedLayer( ).Extensions.begin( );
    auto const ExtensionEndIt = GetUnnamedLayer( ).Extensions.end( );
    auto ResolveExtensionName = [&]( VkExtensionProperties const &Extension ) { return Extension.extensionName; };
    std::transform( ExtensionIt, ExtensionEndIt, std::back_inserter( PresentExtensions ), ResolveExtensionName );

    return ErrorHandle.Succeeded( );
}

bool apemodevk::GraphicsManager::ScanAdapters( ) {
    uint32_t               AdaptersFound = 0;
    std::vector<VkPhysicalDevice> Adapters;

    ResultHandle ErrorHandle;
    ErrorHandle = vkEnumeratePhysicalDevices( hInstance, &AdaptersFound, NULL );
    apemode_assert( ErrorHandle, "vkEnumeratePhysicalDevices failed." );

    Adapters.resize( AdaptersFound );
    ErrorHandle = vkEnumeratePhysicalDevices( hInstance, &AdaptersFound, Adapters.data( ) );
    apemode_assert( ErrorHandle, "vkEnumeratePhysicalDevices failed." );

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
            CurrentNode->RecreateResourcesFor( Adapter, *this );
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
    InstanceDesc->enabledLayerCount       = _Get_collection_length_u( PresentLayers );
    InstanceDesc->ppEnabledLayerNames     = PresentLayers.data( );
    InstanceDesc->enabledExtensionCount   = _Get_collection_length_u( PresentExtensions );
    InstanceDesc->ppEnabledExtensionNames = PresentExtensions.data( );

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

    if ( !ScanAdapters( ) )
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
