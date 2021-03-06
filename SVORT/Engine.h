#pragma once
#include "GLFW\GLFWEngine.h"
#include "VolumeData.h"
#include "CL\cl.h"
#include "VoxelBuilder.h"
#include "OctreeBuilder.h"

class ShaderManager;
class Camera;
class CameraController;

class FBOManager;
class StaticMesh;
class VoxelOctree;

class Vec3;

class Engine : public GLFWEngine
{
public:
	Engine(WindowSettings& w);
	virtual ~Engine(void);
	void Setup();
	void SetupOpenCL();
	void Display();
	void Update(TimeInfo& timeInfo);
	void KeyPressed(int code);
	void KeyReleased(int code);
private:
	ShaderManager& shaders;
	FBOManager& fbos;
	void Init3DTexture();   
	void UpdateCL();
	void CreateRTKernel();
	void CleanupCL();
	void DebugDrawVoxelData();
	void PopulateNormalLookup();
	unsigned int tex3D;
	Camera* cam;
	CameraController* camControl;
	float zCoord;
	StaticMesh* mesh1;
	StaticMesh* mesh2;
	bool drawQuad;
	bool drawMesh1;
	bool drawVol;
	VoxelOctree* vo;
	bool clDraw;
	bool voxels;
	OctreeBuilder octreeBuilder;
	VoxelBuilder voxelBuilder;
	struct
	{
		cl_context context;
		cl_device_id* devices;
		cl_uint deviceNum;
		cl_mem output;
		cl_mem input;
		cl_command_queue queue;
		cl_program rtProgram;
		cl_kernel rtKernel;
		cl_program voxProgram;
		cl_kernel voxKernel;
		cl_mem paramBuffer;
		cl_mem rtCounterBuffer;	
		cl_mem volumeData;
		cl_mem octInfo;
		cl_mem voxInfo;
		cl_program octDrawProgram;
		cl_kernel octDrawKernel;		
		cl_mem octreeData;
		cl_kernel octRTKernel;
		cl_program octRTProgram;
		cl_mem normalLookup;
	} ocl;

	unsigned int* outputImage;
	
	struct
	{
		float invWorldView[16];
		int sizeX;
		int sizeY; 
		int sizeZ;
		int sizeW;
		float invSize[4];
		float lightPos[4];
		float camPos[4];
	} RTParams;
	Vec3* lightPos;
	float averageIterations;
	int mipLevel;
	int counters[2];
};

