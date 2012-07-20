__constant const sampler_t sampler2D = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP;

__kernel void FillVolume(image2d_t input, __global int* output, int4 sizeAndLayer)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	int2 coords = (int2)(x, y);
	
	int pos = sizeAndLayer.w * sizeAndLayer.x * sizeAndLayer.y + y * sizeAndLayer.x + x;
	
	float4 colour = read_imagef(input, sampler2D, coords);
	int4 iCol;
	iCol.x = (int)(colour.x * 255);
	iCol.y = (int)(colour.y * 255);
	iCol.z = (int)(colour.z * 255);
	iCol.w = (int)(colour.w * 255);
	
	int colValue = iCol.w + (iCol.z << 8) + (iCol.y << 16) + (iCol.x << 24);
	
	output[pos] = colValue;
	
}