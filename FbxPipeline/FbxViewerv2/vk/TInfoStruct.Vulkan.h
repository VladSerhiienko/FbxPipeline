#pragma once

namespace Core
{
    namespace Traits
    {
        struct HasInfoStructureTypeResolver
        {
            template <typename TVulkanStructure>
            struct ResolveFor
            {
                static const bool bHas = false;
            };
#pragma region
#define _Define_has_struct_type(T)                                                                 \
    template <>                                                                                    \
    struct ResolveFor<T>                                                                           \
    {                                                                                              \
        static const bool bHas = true;                                                             \
    }

            _Define_has_struct_type(VkCommandBufferBeginInfo);
            _Define_has_struct_type(VkRenderPassBeginInfo);
            _Define_has_struct_type(VkImageMemoryBarrier);
            _Define_has_struct_type(VkFenceCreateInfo);
            _Define_has_struct_type(VkSemaphoreCreateInfo);
            _Define_has_struct_type(VkSubmitInfo);
            _Define_has_struct_type(VkPresentInfoKHR);
            _Define_has_struct_type(VkSwapchainCreateInfoKHR);
            _Define_has_struct_type(VkImageViewCreateInfo);
            _Define_has_struct_type(VkImageCreateInfo);
            _Define_has_struct_type(VkMemoryAllocateInfo);
            _Define_has_struct_type(VkSamplerCreateInfo);
            _Define_has_struct_type(VkBufferCreateInfo);
            _Define_has_struct_type(VkDescriptorSetLayoutCreateInfo);
            _Define_has_struct_type(VkRenderPassCreateInfo);
            _Define_has_struct_type(VkShaderModuleCreateInfo);
            _Define_has_struct_type(VkPipelineDynamicStateCreateInfo);
            _Define_has_struct_type(VkGraphicsPipelineCreateInfo);
            _Define_has_struct_type(VkPipelineVertexInputStateCreateInfo);
            _Define_has_struct_type(VkPipelineInputAssemblyStateCreateInfo);
            _Define_has_struct_type(VkPipelineRasterizationStateCreateInfo);
            _Define_has_struct_type(VkPipelineColorBlendStateCreateInfo);
            _Define_has_struct_type(VkPipelineDepthStencilStateCreateInfo);
            _Define_has_struct_type(VkPipelineViewportStateCreateInfo);
            _Define_has_struct_type(VkPipelineMultisampleStateCreateInfo);
            _Define_has_struct_type(VkPipelineShaderStageCreateInfo);
            _Define_has_struct_type(VkPipelineCacheCreateInfo);
            _Define_has_struct_type(VkDescriptorPoolCreateInfo);
            _Define_has_struct_type(VkDescriptorSetAllocateInfo);
            _Define_has_struct_type(VkWriteDescriptorSet);
            _Define_has_struct_type(VkFramebufferCreateInfo);
            _Define_has_struct_type(VkCommandPoolCreateInfo);
            _Define_has_struct_type(VkCommandBufferAllocateInfo);
            _Define_has_struct_type(VkApplicationInfo);
            _Define_has_struct_type(VkInstanceCreateInfo);
            _Define_has_struct_type(VkDebugReportCallbackCreateInfoEXT);
            _Define_has_struct_type(VkDeviceQueueCreateInfo);
            _Define_has_struct_type(VkDeviceCreateInfo);
            _Define_has_struct_type(VkWin32SurfaceCreateInfoKHR);
            _Define_has_struct_type(VkCommandBufferInheritanceInfo);

#undef _Define_has_struct_type
#pragma endregion 
        };

        struct InfoStructureTypeResolver
        {
            static const VkStructureType UnresolvedType = VK_STRUCTURE_TYPE_MAX_ENUM;

            template <typename TVulkanStructure>
            static bool IsValid(TVulkanStructure const & Desc)
            {
                using HasTypeField = HasInfoStructureTypeResolver::ResolveFor<TVulkanStructure>;

                const auto sType = *reinterpret_cast<VkStructureType const *>(&Desc);
                const bool bMustHaveValidType = HasTypeField::bHas;
                const bool bValid = (!bMustHaveValidType) || (bMustHaveValidType && (sType != UnresolvedType));
                return bValid;
            }

            template <typename TVulkanStructure>
            struct ResolveFor
            {
                static const VkStructureType eType = UnresolvedType;
            };
#pragma region
#define _Define_resolve_struct_type(T, E)                                                          \
    template <>                                                                                    \
    struct ResolveFor<T>                                                                           \
    {                                                                                              \
        static const VkStructureType eType = E;                                                     \
    }

            _Define_resolve_struct_type(VkCommandBufferBeginInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
            _Define_resolve_struct_type(VkRenderPassBeginInfo, VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);
            _Define_resolve_struct_type(VkMemoryBarrier, VK_STRUCTURE_TYPE_MEMORY_BARRIER);
            _Define_resolve_struct_type(VkBufferMemoryBarrier, VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER);
            _Define_resolve_struct_type(VkImageMemoryBarrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
            _Define_resolve_struct_type(VkFenceCreateInfo, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
            _Define_resolve_struct_type(VkSemaphoreCreateInfo, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
            _Define_resolve_struct_type(VkSubmitInfo, VK_STRUCTURE_TYPE_SUBMIT_INFO);
            _Define_resolve_struct_type(VkPresentInfoKHR, VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
            _Define_resolve_struct_type(VkSwapchainCreateInfoKHR, VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);
            _Define_resolve_struct_type(VkImageViewCreateInfo, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
            _Define_resolve_struct_type(VkImageCreateInfo, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
            _Define_resolve_struct_type(VkMemoryAllocateInfo, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
            _Define_resolve_struct_type(VkSamplerCreateInfo, VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
            _Define_resolve_struct_type(VkBufferCreateInfo, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
            _Define_resolve_struct_type(VkDescriptorSetLayoutCreateInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
            _Define_resolve_struct_type(VkRenderPassCreateInfo, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
            _Define_resolve_struct_type(VkShaderModuleCreateInfo, VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
            _Define_resolve_struct_type(VkPipelineDynamicStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO);
            _Define_resolve_struct_type(VkGraphicsPipelineCreateInfo, VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);
            _Define_resolve_struct_type(VkPipelineCacheCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO);
            _Define_resolve_struct_type(VkPipelineVertexInputStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
            _Define_resolve_struct_type(VkPipelineInputAssemblyStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO);
            _Define_resolve_struct_type(VkPipelineRasterizationStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
            _Define_resolve_struct_type(VkPipelineColorBlendStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
            _Define_resolve_struct_type(VkPipelineDepthStencilStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO);
            _Define_resolve_struct_type(VkPipelineViewportStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO);
            _Define_resolve_struct_type(VkPipelineMultisampleStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
            _Define_resolve_struct_type(VkPipelineShaderStageCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
            _Define_resolve_struct_type(VkDescriptorPoolCreateInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);
            _Define_resolve_struct_type(VkDescriptorSetAllocateInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);
            _Define_resolve_struct_type(VkWriteDescriptorSet, VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
            _Define_resolve_struct_type(VkFramebufferCreateInfo, VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
            _Define_resolve_struct_type(VkCommandPoolCreateInfo, VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
            _Define_resolve_struct_type(VkCommandBufferAllocateInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
            _Define_resolve_struct_type(VkApplicationInfo, VK_STRUCTURE_TYPE_APPLICATION_INFO);
            _Define_resolve_struct_type(VkInstanceCreateInfo, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
            _Define_resolve_struct_type(VkDebugReportCallbackCreateInfoEXT, VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT);
            _Define_resolve_struct_type(VkDeviceQueueCreateInfo, VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO);
            _Define_resolve_struct_type(VkDeviceCreateInfo, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
            _Define_resolve_struct_type(VkWin32SurfaceCreateInfoKHR, VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR);
            _Define_resolve_struct_type(VkCommandBufferInheritanceInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO);

#undef _Define_resolve_struct_type
#pragma endregion 
        };
    }

    template <typename TVulkanNativeStruct>
    class TInfoStruct
    {
    private:
        static const bool HasStructType = Traits::HasInfoStructureTypeResolver::ResolveFor<TVulkanNativeStruct>::bHas;
        _Force_inline_function void Validate() const { _Game_engine_Assert(Traits::InfoStructureTypeResolver::IsValid(Desc), "Desc is corrupted."); }

        template <bool _HasStructType = HasStructType> void SetStructType();

    public:
        typedef TVulkanNativeStruct                VulkanNativeStructType;
        typedef TInfoStruct< TVulkanNativeStruct > SelfType;
        typedef std::vector< SelfType >            VectorImpl;

    public:
        struct Vector : public VectorImpl
        {
            static_assert(sizeof(SelfType) == sizeof(TVulkanNativeStruct), "Size mismatch.");
            _Force_inline_function TVulkanNativeStruct * data() { return reinterpret_cast<TVulkanNativeStruct *>(VectorImpl::data()); }
            _Force_inline_function TVulkanNativeStruct const * data() const { return reinterpret_cast<TVulkanNativeStruct const *>(VectorImpl::data()); }
        };

    public:
        TVulkanNativeStruct Desc;

        TInfoStruct() { ZeroMemory(); }
        TInfoStruct(SelfType && Other) : Desc(Other.Desc) {}
        TInfoStruct(SelfType const & Other) : Desc(Other.Desc) {}
        TInfoStruct(TVulkanNativeStruct const & OtherDesc) : Desc(OtherDesc) { SetStructType<>(); }

        void ZeroMemory() { Aux::ZeroMemory(Desc); SetStructType<>(); }

        SelfType & operator =(SelfType && Other) { Desc = Other.Desc; Validate(); return *this; }
        SelfType & operator =(TVulkanNativeStruct && OtherDesc) { Desc = OtherDesc; Validate(); return *this; }
        SelfType & operator =(SelfType const & Other) { Desc = Other.Desc; Validate(); return *this; }
        SelfType & operator =(TVulkanNativeStruct const & OtherDesc) { Desc = OtherDesc; Validate();  return *this; }

        _Force_inline_function TVulkanNativeStruct * operator->() { return &Desc; }
        _Force_inline_function TVulkanNativeStruct const * operator->() const { return &Desc; }
        _Force_inline_function operator TVulkanNativeStruct &() { Validate(); return Desc; }
        _Force_inline_function operator TVulkanNativeStruct *() { Validate(); return &Desc; }
        _Force_inline_function operator TVulkanNativeStruct const &() const { Validate(); return Desc; }
        _Force_inline_function operator TVulkanNativeStruct const *() const { Validate(); return &Desc; }

    private:
        template <>
        void SetStructType<true>()
        {
            _Game_engine_Assert(HasStructType, "Must be a structure with sType (VkStructureType) field.");
            auto sType = reinterpret_cast<VkStructureType *>(&Desc);
            *sType = Traits::InfoStructureTypeResolver::ResolveFor<TVulkanNativeStruct>::eType;
        }

        template <>
        void SetStructType<false>()
        {
        }
    };
}

namespace Aux
{
    template <typename T>
    inline uint32_t GetSizeU(T const & c) {
        return _Get_collection_length_u(c);
    }

    /** Aliasing cares only about size matching. */
    template <typename TVector, typename TNativeDesc>
    static void AliasStructs (TVector const &      Descs,
        TNativeDesc const *& pOutDescs,
        uint32_t &           OutDescCount)
    {
        using ValueType = TVector::value_type;
        static_assert (sizeof (ValueType) == sizeof (TNativeDesc), "Size mismatch, cannot alias.");

        pOutDescs    = reinterpret_cast<TNativeDesc const *> (Descs.data ());
        OutDescCount = GetSizeU (Descs);
    }

    /** Aliasing cares both about type and size matching.
     * @see AliasStructs for the lighter version of the function.
     */
    template <typename TInfoStructVector, typename TNativeDesc>
    static void AliasInfoStructs(TInfoStructVector const & Descs,
                                 TNativeDesc const *&      pOutAliasedDescsRef,
                                 uint32_t &                OutDescCount)
    {
        using InfoStructValueType = TInfoStructVector::value_type;
        using NativeDescValueType = InfoStructValueType::VulkanNativeStructType;
        static_assert(std::is_same<TNativeDesc, NativeDescValueType>::value,
                      "Types are not same, cannot alias.");

        AliasStructs(Descs, pOutAliasedDescsRef, OutDescCount);
    }
}
