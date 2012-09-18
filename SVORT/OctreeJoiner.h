#pragma once

#include <CL\cl.h>
#include "OpenCLStructs.h"

class OctreeJoiner
{
public:
	OctreeJoiner(cl_context context, cl_device_id device);
	~OctreeJoiner(void);
	cl_mem JoinOctrees(HostBlock** octreeData, int* octreeSize);
private:
	struct 
	{
		cl_context context;
		cl_device_id device;
	} ocl;
};

