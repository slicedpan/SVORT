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

struct PackedColour
{
	unsigned a : 8;
	unsigned b : 8;
	unsigned g : 8;
	unsigned r : 8;
};

VoxelBuilder::VoxelBuilder(void)
{
	memset(&ocl, 0, sizeof(ocl));
	debugDraw = false;
	debugDrawShader = 0;
}

VoxelBuilder::~VoxelBuilder(void)
{
	Cleanup();
}

void VoxelBuilder::Cleanup()
{
	if (ocl.voxelData)
		clReleaseMemObject(ocl.voxelData);
	if (ocl.fillKernel)
		clReleaseKernel(ocl.fillKernel);
	if (ocl.fillProgram)
		clReleaseProgram(ocl.fillProgram);
	if (ocl.mipKernel)
		clReleaseKernel(ocl.mipKernel);
	if (ocl.mipProgram)
		clReleaseProgram(ocl.mipProgram);
	if (ocl.queue)
		clReleaseCommandQueue(ocl.queue);
	memset(&ocl, 0, sizeof(ocl));
}


void VoxelBuilder::SetDebugDrawShader(Shader* shader)
{
	debugDrawShader = shader;
}

void VoxelBuilder::SetDebugDraw(bool enabled)
{
	debugDraw = enabled;
}

void VoxelBuilder::GenerateMipmaps()
{
	cl_int4 inSizeOffset;
	cl_int4 outSizeOffset;
	memcpy(inSizeOffset.s, dims, sizeof(int) * 3);
	memcpy(outSizeOffset.s, dims, sizeof(int) * 3);

	inSizeOffset.s[3] = 0;
	outSizeOffset.s[3] = inSizeOffset.s[0] * inSizeOffset.s[1] * inSizeOffset.s[2];

	clSetKernelArg(ocl.mipKernel, 0, sizeof(cl_mem), &ocl.voxelData);
	clSetKernelArg(ocl.mipKernel, 3, sizeof(cl_mem), &ocl.octreeInfo);

	for (int i = 0; i < 3; ++i)
		outSizeOffset.s[i] /= 2;	

	for (int i = 0; i < maxMips; ++i)
	{
		clFinish(ocl.queue);
		clSetKernelArg(ocl.mipKernel, 1, sizeof(cl_int4), &inSizeOffset);
		clSetKernelArg(ocl.mipKernel, 2, sizeof(cl_int4), &outSizeOffset);	

		clEnqueueReadBuffer(ocl.queue, ocl.octreeInfo, true, 0, sizeof(octInfo), &octInfo, 0, NULL, NULL);
		
		size_t workDim[3] = { outSizeOffset.s[0], outSizeOffset.s[1], outSizeOffset.s[2] };

		octInfo.pad = workDim[0] * workDim[0] * workDim[0] * 8;
		clEnqueueWriteBuffer(ocl.queue, ocl.octreeInfo, true, 0, sizeof(octInfo), &octInfo, 0, NULL, NULL);

		clEnqueueNDRangeKernel(ocl.queue, ocl.mipKernel, 3, NULL, workDim, NULL, 0, NULL, NULL);

		outSizeOffset.s[3] += outSizeOffset.s[0] * outSizeOffset.s[1] * outSizeOffset.s[2];
		inSizeOffset.s[3] += inSizeOffset.s[0] * inSizeOffset.s[1] * inSizeOffset.s[2];		

		for (int j = 0; j < 3; ++j)
		{
			outSizeOffset.s[j] /= 2;
			inSizeOffset.s[j] /= 2;
		}		
	}
	clEnqueueReadBuffer(ocl.queue, ocl.octreeInfo, false, 0, sizeof(octInfo), &octInfo, 0, NULL, NULL);
	clFinish(ocl.queue);
	printf("Total number of voxels: %d\nTotal number of leaf voxels: %d\nDetail levels: %d\nCompression ratio: %lf\n", octInfo.numVoxels, octInfo.numLeafVoxels, octInfo.numLevels, (float)octInfo.numVoxels / (float) octInfo.numLeafVoxels);
}

cl_mem VoxelBuilder::GetVoxelData()
{
	return ocl.voxelData;
}

int VoxelBuilder::GetMaxNumMips()
{
	return maxMips;
}

void VoxelBuilder::Build(StaticMesh* mesh, int* dimensions, Shader* meshRenderer, cl_mem octreeInfo, cl_mem normalLookup, int octant)
{
	memcpy(dims, dimensions, sizeof(int) * 3);
	if (octant >= 0)
	{
		dims[0] /= 2;
		dims[1] /= 2;
		dims[2] /= 2;
	}
	cl_int resultCL = 0;
	printf("\n\nBuilding voxel data\n");
	printf("Creating output buffer....\n");
	ocl.octreeInfo = octreeInfo;
	if (ocl.voxelData)
		clReleaseMemObject(ocl.voxelData);
	ocl.voxelData = clCreateBuffer(ocl.context, CL_MEM_READ_WRITE, sizeof(unsigned int) * dims[0] * dims[1] * dims[2] * 2.30, NULL, &resultCL);
	CLGLError(resultCL);
	
	FrameBufferObject* fbo = new FrameBufferObject(dims[0], dims[1], 0, 0, GL_RGBA, GL_TEXTURE_2D, "vox");
	fbo->AttachTexture("colour");
	fbo->AttachTexture("normal");
	fbo->SetDrawBuffers(true);

#ifdef INTEL

	printf("Creating colour input buffer....\n");	
	unsigned int* colourData = (unsigned int*)malloc(sizeof(unsigned int) * dims[0] * dims[1]);
	cl_image_format colourFormat;
	colourFormat.image_channel_order = CL_RGBA;
	colourFormat.image_channel_data_type = CL_UNORM_INT8;
	ocl.inputData[0] = clCreateImage2D(ocl.context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, &colourFormat, dims[0], dims[1], 0, colourData, &resultCL);
	CLGLError(resultCL);

	printf("Creating normal input buffer....\n");
	unsigned int* normalData = (unsigned int*)malloc(sizeof(unsigned int) * dims[0] * dims[1]);	
	ocl.inputData[1] = clCreateImage2D(ocl.context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, &colourFormat, dims[0], dims[1], 0, normalData, &resultCL);
	CLGLError(resultCL);

#else

	printf("Creating colour input buffer....\n");
	ocl.inputData[0] = clCreateFromGLTexture2D(ocl.context, CL_MEM_READ_ONLY, GL_TEXTURE_2D, 0, fbo->GetTextureID("colour"), &resultCL);
	CLGLError(resultCL);
	printf("Creating normal input buffer....\n");
	ocl.inputData[1] = clCreateFromGLTexture2D(ocl.context, CL_MEM_READ_ONLY, GL_TEXTURE_2D, 0, fbo->GetTextureID("normal"), &resultCL);
	CLGLError(resultCL);

#endif

	clSetKernelArg(ocl.fillKernel, 0, sizeof(cl_mem), &(ocl.inputData[0]));
	clSetKernelArg(ocl.fillKernel, 1, sizeof(cl_mem), &(ocl.inputData[1]));
	clSetKernelArg(ocl.fillKernel, 2, sizeof(cl_mem), &ocl.voxelData);
	clSetKernelArg(ocl.fillKernel, 4, sizeof(cl_mem), &normalLookup);

	cl_int4 size;
	memcpy(size.s, dims, sizeof(int) * 3);

	float zDist;
	int numSlices;
	float increment;

	float xMax, xMin, yMax, yMin;

	if (octant < 0)
	{
		zDist = mesh->SubMeshes[0].Max[2] - mesh->SubMeshes[0].Min[2];
		numSlices = dimensions[2];
		increment = zDist / numSlices;
		zDist = - increment;
		xMax = mesh->SubMeshes[0].Max[0];
		xMin = mesh->SubMeshes[0].Min[0];
		yMax = mesh->SubMeshes[0].Max[1];
		yMin = mesh->SubMeshes[0].Min[1];
	}
	else
	{
		numSlices = dimensions[2] / 2;		
		zDist = (mesh->SubMeshes[0].Max[2] - mesh->SubMeshes[0].Min[2]) / 2.0f;
		increment = zDist / numSlices;
		if (octant & 4)
			zDist = (zDist / 2.0f);
		else
			zDist = 0.0f;
		zDist -= increment;

		if (octant & 1)
		{
			xMin = (mesh->SubMeshes[0].Max[0] - mesh->SubMeshes[0].Max[0]) / 2.0f;			
			xMax = mesh->SubMeshes[0].Max[0];
		}
		else
		{
			xMin = mesh->SubMeshes[0].Min[0];
			xMax = (mesh->SubMeshes[0].Max[0] - mesh->SubMeshes[0].Min[0]) / 2.0f;
		}

		if (octant & 2)
		{
			yMin = (mesh->SubMeshes[0].Max[1] - mesh->SubMeshes[0].Min[1]) / 2.0f;
			yMax = mesh->SubMeshes[0].Max[1];
		}
		else
		{
			yMin = mesh->SubMeshes[0].Min[1];
			yMax = (mesh->SubMeshes[0].Max[1] - mesh->SubMeshes[0].Min[1]) / 2.0f;
		}
	}
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	for (int i = 0; i < numSlices; ++i)
	{
		fbo->Bind();

		size.s[3] = i;

		glClear(GL_COLOR_BUFFER_BIT);

		float camDist = mesh->SubMeshes[0].Max[2] - mesh->SubMeshes[0].Min[2];
		camDist /= 2.0f;

		Mat4 ortho = Orthographic(xMin, xMax, yMin, yMax, zDist, zDist + (increment * 5.0f));

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
			glBindTexture(GL_TEXTURE_2D, fbo->GetTextureID("colour"));
			debugDrawShader->Use();
			debugDrawShader->Uniforms["baseTex"].SetValue(0);
			QuadDrawer::DrawQuad(Vec2(-1.0, -1.0), Vec2(1.0, 1.0), Vec2(1.0, 1.0));
		}

#ifdef INTEL
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, colourData);
		glBindTexture(GL_TEXTURE_2D, fbo->GetTextureID("normal"));
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, normalData);
#endif

		glFinish();
		glfwSwapBuffers();

		clSetKernelArg(ocl.fillKernel, 3, sizeof(cl_int4), &size);		
		clEnqueueAcquireGLObjects(ocl.queue, 2, ocl.inputData, 0, NULL, NULL);

		size_t workSize[2];
		workSize[0] = dims[0];
		workSize[1] = dims[1];

		clEnqueueNDRangeKernel(ocl.queue, ocl.fillKernel, 2, NULL, workSize, NULL, 0, NULL, NULL);
		clEnqueueReleaseGLObjects(ocl.queue, 2, ocl.inputData, 0, NULL, NULL);			

		clFinish(ocl.queue);

		char buf[64];
		sprintf(buf, "Creating Volume: %d / %d", i + 1, dimensions[2]);
		glfwSetWindowTitle(buf);

		zDist += increment;

	}

	clReleaseMemObject(ocl.inputData[0]);
	clReleaseMemObject(ocl.inputData[1]);
	delete fbo;
	
	maxMips = 1;
	int counter = dims[0];
	octInfo.numLeafVoxels = 0;
	while(counter >= 1)
	{
		octInfo.numLeafVoxels += counter * counter * counter;
		counter /= 2;
		++maxMips;
	}	
	octInfo.numVoxels = octInfo.numLeafVoxels;
	octInfo.numLevels = maxMips;
	clEnqueueWriteBuffer(ocl.queue, ocl.octreeInfo, false, 0, sizeof(octInfo), &octInfo, 0, NULL, NULL);
	clFinish(ocl.queue);

	/*std::vector<PackedColour> colourData;
	colourData.resize(dimensions[0] * dimensions[0] * dimensions[0]);
	clEnqueueReadBuffer(ocl.queue, ocl.voxelData, true, 0, sizeof(PackedColour) * colourData.size(), &colourData[0], 0, NULL, NULL);*/

	GenerateMipmaps();	//this is not glGenerateMipmaps()
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
