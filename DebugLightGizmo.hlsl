cbuffer CB_DEBUG_LIGHT_GIZMO : register( b0 )
{
    matrix g_mViewProj;
};

struct VS_INPUT
{
    float3 position : POSITION;
    float4 color : COLOR0;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
};

VS_OUTPUT VSMain( VS_INPUT input )
{
    VS_OUTPUT output;
    output.position = mul( float4( input.position, 1.0f ), g_mViewProj );
    output.color = input.color;
    return output;
}

float4 PSMain( VS_OUTPUT input ) : SV_TARGET
{
    return input.color;
}
