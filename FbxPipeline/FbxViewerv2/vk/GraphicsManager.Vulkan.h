#pragma once

#include <NativeDispatchableHandles.Vulkan.h>
#include <TInfoStruct.Vulkan.h>

namespace apemodevk {
    class GraphicsDevice;
    class GraphicsManager : public apemodevk::ScalableAllocPolicy, public apemodevk::NoCopyAssignPolicy {
    public:
        friend GraphicsDevice;

        static uint32_t const kEnableRenderDocLayer = 1; // Must be enabled for RenderDoc tool.
        static uint32_t const kEnableVkTraceLayer   = 2; // Must be enable for vktrace tool.
        static uint32_t const kEnableVkApiDumpLayer = 4; // Traces every call to stdout.
        static uint32_t const kEnableLayers         = 8; // Enables all the layers (except vktrace, api_dump, renderdoc).

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

        bool RecreateGraphicsNodes( uint32_t flags = 0 );
        GraphicsDevice *GetPrimaryGraphicsNode( );
        GraphicsDevice *GetSecondaryGraphicsNode( );
        bool ScanInstanceLayerProperties( uint32_t flags );
        bool ScanAdapters( );
        NativeLayerWrapper &GetUnnamedLayer( );
        bool InitializeInstance( uint32_t flags );

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

        std::unique_ptr< GraphicsDevice > PrimaryNode;
        std::unique_ptr< GraphicsDevice > SecondaryNode;
        APIVersion                        Version;
        std::string                       AppName;
        std::string                       EngineName;
        std::vector< const char * >       PresentLayers;
        std::vector< const char * >       PresentExtensions;
        TDispatchableHandle< VkInstance > hInstance;
        std::vector< NativeLayerWrapper > LayerWrappers;
    };
}
