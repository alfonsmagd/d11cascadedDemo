cbuffer cbSelectionRing : register(b0)
{
    matrix g_mViewProj;
    float4 g_vRingCenterHalfSize;
    float4 g_vRingColorTime;
};

// Controls the hollow center of the ring.
static const float kRingInnerFadeStart = 0.70f;
static const float kRingInnerFadeEnd = 0.82f;
// Controls the outer edge thickness/falloff.
static const float kRingOuterFadeStart = 0.92f;
static const float kRingOuterFadeEnd = 1.00f;
static const float kRingPulseBase = 0.65f;
static const float kRingPulseAmplitude = 0.35f;
// Main heartbeat speed of the ring.
static const float kRingPulseSpeed = 4.5f;
// Rotating highlight sweep around the ring.
static const float kRingSweepAngularSpeed = 1.9f;
static const float kRingSweepSharpness = 18.0f;
static const float kRingDirectionEpsilon = 1.0e-4f;
static const float kRingAlphaBaseWeight = 0.40f;
static const float kRingAlphaPulseWeight = 0.60f;
static const float kRingAlphaSweepWeight = 0.85f;
static const float kRingSweepHighlightStrength = 0.55f;

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 localCoord : TEXCOORD0;
};

VS_OUTPUT VSMain(uint vertexId : SV_VertexID)
{
    VS_OUTPUT output;

    float2 quadCorners[4] =
    {
        float2(-1.0f, -1.0f),
        float2(-1.0f,  1.0f),
        float2( 1.0f, -1.0f),
        float2( 1.0f,  1.0f)
    };

    const float2 localCoord = quadCorners[vertexId];
    const float3 worldPosition = float3(
        g_vRingCenterHalfSize.x + localCoord.x * g_vRingCenterHalfSize.w,
        g_vRingCenterHalfSize.y,
        g_vRingCenterHalfSize.z + localCoord.y * g_vRingCenterHalfSize.w );

    output.position = mul(float4(worldPosition, 1.0f), g_mViewProj);
    output.localCoord = localCoord;
    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    const float radius = length(input.localCoord);
    const float outerFade = 1.0f - smoothstep(kRingOuterFadeStart, kRingOuterFadeEnd, radius);
    const float innerFade = smoothstep(kRingInnerFadeStart, kRingInnerFadeEnd, radius);
    const float ringMask = saturate(outerFade * innerFade);

    const float pulse = kRingPulseBase + kRingPulseAmplitude * sin(g_vRingColorTime.w * kRingPulseSpeed);
    const float2 sweepDirection = float2(cos(g_vRingColorTime.w * kRingSweepAngularSpeed), sin(g_vRingColorTime.w * kRingSweepAngularSpeed));
    const float2 localDirection = normalize(input.localCoord + float2(kRingDirectionEpsilon, 0.0f));
    const float sweep = pow(saturate(dot(localDirection, sweepDirection)), kRingSweepSharpness);

    const float alpha = saturate(ringMask * (kRingAlphaBaseWeight + kRingAlphaPulseWeight * pulse)
        + ringMask * sweep * kRingAlphaSweepWeight);
    const float3 baseColor = g_vRingColorTime.rgb;
    const float3 color = lerp(baseColor, float3(1.0f, 1.0f, 1.0f), sweep * kRingSweepHighlightStrength) * alpha;

    return float4(color, alpha);
}
