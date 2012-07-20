#include "OctreeBuilder.h"
#include <stdio.h>
#include <string.h>
#include "Shader.h"
#include "Utils.h"
#include <CL\cl_gl.h>
#include <GL\glew.h>
#include "CLUtils.h"

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
	oclKernel k = CreateKernelFromFile("Assets/CL/Octree.cl", "CreateOctree", ocl.context, ocl.device);
	if (k.ok)
	{
		ocl.octKernel = k.kernel;
		ocl.octProgram = k.program;
	}	
}

void OctreeBuilder::ReloadProgram()
{
	CreateKernel();
}