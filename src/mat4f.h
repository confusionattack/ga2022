#pragma once

// Matrix support.
// Four columns, four rows.

#include <stdbool.h>

typedef struct quatf_t quatf_t;
typedef struct vec3f_t vec3f_t;

// Base 4x4 matrix object.
typedef struct mat4f_t
{
	float data[4][4];
} mat4f_t;

// Makes the matrix m identity.
void mat4f_make_identity(mat4f_t* m);

// Make a matrix m that translates vectors by t.
void mat4f_make_translation(mat4f_t* m, const vec3f_t* t);

// Make a matrix m that scales vectors by s.
void mat4f_make_scaling(mat4f_t* m, const vec3f_t* s);

// Make a matrix m that rotates vectors by q.
void mat4f_make_rotation(mat4f_t* m, const quatf_t* q);

// Translate matrix m by translation vector t.
void mat4f_translate(mat4f_t* m, const vec3f_t* t);

// Scale matrix m by scale vector s.
void mat4f_scale(mat4f_t* m, const vec3f_t* s);

// Rotate matrix m by quaternion q.
void mat4f_rotate(mat4f_t* m, const quatf_t* q);

// Concatenate matrices a and b to get matrix result.
void mat4f_mul(mat4f_t* result, const mat4f_t* a, const mat4f_t* b);

// Concatenate matrices result and m and store in result.
void mat4f_mul_inplace(mat4f_t* result, const mat4f_t* m);

// Multiples vector in by matrix m and stores the result in out.
void mat4f_transform(const mat4f_t* m, const vec3f_t* in, vec3f_t* out);

// Multiples vector v by matrix m and stores the result back in v.
void mat4f_transform_inplace(const mat4f_t* m, vec3f_t* v);

// Attempt to compute a matrix inverse.
// Returns true on success, returns false if the determinant is zero.
bool mat4f_invert(mat4f_t* m);

// Given a field of view angle in radians, width/height aspect ratio, and depth near+far distances, compute a perspective projection matrix.
void mat4f_make_perspective(mat4f_t* m, float angle, float aspect, float z_near, float z_far);

// Creates a view matrix given an eye location, facing direction, and up vector.
void mat4f_make_lookat(mat4f_t* m, const vec3f_t* eye, const vec3f_t* dir, const vec3f_t* up);
