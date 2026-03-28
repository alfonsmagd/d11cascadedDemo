//====================================================
// VOXELIZATION SHADERS
//====================================================

//----------------------------------------------------
// Resources
//----------------------------------------------------
Texture2D gDiffuseMap : register(t0);
SamplerState gLinearSampler : register(s0);

RWTexture3D<float4> gVoxelAlbedo : register(u0);
RWTexture3D<uint> gVoxelMask : register(u1);
AppendStructuredBuffer<uint> gVoxelInstanceList : register(u2);

//----------------------------------------------------
// Constant Buffers
//----------------------------------------------------
cbuffer cbPerObject : register(b0)
{
    matrix g_mWorldViewProjection;
    matrix m_mWorld;
};

cbuffer cbVoxelParams : register(b2)
{
    float3 gVoxelMin;
    uint gGridSizeX;

    float3 gVoxelMax;
    uint gGridSizeY;

    uint gGridSizeZ;
    float gHeightWarp;
    float gXYFootprintScale;
    float gYZFootprintScale;
};

//----------------------------------------------------
// Input / Output structures
//----------------------------------------------------
struct VS_INPUT
{
    float4 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
};

struct VS_OUTPUT
{
    float3 WorldPos : TEXCOORD0;
    float3 Normal : TEXCOORD1;
    float2 UV : TEXCOORD2;
    float4 PosH : SV_POSITION;
};

struct GS_OUTPUT
{
    float3 WorldPos : TEXCOORD0;
    float3 Normal : TEXCOORD1;
    float2 UV : TEXCOORD2;
    float3 AxisMask : TEXCOORD3;
    float4 PosH : SV_POSITION;
};

struct PS_INPUT
{
    float3 WorldPos : TEXCOORD0;
    float3 Normal : TEXCOORD1;
    float2 UV : TEXCOORD2;
    float3 AxisMask : TEXCOORD3;
    float4 PosH : SV_POSITION;
};

//====================================================
// Helpers
//====================================================
float WarpHeight(float y)
{
    float wy = saturate(y);
    return pow(wy, max(gHeightWarp, 0.001f));
}

float3 SafeWorldToUVW(float3 worldPos)
{
    float3 extent = max(gVoxelMax - gVoxelMin, float3(1e-5f, 1e-5f, 1e-5f));
    float3 uvw = saturate((worldPos - gVoxelMin) / extent);
    uvw.y = WarpHeight(uvw.y);
    return uvw;
}

uint3 WorldToVoxel(float3 worldPos)
{
    float3 uvw = SafeWorldToUVW(worldPos);

    float3 voxelF = uvw * float3(
        (float)(gGridSizeX - 1),
        (float)(gGridSizeY - 1),
        (float)(gGridSizeZ - 1));

    uint3 coord = uint3(floor(voxelF + 0.5f));
    return min(coord, uint3(gGridSizeX - 1, gGridSizeY - 1, gGridSizeZ - 1));
}

void WriteVoxelSafe(int3 coord, float4 albedo)
{
    if (coord.x < 0 || coord.y < 0 || coord.z < 0 ||
        coord.x >= (int)gGridSizeX ||
        coord.y >= (int)gGridSizeY ||
        coord.z >= (int)gGridSizeZ)
    {
        return;
    }

    uint3 voxelCoord = uint3(coord);
    gVoxelAlbedo[voxelCoord] = albedo;

    uint previousMask;
    InterlockedOr(gVoxelMask[voxelCoord], 1u, previousMask);

    if (previousMask == 0u)
    {
        uint packedCoord =
            (voxelCoord.x & 1023u) |
            ((voxelCoord.y & 1023u) << 10u) |
            ((voxelCoord.z & 1023u) << 20u);
        gVoxelInstanceList.Append(packedCoord);
    }
}

float4 ProjectToAxis(float3 worldPos, uint axis, float3 gridMin, float3 gridMax)
{
    float3 extent = max(gridMax - gridMin, float3(1e-5f, 1e-5f, 1e-5f));
    float3 uvw = saturate((worldPos - gridMin) / extent);
    uvw.y = WarpHeight(uvw.y);
    uvw = uvw * 2.0f - 1.0f;

    float4 outPos = float4(0.0f, 0.0f, 0.0f, 1.0f);

    if (axis == 0)
    {
        outPos.xy = uvw.xy;
        outPos.z = uvw.z;
    }
    else if (axis == 1)
    {
        outPos.xy = float2(uvw.x, uvw.z);
        outPos.z = uvw.y;
    }
    else
    {
        outPos.xy = float2(uvw.y, uvw.z);
        outPos.z = uvw.x;
    }

    return outPos;
}

float2 GetProjectedGridDims(uint axis)
{
    if (axis == 0)
    {
        return float2((float)gGridSizeX, (float)gGridSizeY);
    }
    else if (axis == 1)
    {
        return float2((float)gGridSizeX, (float)gGridSizeZ);
    }

    return float2((float)gGridSizeY, (float)gGridSizeZ);
}

//====================================================
// Vertex Shader
//====================================================
VS_OUTPUT CreateVoxelArrayVS(VS_INPUT vin)
{
    VS_OUTPUT vout;

    float4 worldPos = mul(vin.vPosition, m_mWorld);

    vout.WorldPos = worldPos.xyz;
    vout.Normal = normalize(mul(vin.vNormal, (float3x3)m_mWorld));
    vout.UV = vin.vTexcoord;
    vout.PosH = float4(0.0f, 0.0f, 0.0f, 1.0f);

    return vout;
}

//====================================================
// Geometry Shader
//====================================================
[maxvertexcount(9)]
void CreateVoxelArrayGS(
    triangle VS_OUTPUT input[3],
    inout TriangleStream<GS_OUTPUT> triStream)
{
    const float neutralFootprintScale = max(0.1f, 0.5f * (gXYFootprintScale + gYZFootprintScale));

    [unroll]
    for (uint axis = 0; axis < 3; ++axis)
    {
        float4 projectedPos[3];
        [unroll]
        for (int i = 0; i < 3; ++i)
        {
            projectedPos[i] = ProjectToAxis(input[i].WorldPos, axis, gVoxelMin, gVoxelMax);
        }

        float2 projectedGridDims = GetProjectedGridDims(axis);
        float minFootprint = (1.15f * neutralFootprintScale) /
            max(min(projectedGridDims.x, projectedGridDims.y), 1.0f);

        float len01 = length(projectedPos[0].xy - projectedPos[1].xy);
        float len12 = length(projectedPos[1].xy - projectedPos[2].xy);
        float len20 = length(projectedPos[2].xy - projectedPos[0].xy);

        float minLen = max(min(len01, min(len12, len20)), 1e-5f);
        float scale = min(max(1.0f, minFootprint / minLen), 1.5f);

        float2 triCenter = (projectedPos[0].xy + projectedPos[1].xy + projectedPos[2].xy) / 3.0f;
        if (scale > 1.0f)
        {
            [unroll]
            for (int inflateIndex = 0; inflateIndex < 3; ++inflateIndex)
            {
                projectedPos[inflateIndex].xy =
                    triCenter + (projectedPos[inflateIndex].xy - triCenter) * scale;
            }
        }

        float3 axisMask = float3(0.0f, 0.0f, 0.0f);
        if (axis == 0)
        {
            axisMask = float3(1.0f, 0.0f, 0.0f);
        }
        else if (axis == 1)
        {
            axisMask = float3(0.0f, 1.0f, 0.0f);
        }
        else
        {
            axisMask = float3(0.0f, 0.0f, 1.0f);
        }

        [unroll]
        for (int vertexIndex = 0; vertexIndex < 3; ++vertexIndex)
        {
            GS_OUTPUT o;
            o.WorldPos = input[vertexIndex].WorldPos;
            o.Normal = input[vertexIndex].Normal;
            o.UV = input[vertexIndex].UV;
            o.AxisMask = axisMask;
            o.PosH = projectedPos[vertexIndex];
            triStream.Append(o);
        }

        triStream.RestartStrip();
    }
}

//====================================================
// Pixel Shader
//====================================================
void CreateVoxelArrayPS(PS_INPUT pin)
{
    uint3 voxelCoord = WorldToVoxel(pin.WorldPos);
    int3 baseCoord = int3(voxelCoord);
    float4 albedo = gDiffuseMap.Sample(gLinearSampler, pin.UV);

    WriteVoxelSafe(baseCoord, albedo);

    WriteVoxelSafe(baseCoord + int3(1, 0, 0), albedo);
    WriteVoxelSafe(baseCoord - int3(1, 0, 0), albedo);
    WriteVoxelSafe(baseCoord + int3(0, 1, 0), albedo);
    WriteVoxelSafe(baseCoord - int3(0, 1, 0), albedo);
    WriteVoxelSafe(baseCoord + int3(0, 0, 1), albedo);
    WriteVoxelSafe(baseCoord - int3(0, 0, 1), albedo);

    WriteVoxelSafe(baseCoord + int3(1, 1, 0), albedo);
    WriteVoxelSafe(baseCoord + int3(1, -1, 0), albedo);
    WriteVoxelSafe(baseCoord + int3(-1, 1, 0), albedo);
    WriteVoxelSafe(baseCoord + int3(-1, -1, 0), albedo);

    WriteVoxelSafe(baseCoord + int3(1, 0, 1), albedo);
    WriteVoxelSafe(baseCoord + int3(1, 0, -1), albedo);
    WriteVoxelSafe(baseCoord + int3(-1, 0, 1), albedo);
    WriteVoxelSafe(baseCoord + int3(-1, 0, -1), albedo);

    WriteVoxelSafe(baseCoord + int3(0, 1, 1), albedo);
    WriteVoxelSafe(baseCoord + int3(0, 1, -1), albedo);
    WriteVoxelSafe(baseCoord + int3(0, -1, 1), albedo);
    WriteVoxelSafe(baseCoord + int3(0, -1, -1), albedo);
}
