#pragma once
#include "GLFW\GLFWEngine.h"
#include "VolumeData.h"

class ShaderManager;
class Camera;
class CameraController;

class FBOManager;
class StaticMesh;

class Engine : public GLFWEngine
{
public:
   Engine(WindowSettings& w);
   virtual ~Engine(void);
   void Setup();
   void Display();
   void Update(TimeInfo& timeInfo);
   void KeyPressed(int code);
   void KeyReleased(int code);
private:
   VolumeData<unsigned int>* volData;
   ShaderManager& shaders;
   FBOManager& fbos;
   void Init3DTexture();   
   unsigned int tex3D;
   Camera* cam;
   CameraController* camControl;
   float zCoord;
   StaticMesh* mesh1;
   StaticMesh* mesh2;
   bool drawQuad;
   bool drawMesh1;
};

