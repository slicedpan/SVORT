//#define CL_USE_DEPRECATED_OPENCL_1_1_APIS

#include "Engine.h"
#include "GLFW\GLFWEngine.h"
#include <stdlib.h>
#include <time.h>
#include "Colour.h"
#include "FBOManager.h"
#include "ShaderManager.h"
#include "WindowSettings.h"
#include "QuadDrawer.h"
#include "Camera\Camera.h"
#include "Camera\CameraController.h"
#include "CubeDrawer.h"
#include <GL\glfw.h>
#include "BasicObjects.h"
#include <boost\format.hpp>
#include "Utils.h"
#include "StaticMesh.h"
#include <CL\cl_gl.h>
#include "GLGUI\Primitives.h"
#include "CLUtils.h"
#include "OpenCLStructs.h"
#include "CLDefsBegin.h"
#include "Octree.h"
#include "../Build/Assets/CL/colour.h"
#include "CLDefsEnd.h"

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace SVO;

struct _vs
{
	int x;
	int y;
	int z;
} voxelSize;



int RTWidth;
int RTHeight;

Engine::Engine(WindowSettings& w) : GLFWEngine(w),
	fbos(FBOManager::GetSingleton()),
	shaders(ShaderManager::GetSingleton())
{
	srand(time(0));
}

Engine::~Engine(void)
{
	delete camControl;
	delete cam;	
	if (ocl.devices)
		free(ocl.devices);
	CleanupCL();
}

void Engine::CleanupCL()
{
	if (ocl.input)
		clReleaseMemObject(ocl.input);
	if (ocl.output)
		clReleaseMemObject(ocl.output);
	if (ocl.paramBuffer)
		clReleaseMemObject(ocl.paramBuffer);
	if (ocl.rtKernel)
		clReleaseKernel(ocl.rtKernel);
	if (ocl.rtProgram)
		clReleaseProgram(ocl.rtProgram);
	if (ocl.queue)
		clReleaseCommandQueue(ocl.queue);
	if (ocl.context)
		clReleaseContext(ocl.context);
}

void Engine::Init3DTexture()
{
	StaticMesh* mesh = mesh1;
	if (!drawMesh1)
		mesh = mesh2;  	
#ifdef INTEL
	voxelBuilder.BuildCPU(mesh, &voxelSize.x, shaders["Basic"], ocl.voxInfo, ocl.normalLookup);
#else
	voxelBuilder.BuildGL(mesh, &voxelSize.x, shaders["Basic"], ocl.voxInfo, ocl.normalLookup);
#endif
	ocl.volumeData = voxelBuilder.GetVoxelData();	

	OctreeInfo oi;
	oi.numLevels = voxelBuilder.GetMaxNumMips();
	oi.levelSize[0] = voxelSize.x;
	oi.levelOffset[0] = 0;
	for (int i = 1; i < oi.numLevels; ++i)
	{
		oi.levelSize[i] = oi.levelSize[i - 1] / 2;
		oi.levelOffset[i] = oi.levelOffset[i - 1] + (oi.levelSize[i - 1] * oi.levelSize[i - 1] * oi.levelSize[i - 1]);
	}
	clEnqueueWriteBuffer(ocl.queue, ocl.octInfo, false, 0, sizeof(OctreeInfo), &oi, 0, NULL, NULL);
	clFinish(ocl.queue);	

	if (ocl.voxKernel)
		clSetKernelArg(ocl.voxKernel, 1, sizeof(cl_mem), &ocl.volumeData);
}

void Engine::Setup()
{	
	

	clDraw = false;
	voxels = false;

	RTWidth = Window.Width * 0.75;
	RTHeight = Window.Height;

	mipLevel = 3;
#ifdef INTEL
	voxelSize.x = 16;
	voxelSize.y = 16;
	voxelSize.z = 16;
#else
	voxelSize.x = 16;
	voxelSize.y = 16;
	voxelSize.z = 16;
#endif
	
	glGenTextures(1, &tex3D);
	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, tex3D);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);   	

	drawVol = false;
	memset(&ocl, 0, sizeof(ocl));

	ocl.devices = NULL;

	cam = new Camera();
	cam->Position[2] += 20.0f;
	camControl = new CameraController();
	camControl->SetCamera(cam);
	camControl->MaxSpeed = 0.4f;
	cam->SetAspectRatio(1.0f);

	shaders.Add(new Shader("Assets/Shaders/vol.vert","Assets/Shaders/vol.frag", "DVR"));
	shaders.Add(new Shader("Assets/Shaders/copy.vert", "Assets/Shaders/copy.frag", "Copy"));
	shaders.Add(new Shader("Assets/Shaders/testred.vert", "Assets/Shaders/testred.frag", "Testred"));
	shaders.Add(new Shader("Assets/Shaders/copy3D.vert", "Assets/Shaders/copy3D.frag", "Copy3D"));
	shaders.Add(new Shader("Assets/Shaders/basic.vert", "Assets/Shaders/basic.frag", "Basic"));
	shaders.Add(new Shader("Assets/Shaders/SimpleRT.vert", "Assets/Shaders/SimpleRT.frag", "SimpleRT"));
	shaders.Add(new Shader("Assets/Shaders/VolRT.vert", "Assets/Shaders/VolRT.frag", "VolRT"));
	shaders.CompileShaders();

	voxelBuilder.SetDebugDrawShader(shaders["Copy"]);
	voxelBuilder.SetDebugDraw(true);

	printf("Creating FBOS... ");

	fbos.AddFBO(new FrameBufferObject(RTWidth, RTHeight, 0, 0, GL_RGBA, GL_TEXTURE_2D, "RayTrace"));
	fbos["RayTrace"]->AttachTexture("colour", GL_LINEAR, GL_LINEAR);

	printf("done.\n");

	mesh1 = new StaticMesh();
	mesh1->LoadObj("Assets/Meshes/bunny.obj", false, false, false);
	mesh2 = new StaticMesh();
	mesh2->LoadObj("Assets/Meshes/teapot.obj", false, false, false);

	drawQuad = false;
	drawMesh1 = true;
	
	zCoord = 1.35f; 	
	SetupOpenCL();
	Init3DTexture();
	octreeBuilder.Build(ocl.volumeData, &voxelSize.x, ocl.octInfo, ocl.voxInfo);
	ocl.octreeData = octreeBuilder.GetOctreeData();
	if (ocl.octRTKernel)
		clSetKernelArg(ocl.octRTKernel, 1, sizeof(cl_mem), &ocl.octreeData);
	CreateRTKernel();

}

void Engine::CreateRTKernel()
{

	if (ocl.rtKernel)
		clReleaseKernel(ocl.rtKernel);
	if (ocl.rtProgram)
		clReleaseProgram(ocl.rtProgram);

	oclKernel k = CreateKernelFromFile("Assets\\CL\\RT.cl", "VolRT", ocl.context, ocl.devices[0]);	

	if (k.ok)
	{		
		ocl.rtKernel = k.kernel;
		ocl.rtProgram = k.program;	
		clSetKernelArg(ocl.rtKernel, 0, sizeof(cl_mem), &ocl.output);
		if (ocl.volumeData)
			clSetKernelArg(ocl.rtKernel, 1, sizeof(cl_mem), &ocl.volumeData);
		clSetKernelArg(ocl.rtKernel, 2, sizeof(cl_mem), &ocl.paramBuffer);
		clSetKernelArg(ocl.rtKernel, 3, sizeof(cl_mem), &ocl.rtCounterBuffer);
	}

	if (ocl.octRTKernel)
		clReleaseKernel(ocl.octRTKernel);
	if (ocl.octRTProgram)
		clReleaseProgram(ocl.octRTProgram);

	k = CreateKernelFromFile("Assets\\CL\\OctRT.cl", "OctRT", ocl.context, ocl.devices[0]);

	if (k.ok)
	{
		ocl.octRTKernel = k.kernel;
		ocl.octRTProgram = k.program;
		clSetKernelArg(ocl.octRTKernel, 0, sizeof(cl_mem), &ocl.output);
		if (ocl.octreeData)
			clSetKernelArg(ocl.octRTKernel, 1, sizeof(cl_mem), &ocl.octreeData);
		clSetKernelArg(ocl.octRTKernel, 2, sizeof(cl_mem), &ocl.paramBuffer);
		clSetKernelArg(ocl.octRTKernel, 3, sizeof(cl_mem), &ocl.rtCounterBuffer);
	}

	if (ocl.voxKernel)
		clReleaseKernel(ocl.voxKernel);
	if (ocl.voxProgram)
		clReleaseProgram(ocl.voxProgram);

	k = CreateKernelFromFile("Assets/CL/DrawVox.cl", "DrawVoxels", ocl.context, ocl.devices[0]);

	if (k.ok)
	{
		ocl.voxKernel = k.kernel;
		ocl.voxProgram = k.program;
		clSetKernelArg(ocl.voxKernel, 0, sizeof(cl_mem), &ocl.output);
		if (ocl.volumeData)
			clSetKernelArg(ocl.voxKernel, 1, sizeof(cl_mem), &ocl.volumeData);
	}

	k = CreateKernelFromFile("Assets/CL/DrawOct.cl", "DrawOctree", ocl.context, ocl.devices[0]);

	if (k.ok)
	{
		ocl.octDrawKernel = k.kernel;
		ocl.octDrawProgram = k.program;
		clSetKernelArg(ocl.octDrawKernel, 0, sizeof(cl_mem), &ocl.output);
		if (ocl.octreeData)
			clSetKernelArg(ocl.octDrawKernel, 1, sizeof(cl_mem), &ocl.octreeData);
	}

}

void Engine::SetupOpenCL()
{

	cl_int resultCL;

#pragma region CLSetup	

	cl_uint numPlatforms;
	cl_platform_id targetPlatform = NULL;

	resultCL = clGetPlatformIDs(0, NULL, &numPlatforms);

	cl_platform_id* allPlatforms = (cl_platform_id*) malloc(numPlatforms * sizeof(cl_platform_id));

	resultCL = clGetPlatformIDs(numPlatforms, allPlatforms, NULL);
    if (resultCL != CL_SUCCESS)
        throw (std::string("InitCL()::Error: Getting platform ids (clGetPlatformIDs)"));



    char pbuff[128];

	for (int i = 0; i < numPlatforms; ++i)
	{
		clGetPlatformInfo(allPlatforms[i], CL_PLATFORM_NAME, sizeof(pbuff), pbuff, NULL);
#ifdef INTEL
		if (strstr(pbuff, "Intel"))
			targetPlatform = allPlatforms[i];
#else
		if (strstr(pbuff, "AMD"))
			targetPlatform = allPlatforms[i];
#endif
	}

	clGetPlatformInfo(targetPlatform, CL_PLATFORM_NAME, sizeof(pbuff), pbuff, NULL);

	printf("\n\nUsing platform:\n%s\n", pbuff);

    resultCL = clGetPlatformInfo(targetPlatform, CL_PLATFORM_VENDOR, sizeof(pbuff), pbuff, NULL);

	printf("Vendor: %s\n\n\n", pbuff);		

    if (resultCL != CL_SUCCESS)
        throw (std::string("InitCL()::Error: Getting platform info (clGetPlatformInfo)"));    

	
	ocl.devices = (cl_device_id*)malloc(sizeof(cl_device_id) * 16);
#ifdef INTEL
	resultCL = clGetDeviceIDs(targetPlatform, CL_DEVICE_TYPE_CPU, 16, ocl.devices, &ocl.deviceNum);
#else
	resultCL = clGetDeviceIDs(targetPlatform, CL_DEVICE_TYPE_GPU, 16, ocl.devices, &ocl.deviceNum);
#endif

	printf("\n");


	for (int i = 0; i < ocl.deviceNum; ++i)
	{
		cl_device_type type;
		char nameBuf[256];
		cl_ulong memSize;
		int iMem;

		clGetDeviceInfo(ocl.devices[i], CL_DEVICE_NAME, sizeof(nameBuf), nameBuf, NULL);
		clGetDeviceInfo(ocl.devices[i], CL_DEVICE_TYPE, sizeof(cl_device_type), &type, NULL);
		clGetDeviceInfo(ocl.devices[i], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), &memSize, NULL);
		char typeStr[32];
		if (type & CL_DEVICE_TYPE_CPU)
			sprintf(typeStr, "CL_DEVICE_TYPE_CPU");
		if (type & CL_DEVICE_TYPE_GPU)	//we just want GPU(s)
		{			
			sprintf(typeStr, "CL_DEVICE_TYPE_GPU");
		}
		if (type & CL_DEVICE_TYPE_ACCELERATOR)
			sprintf(typeStr, "CL_DEVICE_TYPE_ACCELERATOR");
		iMem = (unsigned int)(memSize / 1024);
		printf("Device %d:\n%s\nType: %s\nMemory : %d KB\n\n", i, nameBuf, typeStr, iMem);

		int maxArgs;

		clGetDeviceInfo(ocl.devices[i], CL_DEVICE_MAX_CONSTANT_ARGS, sizeof(int), &maxArgs, NULL);
		printf("Max constant args: %d\n", maxArgs);
		clGetDeviceInfo(ocl.devices[i], CL_DEVICE_MAX_PARAMETER_SIZE, sizeof(int), &maxArgs, NULL);
		printf("Max argument size: %dB\n", maxArgs);


	}

#ifdef _WIN32
	cl_context_properties properties[] = {
		CL_GL_CONTEXT_KHR, (cl_context_properties) wglGetCurrentContext(),
		CL_WGL_HDC_KHR, (cl_context_properties) wglGetCurrentDC(),
		CL_CONTEXT_PLATFORM, (cl_context_properties) targetPlatform,
		0};
#endif	

	ocl.context = clCreateContext(properties, ocl.deviceNum, ocl.devices, NULL, NULL, &resultCL);

	if (resultCL != CL_SUCCESS)
		printf("Error creating OpenCL context!\n");
	else
		printf("OpenCL context created successfully.\n");	

	ocl.paramBuffer = clCreateBuffer(ocl.context, CL_MEM_READ_ONLY, sizeof(RTParams), NULL, &resultCL);
	if (resultCL != CL_SUCCESS)
		printf("Error allocating params buffer!\n");
	else
		printf("Allocated params buffer successfully.\n");

	ocl.queue = clCreateCommandQueue(ocl.context, ocl.devices[0], 0, &resultCL);
	
	if (resultCL != CL_SUCCESS)
		printf("\nCould not create OpenCL command queue!\n");
	else
		printf("\nCreated OpenCL command queue.\n");

	printf("Creating counter buffer...\n");
	ocl.rtCounterBuffer = clCreateBuffer(ocl.context, CL_MEM_WRITE_ONLY, sizeof(int) * 2, NULL, &resultCL);
	CLGLError(resultCL);

#ifdef INTEL
	printf("Creating output buffer...\n");
	cl_image_format colourFormat;
	colourFormat.image_channel_data_type = CL_UNORM_INT8;
	colourFormat.image_channel_order = CL_RGBA;
	outputImage = (unsigned int*)malloc(sizeof(unsigned int) * RTWidth * RTHeight);
	ocl.output = clCreateImage2D(ocl.context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, &colourFormat, RTWidth, RTHeight, 0, outputImage, &resultCL);
	CLGLError(resultCL);
#else
	printf("Creating output buffer from OpenGL texture...\n");
	ocl.output = clCreateFromGLTexture2D(ocl.context, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, fbos["RayTrace"]->GetTextureID(0), &resultCL);
	CLGLError(resultCL);
#endif

	printf("Creating Voxel Info buffer...\n");
	ocl.voxInfo = clCreateBuffer(ocl.context, CL_MEM_READ_WRITE, sizeof(VoxelInfo), NULL, &resultCL);
	CLGLError(resultCL);

	printf("Creating Octree Info buffer...\n");
	ocl.octInfo = clCreateBuffer(ocl.context, CL_MEM_READ_ONLY, sizeof(OctreeInfo), NULL, &resultCL);
	CLGLError(resultCL);

	printf("Creating Normal Lookup Table...\n");
	ocl.normalLookup = clCreateBuffer(ocl.context, CL_MEM_READ_ONLY, sizeof(cl_float4) * 256, NULL, &resultCL);
	CLGLError(resultCL);

	printf("\n\n");

#pragma endregion

	octreeBuilder.Init(ocl.context, ocl.devices[0]);
	voxelBuilder.Init(ocl.context, ocl.devices[0]);
	
}

void Engine::PopulateNormalLookup()
{
	printf("Creating normal lookup table...\n");
	cl_float4 normals[256];

	printf("Copying to GPU memory...\n");
	clEnqueueWriteBuffer(ocl.queue, ocl.normalLookup, true, 0, sizeof(cl_float4) * 256, normals, 0, NULL, NULL);
}

void Engine::UpdateCL()
{	
	
	Mat4 world(16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 16.0f);
	Mat4 invWorldView = inv(world) * cam->GetTransform();

	memcpy(&RTParams.sizeX, &voxelSize.x, sizeof(int) * 3);
	RTParams.sizeW = voxelBuilder.GetMaxNumMips() - 1;
	memcpy(RTParams.invWorldView, invWorldView.Ref(), sizeof(float) * 16);
	RTParams.invSize[0] = 1.0 / voxelSize.x;
	RTParams.invSize[1] = 1.0 / voxelSize.y;
	RTParams.invSize[2] = 1.0 / voxelSize.z;
#ifndef INTEL
	clEnqueueAcquireGLObjects(ocl.queue, 1, &ocl.output, 0, NULL, NULL);
#endif

	size_t globalWorkSize[] = { RTWidth, RTHeight };
	
	memset(counters, 0, sizeof(counters));

	clEnqueueWriteBuffer(ocl.queue, ocl.paramBuffer, false, 0, sizeof(RTParams), &RTParams, 0, NULL, NULL);
	clEnqueueWriteBuffer(ocl.queue, ocl.rtCounterBuffer, false, 0, sizeof(int) * 2, &counters, 0, NULL, NULL);

	if (voxels)
	{
		if (ocl.rtKernel)
		{
			clEnqueueNDRangeKernel(ocl.queue, ocl.rtKernel, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);		
		}
	}
	else
	{
		if (ocl.octRTKernel)
		{
			clEnqueueNDRangeKernel(ocl.queue, ocl.octRTKernel, 2, NULL, globalWorkSize, 0, NULL, NULL, NULL);
		}
	}

	clEnqueueReadBuffer(ocl.queue, ocl.rtCounterBuffer, false, 0, sizeof(int) * 2, &counters, 0, NULL, NULL);
#ifndef INTEL
	clEnqueueReleaseGLObjects(ocl.queue, 1, &ocl.output, 0, NULL, NULL);
#endif
	clFinish(ocl.queue);
	averageIterations = (float)counters[1] / (float)counters[0];
}

void Engine::DebugDrawVoxelData()
{
	cl_uint4 size;	
	size.s[0] = voxelSize.x;
	size.s[1] = voxelSize.y;
	size.s[2] = voxelSize.z;
	cl_uint4 mipOffset;	
	mipOffset.s[1] = 0;
	for (int i = 1; i < mipLevel % voxelBuilder.GetMaxNumMips(); ++i)
	{
		mipOffset.s[1] += (size.s[0] * size.s[1] * size.s[2]);
		size.s[0] /= 2;
		size.s[1] /= 2;
		size.s[2] /= 2;
	}
	size.s[3] = (uint)(fabs(zCoord) * size.s[2]) % size.s[2];
	mipOffset.s[0] = mipLevel % voxelBuilder.GetMaxNumMips();

	size_t workDim[2];
	workDim[0] = RTWidth;
	workDim[1] = RTHeight;

	if (voxels)
	{			
		if (ocl.voxKernel)
		{			
			clSetKernelArg(ocl.voxKernel, 3, sizeof(cl_uint4), &size);
			clSetKernelArg(ocl.voxKernel, 2, sizeof(cl_uint4), &mipOffset);
#ifndef INTEL
			clEnqueueAcquireGLObjects(ocl.queue, 1, &ocl.output, 0, NULL, NULL);			
#endif
			clEnqueueNDRangeKernel(ocl.queue, ocl.voxKernel, 2, NULL, workDim, NULL, 0, NULL, NULL);
#ifndef INTEL
			clEnqueueReleaseGLObjects(ocl.queue, 1, &ocl.output, 0, NULL, NULL);			
#endif
		}
	}
	else
	{
		if (ocl.octDrawKernel)
		{
			mipOffset.s[2] = voxelBuilder.GetMaxNumMips() - mipOffset.s[0] - 1;
			clSetKernelArg(ocl.octDrawKernel, 3, sizeof(cl_uint4), &size);
			clSetKernelArg(ocl.octDrawKernel, 2, sizeof(cl_uint4), &mipOffset);
#ifndef INTEL
			clEnqueueAcquireGLObjects(ocl.queue, 1, &ocl.output, 0, NULL, NULL);
#endif
			clEnqueueNDRangeKernel(ocl.queue, ocl.octDrawKernel, 2, NULL, workDim, NULL, 0, NULL, NULL);
#ifndef INTEL
			clEnqueueReleaseGLObjects(ocl.queue, 1, &ocl.output, 0, NULL, NULL);
#endif
		}
	}
	clFinish(ocl.queue);
}

void Engine::Display()
{
	Mat4 world(16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 16.0f);
	Mat4 invWorld = inv(world);
	Mat4 I(vl_one);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifdef INTEL
	memset(outputImage, 0, sizeof(unsigned int) * RTWidth * RTHeight);
#else
	fbos["RayTrace"]->Bind();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	fbos["RayTrace"]->Unbind();
	glFinish();
#endif

	if (clDraw)
		UpdateCL();
	else
		DebugDrawVoxelData();

#ifdef INTEL
	glBindTexture(GL_TEXTURE_2D, fbos["RayTrace"]->GetTextureID(0));
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, RTWidth, RTHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, outputImage);
#endif

	shaders["Copy"]->Use();
	shaders["Copy"]->Uniforms["baseTex"].SetValue(0);
	glActiveTexture(GL_TEXTURE0);

	glBindTexture(GL_TEXTURE_2D, fbos["RayTrace"]->GetTextureID(0));
	QuadDrawer::DrawQuad(Vec2(-1.0, -1.0), Vec2(0.5, 1.0));

	boost::format fmter("FPS: %1%, Z Coord: %2%, %3%, average iterations: %4%, %5%");
	fmter % CurrentFPS % zCoord;
	if (clDraw)
		fmter % "Ray Tracer";
	else
		fmter % "View Raw Data";

	fmter % averageIterations;

	if (voxels)
		fmter % "Voxel Data";
	else
		fmter % "Octree Data";

	Vec2 windowSize(Window.Width, Window.Height);
	
	fmter.clear();
	fmter.parse("FPS: %1%");
	fmter % CurrentFPS;
	PrintText(windowSize, Vec2(Window.Width * 0.5f, Window.Height), fmter.str().c_str(), Colour::White);

	glfwSetWindowTitle(fmter.str().c_str());

	fmter.clear();
	fmter.parse("Z Coord: %1%");
	fmter % zCoord;
	PrintText(windowSize, Vec2(Window.Width * 0.5f, Window.Height - 40), fmter.str().c_str(), Colour::White);

	fmter.clear();
	fmter.parse("%1%");
	if (clDraw)
		fmter % "Ray Tracer";
	else
		fmter % "View Raw Data";
	PrintText(windowSize, Vec2(Window.Width * 0.5f, Window.Height - 80), fmter.str().c_str(), Colour::White);

	fmter.clear();
	fmter.parse("%1%");
	if (voxels)
		fmter % "Voxel Data";
	else
		fmter % "Octree Data";
	PrintText(windowSize, Vec2(Window.Width * 0.5f, Window.Height - 120), fmter.str().c_str(), Colour::White);

	fmter.clear();
	fmter.parse("Avg. Iterations:     %1%");
	fmter % averageIterations;
	PrintText(windowSize, Vec2(Window.Width * 0.5f, Window.Height - 160), fmter.str().c_str(), Colour::White);	

}

void Engine::Update(TimeInfo& timeInfo)
{
	if (KeyState[GLFW_KEY_LEFT])
		camControl->ChangeYaw(10.0f);
	if (KeyState[GLFW_KEY_RIGHT])
		camControl->ChangeYaw(-10.0f);
	if (KeyState[GLFW_KEY_UP])
		camControl->ChangePitch(10.0f);
	if (KeyState[GLFW_KEY_DOWN])
		camControl->ChangePitch(-10.0f);
	if (KeyState['W'])
		camControl->MoveForward();
	if (KeyState['S'])
		camControl->MoveBackward();
	if (KeyState['A'])
		camControl->MoveLeft();
	if (KeyState['D'])
		camControl->MoveRight();
	if (KeyState[GLFW_KEY_SPACE])
		camControl->MoveUp();
	if (KeyState[GLFW_KEY_LCTRL] || KeyState['C'])
		camControl->MoveDown();
	if (KeyState['['])
		zCoord -= 0.01f;
	if (KeyState[']'])
		zCoord += 0.01f;
	camControl->Update(timeInfo.fTimeSinceLastFrame);
}

void Engine::KeyPressed(int code)
{
	if (code == GLFW_KEY_ESC)
		Exit();
	if (code == 'R')
	{
		shaders.ReloadShaders();
		CreateRTKernel();
		octreeBuilder.ReloadProgram();
		voxelBuilder.ReloadProgram();
	}
	if (code == 'V')
		voxels = !voxels;
	if (code == 'Z')
		Init3DTexture();
	if (code == 'M')
		drawMesh1 = !drawMesh1;
	if (code == 'O')
		clDraw = !clDraw;
	if (code == '=')
		mipLevel += 1;
	if (code == '-')
		mipLevel -= 1;
}

void Engine::KeyReleased(int code)
{
	
}

