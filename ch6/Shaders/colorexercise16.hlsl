//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

//fixed for exercise16
//예제16번에 맞게 수정되었음

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj; 
	float4 gPulseColor;
	float gTime;
};

struct VertexIn
{
	float3 PosL  : POSITION;
    float4 Color : COLOR;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
    float4 Color : COLOR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
	
	// Just pass vertex color into the pixel shader.
    vout.Color = vin.Color;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	const float pi = 3.14159;

	float s = 0.5f * sin(2 * gTime - 0.25f * pi) + 0.5f;

	float4 c = lerp(pin.Color, gPulseColor, s);

	return c;
}


