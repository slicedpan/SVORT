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

	if (err == CL_SUCCESS)
		printf("Success\n");
	else
	{
		printf("Error: ");
		CLGLErrorName(err);
		printf("\n");
	}
}

void CLGLErrorName(signed int err)
{
	switch(err)
	{
	case CL_SUCCESS:
		printf("CL_SUCCESS");
		break;
	case CL_DEVICE_NOT_FOUND:
		printf("CL_DEVICE_NOT_FOUND");
		break;
	case CL_DEVICE_NOT_AVAILABLE:
		printf("CL_DEVICE_NOT_AVAILABLE");
		break;
	case CL_COMPILER_NOT_AVAILABLE:
		printf("CL_COMPILER_NOT_AVAILABLE");
		break;
	case CL_MEM_OBJECT_ALLOCATION_FAILURE:
		printf("CL_MEM_OBJECT_ALLOCATION_FAILURE");
		break;
	case CL_OUT_OF_RESOURCES:
		printf("CL_OUT_OF_RESOURCES");
		break;
	case CL_OUT_OF_HOST_MEMORY:
		printf("CL_OUT_OF_HOST_MEMORY");
		break;
	case CL_PROFILING_INFO_NOT_AVAILABLE:
		printf("CL_PROFILING_INFO_NOT_AVAILABLE");
		break;
	case CL_MEM_COPY_OVERLAP:
		printf("CL_MEM_COPY_OVERLAP");
		break;
	case CL_IMAGE_FORMAT_MISMATCH:
		printf("CL_IMAGE_FORMAT_MISMATCH");
		break;
	case CL_IMAGE_FORMAT_NOT_SUPPORTED:
		printf("CL_IMAGE_FORMAT_NOT_SUPPORTED");
		break;
	case CL_BUILD_PROGRAM_FAILURE:
		printf("CL_BUILD_PROGRAM_FAILURE");
		break;
	case CL_MAP_FAILURE:
		printf("CL_MAP_FAILURE");
		break;
	case CL_MISALIGNED_SUB_BUFFER_OFFSET:
		printf("CL_MISALIGNED_SUB_BUFFER_OFFSET");
		break;
	case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
		printf("CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST");
		break;
	case CL_INVALID_VALUE:
		printf("CL_INVALID_VALUE");
		break;
	case CL_INVALID_DEVICE_TYPE:
		printf("CL_INVALID_DEVICE_TYPE");
		break;
	case CL_INVALID_PLATFORM:
		printf("CL_INVALID_PLATFORM");
		break;
	case CL_INVALID_DEVICE:
		printf("CL_INVALID_DEVICE");
		break;
	case CL_INVALID_CONTEXT:
		printf("CL_INVALID_CONTEXT");
		break;
	case CL_INVALID_QUEUE_PROPERTIES:
		printf("CL_INVALID_QUEUE_PROPERTIES");
		break;
	case CL_INVALID_COMMAND_QUEUE:
		printf("CL_INVALID_COMMAND_QUEUE");
		break;
	case CL_INVALID_HOST_PTR:
		printf("CL_INVALID_HOST_PTR");
		break;
	case CL_INVALID_MEM_OBJECT:
		printf("CL_INVALID_MEM_OBJECT");
		break;
	case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
		printf("CL_INVALID_IMAGE_FORMAT_DESCRIPTOR");
		break;
	case CL_INVALID_IMAGE_SIZE:
		printf("CL_INVALID_IMAGE_SIZE");
		break;
	case CL_INVALID_SAMPLER:
		printf("CL_INVALID_SAMPLER");
		break;
	case CL_INVALID_BINARY:
		printf("CL_INVALID_BINARY");
		break;
	case CL_INVALID_BUILD_OPTIONS:
		printf("CL_INVALID_BUILD_OPTIONS");
		break;
	case CL_INVALID_PROGRAM:
		printf("CL_INVALID_PROGRAM");
		break;
	case CL_INVALID_PROGRAM_EXECUTABLE:
		printf("CL_INVALID_PROGRAM_EXECUTABLE");
		break;
	case CL_INVALID_KERNEL_NAME:
		printf("CL_INVALID_KERNEL_NAME");
		break;
	case CL_INVALID_KERNEL_DEFINITION:
		printf("CL_INVALID_KERNEL_DEFINITION");
		break;
	case CL_INVALID_KERNEL:
		printf("CL_INVALID_KERNEL");
		break;
	case CL_INVALID_ARG_INDEX:
		printf("CL_INVALID_ARG_INDEX");
		break;
	case CL_INVALID_ARG_VALUE:
		printf("CL_INVALID_ARG_VALUE");
		break;
	case CL_INVALID_ARG_SIZE:
		printf("CL_INVALID_ARG_SIZE");
		break;
	case CL_INVALID_KERNEL_ARGS:
		printf("CL_INVALID_KERNEL_ARGS");
		break;
	case CL_INVALID_WORK_DIMENSION:
		printf("CL_INVALID_WORK_DIMENSION");
		break;
	case CL_INVALID_WORK_GROUP_SIZE:
		printf("CL_INVALID_WORK_GROUP_SIZE");
		break;
	case CL_INVALID_WORK_ITEM_SIZE:
		printf("CL_INVALID_WORK_ITEM_SIZE");
		break;
	case CL_INVALID_GLOBAL_OFFSET:
		printf("CL_INVALID_GLOBAL_OFFSET");
		break;
	case CL_INVALID_EVENT_WAIT_LIST:
		printf("CL_INVALID_EVENT_WAIT_LIST");
		break;
	case CL_INVALID_EVENT:
		printf("CL_INVALID_EVENT");
		break;
	case CL_INVALID_OPERATION:
		printf("CL_INVALID_OPERATION");
		break;
	case CL_INVALID_GL_OBJECT:
		printf("CL_INVALID_GL_OBJECT");
		break;
	case CL_INVALID_BUFFER_SIZE:
		printf("CL_INVALID_BUFFER_SIZE");
		break;
	case CL_INVALID_MIP_LEVEL:
		printf("CL_INVALID_MIP_LEVEL");
		break;
	case CL_INVALID_GLOBAL_WORK_SIZE:
		printf("CL_INVALID_GLOBAL_WORK_SIZE");
		break;
	case CL_INVALID_PROPERTY:
		printf("CL_INVALID_PROPERTY");
		break;
	}
}