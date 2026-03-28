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
};

class SDKSceneMesh : public ISceneMesh
{
public:
    explicit SDKSceneMesh(const WCHAR* szMeshPath)
        : m_szMeshPath(szMeshPath)
        , m_bLoaded(false)
    {
        ResetBounds();
    }

    HRESULT Create(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dDeviceContext) override
    {
        UNREFERENCED_PARAMETER(pd3dDeviceContext);

        ResetBounds();
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
        }

        m_bLoaded = true;
        return S_OK;
    }

    void Destroy() override
    {
        m_Mesh.Destroy();
        m_bLoaded = false;
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
        return XMVectorSet(m_vAABBMin.x, m_vAABBMin.y, m_vAABBMin.z, 1.0f);
    }

    XMVECTOR GetAABBMax() const override
    {
        return XMVectorSet(m_vAABBMax.x, m_vAABBMax.y, m_vAABBMax.z, 1.0f);
    }

private:
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
    }

    HRESULT Create(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dDeviceContext) override
    {
        HRESULT hr = S_OK;

        Destroy();

        WCHAR strPath[MAX_PATH] = {};
        V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, MAX_PATH, m_szMeshPath.c_str()));

        std::wstring objPath = strPath;
        const std::wstring basePath = GetDirectory(objPath);

        std::vector<OBJVertex> vertices;
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
        vbDesc.ByteWidth = UINT(vertices.size() * sizeof(OBJVertex));
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

        V_RETURN(CreateFallbackTexture(pd3dDevice));

        m_Materials.clear();
        m_Materials.resize(materials.size());
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
        m_Subsets.clear();
        SAFE_RELEASE(m_pFallbackDiffuseSRV);
        m_bLoaded = false;
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

        const UINT stride = sizeof(OBJVertex);
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
        return XMVectorSet(m_vAABBMin.x, m_vAABBMin.y, m_vAABBMin.z, 1.0f);
    }

    XMVECTOR GetAABBMax() const override
    {
        return XMVectorSet(m_vAABBMax.x, m_vAABBMax.y, m_vAABBMax.z, 1.0f);
    }

private:
    struct OBJVertex
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

    HRESULT CreateFallbackTexture(ID3D11Device* pd3dDevice)
    {
        if (m_pFallbackDiffuseSRV)
        {
            return S_OK;
        }

        const UINT whitePixel = 0xffffffff;

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
        std::vector<OBJVertex>& vertices,
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

        OBJVertex vertex = {};
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
        std::vector<OBJVertex>& vertices,
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
    std::vector<MaterialResources>  m_Materials;
    D3DXVECTOR3                     m_vAABBMin;
    D3DXVECTOR3                     m_vAABBMax;
};
