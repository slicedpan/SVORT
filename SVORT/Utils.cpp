#include "Utils.h"

Mat4 Orthographic(float left, float right, float bottom, float top, float near, float far)
{
	float tx = - (right + left) / (right - left);
	float ty = - (top + bottom) / (top - bottom);
	float tz = - (far + near) / (far - near);
	float m00 = 2 / (right - left);
	float m11 = 2 / (top - bottom);
	float m22 = 2 / (near - far);
	return Mat4(m00, 0, 0, 0, 0, m11, 0, 0, 0, 0, m22, 0, tx, ty, tz, 1);
}
