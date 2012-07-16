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
#include "CL\cl_gl.h"
#ifdef _WIN32
#include <Windows.h>
#endif

struct _vs
{
	int x;
	int y;
	int z;
} voxelSize;

int RTWidth = 1024;
int RTHeight = 768;

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

	int imageSize = voxelSize.x * voxelSize.y * sizeof(unsigned int);
	unsigned int* data = (unsigned int*)malloc(imageSize * voxelSize.z);

	float zDist = mesh->SubMeshes[0].Max[2] - mesh->SubMeshes[0].Min[2];
	float increment = zDist / voxelSize.z;
	zDist = - increment;

	for (int i = 0; i < voxelSize.z; ++i)
	{

		fbos["Voxel"]->Bind();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		float camHeight = mesh->SubMeshes[0].Max[1] - mesh->SubMeshes[0].Min[1];
		camHeight /= 2.0f;

		float camDist = mesh->SubMeshes[0].Max[2] - mesh->SubMeshes[0].Min[2];
		camDist /= 2.0f;

		Mat4 ortho = Orthographic(mesh->SubMeshes[0].Min[0], mesh->SubMeshes[0].Max[0], mesh->SubMeshes[0].Min[1], mesh->SubMeshes[0].Max[1], zDist, zDist + (increment * 5.0f));

		shaders["Basic"]->Use();
		shaders["Basic"]->Uniforms["World"].SetValue(Mat4(vl_one));
		shaders["Basic"]->Uniforms["View"].SetValue(HTrans4(Vec3(0, 0, -camDist)));
		shaders["Basic"]->Uniforms["Projection"].SetValue(ortho);
		shaders["Basic"]->Uniforms["lightCone"].SetValue(Vec3(0, -1, 0));
		shaders["Basic"]->Uniforms["lightPos"].SetValue(Vec3(0, 10, 0));
		shaders["Basic"]->Uniforms["lightRadius"].SetValue(15.0f);

		glEnable(GL_DEPTH_TEST);
		mesh->SubMeshes[0].Draw();

		fbos["Voxel"]->Unbind();

		shaders["Copy"]->Use();
		shaders["Copy"]->Uniforms["baseTex"].SetValue(0);

		glDisable(GL_DEPTH_TEST);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fbos["Voxel"]->GetTextureID(0));
		QuadDrawer::DrawQuad(Vec2(-1.0f, -1.0f), Vec2(1.0f, 1.0f));

		zDist += increment;

		boost::format fmter("Done: %1% / %2%");
		fmter % (i + 1) % voxelSize.z;
	  
		glfwSetWindowTitle(fmter.str().c_str());
		glfwSwapBuffers();

		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data + (voxelSize.x * voxelSize.y * i));
		//glfwSleep(0.1); This is so that the process is visible

	}

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, tex3D);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, voxelSize.x, voxelSize.y, voxelSize.z, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	//glGenerateMipmap(GL_TEXTURE_3D);	

	glFinish();	

}

void Engine::Setup()
{

	voxelSize.x = 256;
	voxelSize.y = 256;
	voxelSize.z = 256;
	
	glGenTextures(1, &tex3D);
	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, tex3D);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);   	

	drawVol = false;

	memset(&ocl, 0, sizeof(ocl));

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

	Init3DTexture();	
	SetupOpenCL();
}

void Engine::CreateRTKernel()
{

	cl_int resultCL;

	if (ocl.rtKernel)
	{
		clReleaseKernel(ocl.rtKernel);
	}
	if (ocl.rtProgram)
	{
		clReleaseProgram(ocl.rtProgram);
	}

	const char* source = getSourceFromFile("Assets/CL/RT.cl");
	if (source)
	{
		ocl.rtProgram = clCreateProgramWithSource(ocl.context, 1, &source, NULL, &resultCL);
	}
	else
	{	
		printf("Could not open program file!\n");
		CLGLError(resultCL);
		printf("\n");
		return;
	}

	free((char*)source);

	if (resultCL != CL_SUCCESS)
	{
		printf("Error loading program!\n");
		CLGLError(resultCL);
		printf("\n");
	}
	else
		printf("OpenCL program loaded.\n");

	resultCL = clBuildProgram(ocl.rtProgram, ocl.deviceNum, ocl.devices, "", NULL, NULL);
	if (resultCL != CL_SUCCESS)
	{
		printf("Error building program: ");
		CLGLError(resultCL);

		size_t length;
        resultCL = clGetProgramBuildInfo(ocl.rtProgram, 
                                        ocl.devices[0], 
                                        CL_PROGRAM_BUILD_LOG, 
                                        0, 
                                        NULL, 
                                        &length);
        if(resultCL != CL_SUCCESS) 
            printf("InitCL()::Error: Getting Program build info(clGetProgramBuildInfo)\n");

		char* buffer = (char*)malloc(length);
        resultCL = clGetProgramBuildInfo(ocl.rtProgram, 
                                        ocl.devices[0], 
                                        CL_PROGRAM_BUILD_LOG, 
                                        length, 
                                        buffer, 
                                        NULL);
        if(resultCL != CL_SUCCESS) 
            printf("InitCL()::Error: Getting Program build info(clGetProgramBuildInfo)\n");
		else
			printf("%s\n", buffer);
	}
	else
	{
		printf("Program built successfully.\n");
	}

	size_t length;
    resultCL = clGetProgramBuildInfo(ocl.rtProgram, 
                                    ocl.devices[0], 
                                    CL_PROGRAM_BUILD_LOG, 
                                    0, 
                                    NULL, 
                                    &length);
    if(resultCL != CL_SUCCESS) 
        printf("InitCL()::Error: Getting Program build info(clGetProgramBuildInfo)\n");

	char* buffer = (char*)malloc(length);
    resultCL = clGetProgramBuildInfo(ocl.rtProgram, 
                                    ocl.devices[0], 
                                    CL_PROGRAM_BUILD_LOG, 
                                    length, 
                                    buffer, 
                                    NULL);
    if(resultCL != CL_SUCCESS) 
        printf("InitCL()::Error: Getting Program build info(clGetProgramBuildInfo)\n");
	else
		printf("%s\n", buffer);

	ocl.rtKernel = clCreateKernel(ocl.rtProgram, "VolRT", &resultCL);

	if (resultCL != CL_SUCCESS)
	{
		printf("Error creating kernel: ");
		CLGLError(resultCL);
		printf("\n");
	}
	else
	{
		printf("Kernel created successfully.\n");		
		clSetKernelArg(ocl.rtKernel, 0, sizeof(cl_mem), &ocl.output);
		clSetKernelArg(ocl.rtKernel, 1, sizeof(cl_mem), &ocl.input);
		clSetKernelArg(ocl.rtKernel, 2, sizeof(cl_mem), &ocl.paramBuffer);
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

	printf("Creating OpenCL buffer from 3D texture... \n");
	ocl.input = clCreateFromGLTexture3D(ocl.context, CL_MEM_READ_ONLY, GL_TEXTURE_3D, 0, tex3D, &resultCL);
	CLGLError(resultCL);

	ocl.output = clCreateFromGLTexture2D(ocl.context, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, fbos["RayTrace"]->GetTextureID(0), &resultCL);

	if (resultCL != CL_SUCCESS)
	{
		printf("Error: creating OpenCL output buffer from FBO: ");
		CLGLError(resultCL);
		printf("\n");
	}
	else	
		printf("Created OpenCL output buffer from FBO\n");

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

	printf("\n\n");

#pragma endregion
	
	CreateRTKernel();
	//octreeBuilder.Init(ocl.context, ocl.devices[0]);
	
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
		clEnqueueAcquireGLObjects(ocl.queue, 1, &ocl.input, 0, NULL, NULL);
		size_t globalWorkSize[] = { Window.Width, Window.Height };

		clEnqueueWriteBuffer(ocl.queue, ocl.paramBuffer, false, 0, sizeof(RTParams), &RTParams, 0, NULL, NULL);

		clEnqueueNDRangeKernel(ocl.queue, ocl.rtKernel, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);

		clEnqueueReleaseGLObjects(ocl.queue, 1, &ocl.output, 0, NULL, NULL);
		clEnqueueReleaseGLObjects(ocl.queue, 1, &ocl.input, 0, NULL, NULL);
	}
}

void Engine::Display()
{

	Mat4 world(16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 16.0f);
	Mat4 invWorld = inv(world);
	Mat4 I(vl_one);

#pragma region RayTracer

	fbos["RayTrace"]->Bind();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (!clDraw)
	{		
		
		Shader* currentRT = shaders["VolRT"];
		currentRT->Use();	
		currentRT->Uniforms["baseTex"].SetValue(1);
		currentRT->Uniforms["invWorldView"].SetValue(invWorld * cam->GetTransform());
		currentRT->Uniforms["maxDist"].SetValue(1000.0f);
		currentRT->Uniforms["sphereCentre"].SetValue(Vec3(vl_zero));
		currentRT->Uniforms["screenDist"].SetValue(zCoord);
		currentRT->Uniforms["size"].SetValue(voxelSize.x, voxelSize.y, voxelSize.z);
	
		QuadDrawer::DrawQuad(Vec2(-1.0f, -1.0f), Vec2(1.0f, 1.0f));

		fbos["RayTrace"]->Unbind();
	}

	if (clDraw)
	{
		fbos["RayTrace"]->Unbind();
		UpdateCL();
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#pragma endregion

#pragma region DVR

	fbos["DVR"]->Bind();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	DrawCoordFrame(cam->GetViewTransform() * cam->GetProjectionMatrix());

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, tex3D);	

	glEnable(GL_DEPTH_TEST);

	shaders["DVR"]->Use();
	shaders["DVR"]->Uniforms["volTex"].SetValue(1); 
	shaders["DVR"]->Uniforms["World"].SetValue(Mat4(vl_zero));
	shaders["DVR"]->Uniforms["viewProj"].SetValue(cam->GetViewTransform() * cam->GetProjectionMatrix());
	shaders["DVR"]->Uniforms["size"].SetValue(voxelSize.x, voxelSize.y, voxelSize.z);

	
	if (shaders["DVR"]->Compiled && drawVol)
		CubeDrawer::DrawCubes(voxelSize.x * voxelSize.y * voxelSize.z);

	/*shaders["Testred"]->Use();

	QuadDrawer::DrawQuad(Vec2(-0.5, -0.5), Vec2(0.5, 0.5));*/      

	fbos["DVR"]->Unbind();   

#pragma endregion

	glDisable(GL_DEPTH_TEST);

	//Draw DVR version

	shaders["Copy"]->Use();
	shaders["Copy"]->Uniforms["baseTex"].SetValue(0);
	glActiveTexture(GL_TEXTURE0);

	glBindTexture(GL_TEXTURE_2D, fbos["DVR"]->GetTextureID(0));
	//QuadDrawer::DrawQuad(Vec2(-1.0, -1.0), Vec2(0.0, 1.0));

	//Draw RayTraced

	if (clDraw)
		clFinish(ocl.queue);

	glBindTexture(GL_TEXTURE_2D, fbos["RayTrace"]->GetTextureID(0));
	QuadDrawer::DrawQuad(Vec2(-1.0, -1.0), Vec2(1.0, 1.0));

#pragma region Draw3dTexture:

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, tex3D);

	shaders["Copy3D"]->Use();
	shaders["Copy3D"]->Uniforms["baseTex"].SetValue(1);
	shaders["Copy3D"]->Uniforms["zCoord"].SetValue(zCoord);		

	if (drawQuad)
		QuadDrawer::DrawQuad(Vec2(-1.0, -1.0), Vec2(1.0, 1.0));

#pragma endregion

	boost::format fmter("FPS: %1%, CamPos: %2%, %3%, %4%, Pitch: %5%, Yaw: %6%, Z Coord: %7%, %8%");
	fmter % CurrentFPS % cam->Position[0] % cam->Position[1] % cam->Position[2] % cam->Pitch % cam->Yaw % zCoord;
	if (clDraw)
		fmter % "OpenCL";
	else
		fmter % "OpenGL";
	glfwSetWindowTitle(fmter.str().c_str());

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

