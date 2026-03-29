struct BoundingBox
{
    float3 corners[8]; // The 8 corners of the bounding box
    float4 color;
};

StructuredBuffer<BoundingBox> g_BoundingBoxes : register(t0);

cbuffer cbDebugBoundingBox : register(b0)
{
    matrix m_WorldViewProj;
};

//This array defines the indices of the corners for each edge of the bounding box
static const uint g_BoxEdgeCornerIndices[24] =
{
    0, 1,
    1, 2,
    2, 3,
    3, 0,
    4, 5,
    5, 6,
    6, 7,
    7, 4,
    0, 4,
    1, 5,
    2, 6,
    3, 7
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
};

VS_OUTPUT VSMain(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
{
    VS_OUTPUT output;
    BoundingBox box = g_BoundingBoxes[instanceId];
    float3 worldPosition = box.corners[g_BoxEdgeCornerIndices[vertexId]];

    output.position = mul(float4(worldPosition, 1.0f), m_WorldViewProj);
    output.color = box.color;
    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    return input.color;
}
