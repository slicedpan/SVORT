#define CL_USE_DEPRECATED_OPENCL_1_1_APIS

#include "VoxelBuilder.h"
#include <string.h>
#include <stdlib.h>
#include "Shader.h"
#include "Utils.h"
#include "CLUtils.h"
#include "StaticMesh.h"
#include "FrameBufferObject.h"
#include <GL\glfw.h>
#include <CL\cl_gl.h>
#include "QuadDrawer.h"

VoxelBuilder::VoxelBuilder(void)
{
	memset(&ocl, 0, sizeof(ocl));
	debugDraw = false;
	debugDrawShader = 0;
}

VoxelBuilder::~VoxelBuilder(void)
{
	if (ocl.voxelData)
		clReleaseMemObject(ocl.voxelData);
}

void VoxelBuilder::SetDebugDrawShader(Shader* shader)
{
	debugDrawShader = shader;
}

void VoxelBuilder::SetDebugDraw(bool enabled)
{
	debugDraw = enabled;
}

cl_mem VoxelBuilder::GetVoxelData()
{
	return ocl.voxelData;
}

void VoxelBuilder::Build(StaticMesh* mesh, int* dimensions, Shader* meshRenderer)
{
	cl_int resultCL = 0;
	printf("Building voxel data\n");
	printf("Creating output buffer....\n");
	if (!ocl.voxelData)
		ocl.voxelData = clCreateBuffer(ocl.context, CL_MEM_READ_WRITE, sizeof(unsigned int) * dimensions[0] * dimensions[1] * dimensions[2] * 1.15, NULL, &resultCL);
	CLGLError(resultCL);
	
	FrameBufferObject* fbo = new FrameBufferObject(dimensions[0], dimensions[1], 0, 0, GL_RGBA, GL_TEXTURE_2D, "vox");
	fbo->AttachTexture("colour");
	printf("Creating input buffer....\n");
	ocl.inputData = clCreateFromGLTexture2D(ocl.context, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, fbo->GetTextureID(0), &resultCL);
	CLGLError(resultCL);

	clSetKernelArg(ocl.fillKernel, 0, sizeof(cl_mem), &ocl.inputData);
	clSetKernelArg(ocl.fillKernel, 1, sizeof(cl_mem), &ocl.voxelData);

	cl_int4 size;
	memcpy(size.s, dimensions, sizeof(int) * 3);

	float zDist = mesh->SubMeshes[0].Max[2] - mesh->SubMeshes[0].Min[2];
	float increment = zDist / dimensions[2];
	zDist = - increment;

	for (int i = 0; i < dimensions[2]; ++i)
	{
		fbo->Bind();

		size.s[3] = i;

		glClear(GL_COLOR_BUFFER_BIT);

		float camHeight = mesh->SubMeshes[0].Max[1] - mesh->SubMeshes[0].Min[1];
		camHeight /= 2.0f;

		float camDist = mesh->SubMeshes[0].Max[2] - mesh->SubMeshes[0].Min[2];
		camDist /= 2.0f;

		Mat4 ortho = Orthographic(mesh->SubMeshes[0].Min[0], mesh->SubMeshes[0].Max[0], mesh->SubMeshes[0].Min[1], mesh->SubMeshes[0].Max[1], zDist, zDist + (increment * 5.0f));

		meshRenderer->Use();
		meshRenderer->Uniforms["World"].SetValue(Mat4(vl_one));
		meshRenderer->Uniforms["View"].SetValue(HTrans4(Vec3(0, 0, -camDist)));
		meshRenderer->Uniforms["Projection"].SetValue(ortho);
		meshRenderer->Uniforms["lightCone"].SetValue(Vec3(0, -1, 0));
		meshRenderer->Uniforms["lightPos"].SetValue(Vec3(0, 10, 0));
		meshRenderer->Uniforms["lightRadius"].SetValue(15.0f);

		glEnable(GL_DEPTH_TEST);
		mesh->SubMeshes[0].Draw();

		fbo->Unbind();	

		glDisable(GL_DEPTH_TEST);

		if (debugDraw && debugDrawShader)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fbo->GetTextureID(0));
			debugDrawShader->Use();
			debugDrawShader->Uniforms["baseTex"].SetValue(0);
			QuadDrawer::DrawQuad(Vec2(-1.0, -1.0), Vec2(1.0, 1.0), Vec2(1.0, 1.0));
		}

		glFinish();
		glfwSwapBuffers();

		clSetKernelArg(ocl.fillKernel, 2, sizeof(cl_int4), &size);		
		clEnqueueAcquireGLObjects(ocl.queue, 1, &ocl.inputData, 0, NULL, NULL);

		size_t workSize[2];
		workSize[0] = dimensions[0];
		workSize[1] = dimensions[1];

		clEnqueueNDRangeKernel(ocl.queue, ocl.fillKernel, 2, NULL, workSize, NULL, 0, NULL, NULL);
		clEnqueueReleaseGLObjects(ocl.queue, 1, &ocl.inputData, 0, NULL, NULL);			

		clFinish(ocl.queue);

		char buf[64];
		sprintf(buf, "Creating Volume: %d / %d", i + 1, dimensions[2]);
		glfwSetWindowTitle(buf);

		zDist += increment;

	}
	delete fbo;
	clReleaseMemObject(ocl.inputData);

}

void VoxelBuilder::CreateKernels()
{
	oclKernel k;
	k = CreateKernelFromFile("Assets/CL/FillVolume.cl", "FillVolume", ocl.context, ocl.device);

	if (ocl.fillKernel)
		clReleaseKernel(ocl.fillKernel);
	if (ocl.fillProgram)
		clReleaseProgram(ocl.fillProgram);
	if (ocl.mipKernel)
		clReleaseKernel(ocl.mipKernel);
	if (ocl.mipProgram)
		clReleaseProgram(ocl.mipProgram);

	if (k.ok)
	{
		ocl.fillKernel = k.kernel;
		ocl.fillProgram = k.program;
	}

	k = CreateKernelFromFile("Assets/CL/mip.cl", "Mip", ocl.context, ocl.device);
	if (k.ok)
	{
		ocl.mipKernel = k.kernel;
		ocl.mipProgram = k.program;
	}

}

void VoxelBuilder::ReloadProgram()
{
	CreateKernels();
}

void VoxelBuilder::Init(cl_context context, cl_device_id device)
{
	cl_int resultCL;
	ocl.context = context;
	ocl.device = device;
	printf("Creating command queue for Voxel Builder....\n");
	ocl.queue = clCreateCommandQueue(ocl.context, ocl.device, 0, &resultCL);
	CLGLError(resultCL);
	CreateKernels();
}
