
const sampler_t samplerA = CLK_NORMALIZED_COORDS_FALSE | 
										CLK_ADDRESS_CLAMP | 
										CLK_FILTER_NEAREST;

__kernel void VolRT(__write_only image2d_t bmp, __read_only image3d_t volTex, int zCoord)
{

	int x = get_global_id(0);
	int y = get_global_id(1); 
	int2 coords = (int2)(x,y);
	int3 inputCoord = (int3)(x, y, zCoord);	
	
		//Attention to RGBA order
	float4 val = read_imagef(volTex, samplerA, inputCoord);
	write_imagef(bmp, coords, val);	

}