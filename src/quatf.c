#include "quatf.h"

#define _USE_MATH_DEFINES
#include <math.h>

vec3f_t quatf_to_eulers(quatf_t q)
{
	/* From wikipedia: https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles */

	// roll (x-axis rotation)
	float sinr = 2.0f * (q.w * q.x + q.y * q.z);
	float cosr = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
	float roll = atan2f(sinr, cosr);

	// pitch (y-axis rotation)
	float pitch = 0;
	float sinp = 2.0f * (q.w * q.y - q.z * q.x);
	if (fabsf(sinp) >= 1)
	{
		pitch = copysignf((float)M_PI / 2.0f, sinp); // use 90 degrees if out of range
	}
	else
	{
		pitch = asinf(sinp);
	}

	// yaw (z-axis rotation)
	float siny = 2.0f * (q.w * q.z + q.x * q.y);
	float cosy = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
	float yaw = atan2f(siny, cosy);

	return (vec3f_t) { .x = roll, .x = pitch, .x = yaw };
}

quatf_t quatf_from_eulers(vec3f_t euler_angles)
{
	/* From wikipedia: https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles */

	float roll = euler_angles.x;
	float pitch = euler_angles.y;
	float yaw = euler_angles.z;

	float cy = cosf(yaw * 0.5f);
	float sy = sinf(yaw * 0.5f);
	float cr = cosf(roll * 0.5f);
	float sr = sinf(roll * 0.5f);
	float cp = cosf(pitch * 0.5f);
	float sp = sinf(pitch * 0.5f);

	return (quatf_t)
	{
		.w = cy * cr * cp + sy * sr * sp,
		.x = cy * sr * cp - sy * cr * sp,
		.y = cy * cr * sp + sy * sr * cp,
		.z = sy * cr * cp - cy * sr * sp,
	};
}
