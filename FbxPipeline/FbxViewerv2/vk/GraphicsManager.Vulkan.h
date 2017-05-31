#pragma once

#include <NativeDispatchableHandles.Vulkan.h>
#include <ResultHandle.Vulkan.h>
#include <TInfoStruct.Vulkan.h>

namespace apemodevk {
    class GraphicsDevice;
    class GraphicsManager : public apemodevk::ScalableAllocPolicy, public apemodevk::NoCopyAssignPolicy {
    public:
        friend GraphicsDevice;

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
            void DumpExtensions( ) const;
        };

        typedef std::vector< std::string >               String8Vector;
        typedef std::vector< NativeLayerWrapper >        NativeLayerWrapperVector;
        typedef TInfoStruct< VkLayerProperties >::Vector VkLayerPropertiesVector;
        typedef TInfoStruct< VkPhysicalDevice >::Vector  VkPhysicalDeviceVector;
        typedef std::vector< const char * >              LpstrVector;

        GraphicsManager( );
        ~GraphicsManager( );

        bool                RecreateGraphicsNodes( );
        GraphicsDevice *    GetPrimaryGraphicsNode( );
        GraphicsDevice *    GetSecondaryGraphicsNode( );
        bool                ScanInstanceLayerProperties( );
        bool                ScanAdapters( );
        NativeLayerWrapper &GetUnnamedLayer( );
        bool                InitializeInstance( );

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
        LpstrVector                       PresentLayers;
        LpstrVector                       PresentExtensions;
        TDispatchableHandle< VkInstance > InstanceHandle;
        NativeLayerWrapperVector          LayerWrappers;
    };
}
