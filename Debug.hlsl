Texture2D<float> g_textureDebug : register(t0);
SamplerState g_samplerDebug : register(s0);

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VS_OUTPUT VSMain(uint vertexId : SV_VertexID)
{
    VS_OUTPUT output;

    float2 positions[4] =
    {
        float2(-1.0f, -1.0f),
        float2(-1.0f, 1.0f),
        float2(1.0f, -1.0f),
        float2(1.0f, 1.0f)
    };

    float2 uvs[4] =
    {
        float2(0.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(1.0f, 0.0f)
    };

    output.position = float4(positions[vertexId], 0.0f, 1.0f);
    output.uv = uvs[vertexId];

    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    float value = g_textureDebug.Sample(g_samplerDebug, input.uv);
    return float4(value, 0, 0, 1.0f);
}