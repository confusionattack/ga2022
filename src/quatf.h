#pragma once

// Quaternion support.

#include "vec3f.h"

// Quaternion object.
typedef struct quatf_t
{
	union
	{
		struct { float x, y, z, w; };
		struct { vec3f_t v3; float s; };
	};
} quatf_t;

// Creates a quaternion with no rotation.
__forceinline quatf_t quatf_identity()
{
	return (quatf_t){ .x = 0.0f, .y = 0.0f, .z = 0.0f, .w = 1.0f };
}

// Combines the rotation of two quaternions -- a and b -- into a new quaternion.
__forceinline quatf_t quatf_mul(quatf_t a, quatf_t b)
{
	quatf_t result;

	result.v3 = vec3f_cross(a.v3, b.v3);
	result.v3 = vec3f_add(result.v3, vec3f_scale(b.v3, a.s));
	result.v3 = vec3f_add(result.v3, vec3f_scale(a.v3, b.s));

	result.s = (a.s * b.s) - vec3f_dot(a.v3, b.v3);

	return result;
}

// Computes the inverse of a normalized quaternion.
__forceinline quatf_t quatf_conjugate(quatf_t q)
{
	quatf_t result;
	result.v3 = vec3f_negate(q.v3);
	result.s = q.s;
	return result;
}

// Rotates a vector by a quaterion.
// Returns the resulting vector.
__forceinline vec3f_t quatf_rotate_vec(quatf_t q, vec3f_t v)
{
	vec3f_t t = vec3f_scale(vec3f_cross(q.v3, v), 2.0f);
	return vec3f_add(v, vec3f_add(vec3f_scale(t, q.w), vec3f_cross(q.v3, t)));
}

// Converts a quaternion to representation with 3 angles in radians: roll, yaw, pitch.
vec3f_t quatf_to_eulers(quatf_t q);

// Converts roll, yaw, pitch in radians to a quaternion.
quatf_t quatf_from_eulers(vec3f_t euler_angles);
