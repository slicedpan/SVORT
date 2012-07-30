
typedef struct
{
	uint numVoxels;
	uint numLeafVoxels;
	uint numLevels;
	uint pad;
} VoxelInfo;

uint GetGridOffset(uint3 coords, uint3 gridSize)
{
	return coords.z * gridSize.x * gridSize.y + coords.y * gridSize.x + coords.x;
}