#pragma once
#include "GLFW\GLFWEngine.h"
#include "VolumeData.h"
#include "CL\cl.h"
<<<<<<< HEAD
#include "OctreeBuilder.h"
=======
>>>>>>> c07c15a20685d2099ccd41a54596741324ad541f

class ShaderManager;
class Camera;
class CameraController;

class FBOManager;
class StaticMesh;
class VoxelOctree;



class Engine : public GLFWEngine
{
public:
<<<<<<< HEAD
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
=======
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
>>>>>>> c07c15a20685d2099ccd41a54596741324ad541f
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
<<<<<<< HEAD
		cl_program octreeBuildProgram;
		cl_kernel octreeBuildKernel;
		cl_mem paramBuffer;
	} ocl;
	struct
	{		
=======
		cl_mem paramBuffer;
	} ocl;
	struct
	{
>>>>>>> c07c15a20685d2099ccd41a54596741324ad541f
		float invWorldView[16];
		int sizeX;
		int sizeY; 
		int sizeZ;
<<<<<<< HEAD
		int sizeW;
		float invSize[4];
=======
		float invSize[3];
>>>>>>> c07c15a20685d2099ccd41a54596741324ad541f
	} RTParams;
};

