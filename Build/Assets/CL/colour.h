
int PackColour(float4 colour)
{
	int4 iCol;
	iCol.x = (int)(colour.x * 255);
	iCol.y = (int)(colour.y * 255);
	iCol.z = (int)(colour.z * 255);
	iCol.w = (int)(colour.w * 255);
	
	return iCol.w + (iCol.z << 8) + (iCol.y << 16) + (iCol.x << 24);
}

float4 UnpackColour(int iColour)
{
	float4 colour;
	colour.x = (iColour & 0xff000000) >> 24;
	colour.y = (iColour & 0x00ff0000) >> 16;
	colour.z = (iColour & 0x0000ff00) >> 8;
	colour.w = iColour & 0x000000ff;
	
	colour /= 255.0;
	return colour;
}

