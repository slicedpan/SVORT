#include "Utils.h"
#include <CL\cl.h>
#include <CL\cl_gl.h>

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

void CLGLError(signed int err)
{
	switch(err)
	{
	case CL_INVALID_CONTEXT:
		printf("CL_INVALID_CONTEXT");
		break;
	case CL_INVALID_VALUE:
		printf("CL_INVALID_VALUE");
		break;
	case CL_INVALID_MIP_LEVEL:
		printf("CL_INVALID_MIP_LEVEL");
		break;
	case CL_INVALID_GL_OBJECT:
		printf("CL_INVALID_GL_OBJECT");
		break;
	case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
		printf("CL_INVALID_IMAGE_FORMAT_DESCRIPTOR");
		break;
	case CL_OUT_OF_HOST_MEMORY:
		printf("CL_OUT_OF_HOST_MEMORY");
		break;
	default:
		printf("Unknown Error");
	}
}