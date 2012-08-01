
typedef struct 
{
	float4 origin;
	float4 direction;
} Ray;

typedef struct 
{	
	float16 invWorldView;
	uint4 size;
	float4 invSize;
} Params;

typedef struct
{
	uint numSamples;
	uint total;
} Counters;



