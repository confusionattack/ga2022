#include "mat4f.h"

#include "quatf.h"
#include "vec3f.h"

#include <string.h>

void mat4f_make_identity(mat4f_t* m)
{
	memset(m, 0, sizeof(*m));
	for (int i = 0; i < 4; ++i)
	{
		m->data[i][i] = 1.0f;
	}
}

void mat4f_make_translation(mat4f_t* m, const vec3f_t* t)
{
	mat4f_make_identity(m);
	m->data[3][0] = t->x;
	m->data[3][1] = t->y;
	m->data[3][2] = t->z;
	/* m->data[3][3] = 1.0f; */
}

void mat4f_make_scaling(mat4f_t* m, const vec3f_t* s)
{
	mat4f_make_identity(m);
	m->data[0][0] = s->x;
	m->data[1][1] = s->y;
	m->data[2][2] = s->z;
	/* m->data[3][3] = 1.0f; */
}

void mat4f_make_rotation(mat4f_t* m, const quatf_t* q)
{
	m->data[0][0] = 1.0f - 2.0f * (q->y * q->y + q->z * q->z);
	m->data[1][0] = 2.0f * (q->x * q->y - q->z * q->w);
	m->data[2][0] = 2.0f * (q->x * q->z + q->y * q->w);
	m->data[3][0] = 0.0f;

	m->data[0][1] = 2.0f * (q->x * q->y + q->z * q->w);
	m->data[1][1] = 1.0f - 2.0f * (q->x * q->x + q->z * q->z);
	m->data[2][1] = 2.0f * (q->y * q->z - q->x * q->w);
	m->data[3][1] = 0.0f;

	m->data[0][2] = 2.0f * (q->x * q->z - q->y * q->w);
	m->data[1][2] = 2.0f * (q->y * q->z + q->x * q->w);
	m->data[2][2] = 1.0f - 2.0f * (q->x * q->x + q->y * q->y);
	m->data[3][2] = 0.0f;

	m->data[0][3] = 0.0f;
	m->data[1][3] = 0.0f;
	m->data[2][3] = 0.0f;
	m->data[3][3] = 1.0f;
}

void mat4f_translate(mat4f_t* m, const vec3f_t* t)
{
	mat4f_t tmp;
	mat4f_make_translation(&tmp, t);
	mat4f_mul_inplace(m, &tmp);
}

void mat4f_scale(mat4f_t* m, const vec3f_t* s)
{
	mat4f_t tmp;
	mat4f_make_scaling(&tmp, s);
	mat4f_mul_inplace(m, &tmp);
}

void mat4f_rotate(mat4f_t* m, const quatf_t* q)
{
	mat4f_t tmp;
	mat4f_make_rotation(&tmp, q);
	mat4f_mul_inplace(m, &tmp);
}

void mat4f_mul(mat4f_t* result, const mat4f_t* a, const mat4f_t* b)
{
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			float tmp = 0.0f;
			for (int k = 0; k < 4; ++k)
			{
				tmp += a->data[i][k] * b->data[k][j];
			}
			result->data[i][j] = tmp;
		}
	}
}

void mat4f_mul_inplace(mat4f_t* result, const mat4f_t* m)
{
	mat4f_t tmp = *result;
	mat4f_mul(result, &tmp, m);
}

void mat4f_transform(const mat4f_t* m, const vec3f_t* in, vec3f_t* out)
{
	out->x = in->x * m->data[0][0] + in->y * m->data[1][0] + in->z * m->data[2][0] + m->data[3][0];
	out->y = in->x * m->data[0][1] + in->y * m->data[1][1] + in->z * m->data[2][1] + m->data[3][1];
	out->z = in->x * m->data[0][2] + in->y * m->data[1][2] + in->z * m->data[2][2] + m->data[3][2];
}

void mat4f_transform_inplace(const mat4f_t* m, vec3f_t* v)
{
	vec3f_t tmp = *v;
	mat4f_transform(m, &tmp, v);
}

bool mat4f_invert(mat4f_t* m)
{
	float s[6];
	s[0] = m->data[0][0] * m->data[1][1] - m->data[1][0] * m->data[0][1];
	s[1] = m->data[0][0] * m->data[1][2] - m->data[1][0] * m->data[0][2];
	s[2] = m->data[0][0] * m->data[1][3] - m->data[1][0] * m->data[0][3];
	s[3] = m->data[0][1] * m->data[1][2] - m->data[1][1] * m->data[0][2];
	s[4] = m->data[0][1] * m->data[1][3] - m->data[1][1] * m->data[0][3];
	s[5] = m->data[0][2] * m->data[1][3] - m->data[1][2] * m->data[0][3];

	float c[6];
	c[0] = m->data[2][0] * m->data[3][1] - m->data[3][0] * m->data[2][1];
	c[1] = m->data[2][0] * m->data[3][2] - m->data[3][0] * m->data[2][2];
	c[2] = m->data[2][0] * m->data[3][3] - m->data[3][0] * m->data[2][3];
	c[3] = m->data[2][1] * m->data[3][2] - m->data[3][1] * m->data[2][2];
	c[4] = m->data[2][1] * m->data[3][3] - m->data[3][1] * m->data[2][3];
	c[5] = m->data[2][2] * m->data[3][3] - m->data[3][2] * m->data[2][3];

	float det = s[0] * c[5] - s[1] * c[4] + s[2] * c[3] + s[3] * c[2] - s[4] * c[1] + s[5] * c[0];
	if (det == 0.0f)
	{
		return false;
	}
	float inv_det = 1.0f / det;

	mat4f_t tmp;
	tmp.data[0][0] = (m->data[1][1] * c[5] - m->data[1][2] * c[4] + m->data[1][3] * c[3])  * inv_det;
	tmp.data[0][1] = (-m->data[0][1] * c[5] + m->data[0][2] * c[4] - m->data[0][3] * c[3]) * inv_det;
	tmp.data[0][2] = (m->data[3][1] * s[5] - m->data[3][2] * s[4] + m->data[3][3] * s[3])  * inv_det;
	tmp.data[0][3] = (-m->data[2][1] * s[5] + m->data[2][2] * s[4] - m->data[2][3] * s[3]) * inv_det;

	tmp.data[1][0] = (-m->data[1][0] * c[5] + m->data[1][2] * c[2] - m->data[1][3] * c[1]) * inv_det;
	tmp.data[1][1] = (m->data[0][0] * c[5] - m->data[0][2] * c[2] + m->data[0][3] * c[1])  * inv_det;
	tmp.data[1][2] = (-m->data[3][0] * s[5] + m->data[3][2] * s[2] - m->data[3][3] * s[1]) * inv_det;
	tmp.data[1][3] = (m->data[2][0] * s[5] - m->data[2][2] * s[2] + m->data[2][3] * s[1])  * inv_det;

	tmp.data[2][0] = (m->data[1][0] * c[4] - m->data[1][1] * c[2] + m->data[1][3] * c[0])  * inv_det;
	tmp.data[2][1] = (-m->data[0][0] * c[4] + m->data[0][1] * c[2] - m->data[0][3] * c[0]) * inv_det;
	tmp.data[2][2] = (m->data[3][0] * s[4] - m->data[3][1] * s[2] + m->data[3][3] * s[0])  * inv_det;
	tmp.data[2][3] = (-m->data[2][0] * s[4] + m->data[2][1] * s[2] - m->data[2][3] * s[0]) * inv_det;

	tmp.data[3][0] = (-m->data[1][0] * c[3] + m->data[1][1] * c[1] - m->data[1][2] * c[0]) * inv_det;
	tmp.data[3][1] = (m->data[0][0] * c[3] - m->data[0][1] * c[1] + m->data[0][2] * c[0])  * inv_det;
	tmp.data[3][2] = (-m->data[3][0] * s[3] + m->data[3][1] * s[1] - m->data[3][2] * s[0]) * inv_det;
	tmp.data[3][3] = (m->data[2][0] * s[3] - m->data[2][1] * s[1] + m->data[2][2] * s[0])  * inv_det;

	*m = tmp;
	return true;
}

void mat4f_make_perspective(mat4f_t* m, float angle, float aspect, float z_near, float z_far)
{
	if (angle == 0.0f)
	{
		return;
	}

	aspect = __max(FLT_EPSILON, aspect);
	z_near = __max(FLT_EPSILON, z_near);

	float tan_half_vfov = tanf(angle * 0.5f);
	float a = 1.0f / tan_half_vfov;

	m->data[0][0] = 1.0f / (tan_half_vfov * aspect);
	m->data[0][1] = 0.0f;
	m->data[0][2] = 0.0f;
	m->data[0][3] = 0.0f;

	m->data[1][0] = 0.0f;
	m->data[1][1] = a;
	m->data[1][2] = 0.0f;
	m->data[1][3] = 0.0f;

	m->data[2][0] = 0.0f;
	m->data[2][1] = 0.0f;
	m->data[2][2] = 0.0f;
	m->data[2][3] = -1.0f;

	m->data[3][0] = 0.0f;
	m->data[3][1] = 0.0f;
	m->data[3][2] = z_near;
	m->data[3][3] = 0.0f;
}

void mat4f_make_lookat(mat4f_t* m, const vec3f_t* eye, const vec3f_t* dir, const vec3f_t* up)
{
	vec3f_t z_vec = vec3f_negate(vec3f_norm(*dir));
	vec3f_t x_vec = vec3f_norm(vec3f_cross(*up, z_vec));
	vec3f_t y_vec = vec3f_norm(vec3f_cross(z_vec, x_vec));

	m->data[0][0] = x_vec.x;
	m->data[1][0] = x_vec.y;
	m->data[2][0] = x_vec.z;

	m->data[0][1] = y_vec.x;
	m->data[1][1] = y_vec.y;
	m->data[2][1] = y_vec.z;

	m->data[0][2] = z_vec.x;
	m->data[1][2] = z_vec.y;
	m->data[2][2] = z_vec.z;

	m->data[0][3] = 0.0f;
	m->data[1][3] = 0.0f;
	m->data[2][3] = 0.0f;

	m->data[3][0] = -vec3f_dot(x_vec, *eye);
	m->data[3][1] = -vec3f_dot(y_vec, *eye);
	m->data[3][2] = -vec3f_dot(z_vec, *eye);
	m->data[3][3] = 1.0f;
}
