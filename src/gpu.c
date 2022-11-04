#include "gpu.h"

#include "debug.h"
#include "heap.h"
#include "wm.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"

#include <malloc.h>
#include <string.h>

typedef struct gpu_cmd_buffer_t
{
	VkCommandBuffer buffer;
	VkPipelineLayout pipeline_layout;
	int index_count;
	int vertex_count;
} gpu_cmd_buffer_t;

typedef struct gpu_descriptor_t
{
	VkDescriptorSet set;
} gpu_descriptor_t;

typedef struct gpu_mesh_t
{
	VkBuffer index_buffer;
	VkDeviceMemory index_memory;
	int index_count;
	VkIndexType index_type;

	VkBuffer vertex_buffer;
	VkDeviceMemory vertex_memory;
	int vertex_count;
} gpu_mesh_t;

typedef struct gpu_pipeline_t
{
	VkPipelineLayout pipeline_layout;
	VkPipeline pipe;
} gpu_pipeline_t;

typedef struct gpu_shader_t
{
	VkShaderModule vertex_module;
	VkShaderModule fragment_module;
	VkDescriptorSetLayout descriptor_set_layout;
} gpu_shader_t;

typedef struct gpu_uniform_buffer_t
{
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDescriptorBufferInfo descriptor;
} gpu_uniform_buffer_t;

typedef struct gpu_frame_t
{
	VkImage image;
	VkImageView view;
	VkFramebuffer frame_buffer;
	VkFence fence;
	gpu_cmd_buffer_t* cmd_buffer;
} gpu_frame_t;

typedef struct gpu_t
{
	heap_t* heap;
	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDevice logical_device;
	VkPhysicalDeviceMemoryProperties memory_properties;
	VkQueue queue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swap_chain;

	VkRenderPass render_pass;

	VkImage depth_stencil_image;
	VkDeviceMemory depth_stencil_memory;
	VkImageView depth_stencil_view;

	VkCommandPool cmd_pool;
	VkDescriptorPool descriptor_pool;

	VkSemaphore present_complete_sema;
	VkSemaphore render_complete_sema;

	VkPipelineInputAssemblyStateCreateInfo mesh_input_assembly_info[k_gpu_mesh_layout_count];
	VkPipelineVertexInputStateCreateInfo mesh_vertex_input_info[k_gpu_mesh_layout_count];
	VkIndexType mesh_vertex_size[k_gpu_mesh_layout_count];
	VkIndexType mesh_index_size[k_gpu_mesh_layout_count];
	VkIndexType mesh_index_type[k_gpu_mesh_layout_count];

	uint32_t frame_width;
	uint32_t frame_height;

	gpu_frame_t* frames;
	uint32_t frame_count;
	uint32_t frame_index;
} gpu_t;

static void create_mesh_layouts(gpu_t* gpu);
static void destroy_mesh_layouts(gpu_t* gpu);
static uint32_t get_memory_type_index(gpu_t* gpu, uint32_t bits, VkMemoryPropertyFlags properties);

gpu_t* gpu_create(heap_t* heap, wm_window_t* window)
{
	gpu_t* gpu = heap_alloc(heap, sizeof(gpu_t), 8);
	memset(gpu, 0, sizeof(*gpu));
	gpu->heap = heap;

	//////////////////////////////////////////////////////
	// Create VkInstance
	//////////////////////////////////////////////////////

	bool use_validation = GetEnvironmentVariableW(L"VK_LAYER_PATH", NULL, 0) > 0;

	VkApplicationInfo app_info =
	{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "GA 2022",
		.pEngineName = "GA 2022",
		.apiVersion = VK_API_VERSION_1_2,
	};

	const char* k_extensions[] =
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
	};

	const char* k_layers[] =
	{
		"VK_LAYER_KHRONOS_validation",
	};

	VkInstanceCreateInfo instance_info =
	{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &app_info,
		.enabledExtensionCount = _countof(k_extensions),
		.ppEnabledExtensionNames = k_extensions,
		.enabledLayerCount = use_validation ? _countof(k_layers) : 0,
		.ppEnabledLayerNames = k_layers,
	};

	const char* function = NULL;
	VkResult result = vkCreateInstance(&instance_info, NULL, &gpu->instance);
	if (result)
	{
		function = "vkCreateInstance";
		goto fail;
	}

	//////////////////////////////////////////////////////
	// Find our desired VkPhysicalDevice (GPU)
	//////////////////////////////////////////////////////

	uint32_t physical_device_count = 0;
	result = vkEnumeratePhysicalDevices(gpu->instance, &physical_device_count, NULL);
	if (result)
	{
		function = "vkEnumeratePhysicalDevices";
		goto fail;
	}

	if (!physical_device_count)
	{
		debug_print(k_print_error, "No device with Vulkan support found!\n");
		gpu_destroy(gpu);
		return NULL;
	}

	VkPhysicalDevice* physical_devices = alloca(sizeof(VkPhysicalDevice) * physical_device_count);
	result = vkEnumeratePhysicalDevices(gpu->instance, &physical_device_count, physical_devices);
	if (result)
	{
		function = "vkEnumeratePhysicalDevices";
		goto fail;
	}

	gpu->physical_device = physical_devices[0];

	//////////////////////////////////////////////////////
	// Create a VkDevice with graphics queue
	//////////////////////////////////////////////////////

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(gpu->physical_device, &queue_family_count, NULL);

	VkQueueFamilyProperties* queue_families = alloca(sizeof(VkQueueFamilyProperties) * queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(gpu->physical_device, &queue_family_count, queue_families);

	uint32_t queue_family_index = UINT32_MAX;
	uint32_t queue_count = UINT32_MAX;
	for (uint32_t i = 0; i < queue_family_count; ++i)
	{
		if (queue_families[i].queueCount > 0 && queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			queue_family_index = i;
			queue_count = queue_families[i].queueCount;
			break;
		}
	}
	if (queue_count == UINT32_MAX)
	{
		debug_print(k_print_error, "No device with graphics queue found!\n");
		gpu_destroy(gpu);
		return NULL;
	}

	float* queue_priorites = alloca(sizeof(float) * queue_count);
	memset(queue_priorites, 0, sizeof(float) * queue_count);

	VkDeviceQueueCreateInfo queue_info =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = queue_family_index,
		.queueCount = queue_count,
		.pQueuePriorities = queue_priorites,
	};

	const char* device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	VkDeviceCreateInfo device_info =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queue_info,
		.enabledExtensionCount = _countof(device_extensions),
		.ppEnabledExtensionNames = device_extensions,
	};

	result = vkCreateDevice(gpu->physical_device, &device_info, NULL, &gpu->logical_device);
	if (result)
	{
		function = "vkCreateDevice";
		goto fail;
	}

	vkGetPhysicalDeviceMemoryProperties(gpu->physical_device, &gpu->memory_properties);
	vkGetDeviceQueue(gpu->logical_device, queue_family_index, 0, &gpu->queue);

	//////////////////////////////////////////////////////
	// Create a Windows surface on which to render
	//////////////////////////////////////////////////////

	VkWin32SurfaceCreateInfoKHR surface_info =
	{
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = GetModuleHandle(NULL),
		.hwnd = wm_get_raw_window(window),
	};
	result = vkCreateWin32SurfaceKHR(gpu->instance, &surface_info, NULL, &gpu->surface);
	if (result)
	{
		function = "vkCreateWin32SurfaceKHR";
		goto fail;
	}

	VkSurfaceCapabilitiesKHR surface_cap;
	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu->physical_device, gpu->surface, &surface_cap);
	if (result)
	{
		function = "vkGetPhysicalDeviceSurfaceCapabilitiesKHR";
		goto fail;
	}

	gpu->frame_width = surface_cap.currentExtent.width;
	gpu->frame_height = surface_cap.currentExtent.height;

	//////////////////////////////////////////////////////
	// Create a VkSwapchain storing frame buffer images
	//////////////////////////////////////////////////////

	VkSwapchainCreateInfoKHR swapchain_info =
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = gpu->surface,
		.minImageCount = __max(surface_cap.minImageCount + 1, 3),
		.imageFormat = VK_FORMAT_B8G8R8A8_SRGB,
		.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
		.imageExtent = surface_cap.currentExtent,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform = surface_cap.currentTransform,
		.imageArrayLayers = 1,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR,
		.clipped = VK_TRUE,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
	};
	result = vkCreateSwapchainKHR(gpu->logical_device, &swapchain_info, NULL, &gpu->swap_chain);
	if (result)
	{
		function = "vkCreateSwapchainKHR";
		goto fail;
	}

	result = vkGetSwapchainImagesKHR(gpu->logical_device, gpu->swap_chain, &gpu->frame_count, NULL);
	if (result)
	{
		function = "vkGetSwapchainImagesKHR";
		goto fail;
	}

	gpu->frames = heap_alloc(heap, sizeof(gpu_frame_t) * gpu->frame_count, 8);
	memset(gpu->frames, 0, sizeof(gpu_frame_t) * gpu->frame_count);
	VkImage* images = alloca(sizeof(VkImage) * gpu->frame_count);

	result = vkGetSwapchainImagesKHR(gpu->logical_device, gpu->swap_chain, &gpu->frame_count, images);
	if (result)
	{
		function = "vkGetSwapchainImagesKHR";
		goto fail;
	}

	for (uint32_t i = 0; i < gpu->frame_count; i++)
	{
		gpu->frames[i].image = images[i];

		VkImageViewCreateInfo image_view_info =
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.format = VK_FORMAT_B8G8R8A8_SRGB,
			.components =
			{
				VK_COMPONENT_SWIZZLE_R,
				VK_COMPONENT_SWIZZLE_G,
				VK_COMPONENT_SWIZZLE_B,
				VK_COMPONENT_SWIZZLE_A
			},
			.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.subresourceRange.levelCount = 1,
			.subresourceRange.layerCount = 1,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.image = images[i],
		};
		result = vkCreateImageView(gpu->logical_device, &image_view_info, NULL, &gpu->frames[i].view);
		if (result)
		{
			function = "vkCreateImageView";
			goto fail;
		}
	}

	//////////////////////////////////////////////////////
	// Create a depth buffer image
	//////////////////////////////////////////////////////
	{
		VkImageCreateInfo depth_image_info =
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = VK_FORMAT_D32_SFLOAT,
			.extent = { surface_cap.currentExtent.width, surface_cap.currentExtent.height, 1 },
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};
		result = vkCreateImage(gpu->logical_device, &depth_image_info, NULL, &gpu->depth_stencil_image);
		if (result)
		{
			function = "vkCreateImage";
			goto fail;
		}

		VkMemoryRequirements depth_mem_reqs;
		vkGetImageMemoryRequirements(gpu->logical_device, gpu->depth_stencil_image, &depth_mem_reqs);

		VkMemoryAllocateInfo depth_alloc_info =
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = depth_mem_reqs.size,
			.memoryTypeIndex = get_memory_type_index(gpu, depth_mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
		};
		result = vkAllocateMemory(gpu->logical_device, &depth_alloc_info, NULL, &gpu->depth_stencil_memory);
		if (result)
		{
			function = "vkAllocateMemory";
			goto fail;
		}

		result = vkBindImageMemory(gpu->logical_device, gpu->depth_stencil_image, gpu->depth_stencil_memory, 0);
		if (result)
		{
			function = "vkBindImageMemory";
			goto fail;
		}

		VkImageViewCreateInfo depth_view_info =
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = VK_FORMAT_D32_SFLOAT,
			.subresourceRange = { .levelCount = 1, .layerCount = 1 },
			.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
			.image = gpu->depth_stencil_image,
		};
		result = vkCreateImageView(gpu->logical_device, &depth_view_info, NULL, &gpu->depth_stencil_view);
		if (result)
		{
			function = "vkCreateImageView";
			goto fail;
		}
	}

	//////////////////////////////////////////////////////
	// Create a VkRenderPass that draws to the screen
	//////////////////////////////////////////////////////
	{
		VkAttachmentDescription attachments[2] =
		{
			{
				.format = VK_FORMAT_B8G8R8A8_SRGB,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			},
			{
				.format = VK_FORMAT_D32_SFLOAT,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			}
		};

		VkAttachmentReference color_reference =
		{
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};
		VkAttachmentReference depth_reference =
		{
			.attachment = 1,
			.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		VkSubpassDescription subpass =
		{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_reference,
			.pDepthStencilAttachment = &depth_reference,
		};

		VkSubpassDependency dependencies[2] =
		{
			{
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
			},
			{
				.srcSubpass = 0,
				.dstSubpass = VK_SUBPASS_EXTERNAL,
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.dstAccessMask = 0,
				.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
			},
		};

		VkRenderPassCreateInfo render_pass_info =
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = _countof(attachments),
			.pAttachments = attachments,
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = _countof(dependencies),
			.pDependencies = dependencies,
		};
		result = vkCreateRenderPass(gpu->logical_device, &render_pass_info, NULL, &gpu->render_pass);
		if (result)
		{
			function = "vkCreateRenderPass";
			goto fail;
		}
	}

	//////////////////////////////////////////////////////
	// Create VkFramebuffer objects
	//////////////////////////////////////////////////////
	for (uint32_t i = 0; i < gpu->frame_count; i++)
	{
		VkImageView attachments[2] = { gpu->frames[i].view, gpu->depth_stencil_view };

		VkFramebufferCreateInfo frame_buffer_info =
		{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = gpu->render_pass,
			.attachmentCount = _countof(attachments),
			.pAttachments = attachments,
			.width = surface_cap.currentExtent.width,
			.height = surface_cap.currentExtent.height,
			.layers = 1,
		};
		result = vkCreateFramebuffer(gpu->logical_device, &frame_buffer_info, NULL, &gpu->frames[i].frame_buffer);
		if (result)
		{
			function = "vkCreateFramebuffer";
			goto fail;
		}
	}

	//////////////////////////////////////////////////////
	// Create a VkSemaphores for GPU/CPU sychronization
	//////////////////////////////////////////////////////
	VkSemaphoreCreateInfo semaphore_info =
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};
	result = vkCreateSemaphore(gpu->logical_device, &semaphore_info, NULL, &gpu->present_complete_sema);
	if (result)
	{
		function = "vkCreateSemaphore";
		goto fail;
	}
	result = vkCreateSemaphore(gpu->logical_device, &semaphore_info, NULL, &gpu->render_complete_sema);
	if (result)
	{
		function = "vkCreateSemaphore";
		goto fail;
	}

	//////////////////////////////////////////////////////
	// Create a VkDescriptorPool for use during the frame
	//////////////////////////////////////////////////////
	VkDescriptorPoolSize descriptor_pool_sizes[1] =
	{
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 512,
		}
	};
	VkDescriptorPoolCreateInfo descriptor_pool_info =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.poolSizeCount = _countof(descriptor_pool_sizes),
		.pPoolSizes = descriptor_pool_sizes,
		.maxSets = 512,
	};
	result = vkCreateDescriptorPool(gpu->logical_device, &descriptor_pool_info, NULL, &gpu->descriptor_pool);
	if (result)
	{
		function = "vkCreateDescriptorPool";
		goto fail;
	}

	//////////////////////////////////////////////////////
	// Create a VkCommandPool for use during the frame
	//////////////////////////////////////////////////////
	VkCommandPoolCreateInfo cmd_pool_info =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.queueFamilyIndex = queue_family_index,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
	};
	result = vkCreateCommandPool(gpu->logical_device, &cmd_pool_info, NULL, &gpu->cmd_pool);
	if (result)
	{
		function = "vkCreateCommandPool";
		goto fail;
	}

	//////////////////////////////////////////////////////
	// Create VkCommandBuffer objects for each frame
	//////////////////////////////////////////////////////
	for (uint32_t i = 0; i < gpu->frame_count; i++)
	{
		gpu->frames[i].cmd_buffer = heap_alloc(gpu->heap, sizeof(gpu_cmd_buffer_t), 8);
		memset(gpu->frames[i].cmd_buffer, 0, sizeof(gpu_cmd_buffer_t));

		VkCommandBufferAllocateInfo alloc_info =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = gpu->cmd_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};
		result = vkAllocateCommandBuffers(gpu->logical_device, &alloc_info, &gpu->frames[i].cmd_buffer->buffer);
		if (result)
		{
			function = "vkAllocateCommandBuffers";
			goto fail;
		}

		VkFenceCreateInfo fence_info =
		{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		};
		result = vkCreateFence(gpu->logical_device, &fence_info, NULL, &gpu->frames[i].fence);
		if (result)
		{
			function = "vkCreateFence";
			goto fail;
		}
	}

	create_mesh_layouts(gpu);

	return gpu;

fail:
	debug_print(k_print_error, "%s failed: %d\n", function, result);
	gpu_destroy(gpu);
	return NULL;
}

void gpu_destroy(gpu_t* gpu)
{
	if (gpu && gpu->queue)
	{
		vkQueueWaitIdle(gpu->queue);
	}

	if (gpu)
	{
		destroy_mesh_layouts(gpu);
	}
	if (gpu && gpu->render_complete_sema)
	{
		vkDestroySemaphore(gpu->logical_device, gpu->render_complete_sema, NULL);
	}
	if (gpu && gpu->present_complete_sema)
	{
		vkDestroySemaphore(gpu->logical_device, gpu->present_complete_sema, NULL);
	}
	if (gpu && gpu->depth_stencil_view)
	{
		vkDestroyImageView(gpu->logical_device, gpu->depth_stencil_view, NULL);
	}
	if (gpu && gpu->depth_stencil_image)
	{
		vkDestroyImage(gpu->logical_device, gpu->depth_stencil_image, NULL);
	}
	if (gpu && gpu->depth_stencil_memory)
	{
		vkFreeMemory(gpu->logical_device, gpu->depth_stencil_memory, NULL);
	}
	if (gpu && gpu->frames)
	{
		for (uint32_t i = 0; i < gpu->frame_count; i++)
		{
			if (gpu->frames[i].fence)
			{
				vkDestroyFence(gpu->logical_device, gpu->frames[i].fence, NULL);
			}
			if (gpu->frames[i].cmd_buffer)
			{
				vkFreeCommandBuffers(gpu->logical_device, gpu->cmd_pool, 1, &gpu->frames[i].cmd_buffer->buffer);
			}
			if (gpu->frames[i].frame_buffer)
			{
				vkDestroyFramebuffer(gpu->logical_device, gpu->frames[i].frame_buffer, NULL);
			}
			if (gpu->frames[i].view)
			{
				vkDestroyImageView(gpu->logical_device, gpu->frames[i].view, NULL);
			}
		}
		heap_free(gpu->heap, gpu->frames);
	}
	if (gpu && gpu->descriptor_pool)
	{
		vkDestroyDescriptorPool(gpu->logical_device, gpu->descriptor_pool, NULL);
	}
	if (gpu && gpu->cmd_pool)
	{
		vkDestroyCommandPool(gpu->logical_device, gpu->cmd_pool, NULL);
	}
	if (gpu && gpu->render_pass)
	{
		vkDestroyRenderPass(gpu->logical_device, gpu->render_pass, NULL);
	}
	if (gpu && gpu->swap_chain)
	{
		vkDestroySwapchainKHR(gpu->logical_device, gpu->swap_chain, NULL);
	}
	if (gpu && gpu->surface)
	{
		vkDestroySurfaceKHR(gpu->instance, gpu->surface, NULL);
	}
	if (gpu && gpu->logical_device)
	{
		vkDestroyDevice(gpu->logical_device, NULL);
	}
	if (gpu && gpu->instance)
	{
		vkDestroyInstance(gpu->instance, NULL);
	}
	if (gpu)
	{
		heap_free(gpu->heap, gpu);
	}
}

int gpu_get_frame_count(gpu_t* gpu)
{
	return gpu->frame_count;
}

void gpu_wait_until_idle(gpu_t* gpu)
{
	vkQueueWaitIdle(gpu->queue);
}

gpu_descriptor_t* gpu_descriptor_create(gpu_t* gpu, const gpu_descriptor_info_t* info)
{
	gpu_descriptor_t* descriptor = heap_alloc(gpu->heap, sizeof(gpu_descriptor_t), 8);
	memset(descriptor, 0, sizeof(*descriptor));

	VkDescriptorSetAllocateInfo alloc_info =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = gpu->descriptor_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &info->shader->descriptor_set_layout,
	};
	VkResult result = vkAllocateDescriptorSets(gpu->logical_device, &alloc_info, &descriptor->set);
	if (result)
	{
		debug_print(k_print_error, "vkAllocateDescriptorSets failed: %d\n", result);
		gpu_descriptor_destroy(gpu, descriptor);
		return NULL;
	}

	VkWriteDescriptorSet* write_sets = alloca(sizeof(VkWriteDescriptorSet) * info->uniform_buffer_count);
	for (int i = 0; i < info->uniform_buffer_count; ++i)
	{
		write_sets[i] = (VkWriteDescriptorSet)
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptor->set,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &info->uniform_buffers[i]->descriptor,
			.dstBinding = i,
		};
	}
	vkUpdateDescriptorSets(gpu->logical_device, info->uniform_buffer_count, write_sets, 0, NULL);

	return descriptor;
}

void gpu_descriptor_destroy(gpu_t* gpu, gpu_descriptor_t* descriptor)
{
	if (descriptor && descriptor->set)
	{
		vkFreeDescriptorSets(gpu->logical_device, gpu->descriptor_pool, 1, &descriptor->set);
	}
	if (descriptor)
	{
		heap_free(gpu->heap, descriptor);
	}
}

gpu_mesh_t* gpu_mesh_create(gpu_t* gpu, const gpu_mesh_info_t* info)
{
	gpu_mesh_t* mesh = heap_alloc(gpu->heap, sizeof(gpu_mesh_t), 8);
	memset(mesh, 0, sizeof(*mesh));

	mesh->index_type = gpu->mesh_index_type[info->layout];
	mesh->index_count = (int)info->index_data_size / gpu->mesh_index_size[info->layout];
	mesh->vertex_count = (int)info->vertex_data_size / gpu->mesh_vertex_size[info->layout];

	// Vertex data
	{
		VkBufferCreateInfo vertex_buffer_info =
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = info->vertex_data_size,
			.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		};
		VkResult result = vkCreateBuffer(gpu->logical_device, &vertex_buffer_info, NULL, &mesh->vertex_buffer);
		if (result)
		{
			debug_print(k_print_error, "vkCreateBuffer failed: %d\n", result);
			gpu_mesh_destroy(gpu, mesh);
			return NULL;
		}

		VkMemoryRequirements mem_reqs;
		vkGetBufferMemoryRequirements(gpu->logical_device, mesh->vertex_buffer, &mem_reqs);

		VkMemoryAllocateInfo mem_alloc =
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = mem_reqs.size,
			.memoryTypeIndex = get_memory_type_index(gpu, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
		};
		result = vkAllocateMemory(gpu->logical_device, &mem_alloc, NULL, &mesh->vertex_memory);
		if (result)
		{
			debug_print(k_print_error, "vkAllocateMemory failed: %d\n", result);
			gpu_mesh_destroy(gpu, mesh);
			return NULL;
		}

		void* vertex_dest = NULL;
		result = vkMapMemory(gpu->logical_device, mesh->vertex_memory, 0, mem_alloc.allocationSize, 0, &vertex_dest);
		if (result)
		{
			debug_print(k_print_error, "vkMapMemory failed: %d\n", result);
			gpu_mesh_destroy(gpu, mesh);
			return NULL;
		}
		memcpy(vertex_dest, info->vertex_data, info->vertex_data_size);
		vkUnmapMemory(gpu->logical_device, mesh->vertex_memory);

		result = vkBindBufferMemory(gpu->logical_device, mesh->vertex_buffer, mesh->vertex_memory, 0);
		if (result)
		{
			debug_print(k_print_error, "vkBindBufferMemory failed: %d\n", result);
			gpu_mesh_destroy(gpu, mesh);
			return NULL;
		}
	}

	// Index data
	{
		VkBufferCreateInfo index_buffer_info =
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = info->index_data_size,
			.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		};
		VkResult result = vkCreateBuffer(gpu->logical_device, &index_buffer_info, NULL, &mesh->index_buffer);
		if (result)
		{
			debug_print(k_print_error, "vkCreateBuffer failed: %d\n", result);
			gpu_mesh_destroy(gpu, mesh);
			return NULL;
		}

		VkMemoryRequirements mem_reqs;
		vkGetBufferMemoryRequirements(gpu->logical_device, mesh->index_buffer, &mem_reqs);

		VkMemoryAllocateInfo mem_alloc =
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = mem_reqs.size,
			.memoryTypeIndex = get_memory_type_index(gpu, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
		};
		result = vkAllocateMemory(gpu->logical_device, &mem_alloc, NULL, &mesh->index_memory);
		if (result)
		{
			debug_print(k_print_error, "vkAllocateMemory failed: %d\n", result);
			gpu_mesh_destroy(gpu, mesh);
			return NULL;
		}

		void* index_dest = NULL;
		result = vkMapMemory(gpu->logical_device, mesh->index_memory, 0, mem_alloc.allocationSize, 0, &index_dest);
		if (result)
		{
			debug_print(k_print_error, "vkMapMemory failed: %d\n", result);
			gpu_mesh_destroy(gpu, mesh);
			return NULL;
		}
		memcpy(index_dest, info->index_data, info->index_data_size);
		vkUnmapMemory(gpu->logical_device, mesh->index_memory);

		result = vkBindBufferMemory(gpu->logical_device, mesh->index_buffer, mesh->index_memory, 0);
		if (result)
		{
			debug_print(k_print_error, "vkBindBufferMemory failed: %d\n", result);
			gpu_mesh_destroy(gpu, mesh);
			return NULL;
		}
	}

	return mesh;
}

void gpu_mesh_destroy(gpu_t* gpu, gpu_mesh_t* mesh)
{
	if (mesh && mesh->index_buffer)
	{
		vkDestroyBuffer(gpu->logical_device, mesh->index_buffer, NULL);
	}
	if (mesh && mesh->index_memory)
	{
		vkFreeMemory(gpu->logical_device, mesh->index_memory, NULL);
	}
	if (mesh && mesh->vertex_buffer)
	{
		vkDestroyBuffer(gpu->logical_device, mesh->vertex_buffer, NULL);
	}
	if (mesh && mesh->vertex_memory)
	{
		vkFreeMemory(gpu->logical_device, mesh->vertex_memory, NULL);
	}
	if (mesh)
	{
		heap_free(gpu->heap, mesh);
	}
}

gpu_pipeline_t* gpu_pipeline_create(gpu_t* gpu, const gpu_pipeline_info_t* info)
{
	gpu_pipeline_t* pipeline = heap_alloc(gpu->heap, sizeof(gpu_pipeline_t), 8);
	memset(pipeline, 0, sizeof(*pipeline));

	VkPipelineRasterizationStateCreateInfo rasterization_state_info =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.lineWidth = 1.0f,
	};

	VkPipelineColorBlendAttachmentState color_blend_state =
	{
		.colorWriteMask = 0xf,
		.blendEnable = VK_FALSE,
	};
	VkPipelineColorBlendStateCreateInfo color_blend_info =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &color_blend_state,
	};

	VkPipelineViewportStateCreateInfo viewport_state_info =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1,
	};

	VkPipelineDepthStencilStateCreateInfo depth_stencil_info =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
		.depthBoundsTestEnable = VK_FALSE,
		.back.failOp = VK_STENCIL_OP_KEEP,
		.back.passOp = VK_STENCIL_OP_KEEP,
		.back.compareOp = VK_COMPARE_OP_ALWAYS,
		.front.failOp = VK_STENCIL_OP_KEEP,
		.front.passOp = VK_STENCIL_OP_KEEP,
		.front.compareOp = VK_COMPARE_OP_ALWAYS,
		.stencilTestEnable = VK_FALSE,
	};

	VkPipelineMultisampleStateCreateInfo multisample_info =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	};

	VkPipelineShaderStageCreateInfo shader_info[2] =
	{
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = info->shader->vertex_module,
			.pName = "main",
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = info->shader->fragment_module,
			.pName = "main",
		},
	};

	VkDynamicState dynamic_states[2] =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	VkPipelineDynamicStateCreateInfo dynamic_info =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = _countof(dynamic_states),
		.pDynamicStates = dynamic_states,
	};

	VkPipelineLayoutCreateInfo pipeline_layout_info =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &info->shader->descriptor_set_layout,
	};
	VkResult result = vkCreatePipelineLayout(gpu->logical_device, &pipeline_layout_info, NULL, &pipeline->pipeline_layout);
	if (result)
	{
		debug_print(k_print_error, "vkCreatePipelineLayout failed: %d\n", result);
		gpu_pipeline_destroy(gpu, pipeline);
		return NULL;
	}

	VkGraphicsPipelineCreateInfo pipeline_info =
	{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.layout = pipeline->pipeline_layout,
		.renderPass = gpu->render_pass,
		.stageCount = _countof(shader_info),
		.pStages = shader_info,
		.pVertexInputState = &gpu->mesh_vertex_input_info[info->mesh_layout],
		.pInputAssemblyState = &gpu->mesh_input_assembly_info[info->mesh_layout],
		.pRasterizationState = &rasterization_state_info,
		.pColorBlendState = &color_blend_info,
		.pMultisampleState = &multisample_info,
		.pViewportState = &viewport_state_info,
		.pDepthStencilState = &depth_stencil_info,
		.pDynamicState = &dynamic_info,
	};
	result = vkCreateGraphicsPipelines(gpu->logical_device, NULL, 1, &pipeline_info, NULL, &pipeline->pipe);
	if (result)
	{
		debug_print(k_print_error, "vkCreateGraphicsPipelines failed: %d\n", result);
		gpu_pipeline_destroy(gpu, pipeline);
		return NULL;
	}

	return pipeline;
}

void gpu_pipeline_destroy(gpu_t* gpu, gpu_pipeline_t* pipeline)
{
	if (pipeline && pipeline->pipeline_layout)
	{
		vkDestroyPipelineLayout(gpu->logical_device, pipeline->pipeline_layout, NULL);
	}
	if (pipeline && pipeline->pipe)
	{
		vkDestroyPipeline(gpu->logical_device, pipeline->pipe, NULL);
	}
	if (pipeline)
	{
		heap_free(gpu->heap, pipeline);
	}
}

gpu_shader_t* gpu_shader_create(gpu_t* gpu, const gpu_shader_info_t* info)
{
	gpu_shader_t* shader = heap_alloc(gpu->heap, sizeof(gpu_shader_t), 8);
	memset(shader, 0, sizeof(*shader));

	VkShaderModuleCreateInfo vertex_module_info =
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = info->vertex_shader_size,
		.pCode = info->vertex_shader_data,
	};
	VkResult result = vkCreateShaderModule(gpu->logical_device, &vertex_module_info, NULL, &shader->vertex_module);
	if (result)
	{
		debug_print(k_print_error, "vkCreateShaderModule failed: %d\n", result);
		gpu_shader_destroy(gpu, shader);
		return NULL;
	}

	VkShaderModuleCreateInfo fragment_module_info =
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = info->fragment_shader_size,
		.pCode = info->fragment_shader_data,
	};
	result = vkCreateShaderModule(gpu->logical_device, &fragment_module_info, NULL, &shader->fragment_module);
	if (result)
	{
		debug_print(k_print_error, "vkCreateShaderModule failed: %d\n", result);
		gpu_shader_destroy(gpu, shader);
		return NULL;
	}

	VkDescriptorSetLayoutBinding* descriptor_set_layout_bindings = alloca(sizeof(VkDescriptorSetLayoutBinding) * info->uniform_buffer_count);
	for (int i = 0; i < info->uniform_buffer_count; ++i)
	{
		descriptor_set_layout_bindings[i] = (VkDescriptorSetLayoutBinding)
		{
			.binding = i,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		};
	}

	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = info->uniform_buffer_count,
		.pBindings = descriptor_set_layout_bindings,
	};
	result = vkCreateDescriptorSetLayout(gpu->logical_device, &descriptor_set_layout_info, NULL, &shader->descriptor_set_layout);
	if (result)
	{
		debug_print(k_print_error, "vkCreateDescriptorSetLayout failed: %d\n", result);
		gpu_shader_destroy(gpu, shader);
		return NULL;
	}

	return shader;
}

void gpu_shader_destroy(gpu_t* gpu, gpu_shader_t* shader)
{
	if (shader && shader->vertex_module)
	{
		vkDestroyShaderModule(gpu->logical_device, shader->vertex_module, NULL);
	}
	if (shader && shader->fragment_module)
	{
		vkDestroyShaderModule(gpu->logical_device, shader->fragment_module, NULL);
	}
	if (shader && shader->descriptor_set_layout)
	{
		vkDestroyDescriptorSetLayout(gpu->logical_device, shader->descriptor_set_layout, NULL);
	}
	if (shader)
	{
		heap_free(gpu->heap, shader);
	}
}

gpu_uniform_buffer_t* gpu_uniform_buffer_create(gpu_t* gpu, const gpu_uniform_buffer_info_t* info)
{
	gpu_uniform_buffer_t* uniform_buffer = heap_alloc(gpu->heap, sizeof(gpu_uniform_buffer_t), 8);
	memset(uniform_buffer, 0, sizeof(*uniform_buffer));

	VkBufferCreateInfo buffer_info =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = info->size,
		.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	};
	VkResult result = vkCreateBuffer(gpu->logical_device, &buffer_info, NULL, &uniform_buffer->buffer);
	if (result)
	{
		debug_print(k_print_error, "vkCreateBuffer failed: %d\n", result);
		gpu_uniform_buffer_destroy(gpu, uniform_buffer);
		return NULL;
	}

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(gpu->logical_device, uniform_buffer->buffer, &mem_reqs);

	VkMemoryAllocateInfo mem_alloc =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = mem_reqs.size,
		.memoryTypeIndex = get_memory_type_index(gpu, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
	};
	result = vkAllocateMemory(gpu->logical_device, &mem_alloc, NULL, &uniform_buffer->memory);
	if (result)
	{
		debug_print(k_print_error, "vkAllocateMemory failed: %d\n", result);
		gpu_uniform_buffer_destroy(gpu, uniform_buffer);
		return NULL;
	}

	result = vkBindBufferMemory(gpu->logical_device, uniform_buffer->buffer, uniform_buffer->memory, 0);
	if (result)
	{
		debug_print(k_print_error, "vkBindBufferMemory failed: %d\n", result);
		gpu_uniform_buffer_destroy(gpu, uniform_buffer);
		return NULL;
	}

	uniform_buffer->descriptor.buffer = uniform_buffer->buffer;
	uniform_buffer->descriptor.range = info->size;

	gpu_uniform_buffer_update(gpu, uniform_buffer, info->data, info->size);

	return uniform_buffer;
}

void gpu_uniform_buffer_update(gpu_t* gpu, gpu_uniform_buffer_t* buffer, const void* data, size_t size)
{
	void* dest = NULL;
	VkResult result = vkMapMemory(gpu->logical_device, buffer->memory, 0, size, 0, &dest);
	if (!result)
	{
		memcpy(dest, data, size);
		vkUnmapMemory(gpu->logical_device, buffer->memory);
	}
}

void gpu_uniform_buffer_destroy(gpu_t* gpu, gpu_uniform_buffer_t* buffer)
{
	if (buffer && buffer->buffer)
	{
		vkDestroyBuffer(gpu->logical_device, buffer->buffer, NULL);
	}
	if (buffer && buffer->memory)
	{
		vkFreeMemory(gpu->logical_device, buffer->memory, NULL);
	}
	if (buffer)
	{
		heap_free(gpu->heap, buffer);
	}
}

gpu_cmd_buffer_t* gpu_frame_begin(gpu_t* gpu)
{
	gpu_frame_t* frame = &gpu->frames[gpu->frame_index];

	VkCommandBufferBeginInfo begin_info =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	};
	VkResult result = vkBeginCommandBuffer(frame->cmd_buffer->buffer, &begin_info);
	if (result)
	{
		debug_print(k_print_error, "vkBeginCommandBuffer failed: %d\n", result);
		return NULL;
	}

	VkClearValue clear_values[2] =
	{
		{.color = {.float32 = { 0.0f, 0.0f, 0.2f, 1.0f } } },
		{.depthStencil = {.depth = 1.0f, .stencil = 0 } },
	};
	VkRenderPassBeginInfo render_pass_begin_info =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = gpu->render_pass,
		.renderArea.extent.width = gpu->frame_width,
		.renderArea.extent.height = gpu->frame_height,
		.clearValueCount = _countof(clear_values),
		.pClearValues = clear_values,
		.framebuffer = frame->frame_buffer,
	};
	vkCmdBeginRenderPass(frame->cmd_buffer->buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport =
	{
		.height = (float)gpu->frame_height,
		.width = (float)gpu->frame_width,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	vkCmdSetViewport(frame->cmd_buffer->buffer, 0, 1, &viewport);

	VkRect2D scissor =
	{
		.extent.width = gpu->frame_width,
		.extent.height = gpu->frame_height,
	};
	vkCmdSetScissor(frame->cmd_buffer->buffer, 0, 1, &scissor);

	return frame->cmd_buffer;
}

void gpu_frame_end(gpu_t* gpu)
{
	gpu_frame_t* frame = &gpu->frames[gpu->frame_index];
	gpu->frame_index = (gpu->frame_index + 1) % gpu->frame_count;

	vkCmdEndRenderPass(frame->cmd_buffer->buffer);
	VkResult result = vkEndCommandBuffer(frame->cmd_buffer->buffer);
	if (result)
	{
		debug_print(k_print_error, "vkEndCommandBuffer failed: %d\n", result);
	}

	uint32_t image_index;
	result = vkAcquireNextImageKHR(gpu->logical_device, gpu->swap_chain, UINT64_MAX, gpu->present_complete_sema, VK_NULL_HANDLE, &image_index);
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		debug_print(k_print_error, "vkAcquireNextImageKHR failed: %d\n", result);
	}

	result = vkWaitForFences(gpu->logical_device, 1, &frame->fence, VK_TRUE, UINT64_MAX);
	if (result)
	{
		debug_print(k_print_error, "vkWaitForFences failed: %d\n", result);
	}
	result = vkResetFences(gpu->logical_device, 1, &frame->fence);
	if (result)
	{
		debug_print(k_print_error, "vkResetFences failed: %d\n", result);
	}

	VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submit_info =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pWaitDstStageMask = &wait_stage_mask,
		.waitSemaphoreCount = 1,
		.signalSemaphoreCount = 1,
		.pCommandBuffers = &frame->cmd_buffer->buffer,
		.commandBufferCount = 1,
		.pWaitSemaphores = &gpu->present_complete_sema,
		.pSignalSemaphores = &gpu->render_complete_sema,
	};
	result = vkQueueSubmit(gpu->queue, 1, &submit_info, frame->fence);
	if (result)
	{
		debug_print(k_print_error, "vkQueueSubmit failed: %d\n", result);
	}

	VkPresentInfoKHR present_info =
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.swapchainCount = 1,
		.pSwapchains = &gpu->swap_chain,
		.pImageIndices = &image_index,
		.pWaitSemaphores = &gpu->render_complete_sema,
		.waitSemaphoreCount = 1,
	};
	result = vkQueuePresentKHR(gpu->queue, &present_info);
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		debug_print(k_print_error, "vkQueuePresentKHR failed: %d\n", result);
	}
}

void gpu_cmd_pipeline_bind(gpu_t* gpu, gpu_cmd_buffer_t* cmd_buffer, gpu_pipeline_t* pipeline)
{
	vkCmdBindPipeline(cmd_buffer->buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipe);
	cmd_buffer->pipeline_layout = pipeline->pipeline_layout;
}

void gpu_cmd_descriptor_bind(gpu_t* gpu, gpu_cmd_buffer_t* cmd_buffer, gpu_descriptor_t* descriptor)
{
	vkCmdBindDescriptorSets(cmd_buffer->buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cmd_buffer->pipeline_layout, 0, 1, &descriptor->set, 0, NULL);
}

void gpu_cmd_mesh_bind(gpu_t* gpu, gpu_cmd_buffer_t* cmd_buffer, gpu_mesh_t* mesh)
{
	if (mesh->vertex_count)
	{
		VkDeviceSize zero = 0;
		vkCmdBindVertexBuffers(cmd_buffer->buffer, 0, 1, &mesh->vertex_buffer, &zero);
		cmd_buffer->vertex_count = mesh->vertex_count;
	}
	else
	{
		cmd_buffer->vertex_count = 0;
	}
	if (mesh->index_count)
	{
		vkCmdBindIndexBuffer(cmd_buffer->buffer, mesh->index_buffer, 0, mesh->index_type);
		cmd_buffer->index_count = mesh->index_count;
	}
	else
	{
		cmd_buffer->index_count = 0;
	}
}

void gpu_cmd_draw(gpu_t* gpu, gpu_cmd_buffer_t* cmd_buffer)
{
	if (cmd_buffer->index_count)
	{
		vkCmdDrawIndexed(cmd_buffer->buffer, cmd_buffer->index_count, 1, 0, 0, 0);
	}
	else if (cmd_buffer->vertex_count)
	{
		vkCmdDraw(cmd_buffer->buffer, cmd_buffer->vertex_count, 1, 0, 0);
	}
}

static void create_mesh_layouts(gpu_t* gpu)
{
	// k_gpu_mesh_layout_tri_p444_i2
	{
		gpu->mesh_input_assembly_info[k_gpu_mesh_layout_tri_p444_i2] = (VkPipelineInputAssemblyStateCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		};

		VkVertexInputBindingDescription* vertex_binding = heap_alloc(gpu->heap, sizeof(VkVertexInputBindingDescription), 8);
		*vertex_binding = (VkVertexInputBindingDescription)
		{
			.binding = 0,
			.stride = 12,
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		};

		VkVertexInputAttributeDescription* vertex_attributes = heap_alloc(gpu->heap, sizeof(VkVertexInputAttributeDescription) * 1, 8);
		vertex_attributes[0] = (VkVertexInputAttributeDescription)
		{
			.binding = 0,
			.location = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = 0,
		};

		gpu->mesh_vertex_input_info[k_gpu_mesh_layout_tri_p444_i2] = (VkPipelineVertexInputStateCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = vertex_binding,
			.vertexAttributeDescriptionCount = 1,
			.pVertexAttributeDescriptions = vertex_attributes,
		};

		gpu->mesh_index_type[k_gpu_mesh_layout_tri_p444_i2] = VK_INDEX_TYPE_UINT16;
		gpu->mesh_index_size[k_gpu_mesh_layout_tri_p444_i2] = 2;
		gpu->mesh_vertex_size[k_gpu_mesh_layout_tri_p444_i2] = 12;
	}

	// k_gpu_mesh_layout_tri_p444_c444_i2
	{
		gpu->mesh_input_assembly_info[k_gpu_mesh_layout_tri_p444_c444_i2] = (VkPipelineInputAssemblyStateCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		};

		VkVertexInputBindingDescription* vertex_binding = heap_alloc(gpu->heap, sizeof(VkVertexInputBindingDescription), 8);
		*vertex_binding = (VkVertexInputBindingDescription)
		{
			.binding = 0,
			.stride = 24,
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		};

		VkVertexInputAttributeDescription* vertex_attributes = heap_alloc(gpu->heap, sizeof(VkVertexInputAttributeDescription) * 2, 8);
		vertex_attributes[0] = (VkVertexInputAttributeDescription)
		{
			.binding = 0,
			.location = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = 0,
		};
		vertex_attributes[1] = (VkVertexInputAttributeDescription)
		{
			.binding = 0,
			.location = 1,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = 12,
		};

		gpu->mesh_vertex_input_info[k_gpu_mesh_layout_tri_p444_c444_i2] = (VkPipelineVertexInputStateCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = vertex_binding,
			.vertexAttributeDescriptionCount = 2,
			.pVertexAttributeDescriptions = vertex_attributes,
		};

		gpu->mesh_index_type[k_gpu_mesh_layout_tri_p444_c444_i2] = VK_INDEX_TYPE_UINT16;
		gpu->mesh_index_size[k_gpu_mesh_layout_tri_p444_c444_i2] = 2;
		gpu->mesh_vertex_size[k_gpu_mesh_layout_tri_p444_c444_i2] = 24;
	}
}

static void destroy_mesh_layouts(gpu_t* gpu)
{
	for (int i = 0; i < _countof(gpu->mesh_vertex_input_info); ++i)
	{
		if (gpu->mesh_vertex_input_info[i].pVertexBindingDescriptions)
		{
			heap_free(gpu->heap, (void*)gpu->mesh_vertex_input_info[i].pVertexBindingDescriptions);
			heap_free(gpu->heap, (void*)gpu->mesh_vertex_input_info[i].pVertexAttributeDescriptions);
		}
	}
}

static uint32_t get_memory_type_index(gpu_t* gpu, uint32_t bits, VkMemoryPropertyFlags properties)
{
	for (uint32_t i = 0; i < gpu->memory_properties.memoryTypeCount; ++i)
	{
		if (bits & (1UL << i))
		{
			if ((gpu->memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}
	}
	debug_print(k_print_error, "Unable to find memory of type: %x\n", bits);
	return 0;
}
