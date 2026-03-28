Texture3D<float4> gStaticVoxelAlbedo : register(t8);
Texture3D<uint>   gStaticVoxelMask : register(t9);
StructuredBuffer<uint> gStaticVoxelInstances : register(t10);

cbuffer cbVisualizeVoxels : register(b3)
{
    matrix gWorldViewProj;
    matrix gWorld;

    float3 gStaticVoxelMin;
    float gOpacity;

    float3 gStaticVoxelMax;
    float gStaticHeightWarp;

    uint gStaticGridSizeX;
    uint gStaticGridSizeY;
    uint gStaticGridSizeZ;
    float gSurfaceSnap;

    float3 gLightDir;
    float gPadding1;
};

struct VS_INPUT
{
    float3 localPos : POSITION;
    float3 localNormal : NORMAL;
    uint instanceId : SV_InstanceID;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 worldNormal : TEXCOORD1;
    nointerpolation uint3 voxelCoord : TEXCOORD2;
    float3 localPos : TEXCOORD3;
};

float UnwarpHeight(float y, float heightWarp)
{
    float wy = saturate(y);
    return pow(wy, 1.0f / max(heightWarp, 0.001f));
}

uint3 DecodeVoxelCoord(uint packedCoord)
{
    return uint3(
        packedCoord & 1023u,
        (packedCoord >> 10u) & 1023u,
        (packedCoord >> 20u) & 1023u);
}

void VoxelBoundsToWorld(uint3 coord, out float3 worldMin, out float3 worldMax)
{
    float3 worldExtent = max(gStaticVoxelMax - gStaticVoxelMin, 1e-5f.xxx);
    float3 gridSize = max(float3((float)gStaticGridSizeX, (float)gStaticGridSizeY, (float)gStaticGridSizeZ), 1.0f.xxx);

    float x0 = (float)coord.x / gridSize.x;
    float x1 = (float)(coord.x + 1u) / gridSize.x;
    float z0 = (float)coord.z / gridSize.z;
    float z1 = (float)(coord.z + 1u) / gridSize.z;

    float y0 = UnwarpHeight((float)coord.y / gridSize.y, gStaticHeightWarp);
    float y1 = UnwarpHeight((float)(coord.y + 1u) / gridSize.y, gStaticHeightWarp);

    worldMin = gStaticVoxelMin + worldExtent * float3(x0, y0, z0);
    worldMax = gStaticVoxelMin + worldExtent * float3(x1, y1, z1);
}

uint SampleStaticMask(int3 coord)
{
    if (coord.x < 0 || coord.y < 0 || coord.z < 0 ||
        coord.x >= (int)gStaticGridSizeX || coord.y >= (int)gStaticGridSizeY || coord.z >= (int)gStaticGridSizeZ)
    {
        return 0u;
    }

    return gStaticVoxelMask.Load(int4(coord, 0));
}

VS_OUTPUT VSMain(VS_INPUT vin)
{
    VS_OUTPUT vout;

    uint3 voxelCoord = DecodeVoxelCoord(gStaticVoxelInstances[vin.instanceId]);

    float3 voxelMinWorld;
    float3 voxelMaxWorld;
    VoxelBoundsToWorld(voxelCoord, voxelMinWorld, voxelMaxWorld);

    float3 voxelCenter = 0.5f * (voxelMinWorld + voxelMaxWorld);
    float3 voxelHalfExtent = max(0.5f * (voxelMaxWorld - voxelMinWorld), 1e-5f.xxx);
    float3 worldPos = voxelCenter + vin.localPos * voxelHalfExtent;

    vout.worldPos = worldPos;
    vout.worldNormal = normalize(vin.localNormal);
    vout.voxelCoord = voxelCoord;
    vout.localPos = vin.localPos;
    vout.position = mul(float4(worldPos, 1.0f), gWorldViewProj);

    return vout;
}

float ComputeVoxelEdgeMask(float3 localPos, float3 worldNormal)
{
    float3 absNormal = abs(normalize(worldNormal));
    float2 faceCoord = localPos.yz;

    if (absNormal.y >= absNormal.x && absNormal.y >= absNormal.z)
    {
        faceCoord = localPos.xz;
    }
    else if (absNormal.z >= absNormal.x && absNormal.z >= absNormal.y)
    {
        faceCoord = localPos.xy;
    }

    float edgeCoord = max(abs(faceCoord.x), abs(faceCoord.y));
    return smoothstep(0.92f, 0.98f, edgeCoord);
}

float4 PSMain(VS_OUTPUT pin) : SV_TARGET
{
    const int3 voxelCoord = int3(pin.voxelCoord);
    if (SampleStaticMask(voxelCoord) == 0u)
    {
        discard;
    }

    float4 albedo = gStaticVoxelAlbedo.Load(int4(voxelCoord, 0));
    float3 normal = normalize(pin.worldNormal);

    float3 lightDir = normalize(-gLightDir);
    float ndotl = saturate(dot(normal, lightDir));

    float3 baseColor = max(albedo.rgb, float3(0.16f, 0.18f, 0.22f));
    float faceShade = 0.70f + 0.30f * abs(normal.y);
    float3 litColor = baseColor * (0.22f  + 0.95f * ndotl) * faceShade;
    float edgeMask = ComputeVoxelEdgeMask(pin.localPos, normal);
    float3 finalColor = lerp(litColor, 0.0f.xxx, edgeMask);

    return float4(finalColor, gOpacity);
}
