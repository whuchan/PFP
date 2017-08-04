struct PSInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 texcoord:TEXCOORD;
};

cbuffer RootConstants:register(b0)
{
	float4x4 matViewProjection;
};

PSInput main(float3 position:POSITION, float4 color : COLOR,float2 texcoord:TEXCOORD)
{
	PSInput input;
	input.position = mul(float4(position, 1.0f),matViewProjection);
	input.color = color;
	input.texcoord = texcoord;
	return input;
}