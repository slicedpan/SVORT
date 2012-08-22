#pragma once

#include <CL\cl.h>


struct VoxelInfo
{
	cl_uint numVoxels;
	cl_uint numLeafVoxels;
	cl_uint numLevels;
	cl_uint pad;
};

struct CLBlock
{
	cl_uint data;
	cl_uint colour;
	cl_uint normal;
};

struct HostBlock
{
	unsigned child : 24;
	unsigned valid : 8;

	unsigned r : 8;	
	unsigned g : 8;
	unsigned b : 8;
	unsigned a : 8;

	unsigned nx : 8;
	unsigned ny : 8;
	unsigned nz : 8;
	unsigned junk : 8;
};

typedef struct 
{
	cl_uint inOffset;
	cl_uint outOffset;
	cl_uint level;
	cl_uint pad;
} OctParams;