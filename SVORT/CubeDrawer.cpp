
#include "CubeDrawer.h"
#include <cstdlib>
#include <GL\glew.h>
#include <svl\SVL.h>

bool CubeDrawer::initialised = false;
CubeDrawer* CubeDrawer::instance = 0;

void CubeDrawer::DrawCubes(int amount)
{
	if (!instance)
		Initialise();
	glBindVertexArray(instance->vaoID);
	glDrawElementsInstanced(GL_QUADS, 24, GL_UNSIGNED_BYTE, (void*)0, amount);
	glBindVertexArray(0);
}

void CubeDrawer::DrawCube()
{
	if (!instance)
		Initialise();	

	glBindVertexArray(instance->vaoID);	
	glDrawElements(GL_QUADS, 24, GL_UNSIGNED_BYTE, (void*)0);
	glBindVertexArray(0);
}

void CopyIndicesTo(unsigned char* dest, unsigned char i1, unsigned char i2, unsigned char i3, unsigned char i4)
{
	dest[0] = i1;
	dest[1] = i2;
	dest[2] = i3;
	dest[3] = i4;
}

void CubeDrawer::Initialise()
{
	instance = new CubeDrawer();
	glGenBuffers(1, &instance->vboID);
	glGenBuffers(1, &instance->iboID);
	glGenVertexArrays(1, &instance->vaoID);	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, instance->iboID);
	instance->indices = (unsigned char*)malloc(sizeof(unsigned char) * 24);
	CopyIndicesTo(instance->indices, 0, 1, 2, 3);
	CopyIndicesTo(instance->indices + 4, 0, 4, 5, 1);
	CopyIndicesTo(instance->indices + 8, 0, 3, 7, 4);
	CopyIndicesTo(instance->indices + 12, 1, 5, 6, 2);
	CopyIndicesTo(instance->indices + 16, 2, 6, 7, 3);
	CopyIndicesTo(instance->indices + 20, 7, 6, 5, 4);
	
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned char) * 24, instance->indices, GL_STATIC_DRAW);

	instance->vertexData = (_cubeVertex*)malloc(sizeof(_cubeVertex) * 8);
	instance->vertexData[0].position[0] = 0.0f;
	instance->vertexData[0].position[1] = 0.0f;
	instance->vertexData[0].position[2] = 0.0f;

	for (int i = 1; i < 8; ++i)
	{
		memcpy(instance->vertexData + i, instance->vertexData, sizeof(_cubeVertex)); 
	}

	instance->vertexData[1].position[0] = 1.0f;
	
	instance->vertexData[2].position[0] = 1.0f;
	instance->vertexData[2].position[2] = 1.0f;

	instance->vertexData[3].position[2] = 1.0f;

	instance->vertexData[4].position[1] = 1.0f;
	
	instance->vertexData[5].position[0] = 1.0f;
	instance->vertexData[5].position[1] = 1.0f;
	
	instance->vertexData[6].position[0] = 1.0f;
	instance->vertexData[6].position[1] = 1.0f;
	instance->vertexData[6].position[2] = 1.0f;

	instance->vertexData[7].position[1] = 1.0f;
	instance->vertexData[7].position[2] = 1.0f;

	glBindBuffer(GL_ARRAY_BUFFER, instance->vboID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(_cubeVertex) * 8, instance->vertexData, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindVertexArray(instance->vaoID);

	glBindBuffer(GL_ARRAY_BUFFER, instance->vboID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, instance->iboID);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(_cubeVertex), (char*)0);
	glBindVertexArray(0);
	initialised = true;

}

void CubeDrawer::CleanUp()
{
	delete instance;
}

CubeDrawer::~CubeDrawer()
{
	free(indices);
	free(vertexData);

}
