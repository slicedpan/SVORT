#include "OctreeBuilder.h"
#include <stdio.h>
#include <string.h>
#include "Shader.h"
#include "Utils.h"
#include <CL\cl_gl.h>
#include <GL\glew.h>

OctreeBuilder::OctreeBuilder(void)
{
	memset(&ocl, 0, sizeof(ocl));
}

OctreeBuilder::~OctreeBuilder(void)
{
	if (ocl.octKernel)
		clReleaseKernel(ocl.octKernel);
	if (ocl.octProgram)
		clReleaseProgram(ocl.octProgram);
}

void OctreeBuilder::Init(cl_context context, cl_device_id device)
{	
	ocl.context = context;
	ocl.device = device;
	printf("Creating Octree Builder Kernel\n");
	CreateKernel();
}

cl_mem OctreeBuilder::Build(cl_mem inputBuf, int* dimensions)
{

	cl_mem outputBuf;
	cl_int resultCL;

	int width = dimensions[0], height = dimensions[1], depth = dimensions[2];

	printf("Texture dimensions: %dx%dx%d\n", width, height, depth);

	size_t bufSize = width * height * depth / 4;
	printf("Buffer size: %d\n", bufSize);

	glBindTexture(GL_TEXTURE_3D, 0);

	//printf("Creating output buffer...\n");
	//clCreateBuffer(ocl.context, CL_MEM_READ_WRITE, sizeof(CLBlock) * width * depth * height * 0.25, NULL, &resultCL);
	//CLGLError(resultCL);

	return outputBuf;;
}

void OctreeBuilder::CreateKernel()
{

	cl_int resultCL;

	const char* source = getSourceFromFile("Assets/CL/Octree.cl");
	if (source)
	{
		ocl.octProgram = clCreateProgramWithSource(ocl.context, 1, &source, NULL, &resultCL);
		free((char*)source);
	}
	else
	{	
		printf("Could not open program file!\n");
		CLGLError(resultCL);
		printf("\n");
		return;
	}	

	if (resultCL != CL_SUCCESS)
	{
		printf("Error loading program!\n");
		CLGLError(resultCL);
		printf("\n");
	}
	else
		printf("OpenCL program loaded.\n");

	resultCL = clBuildProgram(ocl.octProgram, 1, &ocl.device, "-I Assets/CL/", NULL, NULL);
	if (resultCL != CL_SUCCESS)
	{
		printf("Error building program: ");
		CLGLError(resultCL);

		size_t length;
        resultCL = clGetProgramBuildInfo(ocl.octProgram, 
                                        ocl.device, 
                                        CL_PROGRAM_BUILD_LOG, 
                                        0, 
                                        NULL, 
                                        &length);
        if(resultCL != CL_SUCCESS) 
            printf("InitCL()::Error: Getting Program build info(clGetProgramBuildInfo)\n");

		char* buffer = (char*)malloc(length);
        resultCL = clGetProgramBuildInfo(ocl.octProgram, 
                                        ocl.device, 
                                        CL_PROGRAM_BUILD_LOG, 
                                        length, 
                                        buffer, 
                                        NULL);
        if(resultCL != CL_SUCCESS) 
            printf("InitCL()::Error: Getting Program build info(clGetProgramBuildInfo)\n");
		else
			printf("%s\n", buffer);
	}
	else
	{
		printf("Program built successfully.\n");
	}

	size_t length;
    resultCL = clGetProgramBuildInfo(ocl.octProgram, 
                                    ocl.device, 
                                    CL_PROGRAM_BUILD_LOG, 
                                    0, 
                                    NULL, 
                                    &length);
    if(resultCL != CL_SUCCESS) 
        printf("InitCL()::Error: Getting Program build info(clGetProgramBuildInfo)\n");

	char* buffer = (char*)malloc(length);
    resultCL = clGetProgramBuildInfo(ocl.octProgram, 
                                    ocl.device, 
                                    CL_PROGRAM_BUILD_LOG, 
                                    length, 
                                    buffer, 
                                    NULL);
    if(resultCL != CL_SUCCESS) 
        printf("InitCL()::Error: Getting Program build info(clGetProgramBuildInfo)\n");
	else
		printf("%s\n", buffer);

	ocl.octKernel = clCreateKernel(ocl.octProgram, "CreateOctree", &resultCL);

	if (resultCL != CL_SUCCESS)
	{
		printf("Error creating kernel: ");
		CLGLError(resultCL);
		printf("\n");
	}
}

void OctreeBuilder::ReloadProgram()
{
	CreateKernel();
}