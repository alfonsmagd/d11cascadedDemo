#pragma once

#include <algorithm>
#include <cfloat>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "DXUT.h"
#include "SDKMesh.h"
#include "SDKmisc.h"



struct BoundingBox
{
    XMFLOAT3 corners[8];
    XMFLOAT4 color;
};

inline void GetBoundingBoxMinMax(const BoundingBox& box, D3DXVECTOR3& vMin, D3DXVECTOR3& vMax)
{
    vMin = D3DXVECTOR3(FLT_MAX, FLT_MAX, FLT_MAX);
    vMax = D3DXVECTOR3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (INT cornerIndex = 0; cornerIndex < 8; ++cornerIndex)
    {
        const XMFLOAT3& corner = box.corners[cornerIndex];
        vMin.x = min(vMin.x, corner.x);
        vMin.y = min(vMin.y, corner.y);
        vMin.z = min(vMin.z, corner.z);
        vMax.x = max(vMax.x, corner.x);
        vMax.y = max(vMax.y, corner.y);
        vMax.z = max(vMax.z, corner.z);
    }
}

inline D3DXVECTOR3 TransformPoint(const D3DXVECTOR3& point, const D3DXMATRIX& transform)
{
    D3DXVECTOR3 transformedPoint;
    D3DXVec3TransformCoord(&transformedPoint, &point, &transform);
    return transformedPoint;
}

inline D3DXVECTOR3 TransformDirection(const D3DXVECTOR3& direction, const D3DXMATRIX& transform)
{
    D3DXVECTOR3 transformedDirection;
    D3DXVec3TransformNormal(&transformedDirection, &direction, &transform);
    return transformedDirection;
}

inline bool IntersectRayBoundingBox(const D3DXVECTOR3& rayOrigin,
    const D3DXVECTOR3& rayDirection,
    const BoundingBox& box,
    float& hitDistance)
{
    const float epsilon = 1.0e-6f;
    D3DXVECTOR3 vMin;
    D3DXVECTOR3 vMax;
    GetBoundingBoxMinMax(box, vMin, vMax);

    float tMin = 0.0f;
    float tMax = FLT_MAX;

    const float origin[3] = { rayOrigin.x, rayOrigin.y, rayOrigin.z };
    const float direction[3] = { rayDirection.x, rayDirection.y, rayDirection.z };
    const float boundsMin[3] = { vMin.x, vMin.y, vMin.z };
    const float boundsMax[3] = { vMax.x, vMax.y, vMax.z };

    for (INT axis = 0; axis < 3; ++axis)
    {
        if (fabsf(direction[axis]) < epsilon)
        {
            if (origin[axis] < boundsMin[axis] || origin[axis] > boundsMax[axis])
            {
                return false;
            }
            continue;
        }

        const float inverseDirection = 1.0f / direction[axis];
        float t1 = (boundsMin[axis] - origin[axis]) * inverseDirection;
        float t2 = (boundsMax[axis] - origin[axis]) * inverseDirection;
        if (t1 > t2)
        {
            std::swap(t1, t2);
        }

        tMin = max(tMin, t1);
        tMax = min(tMax, t2);
        if (tMin > tMax)
        {
            return false;
        }
    }

    hitDistance = (tMin >= 0.0f) ? tMin : tMax;
    return hitDistance >= 0.0f;
}

inline bool IntersectRayTriangle(const D3DXVECTOR3& rayOrigin,
    const D3DXVECTOR3& rayDirection,
    const D3DXVECTOR3& v0,
    const D3DXVECTOR3& v1,
    const D3DXVECTOR3& v2,
    float& hitDistance)
{
    const float epsilon = 1.0e-6f;
    const D3DXVECTOR3 edge1 = v1 - v0;
    const D3DXVECTOR3 edge2 = v2 - v0;

    D3DXVECTOR3 pvec;
    D3DXVec3Cross(&pvec, &rayDirection, &edge2);
    const float determinant = D3DXVec3Dot(&edge1, &pvec);
    if (fabsf(determinant) < epsilon)
    {
        return false;
    }

    const float inverseDeterminant = 1.0f / determinant;
    const D3DXVECTOR3 tvec = rayOrigin - v0;
    const float u = D3DXVec3Dot(&tvec, &pvec) * inverseDeterminant;
    if (u < 0.0f || u > 1.0f)
    {
        return false;
    }

    D3DXVECTOR3 qvec;
    D3DXVec3Cross(&qvec, &tvec, &edge1);
    const float v = D3DXVec3Dot(&rayDirection, &qvec) * inverseDeterminant;
    if (v < 0.0f || (u + v) > 1.0f)
    {
        return false;
    }

    const float distance = D3DXVec3Dot(&edge2, &qvec) * inverseDeterminant;
    if (distance < 0.0f)
    {
        return false;
    }

    hitDistance = distance;
    return true;
}

inline BoundingBox MakeBoundingBoxFromMinMax(const D3DXVECTOR3& vMin, const D3DXVECTOR3& vMax, const XMFLOAT4& color)
{
    BoundingBox box = {};
    box.color = color;
    box.corners[0] = XMFLOAT3(vMin.x, vMin.y, vMin.z);
    box.corners[1] = XMFLOAT3(vMax.x, vMin.y, vMin.z);
    box.corners[2] = XMFLOAT3(vMax.x, vMax.y, vMin.z);
    box.corners[3] = XMFLOAT3(vMin.x, vMax.y, vMin.z);
    box.corners[4] = XMFLOAT3(vMin.x, vMin.y, vMax.z);
    box.corners[5] = XMFLOAT3(vMax.x, vMin.y, vMax.z);
    box.corners[6] = XMFLOAT3(vMax.x, vMax.y, vMax.z);
    box.corners[7] = XMFLOAT3(vMin.x, vMax.y, vMax.z);
    return box;
}

inline BoundingBox TransformBoundingBox(const BoundingBox& box, const D3DXMATRIX& transform)
{
    BoundingBox transformedBox = box;
    for (INT cornerIndex = 0; cornerIndex < 8; ++cornerIndex)
    {
        const D3DXVECTOR3 localCorner(box.corners[cornerIndex].x, box.corners[cornerIndex].y, box.corners[cornerIndex].z);
        const D3DXVECTOR3 transformedCorner = TransformPoint(localCorner, transform);
        transformedBox.corners[cornerIndex] = XMFLOAT3(transformedCorner.x, transformedCorner.y, transformedCorner.z);
    }
    return transformedBox;
}

struct SelectedAlbedoInfo
{
    SelectedAlbedoInfo()
        : pDiffuseSRV(NULL)
    {
    }

    ID3D11ShaderResourceView* pDiffuseSRV;
    std::string materialName;
    std::string textureName;
};

class ISceneMesh
{
public:
    virtual ~ISceneMesh() {}

    virtual HRESULT Create(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dDeviceContext) = 0;
    virtual void Destroy() = 0;
    virtual bool IsLoaded() const = 0;
    virtual void Render(ID3D11DeviceContext* pd3dDeviceContext,
        UINT iDiffuseSlot = INVALID_SAMPLER_SLOT,
        UINT iNormalSlot = INVALID_SAMPLER_SLOT,
        UINT iSpecularSlot = INVALID_SAMPLER_SLOT) = 0;
    virtual XMVECTOR GetAABBMin() const = 0;
    virtual XMVECTOR GetAABBMax() const = 0;
    virtual const D3DXMATRIX& GetWorldMatrix() const = 0;
    virtual void Translate(const D3DXVECTOR3& delta) = 0;
    virtual bool TranslateSubMesh(INT subMeshIndex, const D3DXVECTOR3& delta, ID3D11DeviceContext* pd3dDeviceContext) = 0;
    virtual void UpdateGlobalBoundingBox(std::vector<BoundingBox>& globalBoundingBoxes) const = 0;
    virtual void UpdateAllBoundingBoxes(std::vector<BoundingBox>& boundingBoxes) const = 0;
    virtual bool PickSubMesh(const D3DXVECTOR3& rayOrigin, const D3DXVECTOR3& rayDirection, INT& pickedIndex, float& pickedDistance) const = 0;
    virtual bool GetSubMeshAlbedoInfo(INT subMeshIndex, SelectedAlbedoInfo& outInfo) const = 0;
};

class SDKSceneMesh : public ISceneMesh
{

public:
    explicit SDKSceneMesh(const WCHAR* szMeshPath)
        : m_szMeshPath(szMeshPath)
        , m_bLoaded(false)
    {
        ResetBounds();
        ResetTransform();
    }

    bool GetSubMeshAlbedoInfo(INT subMeshIndex, SelectedAlbedoInfo& outInfo) const override
    {
        outInfo = SelectedAlbedoInfo();

        if (subMeshIndex < 0 || subMeshIndex >= INT(m_SubMeshRuntimeData.size()))
        {
            return false;
        }

        CDXUTSDKMesh& meshAccess = const_cast<CDXUTSDKMesh&>(m_Mesh);
        const SubMeshRuntimeData& runtimeData = m_SubMeshRuntimeData[subMeshIndex];
        SDKMESH_MESH* pMesh = meshAccess.GetMesh(runtimeData.meshIndex);
        if (!pMesh || runtimeData.subsetIndex >= pMesh->NumSubsets)
        {
            return false;
        }

        SDKMESH_SUBSET* pSubset = meshAccess.GetSubset(runtimeData.meshIndex, runtimeData.subsetIndex);
        if (!pSubset)
        {
            return false;
        }

        SDKMESH_MATERIAL* pMaterial = meshAccess.GetMaterial(pSubset->MaterialID);
        if (!pMaterial)
        {
            return false;
        }

        outInfo.pDiffuseSRV = IsErrorResource(pMaterial->pDiffuseRV11) ? NULL : pMaterial->pDiffuseRV11;
        outInfo.materialName = pMaterial->Name;
        outInfo.textureName = pMaterial->DiffuseTexture;
        return outInfo.pDiffuseSRV != NULL;
    }

    HRESULT Create(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dDeviceContext) override
    {
        UNREFERENCED_PARAMETER(pd3dDeviceContext);

        ResetBounds();
        ResetTransform();
        m_SubMeshBoundingBoxes.clear();
        m_SubMeshTriangles.clear();
        m_SubMeshPickRanges.clear();
        m_SubMeshRuntimeData.clear();

        HRESULT hr = m_Mesh.Create(pd3dDevice, m_szMeshPath.c_str());
        if (FAILED(hr))
        {
            return hr;
        }

        for (UINT i = 0; i < m_Mesh.GetNumMeshes(); ++i)
        {
            SDKMESH_MESH* pMesh = m_Mesh.GetMesh(i);

            const D3DXVECTOR3 meshMin(
                pMesh->BoundingBoxCenter.x - pMesh->BoundingBoxExtents.x,
                pMesh->BoundingBoxCenter.y - pMesh->BoundingBoxExtents.y,
                pMesh->BoundingBoxCenter.z - pMesh->BoundingBoxExtents.z);
            const D3DXVECTOR3 meshMax(
                pMesh->BoundingBoxCenter.x + pMesh->BoundingBoxExtents.x,
                pMesh->BoundingBoxCenter.y + pMesh->BoundingBoxExtents.y,
                pMesh->BoundingBoxCenter.z + pMesh->BoundingBoxExtents.z);

            m_vAABBMin.x = min(m_vAABBMin.x, meshMin.x);
            m_vAABBMin.y = min(m_vAABBMin.y, meshMin.y);
            m_vAABBMin.z = min(m_vAABBMin.z, meshMin.z);
            m_vAABBMax.x = max(m_vAABBMax.x, meshMax.x);
            m_vAABBMax.y = max(m_vAABBMax.y, meshMax.y);
            m_vAABBMax.z = max(m_vAABBMax.z, meshMax.z);

            if (pMesh->NumVertexBuffers == 0)
            {
                continue;
            }

            BYTE* pRawVertices = m_Mesh.GetRawVerticesAt(pMesh->VertexBuffers[0]);
            BYTE* pRawIndices = m_Mesh.GetRawIndicesAt(pMesh->IndexBuffer);
            const UINT vertexStride = m_Mesh.GetVertexStride(i, 0);
            const SDKMESH_INDEX_TYPE indexType = m_Mesh.GetIndexType(i);
            if (!pRawVertices || !pRawIndices || vertexStride < sizeof(D3DXVECTOR3))
            {
                continue;
            }

            for (UINT subsetIndex = 0; subsetIndex < pMesh->NumSubsets; ++subsetIndex)
            {
                SDKMESH_SUBSET* pSubset = m_Mesh.GetSubset(i, subsetIndex);
                if (!pSubset || pSubset->PrimitiveType != PT_TRIANGLE_LIST)
                {
                    continue;
                }

                const size_t firstTriangle = m_SubMeshTriangles.size();
                D3DXVECTOR3 subsetMin(FLT_MAX, FLT_MAX, FLT_MAX);
                D3DXVECTOR3 subsetMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
                UINT triangleCount = 0;
                std::vector<UINT> subsetVertexIndices;
                subsetVertexIndices.reserve(UINT(pSubset->IndexCount));

                for (UINT64 indexOffset = 0; indexOffset + 2 < pSubset->IndexCount; indexOffset += 3)
                {
                    const UINT64 i0 = pSubset->IndexStart + indexOffset;
                    const UINT64 i1 = pSubset->IndexStart + indexOffset + 1;
                    const UINT64 i2 = pSubset->IndexStart + indexOffset + 2;

                    UINT vertexIndex0 = 0;
                    UINT vertexIndex1 = 0;
                    UINT vertexIndex2 = 0;
                    if (indexType == IT_16BIT)
                    {
                        const WORD* pIndices16 = reinterpret_cast<const WORD*>(pRawIndices);
                        vertexIndex0 = pIndices16[i0];
                        vertexIndex1 = pIndices16[i1];
                        vertexIndex2 = pIndices16[i2];
                    }
                    else
                    {
                        const DWORD* pIndices32 = reinterpret_cast<const DWORD*>(pRawIndices);
                        vertexIndex0 = pIndices32[i0];
                        vertexIndex1 = pIndices32[i1];
                        vertexIndex2 = pIndices32[i2];
                    }

                    subsetVertexIndices.push_back(vertexIndex0);
                    subsetVertexIndices.push_back(vertexIndex1);
                    subsetVertexIndices.push_back(vertexIndex2);

                    const D3DXVECTOR3& v0 = *reinterpret_cast<const D3DXVECTOR3*>(pRawVertices + (vertexIndex0 * vertexStride));
                    const D3DXVECTOR3& v1 = *reinterpret_cast<const D3DXVECTOR3*>(pRawVertices + (vertexIndex1 * vertexStride));
                    const D3DXVECTOR3& v2 = *reinterpret_cast<const D3DXVECTOR3*>(pRawVertices + (vertexIndex2 * vertexStride));

                    m_SubMeshTriangles.push_back(MakeTriangle(v0, v1, v2));
                    ExpandBounds(subsetMin, subsetMax, v0);
                    ExpandBounds(subsetMin, subsetMax, v1);
                    ExpandBounds(subsetMin, subsetMax, v2);
                    ++triangleCount;
                }

                if (triangleCount > 0)
                {
                    std::sort(subsetVertexIndices.begin(), subsetVertexIndices.end());
                    subsetVertexIndices.erase(std::unique(subsetVertexIndices.begin(), subsetVertexIndices.end()), subsetVertexIndices.end());
                    m_SubMeshBoundingBoxes.push_back(MakeBoundingBoxFromMinMax(subsetMin, subsetMax, XMFLOAT4(0, 0, 1, 1)));
                    m_SubMeshPickRanges.push_back(MakePickRange(firstTriangle, triangleCount));
                    SubMeshRuntimeData runtimeData = {};
                    runtimeData.meshIndex = i;
                    runtimeData.subsetIndex = subsetIndex;
                    runtimeData.vertexIndices.swap(subsetVertexIndices);
                    m_SubMeshRuntimeData.push_back(runtimeData);
                }
            }
        }
        RebuildSceneBoundsFromSubMeshes();
        m_SceneBounding = MakeBoundingBoxFromMinMax(m_vAABBMin, m_vAABBMax, XMFLOAT4(1, 0, 1, 1));
      

        m_bLoaded = true;
        return S_OK;
    }

    void Destroy() override
    {
        m_Mesh.Destroy();
        m_bLoaded = false;
        m_SubMeshBoundingBoxes.clear();
        m_SubMeshTriangles.clear();
        m_SubMeshPickRanges.clear();
        m_SubMeshRuntimeData.clear();
        ResetTransform();
        ResetBounds();
    }

    bool IsLoaded() const override
    {
        return m_bLoaded;
    }

    void Render(ID3D11DeviceContext* pd3dDeviceContext,
        UINT iDiffuseSlot = INVALID_SAMPLER_SLOT,
        UINT iNormalSlot = INVALID_SAMPLER_SLOT,
        UINT iSpecularSlot = INVALID_SAMPLER_SLOT) override
    {
        m_Mesh.Render(pd3dDeviceContext, iDiffuseSlot, iNormalSlot, iSpecularSlot);
    }

    XMVECTOR GetAABBMin() const override
    {
        BoundingBox sceneBounding = TransformBoundingBox(m_SceneBounding, m_mWorld);
        D3DXVECTOR3 vMin;
        D3DXVECTOR3 vMax;
        GetBoundingBoxMinMax(sceneBounding, vMin, vMax);
        return XMVectorSet(vMin.x, vMin.y, vMin.z, 1.0f);
    }

    XMVECTOR GetAABBMax() const override
    {
        BoundingBox sceneBounding = TransformBoundingBox(m_SceneBounding, m_mWorld);
        D3DXVECTOR3 vMin;
        D3DXVECTOR3 vMax;
        GetBoundingBoxMinMax(sceneBounding, vMin, vMax);
        return XMVectorSet(vMax.x, vMax.y, vMax.z, 1.0f);
    }

    const D3DXMATRIX& GetWorldMatrix() const override
    {
        return m_mWorld;
    }

    void Translate(const D3DXVECTOR3& delta) override
    {
        m_mWorld._41 += delta.x;
        m_mWorld._42 += delta.y;
        m_mWorld._43 += delta.z;
    }

    bool TranslateSubMesh(INT subMeshIndex, const D3DXVECTOR3& delta, ID3D11DeviceContext* pd3dDeviceContext) override
    {
        if (!pd3dDeviceContext || subMeshIndex < 0 || subMeshIndex >= INT(m_SubMeshRuntimeData.size()))
        {
            return false;
        }

        const SubMeshRuntimeData& runtimeData = m_SubMeshRuntimeData[subMeshIndex];
        SDKMESH_MESH* pMesh = m_Mesh.GetMesh(runtimeData.meshIndex);
        if (!pMesh || pMesh->NumVertexBuffers == 0)
        {
            return false;
        }

        BYTE* pRawVertices = m_Mesh.GetRawVerticesAt(pMesh->VertexBuffers[0]);
        const UINT vertexStride = m_Mesh.GetVertexStride(runtimeData.meshIndex, 0);
        ID3D11Buffer* pVertexBuffer = m_Mesh.GetVB11(runtimeData.meshIndex, 0);
        if (!pRawVertices || !pVertexBuffer || vertexStride < sizeof(D3DXVECTOR3))
        {
            return false;
        }

        for (size_t vertexListIndex = 0; vertexListIndex < runtimeData.vertexIndices.size(); ++vertexListIndex)
        {
            D3DXVECTOR3* pPosition = reinterpret_cast<D3DXVECTOR3*>(pRawVertices + (runtimeData.vertexIndices[vertexListIndex] * vertexStride));
            *pPosition += delta;
        }

        const SubMeshPickRange& range = m_SubMeshPickRanges[subMeshIndex];
        for (size_t triangleIndex = range.firstTriangle; triangleIndex < (range.firstTriangle + range.triangleCount); ++triangleIndex)
        {
            m_SubMeshTriangles[triangleIndex].v0 += delta;
            m_SubMeshTriangles[triangleIndex].v1 += delta;
            m_SubMeshTriangles[triangleIndex].v2 += delta;
        }

        for (INT cornerIndex = 0; cornerIndex < 8; ++cornerIndex)
        {
            m_SubMeshBoundingBoxes[subMeshIndex].corners[cornerIndex].x += delta.x;
            m_SubMeshBoundingBoxes[subMeshIndex].corners[cornerIndex].y += delta.y;
            m_SubMeshBoundingBoxes[subMeshIndex].corners[cornerIndex].z += delta.z;
        }

        RebuildSceneBoundsFromSubMeshes();
        m_SceneBounding = MakeBoundingBoxFromMinMax(m_vAABBMin, m_vAABBMax, XMFLOAT4(1, 0, 1, 1));
        pd3dDeviceContext->UpdateSubresource(pVertexBuffer, 0, NULL, pRawVertices, 0, 0);
        return true;
    }

    BoundingBox GetBoundingBox() const
    {
        return TransformBoundingBox(m_SceneBounding, m_mWorld);
    }

    void UpdateGlobalBoundingBox(std::vector<BoundingBox>& globalBoundingBoxes) const override
    {
        globalBoundingBoxes.push_back(GetBoundingBox());
    }

    void UpdateAllBoundingBoxes(std::vector<BoundingBox>& boundingBoxes) const override
    {
        for (size_t index = 0; index < m_SubMeshBoundingBoxes.size(); ++index)
        {
            boundingBoxes.push_back(TransformBoundingBox(m_SubMeshBoundingBoxes[index], m_mWorld));
        }
    }

    bool PickSubMesh(const D3DXVECTOR3& rayOrigin, const D3DXVECTOR3& rayDirection, INT& pickedIndex, float& pickedDistance) const override
    {
        pickedIndex = -1;
        pickedDistance = FLT_MAX;
        D3DXMATRIX inverseWorld;
        D3DXMatrixInverse(&inverseWorld, NULL, &m_mWorld);
        const D3DXVECTOR3 localRayOrigin = TransformPoint(rayOrigin, inverseWorld);
        D3DXVECTOR3 localRayDirection = TransformDirection(rayDirection, inverseWorld);
        if (D3DXVec3LengthSq(&localRayDirection) <= 0.0f)
        {
            return false;
        }
        D3DXVec3Normalize(&localRayDirection, &localRayDirection);

        for (size_t subMeshIndex = 0; subMeshIndex < m_SubMeshPickRanges.size(); ++subMeshIndex)
        {
            float boxDistance = FLT_MAX;
            if (!IntersectRayBoundingBox(localRayOrigin, localRayDirection, m_SubMeshBoundingBoxes[subMeshIndex], boxDistance))
            {
                continue;
            }

            if (boxDistance > pickedDistance)
            {
                continue;
            }

            const SubMeshPickRange& range = m_SubMeshPickRanges[subMeshIndex];
            for (size_t triangleIndex = range.firstTriangle; triangleIndex < (range.firstTriangle + range.triangleCount); ++triangleIndex)
            {
                const TriangleData& triangle = m_SubMeshTriangles[triangleIndex];
                float hitDistance = FLT_MAX;
                if (IntersectRayTriangle(localRayOrigin, localRayDirection, triangle.v0, triangle.v1, triangle.v2, hitDistance) &&
                    hitDistance < pickedDistance)
                {
                    pickedIndex = INT(subMeshIndex);
                    pickedDistance = hitDistance;
                }
            }
        }

        return pickedIndex >= 0;
    }

private:
    struct TriangleData
    {
        D3DXVECTOR3 v0;
        D3DXVECTOR3 v1;
        D3DXVECTOR3 v2;
    };

    struct SubMeshPickRange
    {
        size_t firstTriangle;
        size_t triangleCount;
    };

    struct SubMeshRuntimeData
    {
        UINT meshIndex;
        UINT subsetIndex;
        std::vector<UINT> vertexIndices;
    };

    static TriangleData MakeTriangle(const D3DXVECTOR3& v0, const D3DXVECTOR3& v1, const D3DXVECTOR3& v2)
    {
        TriangleData triangle = {};
        triangle.v0 = v0;
        triangle.v1 = v1;
        triangle.v2 = v2;
        return triangle;
    }

    static SubMeshPickRange MakePickRange(size_t firstTriangle, size_t triangleCount)
    {
        SubMeshPickRange range = {};
        range.firstTriangle = firstTriangle;
        range.triangleCount = triangleCount;
        return range;
    }

    static void ExpandBounds(D3DXVECTOR3& vMin, D3DXVECTOR3& vMax, const D3DXVECTOR3& point)
    {
        vMin.x = min(vMin.x, point.x);
        vMin.y = min(vMin.y, point.y);
        vMin.z = min(vMin.z, point.z);
        vMax.x = max(vMax.x, point.x);
        vMax.y = max(vMax.y, point.y);
        vMax.z = max(vMax.z, point.z);
    }

    void ResetTransform()
    {
        D3DXMatrixIdentity(&m_mWorld);
    }

    void RebuildSceneBoundsFromSubMeshes()
    {
        if (m_SubMeshBoundingBoxes.empty())
        {
            return;
        }

        ResetBounds();
        for (size_t subMeshIndex = 0; subMeshIndex < m_SubMeshBoundingBoxes.size(); ++subMeshIndex)
        {
            D3DXVECTOR3 vSubMin;
            D3DXVECTOR3 vSubMax;
            GetBoundingBoxMinMax(m_SubMeshBoundingBoxes[subMeshIndex], vSubMin, vSubMax);
            ExpandBounds(m_vAABBMin, m_vAABBMax, vSubMin);
            ExpandBounds(m_vAABBMin, m_vAABBMax, vSubMax);
        }
    }

    void ResetBounds()
    {
        m_vAABBMin = D3DXVECTOR3(FLT_MAX, FLT_MAX, FLT_MAX);
        m_vAABBMax = D3DXVECTOR3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    }

    std::wstring    m_szMeshPath;
    bool            m_bLoaded;
    CDXUTSDKMesh    m_Mesh;
    D3DXVECTOR3     m_vAABBMin;
    D3DXVECTOR3     m_vAABBMax;
    BoundingBox     m_SceneBounding;
    std::vector<BoundingBox> m_SubMeshBoundingBoxes;
    std::vector<TriangleData> m_SubMeshTriangles;
    std::vector<SubMeshPickRange> m_SubMeshPickRanges;
    std::vector<SubMeshRuntimeData> m_SubMeshRuntimeData;
    D3DXMATRIX      m_mWorld;
};

class OBJSceneMesh : public ISceneMesh
{
public:
    explicit OBJSceneMesh(const WCHAR* szMeshPath, float uniformScale = 1.0f)
        : m_szMeshPath(szMeshPath)
        , m_fUniformScale(uniformScale)
        , m_bLoaded(false)
        , m_pVertexBuffer(NULL)
        , m_pIndexBuffer(NULL)
        , m_pFallbackDiffuseSRV(NULL)
    {
        ResetBounds();
        ResetTransform();
    }

    bool GetSubMeshAlbedoInfo(INT subMeshIndex, SelectedAlbedoInfo& outInfo) const override
    {
        outInfo = SelectedAlbedoInfo();

        if (subMeshIndex < 0 || subMeshIndex >= INT(m_Subsets.size()))
        {
            return false;
        }

        const Subset& subset = m_Subsets[subMeshIndex];
        if (subset.MaterialID >= m_Materials.size() || subset.MaterialID >= m_MaterialDescs.size())
        {
            return false;
        }

        const MaterialResources& material = m_Materials[subset.MaterialID];
        const MaterialDesc& materialDesc = m_MaterialDescs[subset.MaterialID];

        outInfo.pDiffuseSRV = material.pDiffuseSRV ? material.pDiffuseSRV : m_pFallbackDiffuseSRV;
        outInfo.materialName = materialDesc.name;
        outInfo.textureName = ToAnsi(materialDesc.diffuseTexture);
        return outInfo.pDiffuseSRV != NULL;
    }


    void UpdateGlobalBoundingBox(std::vector<BoundingBox>& globalBoundingBoxes) const override
    {
        globalBoundingBoxes.push_back(TransformBoundingBox(MakeBoundingBoxFromMinMax(m_vAABBMin, m_vAABBMax, XMFLOAT4(1, 0, 1, 1)), m_mWorld));
    }

    void UpdateAllBoundingBoxes(std::vector<BoundingBox>& boundingBoxes) const override
    {
        for (size_t index = 0; index < m_SubsetBoundingBoxes.size(); ++index)
        {
            boundingBoxes.push_back(TransformBoundingBox(m_SubsetBoundingBoxes[index], m_mWorld));
        }
    }



    HRESULT Create(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dDeviceContext) override
    {
        HRESULT hr = S_OK;

        Destroy();
        ResetTransform();

        WCHAR strPath[MAX_PATH] = {};
        V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, MAX_PATH, m_szMeshPath.c_str()));

        std::wstring objPath = strPath;
        const std::wstring basePath = GetDirectory(objPath);

        std::vector<MeshVertex> vertices;
        std::vector<UINT> indices;
        std::vector<UINT> triangleMaterials;
        std::vector<MaterialDesc> materials;
        std::unordered_map<std::string, UINT> materialLookup;
        std::unordered_map<std::string, UINT> vertexLookup;

        materials.push_back(MaterialDesc());
        materials[0].name = "default";
        materialLookup["default"] = 0;

        std::string mtlFile;
        V_RETURN(ParseOBJ(objPath, mtlFile, vertices, indices, triangleMaterials, materials, materialLookup, vertexLookup));

        if (!mtlFile.empty())
        {
            ParseMTL(basePath + ToWide(mtlFile), materials, materialLookup);
        }

        if (vertices.empty() || indices.empty())
        {
            return E_FAIL;
        }

        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.ByteWidth = UINT(vertices.size() * sizeof(MeshVertex));
        vbDesc.Usage = D3D11_USAGE_DEFAULT;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vbData = {};
        vbData.pSysMem = vertices.data();
        V_RETURN(pd3dDevice->CreateBuffer(&vbDesc, &vbData, &m_pVertexBuffer));

        D3D11_BUFFER_DESC ibDesc = {};
        ibDesc.ByteWidth = UINT(indices.size() * sizeof(UINT));
        ibDesc.Usage = D3D11_USAGE_DEFAULT;
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA ibData = {};
        ibData.pSysMem = indices.data();
        V_RETURN(pd3dDevice->CreateBuffer(&ibDesc, &ibData, &m_pIndexBuffer));

        m_Subsets.clear();
        m_Subsets.reserve(triangleMaterials.size());
        for (size_t triIndex = 0; triIndex < triangleMaterials.size(); ++triIndex)
        {
            const UINT materialId = triangleMaterials[triIndex];
            if (m_Subsets.empty() || m_Subsets.back().MaterialID != materialId)
            {
                Subset subset = {};
                subset.IndexStart = UINT(triIndex * 3);
                subset.IndexCount = 3;
                subset.MaterialID = materialId;
                m_Subsets.push_back(subset);
            }
            else
            {
                m_Subsets.back().IndexCount += 3;
            }
        }

        BuildSubsetBoundingBoxes(vertices, indices);
        m_CpuVertices = vertices;
        m_CpuIndices = indices;
        BuildSubsetVertexIndices();

        V_RETURN(CreateFallbackTexture(pd3dDevice));

        m_Materials.clear();
        m_Materials.resize(materials.size());
        m_MaterialDescs = materials;
        for (size_t i = 0; i < materials.size(); ++i)
        {
            MaterialResources& outMat = m_Materials[i];
            outMat.pDiffuseSRV = m_pFallbackDiffuseSRV;
            if (outMat.pDiffuseSRV)
            {
                outMat.pDiffuseSRV->AddRef();
            }

            LoadTextureIfPresent(pd3dDevice, pd3dDeviceContext, basePath, materials[i].diffuseTexture, true, &outMat.pDiffuseSRV);
            LoadTextureIfPresent(pd3dDevice, pd3dDeviceContext, basePath, materials[i].normalTexture, false, &outMat.pNormalSRV);
            LoadTextureIfPresent(pd3dDevice, pd3dDeviceContext, basePath, materials[i].specularTexture, false, &outMat.pSpecularSRV);
        }

        m_bLoaded = true;
        return hr;
    }

    void Destroy() override
    {
        SAFE_RELEASE(m_pVertexBuffer);
        SAFE_RELEASE(m_pIndexBuffer);

        for (size_t i = 0; i < m_Materials.size(); ++i)
        {
            SAFE_RELEASE(m_Materials[i].pDiffuseSRV);
            SAFE_RELEASE(m_Materials[i].pNormalSRV);
            SAFE_RELEASE(m_Materials[i].pSpecularSRV);
        }

        m_Materials.clear();
        m_MaterialDescs.clear();
        m_Subsets.clear();
        m_SubsetBoundingBoxes.clear();
        m_SubsetVertexIndices.clear();
        m_CpuVertices.clear();
        m_CpuIndices.clear();
        SAFE_RELEASE(m_pFallbackDiffuseSRV);
        m_bLoaded = false;
        ResetTransform();
        ResetBounds();
    }

    bool IsLoaded() const override
    {
        return m_bLoaded;
    }

    void Render(ID3D11DeviceContext* pd3dDeviceContext,
        UINT iDiffuseSlot = INVALID_SAMPLER_SLOT,
        UINT iNormalSlot = INVALID_SAMPLER_SLOT,
        UINT iSpecularSlot = INVALID_SAMPLER_SLOT) override
    {
        if (!m_pVertexBuffer || !m_pIndexBuffer)
        {
            return;
        }

        const UINT stride = sizeof(MeshVertex);
        const UINT offset = 0;
        pd3dDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
        pd3dDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        ID3D11ShaderResourceView* pNullSRV = NULL;

        for (size_t subsetIndex = 0; subsetIndex < m_Subsets.size(); ++subsetIndex)
        {
            const Subset& subset = m_Subsets[subsetIndex];
            const MaterialResources* pMaterial = (subset.MaterialID < m_Materials.size()) ? &m_Materials[subset.MaterialID] : NULL;

            ID3D11ShaderResourceView* pDiffuseSRV = pMaterial ? pMaterial->pDiffuseSRV : m_pFallbackDiffuseSRV;
            ID3D11ShaderResourceView* pNormalSRV = pMaterial ? pMaterial->pNormalSRV : NULL;
            ID3D11ShaderResourceView* pSpecularSRV = pMaterial ? pMaterial->pSpecularSRV : NULL;

            if (iDiffuseSlot != INVALID_SAMPLER_SLOT)
            {
                pd3dDeviceContext->PSSetShaderResources(iDiffuseSlot, 1, &pDiffuseSRV);
            }
            if (iNormalSlot != INVALID_SAMPLER_SLOT)
            {
                pd3dDeviceContext->PSSetShaderResources(iNormalSlot, 1, pNormalSRV ? &pNormalSRV : &pNullSRV);
            }
            if (iSpecularSlot != INVALID_SAMPLER_SLOT)
            {
                pd3dDeviceContext->PSSetShaderResources(iSpecularSlot, 1, pSpecularSRV ? &pSpecularSRV : &pNullSRV);
            }

            pd3dDeviceContext->DrawIndexed(subset.IndexCount, subset.IndexStart, 0);
        }
    }

    XMVECTOR GetAABBMin() const override
    {
        BoundingBox sceneBounding = TransformBoundingBox(MakeBoundingBoxFromMinMax(m_vAABBMin, m_vAABBMax, XMFLOAT4(1, 0, 1, 1)), m_mWorld);
        D3DXVECTOR3 vMin;
        D3DXVECTOR3 vMax;
        GetBoundingBoxMinMax(sceneBounding, vMin, vMax);
        return XMVectorSet(vMin.x, vMin.y, vMin.z, 1.0f);
    }

    XMVECTOR GetAABBMax() const override
    {
        BoundingBox sceneBounding = TransformBoundingBox(MakeBoundingBoxFromMinMax(m_vAABBMin, m_vAABBMax, XMFLOAT4(1, 0, 1, 1)), m_mWorld);
        D3DXVECTOR3 vMin;
        D3DXVECTOR3 vMax;
        GetBoundingBoxMinMax(sceneBounding, vMin, vMax);
        return XMVectorSet(vMax.x, vMax.y, vMax.z, 1.0f);
    }

    const D3DXMATRIX& GetWorldMatrix() const override
    {
        return m_mWorld;
    }

    void Translate(const D3DXVECTOR3& delta) override
    {
        m_mWorld._41 += delta.x;
        m_mWorld._42 += delta.y;
        m_mWorld._43 += delta.z;
    }

    bool TranslateSubMesh(INT subMeshIndex, const D3DXVECTOR3& delta, ID3D11DeviceContext* pd3dDeviceContext) override
    {
        if (!pd3dDeviceContext || !m_pVertexBuffer || subMeshIndex < 0 || subMeshIndex >= INT(m_SubsetVertexIndices.size()))
        {
            return false;
        }

        for (size_t vertexListIndex = 0; vertexListIndex < m_SubsetVertexIndices[subMeshIndex].size(); ++vertexListIndex)
        {
            const UINT vertexIndex = m_SubsetVertexIndices[subMeshIndex][vertexListIndex];
            if (vertexIndex < m_CpuVertices.size())
            {
                m_CpuVertices[vertexIndex].position += delta;
            }
        }

        if (subMeshIndex < INT(m_SubsetBoundingBoxes.size()))
        {
            for (INT cornerIndex = 0; cornerIndex < 8; ++cornerIndex)
            {
                m_SubsetBoundingBoxes[subMeshIndex].corners[cornerIndex].x += delta.x;
                m_SubsetBoundingBoxes[subMeshIndex].corners[cornerIndex].y += delta.y;
                m_SubsetBoundingBoxes[subMeshIndex].corners[cornerIndex].z += delta.z;
            }
        }

        RebuildSceneBoundsFromSubsets();
        pd3dDeviceContext->UpdateSubresource(m_pVertexBuffer, 0, NULL, m_CpuVertices.data(), 0, 0);
        return true;
    }

    bool PickSubMesh(const D3DXVECTOR3& rayOrigin, const D3DXVECTOR3& rayDirection, INT& pickedIndex, float& pickedDistance) const override
    {
        pickedIndex = -1;
        pickedDistance = FLT_MAX;
        D3DXMATRIX inverseWorld;
        D3DXMatrixInverse(&inverseWorld, NULL, &m_mWorld);
        const D3DXVECTOR3 localRayOrigin = TransformPoint(rayOrigin, inverseWorld);
        D3DXVECTOR3 localRayDirection = TransformDirection(rayDirection, inverseWorld);
        if (D3DXVec3LengthSq(&localRayDirection) <= 0.0f)
        {
            return false;
        }
        D3DXVec3Normalize(&localRayDirection, &localRayDirection);

        for (size_t subsetIndex = 0; subsetIndex < m_Subsets.size(); ++subsetIndex)
        {
            const Subset& subset = m_Subsets[subsetIndex];
            for (UINT indexOffset = 0; indexOffset + 2 < subset.IndexCount; indexOffset += 3)
            {
                const UINT i0 = subset.IndexStart + indexOffset;
                const UINT i1 = subset.IndexStart + indexOffset + 1;
                const UINT i2 = subset.IndexStart + indexOffset + 2;
                if (i2 >= m_CpuIndices.size())
                {
                    continue;
                }

                const UINT v0Index = m_CpuIndices[i0];
                const UINT v1Index = m_CpuIndices[i1];
                const UINT v2Index = m_CpuIndices[i2];
                if (v0Index >= m_CpuVertices.size() || v1Index >= m_CpuVertices.size() || v2Index >= m_CpuVertices.size())
                {
                    continue;
                }

                float hitDistance = FLT_MAX;
                if (IntersectRayTriangle(localRayOrigin,
                    localRayDirection,
                    m_CpuVertices[v0Index].position,
                    m_CpuVertices[v1Index].position,
                    m_CpuVertices[v2Index].position,
                    hitDistance) &&
                    hitDistance < pickedDistance)
                {
                    pickedIndex = INT(subsetIndex);
                    pickedDistance = hitDistance;
                }
            }
        }

        return pickedIndex >= 0;
    }

private:
    struct MeshVertex
    {
        D3DXVECTOR3 position;
        D3DXVECTOR3 normal;
        D3DXVECTOR2 texcoord;
    };

    struct MaterialDesc
    {
        std::string name;
        std::wstring diffuseTexture;
        std::wstring normalTexture;
        std::wstring specularTexture;
    };

    struct MaterialResources
    {
        MaterialResources()
            : pDiffuseSRV(NULL)
            , pNormalSRV(NULL)
            , pSpecularSRV(NULL)
        {
        }

        ID3D11ShaderResourceView* pDiffuseSRV;
        ID3D11ShaderResourceView* pNormalSRV;
        ID3D11ShaderResourceView* pSpecularSRV;
    };

    struct Subset
    {
        UINT IndexStart;
        UINT IndexCount;
        UINT MaterialID;
    };

    static std::string ToAnsi(const std::wstring& wide)
    {
        if (wide.empty())
        {
            return std::string();
        }

        const int required = WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, NULL, 0, NULL, NULL);
        std::string out(required > 0 ? required : 0, '\0');
        if (required > 0)
        {
            WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, &out[0], required, NULL, NULL);
            if (!out.empty() && out.back() == '\0')
            {
                out.pop_back();
            }
        }
        return out;
    }

    static std::wstring ToWide(const std::string& narrow)
    {
        if (narrow.empty())
        {
            return std::wstring();
        }

        const int required = MultiByteToWideChar(CP_ACP, 0, narrow.c_str(), -1, NULL, 0);
        std::wstring out(required > 0 ? required : 0, L'\0');
        if (required > 0)
        {
            MultiByteToWideChar(CP_ACP, 0, narrow.c_str(), -1, &out[0], required);
            if (!out.empty() && out.back() == L'\0')
            {
                out.pop_back();
            }
        }
        return out;
    }

    static std::wstring Trim(const std::wstring& value)
    {
        const size_t start = value.find_first_not_of(L" \t\r\n");
        if (start == std::wstring::npos)
        {
            return std::wstring();
        }

        const size_t end = value.find_last_not_of(L" \t\r\n");
        return value.substr(start, end - start + 1);
    }

    static std::wstring GetDirectory(const std::wstring& path)
    {
        const size_t slashPos = path.find_last_of(L"\\/");
        if (slashPos == std::wstring::npos)
        {
            return std::wstring();
        }

        return path.substr(0, slashPos + 1);
    }

    void ResetBounds()
    {
        m_vAABBMin = D3DXVECTOR3(FLT_MAX, FLT_MAX, FLT_MAX);
        m_vAABBMax = D3DXVECTOR3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    }

    void ResetTransform()
    {
        D3DXMatrixIdentity(&m_mWorld);
    }

    void BuildSubsetBoundingBoxes(const std::vector<MeshVertex>& vertices, const std::vector<UINT>& indices)
    {
        m_SubsetBoundingBoxes.clear();
        m_SubsetBoundingBoxes.reserve(m_Subsets.size());

        for (size_t subsetIndex = 0; subsetIndex < m_Subsets.size(); ++subsetIndex)
        {
            const Subset& subset = m_Subsets[subsetIndex];
            D3DXVECTOR3 subsetMin(FLT_MAX, FLT_MAX, FLT_MAX);
            D3DXVECTOR3 subsetMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
            bool hasVertices = false;

            for (UINT indexOffset = 0; indexOffset < subset.IndexCount; ++indexOffset)
            {
                const UINT indexBufferPosition = subset.IndexStart + indexOffset;
                if (indexBufferPosition >= indices.size())
                {
                    continue;
                }

                const UINT vertexIndex = indices[indexBufferPosition];
                if (vertexIndex >= vertices.size())
                {
                    continue;
                }

                const D3DXVECTOR3& position = vertices[vertexIndex].position;
                subsetMin.x = min(subsetMin.x, position.x);
                subsetMin.y = min(subsetMin.y, position.y);
                subsetMin.z = min(subsetMin.z, position.z);
                subsetMax.x = max(subsetMax.x, position.x);
                subsetMax.y = max(subsetMax.y, position.y);
                subsetMax.z = max(subsetMax.z, position.z);
                hasVertices = true;
            }

            if (hasVertices)
            {
                m_SubsetBoundingBoxes.push_back(MakeBoundingBoxFromMinMax(subsetMin, subsetMax, XMFLOAT4(0, 0, 1, 1)));
            }
        }
    }

    void BuildSubsetVertexIndices()
    {
        m_SubsetVertexIndices.clear();
        m_SubsetVertexIndices.resize(m_Subsets.size());

        for (size_t subsetIndex = 0; subsetIndex < m_Subsets.size(); ++subsetIndex)
        {
            const Subset& subset = m_Subsets[subsetIndex];
            std::vector<UINT>& subsetVertexIndices = m_SubsetVertexIndices[subsetIndex];
            subsetVertexIndices.reserve(subset.IndexCount);

            for (UINT indexOffset = 0; indexOffset < subset.IndexCount; ++indexOffset)
            {
                const UINT indexBufferPosition = subset.IndexStart + indexOffset;
                if (indexBufferPosition < m_CpuIndices.size())
                {
                    subsetVertexIndices.push_back(m_CpuIndices[indexBufferPosition]);
                }
            }

            std::sort(subsetVertexIndices.begin(), subsetVertexIndices.end());
            subsetVertexIndices.erase(std::unique(subsetVertexIndices.begin(), subsetVertexIndices.end()), subsetVertexIndices.end());
        }
    }

    void RebuildSceneBoundsFromSubsets()
    {
        if (m_SubsetBoundingBoxes.empty())
        {
            return;
        }

        ResetBounds();
        for (size_t subsetIndex = 0; subsetIndex < m_SubsetBoundingBoxes.size(); ++subsetIndex)
        {
            D3DXVECTOR3 subsetMin;
            D3DXVECTOR3 subsetMax;
            GetBoundingBoxMinMax(m_SubsetBoundingBoxes[subsetIndex], subsetMin, subsetMax);
            m_vAABBMin.x = min(m_vAABBMin.x, subsetMin.x);
            m_vAABBMin.y = min(m_vAABBMin.y, subsetMin.y);
            m_vAABBMin.z = min(m_vAABBMin.z, subsetMin.z);
            m_vAABBMax.x = max(m_vAABBMax.x, subsetMax.x);
            m_vAABBMax.y = max(m_vAABBMax.y, subsetMax.y);
            m_vAABBMax.z = max(m_vAABBMax.z, subsetMax.z);
        }
    }

    HRESULT CreateFallbackTexture(ID3D11Device* pd3dDevice)
    {
        if (m_pFallbackDiffuseSRV)
        {
            return S_OK;
        }

        const UINT whitePixel = 0xff00ffff;

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = 1;
        desc.Height = 1;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA data = {};
        data.pSysMem = &whitePixel;
        data.SysMemPitch = sizeof(UINT);

        ID3D11Texture2D* pTexture = NULL;
        HRESULT hr = pd3dDevice->CreateTexture2D(&desc, &data, &pTexture);
        if (FAILED(hr))
        {
            SAFE_RELEASE(pTexture);
            return hr;
        }

        hr = pd3dDevice->CreateShaderResourceView(pTexture, NULL, &m_pFallbackDiffuseSRV);
        SAFE_RELEASE(pTexture);
        return hr;
    }

    UINT GetOrCreateMaterial(const std::string& name,
        std::vector<MaterialDesc>& materials,
        std::unordered_map<std::string, UINT>& materialLookup)
    {
        std::unordered_map<std::string, UINT>::const_iterator it = materialLookup.find(name);
        if (it != materialLookup.end())
        {
            return it->second;
        }

        const UINT materialId = (UINT)materials.size();
        MaterialDesc material;
        material.name = name;
        materials.push_back(material);
        materialLookup[name] = materialId;
        return materialId;
    }

    static int ResolveOBJIndex(int index, size_t count)
    {
        if (index > 0)
        {
            return index - 1;
        }

        if (index < 0)
        {
            return int(count) + index;
        }

        return -1;
    }

    UINT GetOrCreateVertex(const std::string& token,
        const std::vector<D3DXVECTOR3>& positions,
        const std::vector<D3DXVECTOR3>& normals,
        const std::vector<D3DXVECTOR2>& texcoords,
        std::vector<MeshVertex>& vertices,
        std::unordered_map<std::string, UINT>& vertexLookup)
    {
        std::unordered_map<std::string, UINT>::const_iterator cached = vertexLookup.find(token);
        if (cached != vertexLookup.end())
        {
            return cached->second;
        }

        int positionIndex = 0;
        int texcoordIndex = 0;
        int normalIndex = 0;
        sscanf_s(token.c_str(), "%d/%d/%d", &positionIndex, &texcoordIndex, &normalIndex);

        const int pos = ResolveOBJIndex(positionIndex, positions.size());
        const int uv = ResolveOBJIndex(texcoordIndex, texcoords.size());
        const int nrm = ResolveOBJIndex(normalIndex, normals.size());

        if (pos < 0 || pos >= (int)positions.size())
        {
            return 0;
        }

        MeshVertex vertex = {};
        vertex.position = positions[pos];
        vertex.normal = (nrm >= 0 && nrm < (int)normals.size()) ? normals[nrm] : D3DXVECTOR3(0.0f, 1.0f, 0.0f);
        vertex.texcoord = (uv >= 0 && uv < (int)texcoords.size()) ? texcoords[uv] : D3DXVECTOR2(0.0f, 0.0f);

        const UINT newIndex = (UINT)vertices.size();
        vertices.push_back(vertex);
        vertexLookup[token] = newIndex;
        return newIndex;
    }

    HRESULT ParseOBJ(const std::wstring& objPath,
        std::string& mtlFile,
        std::vector<MeshVertex>& vertices,
        std::vector<UINT>& indices,
        std::vector<UINT>& triangleMaterials,
        std::vector<MaterialDesc>& materials,
        std::unordered_map<std::string, UINT>& materialLookup,
        std::unordered_map<std::string, UINT>& vertexLookup)
    {
        std::ifstream file(ToAnsi(objPath).c_str());
        if (!file)
        {
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }

        std::vector<D3DXVECTOR3> positions;
        std::vector<D3DXVECTOR3> normals;
        std::vector<D3DXVECTOR2> texcoords;

        UINT currentMaterial = 0;
        std::string line;

        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '#')
            {
                continue;
            }

            std::istringstream stream(line);
            std::string command;
            stream >> command;

            if (command == "mtllib")
            {
                stream >> mtlFile;
            }
            else if (command == "usemtl")
            {
                std::string materialName;
                stream >> materialName;
                currentMaterial = GetOrCreateMaterial(materialName, materials, materialLookup);
            }
            else if (command == "v")
            {
                D3DXVECTOR3 position;
                stream >> position.x >> position.y >> position.z;
                position *= m_fUniformScale;
                positions.push_back(position);
                m_vAABBMin.x = min(m_vAABBMin.x, position.x);
                m_vAABBMin.y = min(m_vAABBMin.y, position.y);
                m_vAABBMin.z = min(m_vAABBMin.z, position.z);
                m_vAABBMax.x = max(m_vAABBMax.x, position.x);
                m_vAABBMax.y = max(m_vAABBMax.y, position.y);
                m_vAABBMax.z = max(m_vAABBMax.z, position.z);
            }
            else if (command == "vt")
            {
                D3DXVECTOR2 texcoord;
                stream >> texcoord.x >> texcoord.y;
                texcoords.push_back(texcoord);
            }
            else if (command == "vn")
            {
                D3DXVECTOR3 normal;
                stream >> normal.x >> normal.y >> normal.z;
                normals.push_back(normal);
            }
            else if (command == "f")
            {
                std::vector<UINT> faceIndices;
                std::string token;
                while (stream >> token)
                {
                    faceIndices.push_back(GetOrCreateVertex(token, positions, normals, texcoords, vertices, vertexLookup));
                }

                if (faceIndices.size() < 3)
                {
                    continue;
                }

                for (size_t i = 1; i + 1 < faceIndices.size(); ++i)
                {
                    indices.push_back(faceIndices[0]);
                    indices.push_back(faceIndices[i]);
                    indices.push_back(faceIndices[i + 1]);
                    triangleMaterials.push_back(currentMaterial);
                }
            }
        }

        return S_OK;
    }

    void ParseMTL(const std::wstring& mtlPath,
        std::vector<MaterialDesc>& materials,
        std::unordered_map<std::string, UINT>& materialLookup)
    {
        std::ifstream file(ToAnsi(mtlPath).c_str());
        if (!file)
        {
            return;
        }

        MaterialDesc* currentMaterial = NULL;
        std::string line;

        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '#')
            {
                continue;
            }

            std::istringstream stream(line);
            std::string command;
            stream >> command;

            if (command == "newmtl")
            {
                std::string materialName;
                stream >> materialName;
                const UINT materialId = GetOrCreateMaterial(materialName, materials, materialLookup);
                currentMaterial = &materials[materialId];
            }
            else if (currentMaterial && (command == "map_Kd" || command == "map_Ka"))
            {
                std::string texturePath;
                stream >> texturePath;
                currentMaterial->diffuseTexture = ToWide(texturePath);
            }
            else if (currentMaterial && (command == "map_bump" || command == "bump"))
            {
                std::string texturePath;
                stream >> texturePath;
                currentMaterial->normalTexture = ToWide(texturePath);
            }
            else if (currentMaterial && command == "map_Ks")
            {
                std::string texturePath;
                stream >> texturePath;
                currentMaterial->specularTexture = ToWide(texturePath);
            }
        }
    }

    void LoadTextureIfPresent(ID3D11Device* pd3dDevice,
        ID3D11DeviceContext* pd3dDeviceContext,
        const std::wstring& basePath,
        const std::wstring& textureName,
        bool bSRGB,
        ID3D11ShaderResourceView** ppSRV)
    {
        if (!ppSRV || textureName.empty())
        {
            return;
        }

        std::wstring fullPath = basePath + textureName;
        std::replace(fullPath.begin(), fullPath.end(), L'/', L'\\');

        ID3D11ShaderResourceView* pLoadedSRV = NULL;
        if (SUCCEEDED(DXUTGetGlobalResourceCache().CreateTextureFromFile(
            pd3dDevice, pd3dDeviceContext, fullPath.c_str(), &pLoadedSRV, bSRGB)))
        {
            SAFE_RELEASE(*ppSRV);
            *ppSRV = pLoadedSRV;
        }
    }

    std::wstring                    m_szMeshPath;
    float                           m_fUniformScale;
    bool                            m_bLoaded;
    ID3D11Buffer*                   m_pVertexBuffer;
    ID3D11Buffer*                   m_pIndexBuffer;
    ID3D11ShaderResourceView*       m_pFallbackDiffuseSRV;
    std::vector<Subset>             m_Subsets;
    std::vector<BoundingBox>        m_SubsetBoundingBoxes;
    std::vector<std::vector<UINT>>  m_SubsetVertexIndices;
    std::vector<MaterialResources>  m_Materials;
    std::vector<MaterialDesc>       m_MaterialDescs;
    std::vector<MeshVertex>         m_CpuVertices;
    std::vector<UINT>               m_CpuIndices;
    D3DXVECTOR3                     m_vAABBMin;
    D3DXVECTOR3                     m_vAABBMax;
    D3DXMATRIX                      m_mWorld;
};

class SimpleSceneMesh : public ISceneMesh
{
public:
    SimpleSceneMesh()
        : m_bLoaded(false)
        , m_pVertexBuffer(NULL)
        , m_pIndexBuffer(NULL)
    {
        ResetBounds();
        ResetTransform();
    }

    bool GetSubMeshAlbedoInfo(INT subMeshIndex, SelectedAlbedoInfo& outInfo) const override
    {
        outInfo = SelectedAlbedoInfo();

        if (subMeshIndex < 0 || subMeshIndex >= INT(m_Subsets.size()))
        {
            return false;
        }

        const Subset& subset = m_Subsets[subMeshIndex];
        outInfo.pDiffuseSRV = subset.pDiffuseSRV;
        outInfo.materialName = (subMeshIndex == 0) ? "plane" : "cube";
        outInfo.textureName = "generated solid color";
        return outInfo.pDiffuseSRV != NULL;
    }

    HRESULT Create(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dDeviceContext) override
    {
        UNREFERENCED_PARAMETER(pd3dDeviceContext);

        Destroy();
        ResetTransform();
        ResetBounds();

        BuildGeometry();
        BuildSubsetAccelerationData();
        m_SceneBounding = MakeBoundingBoxFromMinMax(m_vAABBMin, m_vAABBMax, XMFLOAT4(1, 0, 1, 1));

        D3D11_BUFFER_DESC vertexBufferDesc = {};
        vertexBufferDesc.ByteWidth = UINT(sizeof(SimpleVertex) * m_CpuVertices.size());
        vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vertexBufferData = {};
        vertexBufferData.pSysMem = m_CpuVertices.data();
        HRESULT hr = pd3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_pVertexBuffer);
        if (FAILED(hr))
        {
            Destroy();
            return hr;
        }

        D3D11_BUFFER_DESC indexBufferDesc = {};
        indexBufferDesc.ByteWidth = UINT(sizeof(UINT) * m_CpuIndices.size());
        indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA indexBufferData = {};
        indexBufferData.pSysMem = m_CpuIndices.data();
        hr = pd3dDevice->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_pIndexBuffer);
        if (FAILED(hr))
        {
            Destroy();
            return hr;
        }

        hr = CreateSolidColorTexture(pd3dDevice, D3DXCOLOR(0.48f, 0.48f, 0.50f, 1.0f), &m_Subsets[0].pDiffuseSRV);
        if (FAILED(hr))
        {
            Destroy();
            return hr;
        }

        hr = CreateSolidColorTexture(pd3dDevice, D3DXCOLOR(0.92f, 0.90f, 0.86f, 1.0f), &m_Subsets[1].pDiffuseSRV);
        if (FAILED(hr))
        {
            Destroy();
            return hr;
        }

        m_bLoaded = true;
        return S_OK;
    }

    void Destroy() override
    {
        SAFE_RELEASE(m_pVertexBuffer);
        SAFE_RELEASE(m_pIndexBuffer);

        for (size_t subsetIndex = 0; subsetIndex < m_Subsets.size(); ++subsetIndex)
        {
            SAFE_RELEASE(m_Subsets[subsetIndex].pDiffuseSRV);
        }

        m_Subsets.clear();
        m_SubsetBoundingBoxes.clear();
        m_SubsetVertexIndices.clear();
        m_SubMeshTriangles.clear();
        m_SubMeshPickRanges.clear();
        m_CpuVertices.clear();
        m_CpuIndices.clear();
        m_bLoaded = false;
        ResetTransform();
        ResetBounds();
    }

    bool IsLoaded() const override
    {
        return m_bLoaded;
    }

    void Render(ID3D11DeviceContext* pd3dDeviceContext,
        UINT iDiffuseSlot = INVALID_SAMPLER_SLOT,
        UINT iNormalSlot = INVALID_SAMPLER_SLOT,
        UINT iSpecularSlot = INVALID_SAMPLER_SLOT) override
    {
        if (!m_pVertexBuffer || !m_pIndexBuffer)
        {
            return;
        }

        const UINT stride = sizeof(SimpleVertex);
        const UINT offset = 0;
        pd3dDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
        pd3dDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        ID3D11ShaderResourceView* pNullSRV = NULL;

        for (size_t subsetIndex = 0; subsetIndex < m_Subsets.size(); ++subsetIndex)
        {
            const Subset& subset = m_Subsets[subsetIndex];

            if (iDiffuseSlot != INVALID_SAMPLER_SLOT)
            {
                pd3dDeviceContext->PSSetShaderResources(iDiffuseSlot, 1, &subset.pDiffuseSRV);
            }
            if (iNormalSlot != INVALID_SAMPLER_SLOT)
            {
                pd3dDeviceContext->PSSetShaderResources(iNormalSlot, 1, &pNullSRV);
            }
            if (iSpecularSlot != INVALID_SAMPLER_SLOT)
            {
                pd3dDeviceContext->PSSetShaderResources(iSpecularSlot, 1, &pNullSRV);
            }

            pd3dDeviceContext->DrawIndexed(subset.IndexCount, subset.IndexStart, 0);
        }
    }

    XMVECTOR GetAABBMin() const override
    {
        BoundingBox sceneBounding = TransformBoundingBox(m_SceneBounding, m_mWorld);
        D3DXVECTOR3 vMin;
        D3DXVECTOR3 vMax;
        GetBoundingBoxMinMax(sceneBounding, vMin, vMax);
        return XMVectorSet(vMin.x, vMin.y, vMin.z, 1.0f);
    }

    XMVECTOR GetAABBMax() const override
    {
        BoundingBox sceneBounding = TransformBoundingBox(m_SceneBounding, m_mWorld);
        D3DXVECTOR3 vMin;
        D3DXVECTOR3 vMax;
        GetBoundingBoxMinMax(sceneBounding, vMin, vMax);
        return XMVectorSet(vMax.x, vMax.y, vMax.z, 1.0f);
    }

    const D3DXMATRIX& GetWorldMatrix() const override
    {
        return m_mWorld;
    }

    void Translate(const D3DXVECTOR3& delta) override
    {
        m_mWorld._41 += delta.x;
        m_mWorld._42 += delta.y;
        m_mWorld._43 += delta.z;
    }

    bool TranslateSubMesh(INT subMeshIndex, const D3DXVECTOR3& delta, ID3D11DeviceContext* pd3dDeviceContext) override
    {
        if (!pd3dDeviceContext || subMeshIndex < 0 || subMeshIndex >= INT(m_SubsetVertexIndices.size()))
        {
            return false;
        }

        const std::vector<UINT>& subsetVertexIndices = m_SubsetVertexIndices[subMeshIndex];
        for (size_t vertexListIndex = 0; vertexListIndex < subsetVertexIndices.size(); ++vertexListIndex)
        {
            const UINT vertexIndex = subsetVertexIndices[vertexListIndex];
            if (vertexIndex < m_CpuVertices.size())
            {
                m_CpuVertices[vertexIndex].position += delta;
            }
        }

        const SubMeshPickRange& pickRange = m_SubMeshPickRanges[subMeshIndex];
        for (size_t triangleIndex = pickRange.firstTriangle; triangleIndex < (pickRange.firstTriangle + pickRange.triangleCount); ++triangleIndex)
        {
            m_SubMeshTriangles[triangleIndex].v0 += delta;
            m_SubMeshTriangles[triangleIndex].v1 += delta;
            m_SubMeshTriangles[triangleIndex].v2 += delta;
        }

        for (INT cornerIndex = 0; cornerIndex < 8; ++cornerIndex)
        {
            m_SubsetBoundingBoxes[subMeshIndex].corners[cornerIndex].x += delta.x;
            m_SubsetBoundingBoxes[subMeshIndex].corners[cornerIndex].y += delta.y;
            m_SubsetBoundingBoxes[subMeshIndex].corners[cornerIndex].z += delta.z;
        }

        RebuildSceneBoundsFromSubsets();
        m_SceneBounding = MakeBoundingBoxFromMinMax(m_vAABBMin, m_vAABBMax, XMFLOAT4(1, 0, 1, 1));
        pd3dDeviceContext->UpdateSubresource(m_pVertexBuffer, 0, NULL, m_CpuVertices.data(), 0, 0);
        return true;
    }

    void UpdateGlobalBoundingBox(std::vector<BoundingBox>& globalBoundingBoxes) const override
    {
        globalBoundingBoxes.push_back(TransformBoundingBox(m_SceneBounding, m_mWorld));
    }

    void UpdateAllBoundingBoxes(std::vector<BoundingBox>& boundingBoxes) const override
    {
        for (size_t subsetIndex = 0; subsetIndex < m_SubsetBoundingBoxes.size(); ++subsetIndex)
        {
            boundingBoxes.push_back(TransformBoundingBox(m_SubsetBoundingBoxes[subsetIndex], m_mWorld));
        }
    }

    bool PickSubMesh(const D3DXVECTOR3& rayOrigin, const D3DXVECTOR3& rayDirection, INT& pickedIndex, float& pickedDistance) const override
    {
        pickedIndex = -1;
        pickedDistance = FLT_MAX;

        D3DXMATRIX inverseWorld;
        D3DXMatrixInverse(&inverseWorld, NULL, &m_mWorld);
        const D3DXVECTOR3 localRayOrigin = TransformPoint(rayOrigin, inverseWorld);
        D3DXVECTOR3 localRayDirection = TransformDirection(rayDirection, inverseWorld);
        if (D3DXVec3LengthSq(&localRayDirection) <= 0.0f)
        {
            return false;
        }
        D3DXVec3Normalize(&localRayDirection, &localRayDirection);

        for (size_t subMeshIndex = 0; subMeshIndex < m_SubMeshPickRanges.size(); ++subMeshIndex)
        {
            float boxDistance = FLT_MAX;
            if (!IntersectRayBoundingBox(localRayOrigin, localRayDirection, m_SubsetBoundingBoxes[subMeshIndex], boxDistance))
            {
                continue;
            }

            if (boxDistance > pickedDistance)
            {
                continue;
            }

            const SubMeshPickRange& range = m_SubMeshPickRanges[subMeshIndex];
            for (size_t triangleIndex = range.firstTriangle; triangleIndex < (range.firstTriangle + range.triangleCount); ++triangleIndex)
            {
                const TriangleData& triangle = m_SubMeshTriangles[triangleIndex];
                float hitDistance = FLT_MAX;
                if (IntersectRayTriangle(localRayOrigin, localRayDirection, triangle.v0, triangle.v1, triangle.v2, hitDistance) &&
                    hitDistance < pickedDistance)
                {
                    pickedIndex = INT(subMeshIndex);
                    pickedDistance = hitDistance;
                }
            }
        }

        return pickedIndex >= 0;
    }

private:
    struct SimpleVertex
    {
        D3DXVECTOR3 position;
        D3DXVECTOR3 normal;
        D3DXVECTOR2 texcoord;
    };

    struct Subset
    {
        UINT IndexStart;
        UINT IndexCount;
        ID3D11ShaderResourceView* pDiffuseSRV;
    };

    struct TriangleData
    {
        D3DXVECTOR3 v0;
        D3DXVECTOR3 v1;
        D3DXVECTOR3 v2;
    };

    struct SubMeshPickRange
    {
        size_t firstTriangle;
        size_t triangleCount;
    };

    static void ExpandBounds(D3DXVECTOR3& vMin, D3DXVECTOR3& vMax, const D3DXVECTOR3& point)
    {
        vMin.x = min(vMin.x, point.x);
        vMin.y = min(vMin.y, point.y);
        vMin.z = min(vMin.z, point.z);
        vMax.x = max(vMax.x, point.x);
        vMax.y = max(vMax.y, point.y);
        vMax.z = max(vMax.z, point.z);
    }

    static SimpleVertex MakeVertex(float x, float y, float z, float nx, float ny, float nz, float u, float v)
    {
        SimpleVertex vertex = {};
        vertex.position = D3DXVECTOR3(x, y, z);
        vertex.normal = D3DXVECTOR3(nx, ny, nz);
        vertex.texcoord = D3DXVECTOR2(u, v);
        return vertex;
    }

    static HRESULT CreateSolidColorTexture(ID3D11Device* pd3dDevice, const D3DXCOLOR& color, ID3D11ShaderResourceView** ppSRV)
    {
        if (!pd3dDevice || !ppSRV)
        {
            return E_INVALIDARG;
        }

        const UINT rgba =
            (UINT(color.a * 255.0f) << 24) |
            (UINT(color.b * 255.0f) << 16) |
            (UINT(color.g * 255.0f) << 8) |
            UINT(color.r * 255.0f);

        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = 1;
        textureDesc.Height = 1;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA textureData = {};
        textureData.pSysMem = &rgba;
        textureData.SysMemPitch = sizeof(UINT);

        ID3D11Texture2D* pTexture = NULL;
        HRESULT hr = pd3dDevice->CreateTexture2D(&textureDesc, &textureData, &pTexture);
        if (FAILED(hr))
        {
            return hr;
        }

        hr = pd3dDevice->CreateShaderResourceView(pTexture, NULL, ppSRV);
        SAFE_RELEASE(pTexture);
        return hr;
    }

    void BuildGeometry()
    {
        m_Subsets.clear();
        m_SubsetBoundingBoxes.clear();
        m_SubsetVertexIndices.clear();
        m_SubMeshTriangles.clear();
        m_SubMeshPickRanges.clear();
        m_CpuVertices.clear();
        m_CpuIndices.clear();

        AddPlaneSubset();
        AddCubeSubset();
    }

    void AddPlaneSubset()
    {
        const UINT vertexStart = UINT(m_CpuVertices.size());
        const UINT indexStart = UINT(m_CpuIndices.size());
        const float planeHalfExtent = 10.0f;

        m_CpuVertices.push_back(MakeVertex(-planeHalfExtent, 0.0f, -planeHalfExtent, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f));
        m_CpuVertices.push_back(MakeVertex(-planeHalfExtent, 0.0f, planeHalfExtent, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f));
        m_CpuVertices.push_back(MakeVertex(planeHalfExtent, 0.0f, planeHalfExtent, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f));
        m_CpuVertices.push_back(MakeVertex(planeHalfExtent, 0.0f, -planeHalfExtent, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f));

        m_CpuIndices.push_back(vertexStart + 0);
        m_CpuIndices.push_back(vertexStart + 1);
        m_CpuIndices.push_back(vertexStart + 2);
        m_CpuIndices.push_back(vertexStart + 0);
        m_CpuIndices.push_back(vertexStart + 2);
        m_CpuIndices.push_back(vertexStart + 3);

        Subset subset = {};
        subset.IndexStart = indexStart;
        subset.IndexCount = 6;
        subset.pDiffuseSRV = NULL;
        m_Subsets.push_back(subset);
    }

    void AddCubeFace(const D3DXVECTOR3& v0,
        const D3DXVECTOR3& v1,
        const D3DXVECTOR3& v2,
        const D3DXVECTOR3& v3,
        const D3DXVECTOR3& normal)
    {
        const UINT vertexStart = UINT(m_CpuVertices.size());
        m_CpuVertices.push_back(MakeVertex(v0.x, v0.y, v0.z, normal.x, normal.y, normal.z, 0.0f, 1.0f));
        m_CpuVertices.push_back(MakeVertex(v1.x, v1.y, v1.z, normal.x, normal.y, normal.z, 0.0f, 0.0f));
        m_CpuVertices.push_back(MakeVertex(v2.x, v2.y, v2.z, normal.x, normal.y, normal.z, 1.0f, 0.0f));
        m_CpuVertices.push_back(MakeVertex(v3.x, v3.y, v3.z, normal.x, normal.y, normal.z, 1.0f, 1.0f));

        m_CpuIndices.push_back(vertexStart + 0);
        m_CpuIndices.push_back(vertexStart + 1);
        m_CpuIndices.push_back(vertexStart + 2);
        m_CpuIndices.push_back(vertexStart + 0);
        m_CpuIndices.push_back(vertexStart + 2);
        m_CpuIndices.push_back(vertexStart + 3);
    }

    void AddCubeSubset()
    {
        const UINT indexStart = UINT(m_CpuIndices.size());
        const float minX = -1.0f;
        const float maxX = 1.0f;
        const float minY = 0.0f;
        const float maxY = 2.0f;
        const float minZ = -1.0f;
        const float maxZ = 1.0f;

        AddCubeFace(
            D3DXVECTOR3(minX, minY, maxZ),
            D3DXVECTOR3(minX, maxY, maxZ),
            D3DXVECTOR3(maxX, maxY, maxZ),
            D3DXVECTOR3(maxX, minY, maxZ),
            D3DXVECTOR3(0.0f, 0.0f, 1.0f));

        AddCubeFace(
            D3DXVECTOR3(maxX, minY, minZ),
            D3DXVECTOR3(maxX, maxY, minZ),
            D3DXVECTOR3(minX, maxY, minZ),
            D3DXVECTOR3(minX, minY, minZ),
            D3DXVECTOR3(0.0f, 0.0f, -1.0f));

        AddCubeFace(
            D3DXVECTOR3(minX, minY, minZ),
            D3DXVECTOR3(minX, maxY, minZ),
            D3DXVECTOR3(minX, maxY, maxZ),
            D3DXVECTOR3(minX, minY, maxZ),
            D3DXVECTOR3(-1.0f, 0.0f, 0.0f));

        AddCubeFace(
            D3DXVECTOR3(maxX, minY, maxZ),
            D3DXVECTOR3(maxX, maxY, maxZ),
            D3DXVECTOR3(maxX, maxY, minZ),
            D3DXVECTOR3(maxX, minY, minZ),
            D3DXVECTOR3(1.0f, 0.0f, 0.0f));

        AddCubeFace(
            D3DXVECTOR3(minX, maxY, maxZ),
            D3DXVECTOR3(minX, maxY, minZ),
            D3DXVECTOR3(maxX, maxY, minZ),
            D3DXVECTOR3(maxX, maxY, maxZ),
            D3DXVECTOR3(0.0f, 1.0f, 0.0f));

        AddCubeFace(
            D3DXVECTOR3(minX, minY, minZ),
            D3DXVECTOR3(minX, minY, maxZ),
            D3DXVECTOR3(maxX, minY, maxZ),
            D3DXVECTOR3(maxX, minY, minZ),
            D3DXVECTOR3(0.0f, -1.0f, 0.0f));

        Subset subset = {};
        subset.IndexStart = indexStart;
        subset.IndexCount = UINT(m_CpuIndices.size()) - indexStart;
        subset.pDiffuseSRV = NULL;
        m_Subsets.push_back(subset);
    }

    void BuildSubsetAccelerationData()
    {
        ResetBounds();

        for (size_t subsetIndex = 0; subsetIndex < m_Subsets.size(); ++subsetIndex)
        {
            const Subset& subset = m_Subsets[subsetIndex];
            const size_t firstTriangle = m_SubMeshTriangles.size();
            D3DXVECTOR3 subsetMin(FLT_MAX, FLT_MAX, FLT_MAX);
            D3DXVECTOR3 subsetMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
            std::vector<UINT> subsetVertexIndices;

            for (UINT indexOffset = 0; indexOffset + 2 < subset.IndexCount; indexOffset += 3)
            {
                const UINT index0 = m_CpuIndices[subset.IndexStart + indexOffset + 0];
                const UINT index1 = m_CpuIndices[subset.IndexStart + indexOffset + 1];
                const UINT index2 = m_CpuIndices[subset.IndexStart + indexOffset + 2];
                const D3DXVECTOR3& v0 = m_CpuVertices[index0].position;
                const D3DXVECTOR3& v1 = m_CpuVertices[index1].position;
                const D3DXVECTOR3& v2 = m_CpuVertices[index2].position;

                subsetVertexIndices.push_back(index0);
                subsetVertexIndices.push_back(index1);
                subsetVertexIndices.push_back(index2);
                ExpandBounds(subsetMin, subsetMax, v0);
                ExpandBounds(subsetMin, subsetMax, v1);
                ExpandBounds(subsetMin, subsetMax, v2);

                TriangleData triangle = {};
                triangle.v0 = v0;
                triangle.v1 = v1;
                triangle.v2 = v2;
                m_SubMeshTriangles.push_back(triangle);
            }

            std::sort(subsetVertexIndices.begin(), subsetVertexIndices.end());
            subsetVertexIndices.erase(std::unique(subsetVertexIndices.begin(), subsetVertexIndices.end()), subsetVertexIndices.end());
            m_SubsetVertexIndices.push_back(subsetVertexIndices);

            if (subsetIndex == 0)
            {
                subsetMin.y -= 0.02f;
                subsetMax.y += 0.02f;
            }

            m_SubsetBoundingBoxes.push_back(MakeBoundingBoxFromMinMax(subsetMin, subsetMax, XMFLOAT4(0, 0, 1, 1)));

            SubMeshPickRange pickRange = {};
            pickRange.firstTriangle = firstTriangle;
            pickRange.triangleCount = (subset.IndexCount / 3);
            m_SubMeshPickRanges.push_back(pickRange);

            ExpandBounds(m_vAABBMin, m_vAABBMax, subsetMin);
            ExpandBounds(m_vAABBMin, m_vAABBMax, subsetMax);
        }
    }

    void RebuildSceneBoundsFromSubsets()
    {
        ResetBounds();
        for (size_t subsetIndex = 0; subsetIndex < m_SubsetBoundingBoxes.size(); ++subsetIndex)
        {
            D3DXVECTOR3 subsetMin;
            D3DXVECTOR3 subsetMax;
            GetBoundingBoxMinMax(m_SubsetBoundingBoxes[subsetIndex], subsetMin, subsetMax);
            ExpandBounds(m_vAABBMin, m_vAABBMax, subsetMin);
            ExpandBounds(m_vAABBMin, m_vAABBMax, subsetMax);
        }
    }

    void ResetTransform()
    {
        D3DXMatrixIdentity(&m_mWorld);
    }

    void ResetBounds()
    {
        m_vAABBMin = D3DXVECTOR3(FLT_MAX, FLT_MAX, FLT_MAX);
        m_vAABBMax = D3DXVECTOR3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    }

    bool                                m_bLoaded;
    ID3D11Buffer*                       m_pVertexBuffer;
    ID3D11Buffer*                       m_pIndexBuffer;
    std::vector<Subset>                 m_Subsets;
    std::vector<BoundingBox>            m_SubsetBoundingBoxes;
    std::vector<std::vector<UINT>>      m_SubsetVertexIndices;
    std::vector<TriangleData>           m_SubMeshTriangles;
    std::vector<SubMeshPickRange>       m_SubMeshPickRanges;
    std::vector<SimpleVertex>           m_CpuVertices;
    std::vector<UINT>                   m_CpuIndices;
    D3DXVECTOR3                         m_vAABBMin;
    D3DXVECTOR3                         m_vAABBMax;
    BoundingBox                         m_SceneBounding;
    D3DXMATRIX                          m_mWorld;
};
