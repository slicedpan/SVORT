
#ifndef _COLOUR_H
#define _COLOUR_H

uint PackColour(float4 colour)
{
	uint4 iCol;
	iCol.s3 = (uint)(colour.s3 * 255.0f) & 255;
	iCol.s2 = (uint)(colour.s2 * 255.0f) & 255;
	iCol.s1 = (uint)(colour.s1 * 255.0f) & 255;
	iCol.s0 = (uint)(colour.s0 * 255.0f) & 255;
	
	return iCol.s0 + (iCol.s1 << 8) + (iCol.s2 << 16) + (iCol.s3 << 24);
}

float4 UnpackColour(uint iColour)
{
	float4 colour;
	colour.s3 = (iColour & 0xff000000) >> 24;
	colour.s2 = (iColour & 0x00ff0000) >> 16;
	colour.s1 = (iColour & 0x0000ff00) >> 8;
	colour.s0 = iColour & 0x000000ff;
	
	colour.s0 /= 255.0f;
	colour.s1 /= 255.0f;
	colour.s2 /= 255.0f;
	colour.s3 /= 255.0f;
	return colour;
}

uint2 PackColourAndNormal(float4 colour, float4 normal)
{
	uint2 ret;
	ret.s0 = PackColour(colour);
	ret.s1 = PackColour(normal);
	return ret;
}

void PackNormal(float4 normal, uint* iColour, __global float4* normalLookup)	//this could be more efficient, but doesn't really need to be
{
	uint octant = 0;
	if (normal.s0 > 0.0f)
		octant |= 1;
	if (normal.s1 > 0.0f)
		octant |= 2;
	if (normal.s2 > 0.0f)
		octant |= 4;
	uint baseOffset = octant * 32;
	float maxValue = 0.0f;
	int i = 0;
	uint bestMatch;
	for (; i < 32; ++i)
	{
		float4 curNormal = normalLookup[baseOffset + i];
		float dotProduct = dot(normal, curNormal);
		if (dotProduct > maxValue)
		{
			maxValue = dotProduct;
			bestMatch = i;
		}

	}
	*iColour |= (bestMatch + baseOffset);
	
}

#endif
