cbuffer cbDeferredDecal : register( b0 )
{
    float4 g_vDecalCenter;
    float4 g_vDecalHalfSize;
    float4 g_vProjectionNormalOpacity;
    float4 g_vDecalParams;
}

Texture2D<float4> g_tPosition : register( t0 );
Texture2D<float4> g_tNormal : register( t1 );
Texture2D<float4> g_tDecal : register( t2 );
SamplerState g_sDecal : register( s0 );

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VS_OUTPUT VSMain( uint vertexId : SV_VertexID )
{
    VS_OUTPUT output;

    float2 positions[3] =
    {
        float2( -1.0f, -1.0f ),
        float2( -1.0f,  3.0f ),
        float2(  3.0f, -1.0f )
    };

    float2 uvs[3] =
    {
        float2( 0.0f, 1.0f ),
        float2( 0.0f, -1.0f ),
        float2( 2.0f, 1.0f )
    };

    output.position = float4( positions[vertexId], 0.0f, 1.0f );
    output.uv = uvs[vertexId];
    return output;
}

float4 PSMain( VS_OUTPUT input ) : SV_TARGET
{
    const int2 pixelCoord = int2( input.position.xy );
    const float3 worldPos = g_tPosition.Load( int3( pixelCoord, 0 ) ).xyz;
    const float3 worldNormal = normalize( g_tNormal.Load( int3( pixelCoord, 0 ) ).xyz * 2.0f - 1.0f );

    if( dot( worldNormal, normalize( g_vProjectionNormalOpacity.xyz ) ) < g_vDecalParams.x )
    {
        discard;
    }

    const float3 localPos = ( worldPos - g_vDecalCenter.xyz ) / max( g_vDecalHalfSize.xyz, float3( 1.0e-4f, 1.0e-4f, 1.0e-4f ) );
    if( abs( localPos.x ) > 1.0f || abs( localPos.y ) > 1.0f || abs( localPos.z ) > 1.0f )
    {
        discard;
    }

    const float2 decalUV = localPos.xz * 0.5f + 0.5f;
    const float4 decalSample = g_tDecal.Sample( g_sDecal, decalUV );
    return float4( decalSample.rgb, decalSample.a * g_vProjectionNormalOpacity.w );
}
