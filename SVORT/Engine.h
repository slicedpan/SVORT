#pragma once
#include "GLFW\GLFWEngine.h"
#include "VolumeData.h"
#include "CL\cl.h"
#include "OctreeBuilder.h"
#include "VoxelBuilder.h"

class ShaderManager;
class Camera;
class CameraController;

class FBOManager;
class StaticMesh;
class VoxelOctree;



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
		
	} ocl;
	
	struct
	{
		float invWorldView[16];
		int sizeX;
		int sizeY; 
		int sizeZ;
		int sizeW;
		float invSize[4];
	} RTParams;
	float averageIterations;
	int mipLevel;
};

