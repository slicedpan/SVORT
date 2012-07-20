

__kernel void DrawVoxels(__write_only image2d_t output, __global int* input, int4 sizeAndLayer)
{
	int2 coords = (int2)(get_global_id(0), get_global_id(1));
	
	int pos = sizeAndLayer.w * sizeAndLayer.x * sizeAndLayer.y + coords.y * sizeAndLayer.x + coords.x;
	
	float4 colour;
	colour.x = input[pos] & 0xff000000;
	colour.y = input[pos] & 0x00ff0000;
	colour.z = input[pos] & 0x0000ff00;
	colour.w = input[pos] & 0x000000ff;
	
	colour /= 255.0;
	colour.x = 1.0;
	
	colour = (float4)(0.0, 1.0, 0.0, 1.0);
	
	write_imagef(output, coords, colour);
	
}