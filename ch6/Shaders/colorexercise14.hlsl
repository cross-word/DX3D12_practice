//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

//fixed for exercise14
//exercise14에 맞게 수정되었음.

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj; 
	float TotalTime;
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

float4 ColorVariation(float4 Color)
{
	float4 Color_ = { Color.x * sin(TotalTime), Color.y * cos(TotalTime), Color.z * sin(TotalTime), Color.w };
	return Color_;
}

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
	
	// Just pass vertex color into the pixel shader.
    vout.Color = ColorVariation(vin.Color);
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return ColorVariation(pin.Color);
}


