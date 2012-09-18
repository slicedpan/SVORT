#include "OctreeJoiner.h"
#include "Utils.h"
#include <cstring>

OctreeJoiner::OctreeJoiner(cl_context context, cl_device_id device)
{
	ocl.device = device;
	ocl.context = context;
}

OctreeJoiner::~OctreeJoiner(void)
{
}

cl_mem OctreeJoiner::JoinOctrees(HostBlock** octreeData, int* octreeSize)
{
	cl_mem outputBuffer;
	cl_command_queue queue;	

	size_t totalSize = 0;
	int nodeCount = 0;
	int sizes[8];
	for (int i = 0; i < 8; ++i)
	{
		sizes[i] = octreeSize[i] - 1;
		nodeCount += sizes[i];
	}
	totalSize = (nodeCount + 9) * sizeof(HostBlock);	//add one for the root node, and eight for the next level
	HostBlock root;
	HostBlock nextLevel[8];
	root.child = 1;
	root.a = 255;
	HostBlock* blockPtr;

	nodeCount = 9;

	HostBlock* octrees[8];

	int r = 0, g = 0, b = 0;

	for (int i = 0; i < 8; ++i)
	{			
		blockPtr = (HostBlock*)octreeData[i];
		memcpy(nextLevel + i, blockPtr, sizeof(HostBlock));			
		nextLevel[i].child = nodeCount - (1 + i);
		octrees[i] = octreeData[i] + 1;	//set pointer to ignore first block
		r += nextLevel[i].r;
		g += nextLevel[i].g;
		b += nextLevel[i].b;
		nodeCount += octreeSize[i] - 1;	
	}

	root.r = r / 8;
	root.g = g / 8;
	root.b = b / 8;

	cl_int resultCL;
	printf("Creating output buffer for octree...\n");
	outputBuffer = clCreateBuffer(ocl.context, CL_MEM_READ_WRITE, totalSize, NULL, &resultCL);	
	CLGLError(resultCL);
	printf("Allocated %dKB\n", totalSize / 1024);

	printf("Creating command queue for octree joiner...\n");
	queue = clCreateCommandQueue(ocl.context, ocl.device, 0, &resultCL);
	CLGLError(resultCL);

	size_t offset = 0;

	clEnqueueWriteBuffer(queue, outputBuffer, false, 0, sizeof(HostBlock), &root, 0, NULL, NULL);
	offset = sizeof(HostBlock);
	clEnqueueWriteBuffer(queue, outputBuffer, false, offset, sizeof(HostBlock) * 8, nextLevel, 0, NULL, NULL);
	offset += sizeof(HostBlock) * 8;
	for (int i = 0; i < 8; ++i)
	{
		clEnqueueWriteBuffer(queue, outputBuffer, false, offset, sizeof(HostBlock) * sizes[i], octrees[i], 0, NULL, NULL);
		offset += sizeof(HostBlock) * sizes[i];
	}
	clFinish(queue);
	clReleaseCommandQueue(queue);

	return outputBuffer;
}
