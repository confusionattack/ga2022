#pragma once

#include <stdbool.h>

typedef struct gpu_t gpu_t;
typedef struct gpu_cmd_buffer_t gpu_cmd_buffer_t;
typedef struct gpu_descriptor_t gpu_descriptor_t;
typedef struct gpu_mesh_t gpu_mesh_t;
typedef struct gpu_pipeline_t gpu_pipeline_t;
typedef struct gpu_shader_t gpu_shader_t;
typedef struct gpu_uniform_buffer_t gpu_uniform_buffer_t;

typedef struct heap_t heap_t;
typedef struct wm_window_t wm_window_t;

typedef struct gpu_descriptor_info_t
{
	gpu_shader_t* shader;
	gpu_uniform_buffer_t** uniform_buffers;
	int uniform_buffer_count;
} gpu_descriptor_info_t;

typedef enum gpu_mesh_layout_t
{
	k_gpu_mesh_layout_tri_p444_i2,
	k_gpu_mesh_layout_tri_p444_c444_i2,

	k_gpu_mesh_layout_count,
} gpu_mesh_layout_t;

typedef struct gpu_mesh_info_t
{
	gpu_mesh_layout_t layout;
	void* vertex_data;
	size_t vertex_data_size;
	void* index_data;
	size_t index_data_size;
} gpu_mesh_info_t;

typedef struct gpu_pipeline_info_t
{
	gpu_shader_t* shader;
	gpu_mesh_layout_t mesh_layout;
} gpu_pipeline_info_t;

typedef struct gpu_shader_info_t
{
	void* vertex_shader_data;
	size_t vertex_shader_size;
	void* fragment_shader_data;
	size_t fragment_shader_size;
	int uniform_buffer_count;
} gpu_shader_info_t;

typedef struct gpu_uniform_buffer_info_t
{
	void* data;
	size_t size;
} gpu_uniform_buffer_info_t;

// Create an instance of Vulkan on the provided window.
gpu_t* gpu_create(heap_t* heap, wm_window_t* window);

// Destroy the previously created Vulkan.
void gpu_destroy(gpu_t* gpu);

// Binds uniform buffers (and textures if we had them) to a given shader layout.
gpu_descriptor_t* gpu_descriptor_create(gpu_t* gpu, const gpu_descriptor_info_t* info);

// Destroys a descriptor.
void gpu_descriptor_destroy(gpu_t* gpu, gpu_descriptor_t* descriptor);

// Create a drawable piece of geometry with vertex and index data.
gpu_mesh_t* gpu_mesh_create(gpu_t* gpu, const gpu_mesh_info_t* info);

// Destroy some geometry.
void gpu_mesh_destroy(gpu_t* gpu, gpu_mesh_t* mesh);

// Setup an object that binds a shader to a mesh layout for rendering.
gpu_pipeline_t* gpu_pipeline_create(gpu_t* gpu, const gpu_pipeline_info_t* info);

// Destroy a pipeline.
void gpu_pipeline_destroy(gpu_t* gpu, gpu_pipeline_t* pipeline);

// Create a shader object with vertex and fragment shader programs.
gpu_shader_t* gpu_shader_create(gpu_t* gpu, const gpu_shader_info_t* info);

// Destroy a shader.
void gpu_shader_destroy(gpu_t* gpu, gpu_shader_t* shader);

// Create a uniform buffer with specified size and contents.
// Will be consumed by a shader.
gpu_uniform_buffer_t* gpu_uniform_buffer_create(gpu_t* gpu, const gpu_uniform_buffer_info_t* info);

// Modify an existing uniform buffer.
void gpu_uniform_buffer_update(gpu_t* gpu, gpu_uniform_buffer_t* buffer, const void* data, size_t size);

// Destroy a uniform buffer.
void gpu_uniform_buffer_destroy(gpu_t* gpu, gpu_uniform_buffer_t* buffer);

// Start a new frame of rendering. May wait on a prior frame to complete.
// Returns a command buffer for all rendering in that frame.
gpu_cmd_buffer_t* gpu_frame_begin(gpu_t* gpu);

// Finish rendering frame.
void gpu_frame_end(gpu_t* gpu);

// Set the current pipeline for this command buffer.
void gpu_cmd_pipeline_bind(gpu_t* gpu, gpu_cmd_buffer_t* cmd_buffer, gpu_pipeline_t* pipeline);

// Set the current mesh for this command buffer.
void gpu_cmd_mesh_bind(gpu_t* gpu, gpu_cmd_buffer_t* cmd_buffer, gpu_mesh_t* mesh);

// Set the current descriptor for this command buffer.
void gpu_cmd_descriptor_bind(gpu_t* gpu, gpu_cmd_buffer_t* cmd_buffer, gpu_descriptor_t* descriptor);

// Draw given current pipeline, mesh, and descriptor.
void gpu_cmd_draw(gpu_t* gpu, gpu_cmd_buffer_t* cmd_buffer);
