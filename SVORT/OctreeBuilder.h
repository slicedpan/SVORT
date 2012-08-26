#pragma once

#include <CL\cl.h>
#include <vector>
#include "OpenCLStructs.h"

class OctreeBuilder
{
public:
	OctreeBuilder(void);
	~OctreeBuilder(void);
	void Init(cl_context context, cl_device_id device);
	void Build(cl_mem inputBuf, int* dimensions, cl_mem octreeInfo, cl_mem voxInfo);
	void ReloadProgram();	
	cl_mem GetOctreeData();
private:
	void CreateKernel();
	struct
	{
		cl_context context;
		cl_kernel octKernel;
		cl_program octProgram;
		cl_device_id device;
		cl_command_queue queue;
		cl_mem octData;
	} ocl;
	void* octDataPtr;
	std::vector<HostBlock> blocks;
};

