#pragma once
class VoxelOctree
{
public:
	VoxelOctree(void);
	~VoxelOctree(void);
	void Load(unsigned int* colourData, int xSize, int ySize, int zSize);
};

