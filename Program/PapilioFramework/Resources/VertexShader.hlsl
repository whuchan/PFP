struct PSInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 texcoord:TEXCOORD;
};

PSInput main(float3 position:POSITION, float4 color : COLOR,float2 texcoord:TEXCOORD)
{
	PSInput input;
	input.position = float4(position, 1.0f);
	input.color = color;
	input.texcoord = texcoord;
	return input;
}