#pragma once

#include <NativeDispatchableHandles.Vulkan.h>
#include <ResultHandle.Vulkan.h>
#include <TInfoStruct.Vulkan.h>

namespace apemode {
    class GraphicsDevice;
    class GraphicsManager : public apemode::ScalableAllocPolicy, public apemode::NoCopyAssignPolicy {
    public:
        struct PrivateContent;
        struct PrivateCreateDeviceArgs;

        // TODO:
        //      Add flags: allow multiple devices, prefer discrete GPUs, etc.
        enum EFlags { kFlag_None };

    public:
        GraphicsManager( );
        ~GraphicsManager( );

    public:
        inline GraphicsDevice *GetPrimaryGraphicsNode( ) {
            return PrimaryNode.get( );
        }
        inline GraphicsDevice *GetSecondaryGraphicsNode( ) {
            return SecondaryNode.get( );
        }
        inline GraphicsDevice &GetPrimaryGraphicsNodeByRef( ) {
            return *PrimaryNode;
        }
        inline GraphicsDevice &GetSecondaryGraphicsNodeByRef( ) {
            return *SecondaryNode;
        }

    public:
        bool RecreateGraphicsNodes( EFlags Flags = kFlag_None );

        struct APIVersion : public apemode::ScalableAllocPolicy {
            uint32_t Major, Minor, Patch;
            APIVersion( bool bDump = true );
        };

        struct NativeLayerWrapper : public apemode::ScalableAllocPolicy {
            typedef TInfoStruct< VkExtensionProperties >::Vector VkExtensionPropertiesVector;

            bool                             bIsUnnamed;
            TInfoStruct< VkLayerProperties > Layer;
            VkExtensionPropertiesVector      Extensions;

            bool IsUnnamedLayer( ) const;
            bool IsValidInstanceLayer( ) const;
            bool IsValidDeviceLayer( ) const;
            void DumpExtensions( ) const;
        };

        typedef GraphicsManager GraphicsEcosystem;

        typedef std::vector< std::string >               String8Vector;
        typedef std::vector< NativeLayerWrapper >        NativeLayerWrapperVector;
        typedef TInfoStruct< VkLayerProperties >::Vector VkLayerPropertiesVector;
        typedef TInfoStruct< VkPhysicalDevice >::Vector  VkPhysicalDeviceVector;
        typedef std::vector< const char * >              LpstrVector;

        APIVersion                        Version;
        std::string                       AppName;
        std::string                       EngineName;
        LpstrVector                       PresentLayers;
        LpstrVector                       PresentExtensions;
        TDispatchableHandle< VkInstance > InstanceHandle;
        NativeLayerWrapperVector          LayerWrappers;

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

    private:
        typedef apemode::TSafeDeleteObjOp< PrivateContent > PrivateContentDeleter;
        typedef std::unique_ptr< PrivateContent, PrivateContentDeleter > PrivateContentUqPtr;

        friend GraphicsDevice;
        friend PrivateContent;

        // NOTE:
        //      The destruction order is important here.
        //      Devices should be deleted before the instance is.

        std::unique_ptr< GraphicsDevice > PrimaryNode;
        std::unique_ptr< GraphicsDevice > SecondaryNode;
    };
}

_Game_engine_Define_enum_flag_operators( apemode::GraphicsManager::EFlags );
