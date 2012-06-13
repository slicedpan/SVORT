#ifndef VOLUMEDATA_H
#define VOLUMEDATA_H

#include <stdlib.h>

template<typename T>
class VolumeData
{
public:
	VolumeData(int sizeX, int sizeY, int sizeZ)
		: SizeX(this->sizeX),
		SizeY(this->sizeY),
		SizeZ(this->sizeZ),
		Count(dataCount),
		DataSize(dataSize)
	{
		this->sizeX = sizeX;
		this->sizeY = sizeY;
		this->sizeZ = sizeZ;
		dataCount = sizeX * sizeY * sizeZ;
		dataSize = sizeof(T) * dataCount;
		data = (T*)malloc(dataSize);
	}
	void ZeroData()
	{
		memset(data, 0, dataSize);
	}
	~VolumeData(void);
	T& Get(int x, int y, int z)
	{
		return *(data + x + y * sizeX + z * sizeX * sizeY);
	}
	T& Get(int x, int y, int z) const
	{
		return *(data + x + y * sizeX + z * sizeX * sizeY);
	}
	T& operator() (int x, int y, int z)
	{
		return Get(x, y, z);
	}
	T& operator() (int x, int y, int z) const
	{
		return Get(x, y, z);
	}
	T* GetPtr()
	{
		return data;
	}
	const int* GetSizePtr() const
	{
		return (const int*)&sizeX;
	}
	const int& SizeX;
	const int& SizeY;
	const int& SizeZ;
	const int& Count;
	const size_t& DataSize;
private:
	T* data;
	int sizeX;
	int sizeY;
	int sizeZ;
	size_t dataSize;
	int dataCount;
};

#endif

