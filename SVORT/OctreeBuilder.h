#pragma once

#include <CL\cl.h>

struct CLBlock
{
	unsigned int data;
	unsigned int color;
};

struct HostBlock
{
	unsigned child : 16;
	unsigned valid : 8;
	unsigned leaf : 8;
	unsigned r : 8;
	unsigned g : 8;
	unsigned b : 8;
	unsigned a : 8;
};

class OctreeBuilder
{
public:
	OctreeBuilder(void);
	~OctreeBuilder(void);
	void Init(cl_context context, cl_device_id device);
	cl_mem Build(cl_mem inputBuf, int* dimensions);
	void ReloadProgram();	
private:
	void CreateKernel();
	struct
	{
		cl_context context;
		cl_kernel octKernel;
		cl_program octProgram;
		cl_device_id device;
		cl_kernel mipKernel;
		cl_program mipProgram;
	} ocl;
};

