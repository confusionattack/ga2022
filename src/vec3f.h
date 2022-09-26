#pragma once

#include <stdlib.h>

#include "math.h"

typedef struct vec3f_t
{
	union
	{
		float x, y, z;
		float a[3];
	};
} vec3f_t;

__forceinline vec3f_t vec3f_x()
{
	return (vec3f_t) { .x = 1.0f };
}

__forceinline vec3f_t vec3f_y()
{
	return (vec3f_t) { .y = 1.0f };
}

__forceinline vec3f_t vec3f_z()
{
	return (vec3f_t) { .z = 1.0f };
}

__forceinline vec3f_t vec3f_one()
{
	return (vec3f_t) { .x = 1.0f,.y = 1.0f, .z = 1.0f };
}

__forceinline vec3f_t vec3f_zero()
{
	return (vec3f_t) { .x = 0.0f, .y = 0.0f, .z = 0.0f };
}

__forceinline vec3f_t vec3f_forward()
{
	return (vec3f_t) { .x = -1.0f };
}

__forceinline vec3f_t vec3f_right()
{
	return (vec3f_t) { .y = 1.0f };
}

__forceinline vec3f_t vec3f_up()
{
	return (vec3f_t) { .z = 1.0f };
}

__forceinline vec3f_t vec3f_add(vec3f_t a, vec3f_t b)
{
	return (vec3f_t)
	{
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z
	};
}

__forceinline vec3f_t vec3f_sub(vec3f_t a, vec3f_t b)
{
	return (vec3f_t)
	{
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z
	};
}

__forceinline vec3f_t vec3f_min(vec3f_t a, vec3f_t b)
{
	return (vec3f_t)
	{
		.x = __min(a.x, b.x),
		.y = __min(a.y, b.y),
		.z = __min(a.z, b.z),
	};
}

__forceinline vec3f_t vec3f_max(vec3f_t a, vec3f_t b)
{
	return (vec3f_t)
	{
		.x = __max(a.x, b.x),
		.y = __max(a.y, b.y),
		.z = __max(a.z, b.z),
	};
}

__forceinline vec3f_t vec3f_scale(vec3f_t v, float f)
{
	return (vec3f_t)
	{
		.x = v.x * f,
		.y = v.y * f,
		.z = v.z * f,
	};
}

__forceinline vec3f_t vec3f_lerp(vec3f_t a, vec3f_t b, float f)
{
	return (vec3f_t)
	{
		.x = lerpf(a.x, b.x, f),
		.y = lerpf(a.y, b.y, f),
		.z = lerpf(a.z, b.z, f),
	};
}

__forceinline float vec3f_mag2(vec3f_t v)
{
	return (v.x * v.x) + (v.y * v.y) + (v.z * v.z);
}

__forceinline float vec3f_mag(vec3f_t v)
{
	return sqrtf(vec3f_mag2(v));
}

__forceinline float vec3f_dist2(vec3f_t a, vec3f_t b)
{
	vec3f_t diff = vec3f_sub(a, b);
	return vec3f_mag2(diff);
}

__forceinline float vec3f_dist(vec3f_t a, vec3f_t b)
{
	return sqrtf(vec3f_dist2(a, b));
}

__forceinline vec3f_t vec3f_norm(vec3f_t v)
{
	float m = vec3f_mag(v);
	if (almost_equalf(m, 0.0f))
	{
		return v;
	}
	return vec3f_scale(v, 1.0f / m);
}

__forceinline float vec3f_dot(vec3f_t a, vec3f_t b)
{
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

__forceinline vec3f_t vec3f_reflect(vec3f_t v, vec3f_t n)
{
	float d = vec3f_dot(v, n);
	vec3f_t r = vec3f_scale(n, d * 2.0f);
	return vec3f_sub(v, r);
}

__forceinline vec3f_t vec3f_cross(vec3f_t a, vec3f_t b)
{
	return (vec3f_t)
	{
		.x = a.y * b.z - a.z * b.y,
		.y = a.z * b.x - a.x * b.z,
		.z = a.x * b.y - a.y * b.x,
	};
}
