#pragma once

#ifdef _Define_resolve_struct_type
#undef _Define_resolve_struct_type
#endif

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
_Define_resolve_struct_type(VkPipelineLayoutCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
_Define_resolve_struct_type(VkApplicationInfo, VK_STRUCTURE_TYPE_APPLICATION_INFO);
_Define_resolve_struct_type(VkInstanceCreateInfo, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
_Define_resolve_struct_type(VkDebugReportCallbackCreateInfoEXT, VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT);
_Define_resolve_struct_type(VkDeviceQueueCreateInfo, VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO);
_Define_resolve_struct_type(VkDeviceCreateInfo, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
_Define_resolve_struct_type(VkWin32SurfaceCreateInfoKHR, VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR);
_Define_resolve_struct_type(VkCommandBufferInheritanceInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO);
_Define_resolve_struct_type(VkMappedMemoryRange, VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE);

#undef _Define_resolve_struct_type
