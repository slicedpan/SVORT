
#ifndef _RT_H
#define _RT_H

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
	float4 lightPos;
	float4 camPos;
} Params;

typedef struct
{
	uint numSamples;
	uint total;
} Counters;

#endif

