#pragma once

struct apemode::GraphicsEcosystem::PrivateContent
    : public apemode::ScalableAllocPolicy
    , public apemode::NoCopyAssignPolicy
{
    struct APIVersion
        : public apemode::ScalableAllocPolicy
    {
        uint32_t Major, Minor, Patch;
        APIVersion(bool bDump = true);
    };

    struct NativeLayerWrapper
        : public apemode::ScalableAllocPolicy
    {
        typedef TInfoStruct<VkExtensionProperties>::Vector VkExtensionPropertiesVector;

        bool                           bIsUnnamed;
        TInfoStruct<VkLayerProperties> Layer;
        VkExtensionPropertiesVector    Extensions;

        bool IsUnnamedLayer() const;
        bool IsValidInstanceLayer() const;
        bool IsValidDeviceLayer() const;
        void DumpExtensions() const;
    };

    typedef GraphicsEcosystem GraphicsEcosystem;

    typedef std::vector<std::string>       String8Vector;
    typedef std::vector<NativeLayerWrapper> NativeLayerWrapperVector;
    typedef TInfoStruct<VkLayerProperties>::Vector VkLayerPropertiesVector;
    typedef TInfoStruct<VkPhysicalDevice>::Vector  VkPhysicalDeviceVector;
    typedef std::vector<const char *> LpstrVector;

    APIVersion               Version;
    std::string             AppName;
    std::string             EngineName;
    LpstrVector              PresentLayers;
    LpstrVector              PresentExtensions;
    TDispatchableHandle<VkInstance>     InstanceHandle;
    NativeLayerWrapperVector LayerWrappers;

    bool                 ScanInstanceLayerProperties();
    bool                 ScanAdapters(GraphicsEcosystem &);
    NativeLayerWrapper & GetUnnamedLayer();
    bool                 InitializeInstance(GraphicsEcosystem &);

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkFlags                    msgFlags,
                                                        VkDebugReportObjectTypeEXT objType,
                                                        uint64_t                   srcObject,
                                                        size_t                     location,
                                                        int32_t                    msgCode,
                                                        const char *               pLayerPrefix,
                                                        const char *               pMsg,
                                                        void *                     pUserData);

    static VKAPI_ATTR VkBool32 VKAPI_CALL BreakCallback(VkFlags                    msgFlags,
                                                        VkDebugReportObjectTypeEXT objType,
                                                        uint64_t                   srcObject,
                                                        size_t                     location,
                                                        int32_t                    msgCode,
                                                        const char *               pLayerPrefix,
                                                        const char *               pMsg,
                                                        void *                     pUserData);

    PrivateContent();
};