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
#include <GL\glfw.h>
#include "BasicObjects.h"
#include <boost\format.hpp>


Engine::Engine(WindowSettings& w) : GLFWEngine(w),
	fbos(FBOManager::GetSingleton()),
	shaders(ShaderManager::GetSingleton())
{
	srand(time(0));
}

Engine::~Engine(void)
{
}

void Engine::Init3DTexture()
{
	glGenTextures(1, &tex3D);
	glBindTexture(GL_TEXTURE_3D, tex3D);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, volData->SizeX, volData->SizeY, volData->SizeZ, 0, GL_RGBA, GL_UNSIGNED_BYTE, volData->GetPtr());
}

void Engine::Setup()
{
	cam = new Camera();
	cam->Position[2] += 20.0f;
	camControl = new CameraController();
	camControl->SetCamera(cam);
	camControl->MaxSpeed = 0.4f;

	volData = new VolumeData<unsigned int>(16, 16, 16);
	for (int i = 0; i < 255; ++i)
	{
		int x, y, z;
		x = rand() % volData->SizeX;
		y = rand() % volData->SizeY;
		z = rand() % volData->SizeZ;
		unsigned int colour = rand() % 255 + (rand() % 255 << 8) + (rand() % 255 << 16) + (255 << 24);
		volData->Get(x, y, z) = colour;
	}
	shaders.Add(new Shader("Assets/Shaders/vol.vert","Assets/Shaders/vol.frag", "DVR"));
	shaders.Add(new Shader("Assets/Shaders/copy.vert", "Assets/Shaders/copy.frag", "Copy"));
	shaders.Add(new Shader("Assets/Shaders/testred.vert", "Assets/Shaders/testred.frag", "Testred"));
	shaders.CompileShaders();
	fbos.AddFBO(new FrameBufferObject(Window.Width / 2, Window.Height, 24, 0, GL_RGBA, GL_TEXTURE_2D, "DVR"));
	fbos["DVR"]->AttachTexture("colour");
	fbos.AddFBO(new FrameBufferObject(Window.Width / 2, Window.Height, 24, 0, GL_RGBA, GL_TEXTURE_2D, "RayTrace"));
	fbos["RayTrace"]->AttachTexture("colour");
}

void Engine::Display()
{

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	fbos["DVR"]->Bind();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_BLEND);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, tex3D);

	shaders["DVR"]->Use();
	shaders["DVR"]->Uniforms["volTex"].SetValue(1);	
	shaders["DVR"]->Uniforms["size"].SetArrayValue(volData->GetSizePtr(), 3);	
	shaders["DVR"]->Uniforms["View"].SetValue(cam->GetViewTransform());
	shaders["DVR"]->Uniforms["Projection"].SetValue(cam->GetProjectionMatrix());
	shaders["DVR"]->Uniforms["World"].SetValue(HTrans4(Vec3(0, 0, -5)));

	QuadDrawer::DrawQuads(volData->Count);

	/*shaders["Testred"]->Use();

	QuadDrawer::DrawQuad(Vec2(-0.5, -0.5), Vec2(0.5, 0.5));*/

	DrawCoordFrame(cam->GetViewTransform() * cam->GetProjectionMatrix());

	fbos["DVR"]->Unbind();	

	shaders["Copy"]->Use();
	shaders["Copy"]->Uniforms["baseTex"].SetValue(0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fbos["DVR"]->GetTextureID(0));
	QuadDrawer::DrawQuad(Vec2(-1.0, -1.0), Vec2(0.0, 1.0));
	glBindTexture(GL_TEXTURE_2D, fbos["RayTrace"]->GetTextureID(0));
	QuadDrawer::DrawQuad(Vec2(0.0, -1.0), Vec2(1.0, 1.0));

	boost::format fmter("FPS: %1%, Yaw: %2%");
	fmter % CurrentFPS % cam->Yaw;

	glfwSetWindowTitle(fmter.str().c_str());

}

void Engine::Update(TimeInfo& timeInfo)
{
	if (KeyState[GLFW_KEY_LEFT])
		camControl->ChangeYaw(10.0f);
	if (KeyState[GLFW_KEY_RIGHT])
		camControl->ChangeYaw(-10.0f);
	if (KeyState['W'])
		camControl->MoveForward();
	if (KeyState['S'])
		camControl->MoveBackward();
	if (KeyState['A'])
		camControl->MoveLeft();
	if (KeyState['D'])
		camControl->MoveRight();
	camControl->Update(timeInfo.fTimeSinceLastFrame);
}

void Engine::KeyPressed(int code)
{
	if (code == GLFW_KEY_ESC)
		Exit();
	if (code == 'R')
		shaders.ReloadShaders();
}

void Engine::KeyReleased(int code)
{

}
