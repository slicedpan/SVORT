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

int voxelSize = 64;


Engine::Engine(WindowSettings& w) : GLFWEngine(w),
	fbos(FBOManager::GetSingleton()),
	shaders(ShaderManager::GetSingleton())
{
	srand(time(0));
}

Engine::~Engine(void)
{
	delete volData;
	delete camControl;
	delete cam;
}

void Engine::Init3DTexture()
{
	StaticMesh* mesh = mesh1;
	if (!drawMesh1)
		mesh = mesh2;
	if (tex3D < 0)		
	{
		glGenTextures(1, &tex3D);
	}
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, tex3D);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);   	

	int imageSize = voxelSize * voxelSize * sizeof(unsigned int);
	unsigned int* data = (unsigned int*)malloc(imageSize * voxelSize);

	float zDist = mesh->SubMeshes[0].Max[2] - mesh->SubMeshes[0].Min[2];
	float increment = zDist / voxelSize;
	zDist = - increment;

	for (int i = 0; i < voxelSize; ++i)
	{

		fbos["RayTrace"]->Bind();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		float camHeight = mesh->SubMeshes[0].Max[1] - mesh->SubMeshes[0].Min[1];
		camHeight /= 2.0f;

		float camDist = mesh->SubMeshes[0].Max[2] - mesh->SubMeshes[0].Min[2];
		camDist /= 2.0f;

		Mat4 ortho = Orthographic(mesh->SubMeshes[0].Min[0], mesh->SubMeshes[0].Max[0], mesh->SubMeshes[0].Min[1], mesh->SubMeshes[0].Max[1], zDist, zDist + (increment * 2.5f));

		shaders["Basic"]->Use();
		shaders["Basic"]->Uniforms["World"].SetValue(Mat4(vl_one));
		shaders["Basic"]->Uniforms["View"].SetValue(HTrans4(Vec3(0, 0, -camDist)));
		shaders["Basic"]->Uniforms["Projection"].SetValue(ortho);
		shaders["Basic"]->Uniforms["lightCone"].SetValue(Vec3(0, -1, 0));
		shaders["Basic"]->Uniforms["lightPos"].SetValue(Vec3(0, 10, 0));
		shaders["Basic"]->Uniforms["lightRadius"].SetValue(15.0f);

		glEnable(GL_DEPTH_TEST);
		mesh->SubMeshes[0].Draw();

		fbos["RayTrace"]->Unbind();

		shaders["Copy"]->Use();
		shaders["Copy"]->Uniforms["baseTex"].SetValue(0);

		glDisable(GL_DEPTH_TEST);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fbos["RayTrace"]->GetTextureID(0));
		QuadDrawer::DrawQuad(Vec2(-1.0f, -1.0f), Vec2(1.0f, 1.0f));

		zDist += increment;

		boost::format fmter("Done: %1% / %2%");
		fmter % (i + 1) % voxelSize;
	  
		glfwSetWindowTitle(fmter.str().c_str());
		glfwSwapBuffers();

		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data + (voxelSize * voxelSize * i));

	}

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, tex3D);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, voxelSize, voxelSize, voxelSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	free(data);

	//unsigned int* dat = (unsigned int*)malloc(sizeof(unsigned int) * volData->Count);
	//glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_UNSIGNED_BYTE, dat);
	//unsigned char c[4];
	//for (int i = 0; i < volData->Count; ++i)
	//{
		// if (dat[i] != volData->GetPtr()[i])
			//  throw;
		// memcpy(c, (dat + i), sizeof(unsigned int));
	//}
}

void Engine::Setup()
{

	tex3D = -1;

	cam = new Camera();
	cam->Position[2] += 20.0f;
	camControl = new CameraController();
	camControl->SetCamera(cam);
	camControl->MaxSpeed = 0.4f;

	volData = new VolumeData<unsigned int>(16, 16, 16);
	volData->ZeroData();
	//volData->SetAll(Colour::AsBytes<unsigned int>(Colour::Red));
   
	for (int i = 0; i < 256; ++i)
	{
		int x, y, z;
		x = rand() % volData->SizeX;
		y = rand() % volData->SizeY;
		z = rand() % volData->SizeZ;
		unsigned int colour = rand() % 255 + (rand() % 255 << 8) + (rand() % 255 << 16) + (255 << 24);
		unsigned char* c = (unsigned char*)&colour;
		printf("%d, %d, %d, %d\n", c[0], c[1], c[2], c[3]);
		volData->Get(x, y, z) = colour;	   
	}
	shaders.Add(new Shader("Assets/Shaders/vol.vert","Assets/Shaders/vol.frag", "DVR"));
	shaders.Add(new Shader("Assets/Shaders/copy.vert", "Assets/Shaders/copy.frag", "Copy"));
	shaders.Add(new Shader("Assets/Shaders/testred.vert", "Assets/Shaders/testred.frag", "Testred"));
	shaders.Add(new Shader("Assets/Shaders/copy3D.vert", "Assets/Shaders/copy3D.frag", "Copy3D"));
	shaders.Add(new Shader("Assets/Shaders/basic.vert", "Assets/Shaders/basic.frag", "Basic"));
	shaders.CompileShaders();
	fbos.AddFBO(new FrameBufferObject(Window.Width / 2, Window.Height, 24, 0, GL_RGBA, GL_TEXTURE_2D, "DVR"));
	fbos["DVR"]->AttachTexture("colour");
	fbos.AddFBO(new FrameBufferObject(voxelSize, voxelSize, 24, 0, GL_RGBA, GL_TEXTURE_2D, "RayTrace"));
	fbos["RayTrace"]->AttachTexture("colour");

	mesh1 = new StaticMesh();
	mesh1->LoadObj("Assets/Meshes/bunny.obj", false, false, false);  
	mesh2 = new StaticMesh();
	mesh2->LoadObj("ASsets/Meshes/teapot.obj", false, false, false);

	drawQuad = false;
	drawMesh1 = true;

	zCoord = 0.0f; 

}

void Engine::Display()
{

	Mat4 world(16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 16.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
	shaders["DVR"]->Uniforms["sideLength"].SetValue(voxelSize);
	
	if (shaders["DVR"]->Compiled)
		CubeDrawer::DrawCubes(voxelSize * voxelSize * voxelSize);

	/*shaders["Testred"]->Use();

	QuadDrawer::DrawQuad(Vec2(-0.5, -0.5), Vec2(0.5, 0.5));*/      

	fbos["DVR"]->Unbind();   

	shaders["Copy"]->Use();
	shaders["Copy"]->Uniforms["baseTex"].SetValue(0);
	glActiveTexture(GL_TEXTURE0);

	glBindTexture(GL_TEXTURE_2D, fbos["DVR"]->GetTextureID(0));
	QuadDrawer::DrawQuad(Vec2(-1.0, -1.0), Vec2(1.0, 1.0));

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, tex3D);

	shaders["Copy3D"]->Use();
	shaders["Copy3D"]->Uniforms["baseTex"].SetValue(1);
	shaders["Copy3D"]->Uniforms["zCoord"].SetValue(zCoord);

	glDisable(GL_DEPTH_TEST);

	if (drawQuad)
		QuadDrawer::DrawQuad(Vec2(-1.0, -1.0), Vec2(0.0, 0.0));

	/* glBindTexture(GL_TEXTURE_2D, fbos["DVR"]->GetTextureID(0));
	QuadDrawer::DrawQuad(Vec2(-1.0, -1.0), Vec2(0.0, 1.0));

	shaders["Copy"]->Use();
	shaders["Copy"]->Uniforms["baseTex"].SetValue(0);
	glActiveTexture(GL_TEXTURE0);

	glBindTexture(GL_TEXTURE_2D, fbos["RayTrace"]->GetTextureID(0));

	*/

	boost::format fmter("FPS: %1%, CamPos: %2%, %3%, %4%, Pitch: %5%, Yaw: %6%, Z Coord: %7%");
	fmter % CurrentFPS % cam->Position[0] % cam->Position[1] % cam->Position[2] % cam->Pitch % cam->Yaw % zCoord;

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
		shaders.ReloadShaders();
	if (code == 'V')
		drawQuad = !drawQuad;
	if (code == 'Z')
		Init3DTexture();
	if (code == 'M')
		drawMesh1 = !drawMesh1;
}

void Engine::KeyReleased(int code)
{

}

