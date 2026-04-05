cbuffer cbGBufferData : register( b0 )
{
    matrix m_mWorldViewProjection;
    matrix m_mWorld;
}

struct VS_INPUT
{
    float4 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 vPositionCS : SV_POSITION;
    float3 vWorldPos : TEXCOORD0;
    float3 vWorldNormal : TEXCOORD1;
};

struct PS_GBUFFER_OUTPUT
{
    float4 position : SV_Target0;
    float4 normal : SV_Target1;
    float4 tangent : SV_Target2;
    float4 motionVector : SV_Target3;
};

VS_OUTPUT VSMain( VS_INPUT input )
{
    VS_OUTPUT output;
    float4 worldPos = mul( input.vPosition, m_mWorld );

    output.vPositionCS = mul( input.vPosition, m_mWorldViewProjection );
    output.vWorldPos = worldPos.xyz;
    output.vWorldNormal = normalize( mul( input.vNormal, (float3x3)m_mWorld ) );
    return output;
}

PS_GBUFFER_OUTPUT PSGBuffer( VS_OUTPUT input )
{
    PS_GBUFFER_OUTPUT output;
    float3 normalWS = normalize( input.vWorldNormal );

    float3 upAxis = abs( normalWS.y ) < 0.999f ? float3( 0.0f, 1.0f, 0.0f ) : float3( 1.0f, 0.0f, 0.0f );
    float3 tangentWS = normalize( cross( upAxis, normalWS ) );

    output.position = float4( input.vWorldPos, 1.0f );
    output.normal = float4( normalWS * 0.5f + 0.5f, 1.0f );
    output.tangent = float4( tangentWS * 0.5f + 0.5f, 1.0f );
    output.motionVector = float4( 0.0f, 0.0f, 0.0f, 1.0f );
    return output;
}

