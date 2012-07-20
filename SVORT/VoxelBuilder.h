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
	void Build(StaticMesh* mesh, int* dimensions, Shader* meshRenderer);
	void ReloadProgram();
	cl_mem GetVoxelData();
	void SetDebugDrawShader(Shader* shader);
	void SetDebugDraw(bool enabled);
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
		cl_mem inputData;
		cl_command_queue queue;
	} ocl;
	bool debugDraw;
	Shader* debugDrawShader;
};

