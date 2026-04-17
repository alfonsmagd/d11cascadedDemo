Texture2D g_tLeft : register( t0 );
Texture2D g_tRight : register( t1 );
SamplerState g_sPoint : register( s0 );

cbuffer ComparatorConstants : register( b0 )
{
    float4 g_vViewportSizeAndSplit;
    float4 g_vLineWidthHandleRadius;
    float4 g_vLineColor;
}

struct VS_OUTPUT
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
};

VS_OUTPUT VSMain( uint vertexID : SV_VertexID )
{
    VS_OUTPUT output;

    float2 positions[3] =
    {
        float2( -1.0f, -1.0f ),
        float2( -1.0f,  3.0f ),
        float2(  3.0f, -1.0f )
    };

    float2 texCoords[3] =
    {
        float2( 0.0f, 1.0f ),
        float2( 0.0f, -1.0f ),
        float2( 2.0f, 1.0f )
    };

    output.Position = float4( positions[vertexID], 0.0f, 1.0f );
    output.TexCoord = texCoords[vertexID];
    return output;
}

float4 PSMain( VS_OUTPUT input, float4 screenPosition : SV_Position ) : SV_Target
{
    float2 uv = input.TexCoord;
    float split = saturate( g_vViewportSizeAndSplit.z );
    float splitX = g_vViewportSizeAndSplit.x * split;

    float4 leftColor = g_tLeft.Sample( g_sPoint, uv );
    float4 rightColor = g_tRight.Sample( g_sPoint, uv );
    float4 outColor = ( uv.x <= split ) ? leftColor : rightColor;

    float lineWidth = max( 1.0f, g_vLineWidthHandleRadius.x );
    float handleRadius = max( 2.0f, g_vLineWidthHandleRadius.y );

    if( abs( screenPosition.x - splitX ) <= lineWidth )
    {
        outColor = g_vLineColor;
    }

    float2 handleCenter = float2( splitX, g_vViewportSizeAndSplit.y * 0.5f );
    float handleDistance = distance( screenPosition.xy, handleCenter );
    if( handleDistance <= handleRadius )
    {
        outColor = g_vLineColor;
    }

    return outColor;
}
