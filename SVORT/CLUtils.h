#pragma once

#include <CL\cl.h>
#include "Utils.h"

struct oclKernel
{
	cl_kernel kernel;
	cl_program program;
	bool ok;
};

inline oclKernel CreateKernel(const char* source, const char* kernelName, cl_context context, cl_device_id device, const char* filename = "")
{
	oclKernel ret;
	ret.ok = true;
	cl_int resultCL;

	printf("\n\nKernel %s: \n\n", kernelName);

	if (!source)
	{
		printf("Could not find source for kernel %s\n\n", kernelName);
		ret.ok = false;
		return ret;
	}

	char buildOptions[256];
	sprintf(buildOptions, "-I Assets\\CL");
#ifdef INTEL
	sprintf(buildOptions + 12, " -g -s \"C:\\Users\\Owen\\Documents\\Visual Studio 2010\\Projects\\SVORT\\Build\\%s\"", filename);
#endif
	
	printf("Creating program...\n");
	printf("Build options: %s\n", buildOptions);
	ret.program = clCreateProgramWithSource(context, 1, &source, NULL, &resultCL);
	CLGLError(resultCL);

	printf("Building program...\n");
	resultCL = clBuildProgram(ret.program, 1, &device, buildOptions, NULL, NULL);
	CLGLError(resultCL);

	bool compiled = resultCL == CL_SUCCESS;
	
	size_t length;
    resultCL = clGetProgramBuildInfo(ret.program, 
                                    device, 
                                    CL_PROGRAM_BUILD_LOG, 
                                    0, 
                                    NULL, 
                                    &length);
    if(resultCL != CL_SUCCESS) 
        printf("Error: Getting Program build info(clGetProgramBuildInfo)\n");

	char* buffer = (char*)malloc(length);
	memset(buffer, 0, length);
    resultCL = clGetProgramBuildInfo(ret.program, 
                                    device, 
                                    CL_PROGRAM_BUILD_LOG, 
                                    length, 
                                    buffer, 
                                    NULL);

	if (buffer[0])
	{
		if (compiled)
			printf("Warnings: \n%s\n", buffer);
		else
			printf("%s\n", buffer);
	}

	if (!compiled)
	{
		ret.ok = false;			
		return ret;
	}	

	printf("Creating Kernel...\n");
	ret.kernel = clCreateKernel(ret.program, kernelName, &resultCL);
	CLGLError(resultCL);
	if (resultCL != CL_SUCCESS)
		ret.ok = false;
	
	return ret;
}

inline oclKernel CreateKernelFromFile(const char* filename, const char* kernelName, cl_context context, cl_device_id device)
{
	char* source = getSourceFromFile(filename);
	oclKernel ret;
	if (!source)
	{
		ret.ok = false;
		printf("Could not open file %s!\n", filename);
		return ret;
	}
	ret = CreateKernel(source, kernelName, context, device, filename);
	free(source);
	return ret;
}