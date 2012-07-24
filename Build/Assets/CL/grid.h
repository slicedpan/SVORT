

int GetGridOffset(int3 coords, int3 gridSize)
{
	return coords.z * gridSize.x * gridSize.y + coords.y * gridSize.x + coords.x;
}