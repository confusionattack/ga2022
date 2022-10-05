#include "transform.h"

void transform_identity(transform_t* transform)
{
	transform->translation = vec3f_zero();
	transform->scale = vec3f_one();
	transform->rotation = quatf_identity();
}

void transform_to_matrix(const transform_t* transform, mat4f_t* output)
{
	const quatf_t* q = &transform->rotation;
	float r00 = 1.0f - 2.0f * (q->y * q->y + q->z * q->z);
	float r10 = 2.0f * (q->x * q->y - q->z * q->w);
	float r20 = 2.0f * (q->x * q->z + q->y * q->w);
	float r01 = 2.0f * (q->x * q->y + q->z * q->w);
	float r11 = 1.0f - 2.0f * (q->x * q->x + q->z * q->z);
	float r21 = 2.0f * (q->y * q->z - q->x * q->w);
	float r02 = 2.0f * (q->x * q->z - q->y * q->w);
	float r12 = 2.0f * (q->y * q->z + q->x * q->w);
	float r22 = 1.0f - 2.0f * (q->x * q->x + q->y * q->y);

	const vec3f_t* s = &transform->scale;
	float m00 = s->x * r00;
	float m10 = s->y * r10;
	float m20 = s->z * r20;
	float m01 = s->x * r01;
	float m11 = s->y * r11;
	float m21 = s->z * r21;
	float m02 = s->x * r02;
	float m12 = s->y * r12;
	float m22 = s->z * r22;

	const vec3f_t* t = &transform->translation;
	output->data[0][0] = m00;
	output->data[0][1] = m01;
	output->data[0][2] = m02;
	output->data[0][3] = 0.0f;
	output->data[1][0] = m10;
	output->data[1][1] = m11;
	output->data[1][2] = m12;
	output->data[1][3] = 0.0f;
	output->data[2][0] = m20;
	output->data[2][1] = m21;
	output->data[2][2] = m22;
	output->data[2][3] = 0.0f;
	output->data[3][0] = t->x;
	output->data[3][1] = t->y;
	output->data[3][2] = t->z;
	output->data[3][3] = 1.0f;
}

void transform_multiply(transform_t* result, const transform_t* t)
{
	const vec3f_t scaled_translation = vec3f_mul(result->translation, t->scale);
	const vec3f_t rotated_translation = quatf_rotate_vec(t->rotation, scaled_translation);

	result->rotation = quatf_mul(t->rotation, result->rotation);
	result->scale = vec3f_mul(result->scale, t->scale);
	result->translation = vec3f_add(rotated_translation, t->translation);
}

void transform_invert(transform_t* transform)
{
	transform->scale.x = transform->scale.x != 0.0f ? 1.0f / transform->scale.x : 0.0f;
	transform->scale.y = transform->scale.y != 0.0f ? 1.0f / transform->scale.y : 0.0f;
	transform->scale.z = transform->scale.z != 0.0f ? 1.0f / transform->scale.z : 0.0f;
	transform->rotation = quatf_conjugate(transform->rotation);
	transform->translation = vec3f_mul(transform->scale, quatf_rotate_vec(transform->rotation, vec3f_negate(transform->translation)));
}

vec3f_t transform_transform_vec3(const transform_t* transform, vec3f_t v)
{
	const vec3f_t scaled_vector = vec3f_mul(v, transform->scale);
	const vec3f_t rotated_translation = quatf_rotate_vec(transform->rotation, scaled_vector);
	return vec3f_add(rotated_translation, transform->translation);
}
