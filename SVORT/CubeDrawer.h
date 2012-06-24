#pragma once

class Vec2;

struct _cubeVertex
{
	float position[4];
};

class CubeDrawer
{
public:
	static void DrawCube();
	static void DrawCubes(int number);
	~CubeDrawer();
private:
	static bool initialised;
	static CubeDrawer* instance;
	static void Initialise();
	static void CleanUp();
	void CreateVBO();
	unsigned int vboID;
	unsigned int iboID;
	unsigned char * indices;
	_cubeVertex * vertexData;
	unsigned int vaoID;
	Vec2* zero;
};