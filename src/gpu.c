#include "gpu.h"

#include "debug.h"
#include "heap.h"
#include "wm.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"

#include <malloc.h>
#include <string.h>

typedef struct gpu_pipeline_t
{
	int placeholder;
} gpu_pipeline_t;

typedef struct gpu_material_t
{
	int placeholder;
} gpu_material_t;

typedef struct gpu_mesh_t
{
	int placeholder;
} gpu_mesh_t;

typedef struct gpu_cmd_buffer_t
{
	int placeholder;
} gpu_cmd_buffer_t;

typedef struct gpu_frame_t
{
	VkImage image;
	VkImageView view;
} gpu_frame_t;

typedef struct gpu_t
{
	heap_t* heap;
	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDevice logical_device;
	VkQueue queue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swap_chain;

	VkSemaphore image_available_sema;
	VkSemaphore present_complete_sema;
	VkSemaphore render_complete_sema;

	gpu_frame_t* frames;
	uint32_t frame_count;
} gpu_t;

gpu_t* gpu_create(heap_t* heap, wm_window_t* window)
{
	gpu_t* gpu = heap_alloc(heap, sizeof(gpu_t), 8);
	memset(gpu, 0, sizeof(*gpu));
	gpu->heap = heap;

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
		//"VK_LAYER_KHRONOS_validation",
	};

	VkInstanceCreateInfo instance_info =
	{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &app_info,
		.enabledExtensionCount = _countof(k_extensions),
		.ppEnabledExtensionNames = k_extensions,
	};

	const char* function = NULL;
	VkResult result = vkCreateInstance(&instance_info, NULL, &gpu->instance);
	if (result)
	{
		function = "vkCreateInstance";
		goto fail;
	}

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

	vkGetDeviceQueue(gpu->logical_device, queue_family_index, 0, &gpu->queue);

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

	VkSwapchainCreateInfoKHR swapchain_info =
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = gpu->surface,
		.minImageCount = surface_cap.minImageCount + 1,
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

	VkSemaphoreCreateInfo semaphore_info =
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};
	result = vkCreateSemaphore(gpu->logical_device, &semaphore_info, NULL, &gpu->image_available_sema);
	if (result)
	{
		function = "vkCreateSemaphore";
		goto fail;
	}
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

	// TODO depth image
	// TODO frame buffer
	// TODO render pass

	return gpu;

fail:
	debug_print(k_print_error, "%s failed: %d\n", function, result);
	gpu_destroy(gpu);
	return NULL;
}

void gpu_destroy(gpu_t* gpu)
{
	if (gpu && gpu->render_complete_sema)
	{
		vkDestroySemaphore(gpu->logical_device, gpu->render_complete_sema, NULL);
	}
	if (gpu && gpu->present_complete_sema)
	{
		vkDestroySemaphore(gpu->logical_device, gpu->present_complete_sema, NULL);
	}
	if (gpu && gpu->image_available_sema)
	{
		vkDestroySemaphore(gpu->logical_device, gpu->image_available_sema, NULL);
	}
	if (gpu && gpu->frames)
	{
		for (uint32_t i = 0; i < gpu->frame_count; i++)
		{
			if (gpu->frames[i].view)
			{
				vkDestroyImageView(gpu->logical_device, gpu->frames[i].view, NULL);
			}
		}
		heap_free(gpu->heap, gpu->frames);
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

gpu_pipeline_t* gpu_pipeline_create(gpu_t* gpu)
{
	return NULL;
}

void gpu_pipeline_destroy(gpu_t* gpu, gpu_pipeline_t* pipeline)
{

}

gpu_material_t* gpu_material_create(gpu_t* gpu)
{
	return NULL;
}

void gpu_material_destroy(gpu_t* gpu, gpu_material_t* material)
{

}

gpu_mesh_t* gpu_mesh_create(gpu_t* gpu)
{
	return NULL;
}

void gpu_mesh_destroy(gpu_t* gpu, gpu_mesh_t* material)
{

}

void gpu_frame_begin(gpu_t* gpu)
{

}

void gpu_frame_end(gpu_t* gpu)
{

}

gpu_cmd_buffer_t* gpu_cmd_begin(gpu_t* gpu)
{
	return NULL;
}

void gpu_cmd_end(gpu_t* gpu, gpu_cmd_buffer_t* cmd_buffer)
{

}

void gpu_cmd_pipeline_bind(gpu_t* gpu, gpu_cmd_buffer_t* cmd_buffer, gpu_pipeline_t* pipeline)
{

}

void gpu_cmd_material_bind(gpu_t* gpu, gpu_cmd_buffer_t* cmd_buffer, gpu_material_t* material)
{

}

void gpu_cmd_draw(gpu_t* gpu, gpu_cmd_buffer_t* cmd_buffer, gpu_mesh_t* mesh)
{

}
