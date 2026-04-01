cbuffer cbSelectionBeam : register(b0)
{
    matrix g_mViewProj;
    float4 g_vBeamCenterWidth;
    float4 g_vBeamColorTime;
    float4 g_vBeamParams;
};

static const float kBeamQuadMinX = -1.0f;
static const float kBeamQuadMaxX = 1.0f;
static const float kBeamQuadMinY = 0.0f;
static const float kBeamQuadMaxY = 0.5f;
// Beam body width across the quad.
static const float kBeamHorizontalCoreStart = 0.10f;
static const float kBeamHorizontalCoreEnd = 1.00f;
// Soft fade-in from the base and fade-out towards the top.
static const float kBeamVerticalFadeInStart = 0.00f;
static const float kBeamVerticalFadeInEnd = 0.08f;
static const float kBeamVerticalFadeOutStart = 0.68f;
static const float kBeamVerticalFadeOutEnd = 1.00f;
static const float kBeamPulseBase = 0.72f;
static const float kBeamPulseAmplitude = 0.28f;
// Global intensity pulse speed.
static const float kBeamPulseSpeed = 110.4f;
// Internal moving bands inside the beam.
static const float kBeamShimmerSpatialFrequency = 13.0f;
static const float kBeamShimmerTemporalSpeed = 5.2f;
static const float kBeamShimmerPower = 4.5f;
static const float kBeamCorePower = 1.8f;
static const float kBeamHaloPower = 0.65f;
static const float kBeamHaloStrength = 0.35f;
static const float kBeamAlphaCoreBase = 0.45f;
static const float kBeamAlphaCorePulse = 0.45f;
static const float kBeamAlphaCoreShimmer = 0.35f;
static const float kBeamWhiteMixStrength = 0.45f;

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 localCoord : TEXCOORD0;
};

VS_OUTPUT VSMain(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
{
    VS_OUTPUT output;

    float2 quadCorners[4] =
    {
        float2(kBeamQuadMinX, kBeamQuadMinY),
        float2(kBeamQuadMinX, kBeamQuadMaxY),
        float2(kBeamQuadMaxX, kBeamQuadMinY),
        float2(kBeamQuadMaxX, kBeamQuadMaxY)
    };

    const float2 localCoord = quadCorners[vertexId];
    const float3 horizontalAxis = (instanceId == 0) ? float3(1.0f, 0.0f, 0.0f) : float3(0.0f, 0.0f, 1.0f);
    const float3 worldPosition = float3(g_vBeamCenterWidth.x, g_vBeamCenterWidth.y, g_vBeamCenterWidth.z)
        + horizontalAxis * (localCoord.x * g_vBeamCenterWidth.w)
        + float3(0.0f, localCoord.y * g_vBeamParams.x, 0.0f);

    output.position = mul(float4(worldPosition, 1.0f), g_mViewProj);
    output.localCoord = localCoord;
    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    const float horizontalMask = 1.0f - smoothstep(kBeamHorizontalCoreStart, kBeamHorizontalCoreEnd, abs(input.localCoord.x));
    const float verticalMask = smoothstep(kBeamVerticalFadeInStart, kBeamVerticalFadeInEnd, input.localCoord.y)
        * (1.0f - smoothstep(kBeamVerticalFadeOutStart, kBeamVerticalFadeOutEnd, input.localCoord.y));
    const float pulse = kBeamPulseBase + kBeamPulseAmplitude * sin(g_vBeamColorTime.w * kBeamPulseSpeed);
    const float shimmer = pow(saturate(0.5f + 0.5f * sin(input.localCoord.y * kBeamShimmerSpatialFrequency - g_vBeamColorTime.w * kBeamShimmerTemporalSpeed)), kBeamShimmerPower);
    const float core = pow(horizontalMask, kBeamCorePower) * verticalMask;
    const float halo = pow(horizontalMask, kBeamHaloPower) * verticalMask * kBeamHaloStrength;

    const float alpha = saturate(core * (kBeamAlphaCoreBase + kBeamAlphaCorePulse * pulse + kBeamAlphaCoreShimmer * shimmer) + halo);
    const float3 baseColor = g_vBeamColorTime.rgb;
    const float3 color = lerp(baseColor, float3(1.0f, 1.0f, 1.0f), shimmer * kBeamWhiteMixStrength) * alpha;

    return float4(color, alpha);
}
