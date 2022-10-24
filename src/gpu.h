#pragma once

typedef struct gpu_t gpu_t;
typedef struct gpu_cmd_buffer_t gpu_cmd_buffer_t;
typedef struct gpu_material_t gpu_material_t;
typedef struct gpu_mesh_t gpu_mesh_t;
typedef struct gpu_pipeline_t gpu_pipeline_t;

typedef struct heap_t heap_t;
typedef struct wm_window_t wm_window_t;

gpu_t* gpu_create(heap_t* heap, wm_window_t* window);
void gpu_destroy(gpu_t* gpu);

gpu_pipeline_t* gpu_pipeline_create(gpu_t* gpu);
void gpu_pipeline_destroy(gpu_t* gpu, gpu_pipeline_t* pipeline);

gpu_material_t* gpu_material_create(gpu_t* gpu);
void gpu_material_destroy(gpu_t* gpu, gpu_material_t* material);

gpu_mesh_t* gpu_mesh_create(gpu_t* gpu);
void gpu_mesh_destroy(gpu_t* gpu, gpu_mesh_t* material);

void gpu_frame_begin(gpu_t* gpu);
void gpu_frame_end(gpu_t* gpu);

gpu_cmd_buffer_t* gpu_cmd_begin(gpu_t* gpu);
void gpu_cmd_end(gpu_t* gpu, gpu_cmd_buffer_t* cmd_buffer);

void gpu_cmd_pipeline_bind(gpu_t* gpu, gpu_cmd_buffer_t* cmd_buffer, gpu_pipeline_t* pipeline);
void gpu_cmd_material_bind(gpu_t* gpu, gpu_cmd_buffer_t* cmd_buffer, gpu_material_t* material);
void gpu_cmd_draw(gpu_t* gpu, gpu_cmd_buffer_t* cmd_buffer, gpu_mesh_t* mesh);
