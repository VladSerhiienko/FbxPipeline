#pragma once

#include <NativeDispatchableHandles.Vulkan.h>
#include <TInfoStruct.Vulkan.h>

namespace apemodevk {
    class GraphicsDevice;

    class GraphicsManager : public apemodevk::NoCopyAssignPolicy {
    public:
        friend GraphicsDevice;

        static uint32_t const kEnable_RENDERDOC_Capture = 1; // Must be enabled for RenderDoc tool.
        static uint32_t const kEnable_LUNARG_vktrace    = 2; // Must be enable for vktrace tool.
        static uint32_t const kEnable_LUNARG_api_dump   = 4; // Traces every call to stdout.

        struct APIVersion : public apemodevk::ScalableAllocPolicy {
            uint32_t Major, Minor, Patch;
            APIVersion( bool bDump = true );
        };

        struct NativeLayerWrapper : public apemodevk::ScalableAllocPolicy {
            typedef TInfoStruct< VkExtensionProperties >::Vector VkExtensionPropertiesVector;

            bool                             bIsUnnamed;
            TInfoStruct< VkLayerProperties > Layer;
            VkExtensionPropertiesVector      Extensions;

            bool IsUnnamedLayer( ) const;
            bool IsValidInstanceLayer( ) const;
            bool IsValidDeviceLayer( ) const;
        };

        GraphicsManager( );
        ~GraphicsManager( );

        bool                RecreateGraphicsNodes( uint32_t flags = 0 );
        GraphicsDevice *    GetPrimaryGraphicsNode( );
        GraphicsDevice *    GetSecondaryGraphicsNode( );
        bool                ScanInstanceLayerProperties( uint32_t flags );
        bool                ScanAdapters( uint32_t flags );
        NativeLayerWrapper &GetUnnamedLayer( );
        bool                InitializeInstance( uint32_t flags );

        static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback( VkFlags                    msgFlags,
                                                             VkDebugReportObjectTypeEXT objType,
                                                             uint64_t                   srcObject,
                                                             size_t                     location,
                                                             int32_t                    msgCode,
                                                             const char *               pLayerPrefix,
                                                             const char *               pMsg,
                                                             void *                     pUserData );

        static VKAPI_ATTR VkBool32 VKAPI_CALL BreakCallback( VkFlags                    msgFlags,
                                                             VkDebugReportObjectTypeEXT objType,
                                                             uint64_t                   srcObject,
                                                             size_t                     location,
                                                             int32_t                    msgCode,
                                                             const char *               pLayerPrefix,
                                                             const char *               pMsg,
                                                             void *                     pUserData );

        std::unique_ptr< GraphicsDevice >    PrimaryNode;
        std::unique_ptr< GraphicsDevice >    SecondaryNode;
        APIVersion                           Version;
        std::string                          AppName;
        std::string                          EngineName;
        std::vector< const char * >          InstanceLayers;
        std::vector< const char * >          InstanceExtensions;
        std::vector< VkLayerProperties >     InstanceLayerProps;
        std::vector< VkExtensionProperties > InstanceExtensionProps;
        TDispatchableHandle< VkInstance >    hInstance;
        std::vector< NativeLayerWrapper >    LayerWrappers;
    };
}
