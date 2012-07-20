#define CL_USE_DEPRECATED_OPENCL_1_1_APIS

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
#include "VoxelOctree.h"
#include "CL\cl_gl.h"
#include "GLGUI\Primitives.h"
#include "CLUtils.h"

#ifdef _WIN32
#include <Windows.h>
#endif

struct _vs
{
	int x;
	int y;
	int z;
} voxelSize;

int RTWidth = 512;
int RTHeight = 384;

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

	voxelBuilder.Build(mesh, &voxelSize.x, shaders["Basic"]);
	ocl.volumeData = voxelBuilder.GetVoxelData();	
	if (ocl.voxKernel)
		clSetKernelArg(ocl.voxKernel, 1, sizeof(cl_mem), &ocl.volumeData);
}

void Engine::Setup()
{

	clDraw = true;

	voxelSize.x = 128;
	voxelSize.y = 128;
	voxelSize.z = 128;
	
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

	fbos.AddFBO(new FrameBufferObject(Window.Width / 2, Window.Height, 24, 0, GL_RGBA, GL_TEXTURE_2D, "DVR"));
	fbos["DVR"]->AttachTexture("colour");
	fbos.AddFBO(new FrameBufferObject(voxelSize.x, voxelSize.y, 24, 0, GL_RGBA, GL_TEXTURE_2D, "Voxel"));
	fbos["Voxel"]->AttachTexture("colour");
	fbos.AddFBO(new FrameBufferObject(RTWidth, RTHeight, 0, 0, GL_RGBA, GL_TEXTURE_2D, "RayTrace"));
	fbos["RayTrace"]->AttachTexture("colour", GL_LINEAR, GL_LINEAR);

	printf("done.\n");

	mesh1 = new StaticMesh();
	mesh1->LoadObj("Assets/Meshes/bunny.obj", false, false, false);
	mesh2 = new StaticMesh();
	mesh2->LoadObj("Assets/Meshes/teapot.obj", false, false, false);

	drawQuad = false;
	drawMesh1 = true;
	
	zCoord = 1.0f; 	
	SetupOpenCL();
	Init3DTexture();
}

void Engine::CreateRTKernel()
{

	if (ocl.rtKernel)
		clReleaseKernel(ocl.rtKernel);
	if (ocl.rtProgram)
		clReleaseProgram(ocl.rtProgram);

	oclKernel k = CreateKernelFromFile("Assets/CL/RT.cl", "VolRT", ocl.context, ocl.devices[0]);	

	if (k.ok)
	{		
		ocl.rtKernel = k.kernel;
		ocl.rtProgram = k.program;	
		clSetKernelArg(ocl.rtKernel, 0, sizeof(cl_mem), &ocl.output);
		clSetKernelArg(ocl.rtKernel, 1, sizeof(cl_mem), &ocl.input);
		clSetKernelArg(ocl.rtKernel, 2, sizeof(cl_mem), &ocl.paramBuffer);
		clSetKernelArg(ocl.rtKernel, 3, sizeof(cl_mem), &ocl.rtCounterBuffer);
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

	targetPlatform = allPlatforms[0];

    char pbuff[128];

	clGetPlatformInfo(targetPlatform, CL_PLATFORM_NAME, sizeof(pbuff), pbuff, NULL);

	printf("\n\nUsing platform:\n%s\n", pbuff);

    resultCL = clGetPlatformInfo(targetPlatform, CL_PLATFORM_VENDOR, sizeof(pbuff), pbuff, NULL);

	printf("Vendor: %s\n\n\n", pbuff);		

    if (resultCL != CL_SUCCESS)
        throw (std::string("InitCL()::Error: Getting platform info (clGetPlatformInfo)"));    

	
	ocl.devices = (cl_device_id*)malloc(sizeof(cl_device_id) * 16);	
	resultCL = clGetDeviceIDs(targetPlatform, CL_DEVICE_TYPE_GPU, 16, ocl.devices, &(ocl.deviceNum));

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

	printf("Creating output buffer from OpenGL texture...\n");
	ocl.output = clCreateFromGLTexture2D(ocl.context, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, fbos["RayTrace"]->GetTextureID(0), &resultCL);
	CLGLError(resultCL);

	printf("\n\n");

#pragma endregion
	
	CreateRTKernel();
	octreeBuilder.Init(ocl.context, ocl.devices[0]);
	voxelBuilder.Init(ocl.context, ocl.devices[0]);
	
}

void Engine::UpdateCL()
{	
	if (ocl.rtKernel)
	{
		Mat4 world(16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 16.0f);
		Mat4 invWorldView = inv(world) * cam->GetTransform();

		memcpy(&RTParams.sizeX, &voxelSize.x, sizeof(int) * 3);
		memcpy(RTParams.invWorldView, invWorldView.Ref(), sizeof(float) * 16);
		RTParams.invSize[0] = 1.0 / voxelSize.x;
		RTParams.invSize[1] = 1.0 / voxelSize.y;
		RTParams.invSize[2] = 1.0 / voxelSize.z;

		clEnqueueAcquireGLObjects(ocl.queue, 1, &ocl.output, 0, NULL, NULL);

		size_t globalWorkSize[] = { Window.Width, Window.Height * 0.75 };

		int counters[2];
		memset(counters, 0, sizeof(counters));

		clEnqueueWriteBuffer(ocl.queue, ocl.paramBuffer, false, 0, sizeof(RTParams), &RTParams, 0, NULL, NULL);
		clEnqueueWriteBuffer(ocl.queue, ocl.rtCounterBuffer, false, 0, sizeof(int) * 2, &counters, 0, NULL, NULL);

		clEnqueueNDRangeKernel(ocl.queue, ocl.rtKernel, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);

		clEnqueueReleaseGLObjects(ocl.queue, 1, &ocl.output, 0, NULL, NULL);

		clEnqueueReadBuffer(ocl.queue, ocl.rtCounterBuffer, true, 0, sizeof(int) * 2, &counters, 0, NULL, NULL);

		averageIterations = (float)counters[1] / (float)counters[0];
		
	}
}

void Engine::DebugDrawVoxelData()
{
	cl_int4 size;
	memcpy(size.s, &voxelSize.x, sizeof(int) * 3);
	size.s[3] = zCoord * 255;
	if (ocl.voxKernel)
	{
		size_t workDim[2];
		workDim[0] = Window.Width;
		workDim[1] = Window.Height * 0.75;
		clSetKernelArg(ocl.voxKernel, 2, sizeof(cl_int4), &size);
		clEnqueueAcquireGLObjects(ocl.queue, 1, &ocl.output, 0, NULL, NULL);
		clEnqueueNDRangeKernel(ocl.queue, ocl.voxKernel, 2, NULL, workDim, NULL, 0, NULL, NULL);
		clEnqueueReleaseGLObjects(ocl.queue, 1, &ocl.output, 0, NULL, NULL);

	}
}

void Engine::Display()
{
	Mat4 world(16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 16.0f);
	Mat4 invWorld = inv(world);
	Mat4 I(vl_one);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	fbos["RayTrace"]->Bind();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	fbos["RayTrace"]->Unbind();

	if (clDraw)
		UpdateCL();
	else
		DebugDrawVoxelData();	

	clFinish(ocl.queue);

	shaders["Copy"]->Use();
	shaders["Copy"]->Uniforms["baseTex"].SetValue(0);
	glActiveTexture(GL_TEXTURE0);

	glBindTexture(GL_TEXTURE_2D, fbos["RayTrace"]->GetTextureID(0));
	QuadDrawer::DrawQuad(Vec2(-1.0, -0.5), Vec2(1.0, 1.0));

	boost::format fmter("FPS: %1%, CamPos: %2%, %3%, %4%, Pitch: %5%, Yaw: %6%, Z Coord: %7%, %8%, average iterations: %9%");
	fmter % CurrentFPS % cam->Position[0] % cam->Position[1] % cam->Position[2] % cam->Pitch % cam->Yaw % zCoord;
	if (clDraw)
		fmter % "OpenCL";
	else
		fmter % "OpenGL";

	fmter % averageIterations;

	glfwSetWindowTitle(fmter.str().c_str());

	fmter.clear();
	fmter.parse("Avg iterations: %1%");
	fmter % averageIterations;

	PrintText(Vec2(Window.Width, Window.Height), Vec2(-Window.Width, -Window.Height / 2.0), fmter.str().c_str(), Colour::White);

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
		drawQuad = !drawQuad;
	if (code == 'Z')
		Init3DTexture();
	if (code == 'M')
		drawMesh1 = !drawMesh1;
	if (code == 'O')
		clDraw = !clDraw;
}

void Engine::KeyReleased(int code)
{
	
}

