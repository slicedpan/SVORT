#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

#include "Octree.h"
#include "grid.h"
#include "colour.h"

typedef struct 
{
	uint inOffset;
	uint outOffset;
	uint level;
	uint pad;
} OctParams;

__kernel void CreateOctree(__global int* input, __global Block* output, __constant OctreeInfo* octInfo,  __global int* counters, OctParams op)
{
	uint3 coords = (uint3)(get_global_id(0), get_global_id(1), get_global_id(2));
	uint3 workSize = (uint3)(get_global_size(0), get_global_size(1), get_global_size(2));	
	
	uint gridOffset = GetGridOffset(coords, workSize);	
	__global Block* current;
	if (op.level == 0)		//outermost level
	{
		current = output + gridOffset;
		current->colour = input[op.inOffset + gridOffset];
		current->data = 0;
		setChildPtr(current, atom_add(counters, 8) - gridOffset);
	}
	else
	{
		uint3 relativeCoords = coords;
		uint3 relativeSize = workSize;
		uint curPos = 0;
		uint octant;
		__global Block* parent = 0;		
		for (int i = 0; i < op.level && i < 16; ++i)	//descend hierarchy
		{			
			//figure out which octant this voxel is in
			octant = getOctant(relativeCoords, relativeSize);

			if (parent)	//parent pointer was set last iteration
			{
				if (!getValid(parent, octant))	//we can finish here, since there are no valid subvoxels
					return;			
			}
		
			//get pointer to parent
			curPos += octant;
			parent = output + curPos;
			curPos += getChildPtr(parent);				

			reduceOctant(&relativeCoords, &relativeSize);
		}		
		
		octant = getOctant(relativeCoords, relativeSize);

		curPos += octant;
		current = output + curPos;
		current->colour = input[op.inOffset + gridOffset];	//read colour data
		current->data = 0;		
		float4 fCol = UnpackColour(current->colour);
		
		if (fCol.w != 0.0)
		{			
			if (op.level + 1 < octInfo->numLevels)
			{
				setChildPtr(current, atom_add(counters, 8) - curPos);	//only set child pointer if this is a non-empty non-leaf voxel
			}
		}
		else
		{
			atom_add(&parent->data, validFlagValue(octant));	//set as invalid
		}		
	}
}

__kernel void CreateTestOctree(__global int* input, __global Block* output, __constant OctreeInfo* octInfo,  __global int* counters, OctParams op)
{
	uint3 coords = (uint3)(get_global_id(0), get_global_id(1), get_global_id(2));
	uint3 workSize = (uint3)(get_global_size(0), get_global_size(1), get_global_size(2));	

	float3 relCoords = (float3)(coords.x / (float)workSize.x, coords.y / (float)workSize.y, coords.z / (float)workSize.z);
	
	uint gridOffset = GetGridOffset(coords, workSize);	
	__global Block* current;
	if (op.level == 0)		//outermost level
	{
		current = output + gridOffset;
		current->colour = input[op.inOffset + gridOffset];
		current->data = 0;
		setChildPtr(current, atom_add(counters, 8) - gridOffset);
	}
	else
	{
		uint3 relativeCoords = coords;
		uint3 relativeSize = workSize;
		uint curPos = 0;
		uint octant;
		__global Block* parent = 0;		
		for (int i = 0; i < op.level && i < 16; ++i)	//descend hierarchy
		{			
			//figure out which octant this voxel is in
			octant = getOctant(relativeCoords, relativeSize);

			if (parent)	//parent pointer was set last iteration
			{
				if (!getValid(parent, octant))	//we can finish here, since there are no valid subvoxels
					return;					
			}
		
			//get pointer to parent
			curPos += octant;
			parent = output + curPos;
			curPos += getChildPtr(parent);				

			reduceOctant(&relativeCoords, &relativeSize);
		}		
		
		octant = getOctant(relativeCoords, relativeSize);

		curPos += octant;
		current = output + curPos;
		current->colour = PackColour((float4)(relCoords.x, relCoords.y, relCoords.z, 1.0));	//read colour data
		current->data = 0;		
		float4 fCol = UnpackColour(current->colour);
		
		if (fCol.w != 0.0)
		{			
			if (op.level + 1 < octInfo->numLevels)
			{
				setChildPtr(current, atom_add(counters, 8) - curPos);	//only set child pointer if this is a non-empty non-leaf voxel
			}
		}
		else
		{
			atom_add(&parent->data, validFlagValue(octant));	//set as invalid
		}		
	}
}

