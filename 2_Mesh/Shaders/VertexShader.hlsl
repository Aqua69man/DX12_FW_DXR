struct ModelViewProjection
{
    matrix MVP;
};

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

struct VertexPosColor
{
    float3 Position : POSITION;
	float3 Color    : COLOR;
	float3 Normal   : NORMAL;
	float2 UV		: TEXCOORD;
};

struct VertexShaderOutput
{
	float4 Color    : COLOR;
    float4 Position : SV_Position;
};

VertexShaderOutput main(VertexPosColor IN)
{
    VertexShaderOutput OUT;

    OUT.Position = mul(ModelViewProjectionCB.MVP, float4(IN.Position, 1.0f));
	//OUT.Color = float4(IN.Position, 1.0f);
	//OUT.Color = float4(IN.Color, 1.0f);
	//OUT.Color = float4(IN.Normal, 1.0f);
	OUT.Color = float4(1 - IN.UV.x, IN.UV.x, IN.UV.y, 1.0f);

    return OUT;
}