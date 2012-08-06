#pragma once

#include <CL\cl.h>

class StaticMesh;
class Shader;

class VoxelBuilder
{
public:
	VoxelBuilder(void);
	~VoxelBuilder(void);
	void Init(cl_context context, cl_device_id device);
	void Build(StaticMesh* mesh, int* dimensions, Shader* meshRenderer, cl_mem octreeInfo, cl_mem normalLookup);
	void ReloadProgram();
	cl_mem GetVoxelData();
	void SetDebugDrawShader(Shader* shader);
	void SetDebugDraw(bool enabled);
	void GenerateMipmaps();
	int GetMaxNumMips();
	void Cleanup();
private:
	void CreateKernels();
	struct
	{
		cl_context context;
		cl_kernel fillKernel;
		cl_program fillProgram;
		cl_device_id device;
		cl_kernel mipKernel;
		cl_program mipProgram;
		cl_mem voxelData;
		cl_mem inputData[2];
		cl_command_queue queue;
		cl_mem octreeInfo;		
	} ocl;
	struct
	{
		int numVoxels;
		int numLeafVoxels;
		int numLevels;
		int pad;
	} octInfo;
	bool debugDraw;
	Shader* debugDrawShader;
	int maxMips;
	int dims[3];
};

