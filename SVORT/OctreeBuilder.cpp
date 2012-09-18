#include "OctreeBuilder.h"
#include <stdio.h>
#include <string.h>
#include "Shader.h"
#include "Utils.h"
#include <CL\cl_gl.h>
#include <GL\glew.h>
#include "CLUtils.h"
#include "OpenCLStructs.h"
#include <vector>
#include "CLDefsBegin.h"
#include "Octree.h"
#include "CLDefsEnd.h"

using namespace SVO;

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
	cl_int resultCL;
	printf("Creating command queue for Octree Builder\n");
	ocl.queue = clCreateCommandQueue(ocl.context, device, 0, &resultCL);
	CLGLError(resultCL);
	CreateKernel();
}

void OctreeBuilder::Build(cl_mem inputBuf, int* dimensions, cl_mem octreeInfo, cl_mem voxInfo)
{
	int width = dimensions[0], height = dimensions[1], depth = dimensions[2];
	OctreeInfo oi;
	VoxelInfo vi;
	clEnqueueReadBuffer(ocl.queue, octreeInfo, false, 0, sizeof(OctreeInfo), &oi, 0, NULL, NULL);
	clEnqueueReadBuffer(ocl.queue, voxInfo, false, 0, sizeof(VoxelInfo), &vi, 0, NULL, NULL);
	clFinish(ocl.queue);

	cl_int resultCL;
	printf("Creating octree data buffer, size %dKB...\n", (sizeof(CLBlock) * vi.numLeafVoxels) / 1024);
#ifdef INTEL
	octDataPtr = malloc(sizeof(CLBlock) * vi.numLeafVoxels * 2);
	ocl.octData = clCreateBuffer(ocl.context, CL_MEM_USE_HOST_PTR, sizeof(CLBlock) * vi.numLeafVoxels * 2, octDataPtr, &resultCL);
#else
	ocl.octData = clCreateBuffer(ocl.context, CL_MEM_READ_WRITE, sizeof(CLBlock) * vi.numLeafVoxels, NULL, &resultCL);
#endif
	CLGLError(resultCL);
	size_t workDim[3];

	printf("Creating counter buffer...\n");
	cl_mem counters = clCreateBuffer(ocl.context, CL_MEM_READ_WRITE, sizeof(int) * 4, NULL, &resultCL);	
	CLGLError(resultCL);

	int init[4];
	memset(init, 0, sizeof(init));
	init[0] = 1;	//this is used as the next available block position
	clEnqueueWriteBuffer(ocl.queue, counters, false, 0, sizeof(int) * 4, init, 0, NULL, NULL);
	clFinish(ocl.queue);

	clSetKernelArg(ocl.octKernel, 0, sizeof(cl_mem), &inputBuf);
	clSetKernelArg(ocl.octKernel, 1, sizeof(cl_mem), &ocl.octData);
	clSetKernelArg(ocl.octKernel, 2, sizeof(cl_mem), &octreeInfo);
	clSetKernelArg(ocl.octKernel, 3, sizeof(cl_mem), &counters);

	cl_uint baseSize = 1;	

	for (int i = 1; i < oi.numLevels; ++i)
	{
		workDim[0] = baseSize;
		workDim[1] = baseSize;
		workDim[2] = baseSize;				
		OctParams op;
		op.inOffset = oi.levelOffset[oi.numLevels - i - 1];
		op.level = i - 1;
		clSetKernelArg(ocl.octKernel, 4, sizeof(OctParams), &op);
		clEnqueueNDRangeKernel(ocl.queue, ocl.octKernel, 3, NULL, workDim, NULL, 0, NULL, NULL);
		clEnqueueReadBuffer(ocl.queue, counters, false, 0, sizeof(int) * 4, init, 0, NULL, NULL);
		clFinish(ocl.queue);
		baseSize *= 2;
	}

	
	
	clFinish(ocl.queue);
	clReleaseMemObject(counters);
	
	//*	
	
	blocks.resize(vi.numLeafVoxels);
	clEnqueueReadBuffer(ocl.queue, ocl.octData, true, 0, sizeof(CLBlock) * vi.numLeafVoxels, &blocks[0], 0, NULL, NULL);


	//*/

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

cl_mem OctreeBuilder::GetOctreeData()
{
	return ocl.octData;
}
