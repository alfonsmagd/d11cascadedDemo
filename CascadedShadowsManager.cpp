//--------------------------------------------------------------------------------------
// File: CascadedShadowsManger.cpp
//
// This is where the shadows are calcaulted and rendered.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


#include "dxut.h"

#include "CascadedShadowsManager.h"
#include "DXUTcamera.h"
#include "SceneMesh.h"
#include "SDKMesh.h"
#include "xnacollision.h"
#include "SDKmisc.h"
#include "resource.h"

#include "VoxelizationMisc.h"

static const XMVECTORF32 g_vFLTMAX = { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };
static const XMVECTORF32 g_vFLTMIN = { -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX };
static const XMVECTORF32 g_vHalfVector = { 0.5f, 0.5f, 0.5f, 0.5f };
static const XMVECTORF32 g_vMultiplySetzwToZero = { 1.0f, 1.0f, 0.0f, 0.0f };
static const XMVECTORF32 g_vZero = { 0.0f, 0.0f, 0.0f, 0.0f };

struct VOXEL_CUBE_VERTEX
{
    D3DXVECTOR3 Position;
    D3DXVECTOR3 Normal;
};

static const VOXEL_CUBE_VERTEX g_VoxelCubeVertices[] =
{
    { D3DXVECTOR3(-1.0f, -1.0f,  1.0f), D3DXVECTOR3( 0.0f,  0.0f,  1.0f) },
    { D3DXVECTOR3( 1.0f, -1.0f,  1.0f), D3DXVECTOR3( 0.0f,  0.0f,  1.0f) },
    { D3DXVECTOR3( 1.0f,  1.0f,  1.0f), D3DXVECTOR3( 0.0f,  0.0f,  1.0f) },
    { D3DXVECTOR3(-1.0f, -1.0f,  1.0f), D3DXVECTOR3( 0.0f,  0.0f,  1.0f) },
    { D3DXVECTOR3( 1.0f,  1.0f,  1.0f), D3DXVECTOR3( 0.0f,  0.0f,  1.0f) },
    { D3DXVECTOR3(-1.0f,  1.0f,  1.0f), D3DXVECTOR3( 0.0f,  0.0f,  1.0f) },

    { D3DXVECTOR3( 1.0f, -1.0f, -1.0f), D3DXVECTOR3( 0.0f,  0.0f, -1.0f) },
    { D3DXVECTOR3(-1.0f, -1.0f, -1.0f), D3DXVECTOR3( 0.0f,  0.0f, -1.0f) },
    { D3DXVECTOR3(-1.0f,  1.0f, -1.0f), D3DXVECTOR3( 0.0f,  0.0f, -1.0f) },
    { D3DXVECTOR3( 1.0f, -1.0f, -1.0f), D3DXVECTOR3( 0.0f,  0.0f, -1.0f) },
    { D3DXVECTOR3(-1.0f,  1.0f, -1.0f), D3DXVECTOR3( 0.0f,  0.0f, -1.0f) },
    { D3DXVECTOR3( 1.0f,  1.0f, -1.0f), D3DXVECTOR3( 0.0f,  0.0f, -1.0f) },

    { D3DXVECTOR3(-1.0f, -1.0f, -1.0f), D3DXVECTOR3(-1.0f,  0.0f,  0.0f) },
    { D3DXVECTOR3(-1.0f, -1.0f,  1.0f), D3DXVECTOR3(-1.0f,  0.0f,  0.0f) },
    { D3DXVECTOR3(-1.0f,  1.0f,  1.0f), D3DXVECTOR3(-1.0f,  0.0f,  0.0f) },
    { D3DXVECTOR3(-1.0f, -1.0f, -1.0f), D3DXVECTOR3(-1.0f,  0.0f,  0.0f) },
    { D3DXVECTOR3(-1.0f,  1.0f,  1.0f), D3DXVECTOR3(-1.0f,  0.0f,  0.0f) },
    { D3DXVECTOR3(-1.0f,  1.0f, -1.0f), D3DXVECTOR3(-1.0f,  0.0f,  0.0f) },

    { D3DXVECTOR3( 1.0f, -1.0f,  1.0f), D3DXVECTOR3( 1.0f,  0.0f,  0.0f) },
    { D3DXVECTOR3( 1.0f, -1.0f, -1.0f), D3DXVECTOR3( 1.0f,  0.0f,  0.0f) },
    { D3DXVECTOR3( 1.0f,  1.0f, -1.0f), D3DXVECTOR3( 1.0f,  0.0f,  0.0f) },
    { D3DXVECTOR3( 1.0f, -1.0f,  1.0f), D3DXVECTOR3( 1.0f,  0.0f,  0.0f) },
    { D3DXVECTOR3( 1.0f,  1.0f, -1.0f), D3DXVECTOR3( 1.0f,  0.0f,  0.0f) },
    { D3DXVECTOR3( 1.0f,  1.0f,  1.0f), D3DXVECTOR3( 1.0f,  0.0f,  0.0f) },

    { D3DXVECTOR3(-1.0f,  1.0f,  1.0f), D3DXVECTOR3( 0.0f,  1.0f,  0.0f) },
    { D3DXVECTOR3( 1.0f,  1.0f,  1.0f), D3DXVECTOR3( 0.0f,  1.0f,  0.0f) },
    { D3DXVECTOR3( 1.0f,  1.0f, -1.0f), D3DXVECTOR3( 0.0f,  1.0f,  0.0f) },
    { D3DXVECTOR3(-1.0f,  1.0f,  1.0f), D3DXVECTOR3( 0.0f,  1.0f,  0.0f) },
    { D3DXVECTOR3( 1.0f,  1.0f, -1.0f), D3DXVECTOR3( 0.0f,  1.0f,  0.0f) },
    { D3DXVECTOR3(-1.0f,  1.0f, -1.0f), D3DXVECTOR3( 0.0f,  1.0f,  0.0f) },

    { D3DXVECTOR3(-1.0f, -1.0f, -1.0f), D3DXVECTOR3( 0.0f, -1.0f,  0.0f) },
    { D3DXVECTOR3( 1.0f, -1.0f, -1.0f), D3DXVECTOR3( 0.0f, -1.0f,  0.0f) },
    { D3DXVECTOR3( 1.0f, -1.0f,  1.0f), D3DXVECTOR3( 0.0f, -1.0f,  0.0f) },
    { D3DXVECTOR3(-1.0f, -1.0f, -1.0f), D3DXVECTOR3( 0.0f, -1.0f,  0.0f) },
    { D3DXVECTOR3( 1.0f, -1.0f,  1.0f), D3DXVECTOR3( 0.0f, -1.0f,  0.0f) },
    { D3DXVECTOR3(-1.0f, -1.0f,  1.0f), D3DXVECTOR3( 0.0f, -1.0f,  0.0f) },
};


//--------------------------------------------------------------------------------------
// Initialize the Manager.  The manager performs all the work of caculating the render 
// paramters of the shadow, creating the D3D resources, rendering the shadow, and rendering
// the actual scene.
//--------------------------------------------------------------------------------------
CascadedShadowsManager::CascadedShadowsManager()
    : m_pVertexLayoutMesh(NULL),
    m_pSamLinear(NULL),
    m_pSamShadowPCF(NULL),
    m_pSamShadowPoint(NULL),
    m_pCascadedShadowMapTexture(NULL),
    m_pCascadedShadowMapDSV(NULL),
    m_pCascadedShadowMapSRV(NULL),
    m_iBlurBetweenCascades(0),
    m_fBlurBetweenCascadesAmount(0.005f),
    m_RenderOneTileVP(m_RenderVP[0]),
    m_pcbGlobalConstantBuffer(NULL),
    m_pcbVoxelParams(NULL),
    m_prsShadow(NULL),
    m_prsShadowPancake(NULL),
    m_prsScene(NULL),
    m_prsVoxelization(NULL),
    m_prsDebug(NULL),
    m_iPCFBlurSize(3),
    m_fPCFOffset(0.002f),
    m_iDerivativeBasedOffset(0),
    m_pvsRenderOrthoShadowBlob(NULL)
{
    sprintf_s(m_cvsModel, "vs_5_0");
    sprintf_s(m_cpsModel, "ps_5_0");
    sprintf_s(m_cgsModel, "gs_5_0");

    for (INT index = 0; index < MAX_CASCADES; ++index)
    {
        m_RenderVP[index].Height = (FLOAT)m_CopyOfCascadeConfig.m_iBufferSize;
        m_RenderVP[index].Width = (FLOAT)m_CopyOfCascadeConfig.m_iBufferSize;
        m_RenderVP[index].MaxDepth = 1.0f;
        m_RenderVP[index].MinDepth = 0.0f;
        m_RenderVP[index].TopLeftX = 0;
        m_RenderVP[index].TopLeftY = 0;
        m_pvsRenderScene[index] = NULL;
        m_pvsRenderSceneBlob[index] = NULL;
        for (int x1 = 0; x1 < 2; ++x1)
        {
            for (int x2 = 0; x2 < 2; ++x2)
            {
                for (int x3 = 0; x3 < 2; ++x3)
                {
                    m_ppsRenderSceneAllShaders[index][x1][x2][x3] = NULL;
                    m_ppsRenderSceneAllShadersBlob[index][x1][x2][x3] = NULL;
                }
            }
        }

    }

};

HRESULT CascadedShadowsManager::EnsureRenderSceneVertexShader(ID3D11Device* pd3dDevice, INT cascadeIndex)
{
    HRESULT hr = S_OK;
    cascadeIndex = max(0, min(cascadeIndex, MAX_CASCADES - 1));

    if (m_pvsRenderSceneBlob[cascadeIndex] == NULL)
    {
        D3D_SHADER_MACRO defines[] =
        {
            "CASCADE_COUNT_FLAG", "1",
            "USE_DERIVATIVES_FOR_DEPTH_OFFSET_FLAG", "0",
            "BLEND_BETWEEN_CASCADE_LAYERS_FLAG", "0",
            "SELECT_CASCADE_BY_INTERVAL_FLAG", "0",
            NULL, NULL
        };

        char cCascadeDefinition[32];
        sprintf_s(cCascadeDefinition, "%d", cascadeIndex + 1);
        defines[0].Definition = cCascadeDefinition;

        V_RETURN(CompileShaderFromFile(L"RenderCascadeScene.hlsl", defines, "VSMain",
            m_cvsModel, &m_pvsRenderSceneBlob[cascadeIndex]));
    }

    if (m_pvsRenderScene[cascadeIndex] == NULL)
    {
        V_RETURN(pd3dDevice->CreateVertexShader(
            m_pvsRenderSceneBlob[cascadeIndex]->GetBufferPointer(),
            m_pvsRenderSceneBlob[cascadeIndex]->GetBufferSize(),
            NULL,
            &m_pvsRenderScene[cascadeIndex]));
        DXUT_SetDebugName(m_pvsRenderScene[cascadeIndex], "RenderCascadeScene");
    }

    return hr;
}

HRESULT CascadedShadowsManager::EnsureRenderScenePixelShader(ID3D11Device* pd3dDevice,
    INT cascadeIndex,
    INT derivativeIndex,
    INT blendIndex,
    INT intervalIndex)
{
    HRESULT hr = S_OK;

    cascadeIndex = max(0, min(cascadeIndex, MAX_CASCADES - 1));
    derivativeIndex = max(0, min(derivativeIndex, 1));
    blendIndex = max(0, min(blendIndex, 1));
    intervalIndex = max(0, min(intervalIndex, 1));

    if (m_ppsRenderSceneAllShadersBlob[cascadeIndex][derivativeIndex][blendIndex][intervalIndex] == NULL)
    {
        D3D_SHADER_MACRO defines[] =
        {
            "CASCADE_COUNT_FLAG", "1",
            "USE_DERIVATIVES_FOR_DEPTH_OFFSET_FLAG", "0",
            "BLEND_BETWEEN_CASCADE_LAYERS_FLAG", "0",
            "SELECT_CASCADE_BY_INTERVAL_FLAG", "0",
            NULL, NULL
        };

        char cCascadeDefinition[32];
        char cDerivativeDefinition[32];
        char cBlendDefinition[32];
        char cIntervalDefinition[32];

        sprintf_s(cCascadeDefinition, "%d", cascadeIndex + 1);
        sprintf_s(cDerivativeDefinition, "%d", derivativeIndex);
        sprintf_s(cBlendDefinition, "%d", blendIndex);
        sprintf_s(cIntervalDefinition, "%d", intervalIndex);

        defines[0].Definition = cCascadeDefinition;
        defines[1].Definition = cDerivativeDefinition;
        defines[2].Definition = cBlendDefinition;
        defines[3].Definition = cIntervalDefinition;

        V_RETURN(CompileShaderFromFile(L"RenderCascadeScene.hlsl", defines, "PSMain",
            m_cpsModel, &m_ppsRenderSceneAllShadersBlob[cascadeIndex][derivativeIndex][blendIndex][intervalIndex]));
    }

    if (m_ppsRenderSceneAllShaders[cascadeIndex][derivativeIndex][blendIndex][intervalIndex] == NULL)
    {
        V_RETURN(pd3dDevice->CreatePixelShader(
            m_ppsRenderSceneAllShadersBlob[cascadeIndex][derivativeIndex][blendIndex][intervalIndex]->GetBufferPointer(),
            m_ppsRenderSceneAllShadersBlob[cascadeIndex][derivativeIndex][blendIndex][intervalIndex]->GetBufferSize(),
            NULL,
            &m_ppsRenderSceneAllShaders[cascadeIndex][derivativeIndex][blendIndex][intervalIndex]));
        DXUT_SetDebugName(m_ppsRenderSceneAllShaders[cascadeIndex][derivativeIndex][blendIndex][intervalIndex], "RenderCascadeScene");
    }

    return hr;
}


//--------------------------------------------------------------------------------------
// Call into deallocator.  
//--------------------------------------------------------------------------------------
CascadedShadowsManager::~CascadedShadowsManager()
{
    DestroyAndDeallocateShadowResources();
    SAFE_RELEASE(m_pvsDebugBlob);
    SAFE_RELEASE(m_ppsDebugBlob);
    SAFE_RELEASE(m_pvsVisualizeVoxelizationBlob);
    SAFE_RELEASE(m_ppsVisualizeVoxelizationBlob);
    SAFE_RELEASE(m_pvsRenderOrthoShadowBlob);

    for (int index = 0; index < MAX_CASCADES; ++index)
    {
        SAFE_RELEASE(m_pvsRenderSceneBlob[index]);
        for (int x1 = 0; x1 < 2; ++x1)
        {
            for (int x2 = 0; x2 < 2; ++x2)
            {
                for (int x3 = 0; x3 < 2; ++x3)
                {
                    SAFE_RELEASE(m_ppsRenderSceneAllShadersBlob[index][x1][x2][x3]);
                }
            }
        }
    }
};


//--------------------------------------------------------------------------------------
// Create the resources, compile shaders, etc.
// The rest of the resources are create in the allocator when the scene changes.
//--------------------------------------------------------------------------------------
HRESULT CascadedShadowsManager::Init(ID3D11Device* pd3dDevice,
    ID3D11DeviceContext* pd3dImmediateContext,
    ISceneMesh* pMesh,
    CFirstPersonCamera* pViewerCamera,
    CFirstPersonCamera* pLightCamera,
    CascadeConfig* pCascadeConfig
)
{
    HRESULT hr = S_OK;

    m_CopyOfCascadeConfig = *pCascadeConfig;
    // Initialize m_iBufferSize to 0 to trigger a reallocate on the first frame.   
    m_CopyOfCascadeConfig.m_iBufferSize = 0;
    // Save a pointer to cascade config.  Each frame we check our copy against the pointer.
    m_pCascadeConfig = pCascadeConfig;

    m_vSceneAABBMin = pMesh->GetAABBMin();
    m_vSceneAABBMax = pMesh->GetAABBMax();

    m_vStaticVoxelAABBMin = m_vSceneAABBMin;
    m_vStaticVoxelAABBMax = m_vSceneAABBMax;
    m_vDynamicVoxelAABBMin = m_vSceneAABBMin;
    m_vDynamicVoxelAABBMax = m_vSceneAABBMax;
    m_bStaticVoxelizationDirty = true;

    m_pViewerCamera = pViewerCamera;
    m_pLightCamera = pLightCamera;


    if (m_pvsRenderOrthoShadowBlob == NULL)
    {
        V_RETURN(CompileShaderFromFile(
            L"RenderCascadeShadow.hlsl", NULL, "VSMain", m_cvsModel, &m_pvsRenderOrthoShadowBlob));
    }

    if (m_pvsDebug == NULL && m_ppsDebug == NULL)
    {
        V_RETURN(CompileShaderFromFile(L"Debug.hlsl", NULL, "VSMain", m_cvsModel, &m_pvsDebugBlob));
        V_RETURN(CompileShaderFromFile(L"Debug.hlsl", NULL, "PSMain", m_cpsModel, &m_ppsDebugBlob));

    }
    if (m_pvsVisualizeVoxelization == NULL && m_ppsVisualizeVoxelization == NULL)
    {
        V_RETURN(CompileShaderFromFile(L"VisualizeVoxelization.hlsl", NULL, "VSMain", m_cvsModel, &m_pvsVisualizeVoxelizationBlob));
        V_RETURN(CompileShaderFromFile(L"VisualizeVoxelization.hlsl", NULL, "PSMain", m_cpsModel, &m_ppsVisualizeVoxelizationBlob));
    }
    if (m_pvsVoxelization == NULL && m_ppsVoxelization == NULL && m_pgsVoxelization == NULL)
    {
        V_RETURN(CompileShaderFromFile(L"Voxelization.hlsl", NULL, "CreateVoxelArrayVS", m_cvsModel, &m_pvsVoxelizationBlob));
        V_RETURN(CompileShaderFromFile(L"Voxelization.hlsl", NULL, "CreateVoxelArrayPS", m_cpsModel, &m_ppsVoxelizationBlob));
        V_RETURN(CompileShaderFromFile(L"Voxelization.hlsl", NULL, "CreateVoxelArrayGS", m_cgsModel, &m_pgsVoxelizationBlob));
    }
    V_RETURN(pd3dDevice->CreateVertexShader(
        m_pvsDebugBlob->GetBufferPointer(), m_pvsDebugBlob->GetBufferSize(), NULL, &m_pvsDebug));
    DXUT_SetDebugName(m_pvsDebug, "Debug_vs");

    V_RETURN(pd3dDevice->CreatePixelShader(
        m_ppsDebugBlob->GetBufferPointer(), m_ppsDebugBlob->GetBufferSize(), NULL, &m_ppsDebug));
    DXUT_SetDebugName(m_ppsDebug, "Debug_ps");

    V_RETURN(pd3dDevice->CreateVertexShader(
        m_pvsVisualizeVoxelizationBlob->GetBufferPointer(), m_pvsVisualizeVoxelizationBlob->GetBufferSize(), NULL, &m_pvsVisualizeVoxelization));
    DXUT_SetDebugName(m_pvsVisualizeVoxelization, "VisualizeVoxelization_vs");

    V_RETURN(pd3dDevice->CreatePixelShader(
        m_ppsVisualizeVoxelizationBlob->GetBufferPointer(), m_ppsVisualizeVoxelizationBlob->GetBufferSize(), NULL, &m_ppsVisualizeVoxelization));
    DXUT_SetDebugName(m_ppsVisualizeVoxelization, "VisualizeVoxelization_ps");

    V_RETURN(pd3dDevice->CreateVertexShader(
        m_pvsVoxelizationBlob->GetBufferPointer(), m_pvsVoxelizationBlob->GetBufferSize(), NULL, &m_pvsVoxelization));
    DXUT_SetDebugName(m_pvsVoxelization, "Voxelization_vs");

    V_RETURN(pd3dDevice->CreatePixelShader(
        m_ppsVoxelizationBlob->GetBufferPointer(), m_ppsVoxelizationBlob->GetBufferSize(), NULL, &m_ppsVoxelization));
    DXUT_SetDebugName(m_ppsVoxelization, "Voxelization_ps");

    V_RETURN(pd3dDevice->CreateGeometryShader(
        m_pgsVoxelizationBlob->GetBufferPointer(), m_pgsVoxelizationBlob->GetBufferSize(), NULL, &m_pgsVoxelization));
    DXUT_SetDebugName(m_pgsVoxelization, "Voxelization_gs");


    V_RETURN(pd3dDevice->CreateVertexShader(
        m_pvsRenderOrthoShadowBlob->GetBufferPointer(), m_pvsRenderOrthoShadowBlob->GetBufferSize(),
        NULL, &m_pvsRenderOrthoShadow));
    DXUT_SetDebugName(m_pvsRenderOrthoShadow, "RenderCascadeShadow");

    // Delay the heavy RenderCascadeScene shader matrix until first use.
    V_RETURN(EnsureRenderSceneVertexShader(pd3dDevice, 0));

    const INT initialCascadeIndex = max(0, min((INT)m_pCascadeConfig->m_nCascadeLevels - 1, MAX_CASCADES - 1));
    V_RETURN(EnsureRenderSceneVertexShader(pd3dDevice, initialCascadeIndex));
    V_RETURN(EnsureRenderScenePixelShader(pd3dDevice,
        initialCascadeIndex,
        m_iDerivativeBasedOffset,
        m_iBlurBetweenCascades,
        m_eSelectedCascadeSelection));

    const D3D11_INPUT_ELEMENT_DESC layout_mesh[] =
    {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    V_RETURN(pd3dDevice->CreateInputLayout(
        layout_mesh, ARRAYSIZE(layout_mesh),
        m_pvsRenderSceneBlob[0]->GetBufferPointer(),
        m_pvsRenderSceneBlob[0]->GetBufferSize(),
        &m_pVertexLayoutMesh));
    DXUT_SetDebugName(m_pVertexLayoutMesh, "CascadedShadowsManager");

    const D3D11_INPUT_ELEMENT_DESC layout_voxel_visualize[] =
    {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    V_RETURN(pd3dDevice->CreateInputLayout(
        layout_voxel_visualize, ARRAYSIZE(layout_voxel_visualize),
        m_pvsVisualizeVoxelizationBlob->GetBufferPointer(),
        m_pvsVisualizeVoxelizationBlob->GetBufferSize(),
        &m_pVertexLayoutVoxelVisualize));
    DXUT_SetDebugName(m_pVertexLayoutVoxelVisualize, "VoxelVisualizeLayout");

    D3D11_BUFFER_DESC voxelCubeVBDesc = {};
    voxelCubeVBDesc.ByteWidth = sizeof(g_VoxelCubeVertices);
    voxelCubeVBDesc.Usage = D3D11_USAGE_IMMUTABLE;
    voxelCubeVBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA voxelCubeVBData = {};
    voxelCubeVBData.pSysMem = g_VoxelCubeVertices;
    V_RETURN(pd3dDevice->CreateBuffer(&voxelCubeVBDesc, &voxelCubeVBData, &m_pVoxelCubeVertexBuffer));
    DXUT_SetDebugName(m_pVoxelCubeVertexBuffer, "VoxelCubeVB");

    const UINT maxVoxelInstances = GRID_SIZE_X * GRID_SIZE_Y * GRID_SIZE_Z;
    D3D11_BUFFER_DESC voxelInstanceDesc = {};
    voxelInstanceDesc.ByteWidth = maxVoxelInstances * sizeof(UINT);
    voxelInstanceDesc.Usage = D3D11_USAGE_DEFAULT;
    voxelInstanceDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    voxelInstanceDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    voxelInstanceDesc.StructureByteStride = sizeof(UINT);
    V_RETURN(pd3dDevice->CreateBuffer(&voxelInstanceDesc, NULL, &m_pVoxelInstanceBuffer));
    DXUT_SetDebugName(m_pVoxelInstanceBuffer, "VoxelInstanceBuffer");

    D3D11_UNORDERED_ACCESS_VIEW_DESC voxelInstanceUAVDesc = {};
    voxelInstanceUAVDesc.Format = DXGI_FORMAT_UNKNOWN;
    voxelInstanceUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    voxelInstanceUAVDesc.Buffer.FirstElement = 0;
    voxelInstanceUAVDesc.Buffer.NumElements = maxVoxelInstances;
    voxelInstanceUAVDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;
    V_RETURN(pd3dDevice->CreateUnorderedAccessView(m_pVoxelInstanceBuffer, &voxelInstanceUAVDesc, &m_pVoxelInstanceUAV));
    DXUT_SetDebugName(m_pVoxelInstanceUAV, "VoxelInstanceUAV");

    D3D11_SHADER_RESOURCE_VIEW_DESC voxelInstanceSRVDesc = {};
    voxelInstanceSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    voxelInstanceSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    voxelInstanceSRVDesc.Buffer.FirstElement = 0;
    voxelInstanceSRVDesc.Buffer.NumElements = maxVoxelInstances;
    V_RETURN(pd3dDevice->CreateShaderResourceView(m_pVoxelInstanceBuffer, &voxelInstanceSRVDesc, &m_pVoxelInstanceSRV));
    DXUT_SetDebugName(m_pVoxelInstanceSRV, "VoxelInstanceSRV");

    const UINT voxelDrawArgs[4] = { ARRAYSIZE(g_VoxelCubeVertices), 0u, 0u, 0u };
    D3D11_BUFFER_DESC voxelDrawArgsDesc = {};
    voxelDrawArgsDesc.ByteWidth = sizeof(voxelDrawArgs);
    voxelDrawArgsDesc.Usage = D3D11_USAGE_DEFAULT;
    voxelDrawArgsDesc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
    D3D11_SUBRESOURCE_DATA voxelDrawArgsData = {};
    voxelDrawArgsData.pSysMem = voxelDrawArgs;
    V_RETURN(pd3dDevice->CreateBuffer(&voxelDrawArgsDesc, &voxelDrawArgsData, &m_pVoxelDrawArgsBuffer));
    DXUT_SetDebugName(m_pVoxelDrawArgsBuffer, "VoxelDrawArgs");

    D3D11_RASTERIZER_DESC drd =
    {
        D3D11_FILL_SOLID,//D3D11_FILL_MODE FillMode;
        D3D11_CULL_NONE,//D3D11_CULL_MODE CullMode;
        FALSE,//BOOL FrontCounterClockwise;
        0,//INT DepthBias;
        0.0,//FLOAT DepthBiasClamp;
        0.0,//FLOAT SlopeScaledDepthBias;
        TRUE,//BOOL DepthClipEnable;
        FALSE,//BOOL ScissorEnable;
        TRUE,//BOOL MultisampleEnable;
        FALSE//BOOL AntialiasedLineEnable;   
    };

    pd3dDevice->CreateRasterizerState(&drd, &m_prsScene);
    DXUT_SetDebugName(m_prsScene, "CSM Scene");

    drd.DepthClipEnable = FALSE;
    drd.MultisampleEnable = FALSE;
    pd3dDevice->CreateRasterizerState(&drd, &m_prsVoxelization);
    DXUT_SetDebugName(m_prsVoxelization, "CSM Voxelization");

    drd.DepthClipEnable = FALSE;
    drd.MultisampleEnable = TRUE;
    pd3dDevice->CreateRasterizerState(&drd, &m_prsDebug);
    DXUT_SetDebugName(m_prsDebug, "CSM Debug_Rasterizer");

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    V_RETURN(pd3dDevice->CreateBlendState(&blendDesc, &m_pbsVoxelVisualize));
    DXUT_SetDebugName(m_pbsVoxelVisualize, "VoxelVisualize_Blend");


    // Setting the slope scale depth biase greatly decreases surface acne and incorrect self shadowing.
    drd.DepthClipEnable = TRUE;
    drd.SlopeScaledDepthBias = 1.0;
    pd3dDevice->CreateRasterizerState(&drd, &m_prsShadow);
    DXUT_SetDebugName(m_prsShadow, "CSM Shadow");
    drd.DepthClipEnable = false;
    pd3dDevice->CreateRasterizerState(&drd, &m_prsShadowPancake);
    DXUT_SetDebugName(m_prsShadowPancake, "CSM Pancake");

    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Desc.MiscFlags = 0;

    Desc.ByteWidth = sizeof(CB_ALL_SHADOW_DATA);
    V_RETURN(pd3dDevice->CreateBuffer(&Desc, NULL, &m_pcbGlobalConstantBuffer));
    DXUT_SetDebugName(m_pcbGlobalConstantBuffer, "CB_ALL_SHADOW_DATACB_ALL_SHADOW_DATA");


    D3D11_BUFFER_DESC DescVoxel;
    DescVoxel.Usage = D3D11_USAGE_DYNAMIC;
    DescVoxel.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    DescVoxel.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    DescVoxel.MiscFlags = 0;
    DescVoxel.ByteWidth = sizeof(CB_VOXELIZATION_PARAMS);   

    V_RETURN(pd3dDevice->CreateBuffer(&DescVoxel, NULL, &m_pcbVoxelParams));
    DXUT_SetDebugName(m_pcbVoxelParams, "CB_VOXELIZATION_PARAMS");

    D3D11_BUFFER_DESC DescVisualizeVoxel = {};
    DescVisualizeVoxel.Usage = D3D11_USAGE_DYNAMIC;
    DescVisualizeVoxel.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    DescVisualizeVoxel.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    DescVisualizeVoxel.MiscFlags = 0;
    DescVisualizeVoxel.ByteWidth = sizeof(CB_VISUALIZE_VOXELS);

    V_RETURN(pd3dDevice->CreateBuffer(&DescVisualizeVoxel, NULL, &m_pcbVisualizeVoxels));
    DXUT_SetDebugName(m_pcbVisualizeVoxels, "CB_VISUALIZE_VOXELS");

    return hr;
}


//--------------------------------------------------------------------------------------
// These resources must be reallocated based on GUI control settings change.
//--------------------------------------------------------------------------------------
HRESULT CascadedShadowsManager::DestroyAndDeallocateShadowResources()
{

    SAFE_RELEASE(m_pVertexLayoutMesh);
    SAFE_RELEASE(m_pSamLinear);
    SAFE_RELEASE(m_pSamShadowPoint);
    SAFE_RELEASE(m_pSamShadowPCF);

    SAFE_RELEASE(m_pCascadedShadowMapTexture);
    SAFE_RELEASE(m_pCascadedShadowMapDSV);
    SAFE_RELEASE(m_pCascadedShadowMapSRV);

    SAFE_RELEASE(m_pVoxelAlbedoTex);
    SAFE_RELEASE(m_pVoxelAlbedoUAV);
    SAFE_RELEASE(m_pVoxelAlbedoSRV);

    SAFE_RELEASE(m_pVoxelMaskTex);
    SAFE_RELEASE(m_pVoxelMaskUAV);
    SAFE_RELEASE(m_pVoxelMaskSRV);

    SAFE_RELEASE(m_pVoxelInstanceBuffer);
    SAFE_RELEASE(m_pVoxelInstanceSRV);
    SAFE_RELEASE(m_pVoxelInstanceUAV);
    SAFE_RELEASE(m_pVoxelDrawArgsBuffer);
    SAFE_RELEASE(m_pVoxelCubeVertexBuffer);

    SAFE_RELEASE(m_pDynamicVoxelAlbedoTex);
    SAFE_RELEASE(m_pDynamicVoxelAlbedoUAV);
    SAFE_RELEASE(m_pDynamicVoxelAlbedoSRV);

    SAFE_RELEASE(m_pDynamicVoxelMaskTex);
    SAFE_RELEASE(m_pDynamicVoxelMaskUAV);
    SAFE_RELEASE(m_pDynamicVoxelMaskSRV);

    SAFE_RELEASE(m_pcbGlobalConstantBuffer);
    SAFE_RELEASE(m_pcbVoxelParams);
    SAFE_RELEASE(m_pcbVisualizeVoxels);

    SAFE_RELEASE(m_prsShadow);
    SAFE_RELEASE(m_prsDebug);
    SAFE_RELEASE(m_prsShadowPancake);
    SAFE_RELEASE(m_prsScene);
    SAFE_RELEASE(m_prsVoxelization);
    SAFE_RELEASE(m_pbsVoxelVisualize);

    SAFE_RELEASE(m_pvsRenderOrthoShadow);
    SAFE_RELEASE(m_pVertexLayoutVoxelVisualize);

    SAFE_RELEASE(m_pvsDebug);
    SAFE_RELEASE(m_ppsDebug);
    SAFE_RELEASE(m_pvsVisualizeVoxelization);
    SAFE_RELEASE(m_ppsVisualizeVoxelization);

    SAFE_RELEASE(m_pvsVoxelization);
    SAFE_RELEASE(m_ppsVoxelization);
    SAFE_RELEASE(m_pgsVoxelization);

    for (INT iCascadeIndex = 0; iCascadeIndex < MAX_CASCADES; ++iCascadeIndex)
    {
        SAFE_RELEASE(m_pvsRenderScene[iCascadeIndex]);
        for (INT iDerivativeIndex = 0; iDerivativeIndex < 2; ++iDerivativeIndex)
        {
            for (INT iBlendIndex = 0; iBlendIndex < 2; ++iBlendIndex)
            {
                for (INT iIntervalIndex = 0; iIntervalIndex < 2; ++iIntervalIndex)
                {
                    SAFE_RELEASE(m_ppsRenderSceneAllShaders[iCascadeIndex][iDerivativeIndex][iBlendIndex][iIntervalIndex]);
                }
            }
        }
    }
    return S_OK;
}


//--------------------------------------------------------------------------------------
// These settings must be recreated based on GUI control.
//--------------------------------------------------------------------------------------
HRESULT CascadedShadowsManager::ReleaseAndAllocateNewShadowResources(ID3D11Device* pd3dDevice)
{
    HRESULT hr = S_OK;
    // If any of these 3 paramaters was changed, we must reallocate the D3D resources.
    if (m_CopyOfCascadeConfig.m_nCascadeLevels != m_pCascadeConfig->m_nCascadeLevels
        || m_CopyOfCascadeConfig.m_ShadowBufferFormat != m_pCascadeConfig->m_ShadowBufferFormat
        || m_CopyOfCascadeConfig.m_iBufferSize != m_pCascadeConfig->m_iBufferSize)
    {

        m_CopyOfCascadeConfig = *m_pCascadeConfig;

        SAFE_RELEASE(m_pSamLinear);
        SAFE_RELEASE(m_pSamShadowPCF);
        SAFE_RELEASE(m_pSamShadowPoint);

        D3D11_SAMPLER_DESC SamDesc;
        SamDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        SamDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        SamDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        SamDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        SamDesc.MipLODBias = 0.0f;
        SamDesc.MaxAnisotropy = 1;
        SamDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        SamDesc.BorderColor[0] = SamDesc.BorderColor[1] = SamDesc.BorderColor[2] = SamDesc.BorderColor[3] = 0;
        SamDesc.MinLOD = 0;
        SamDesc.MaxLOD = D3D11_FLOAT32_MAX;
        V_RETURN(pd3dDevice->CreateSamplerState(&SamDesc, &m_pSamLinear));
        DXUT_SetDebugName(m_pSamLinear, "CSM Linear");

        D3D11_SAMPLER_DESC SamDescShad =
        {
            D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,// D3D11_FILTER Filter;
            D3D11_TEXTURE_ADDRESS_BORDER, //D3D11_TEXTURE_ADDRESS_MODE AddressU;
            D3D11_TEXTURE_ADDRESS_BORDER, //D3D11_TEXTURE_ADDRESS_MODE AddressV;
            D3D11_TEXTURE_ADDRESS_BORDER, //D3D11_TEXTURE_ADDRESS_MODE AddressW;
            0,//FLOAT MipLODBias;
            0,//UINT MaxAnisotropy;
            D3D11_COMPARISON_LESS , //D3D11_COMPARISON_FUNC ComparisonFunc;
            0.0,0.0,0.0,0.0,//FLOAT BorderColor[ 4 ];
            0,//FLOAT MinLOD;
            0//FLOAT MaxLOD;   
        };

        V_RETURN(pd3dDevice->CreateSamplerState(&SamDescShad, &m_pSamShadowPCF));
        DXUT_SetDebugName(m_pSamShadowPCF, "CSM Shadow PCF");

        SamDescShad.MaxAnisotropy = 15;
        SamDescShad.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        SamDescShad.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        SamDescShad.Filter = D3D11_FILTER_ANISOTROPIC;
        SamDescShad.ComparisonFunc = D3D11_COMPARISON_NEVER;
        V_RETURN(pd3dDevice->CreateSamplerState(&SamDescShad, &m_pSamShadowPoint));
        DXUT_SetDebugName(m_pSamShadowPoint, "CSM Shadow Point");

        for (INT index = 0; index < m_CopyOfCascadeConfig.m_nCascadeLevels; ++index)
        {
            m_RenderVP[index].Height = (FLOAT)m_CopyOfCascadeConfig.m_iBufferSize;
            m_RenderVP[index].Width = (FLOAT)m_CopyOfCascadeConfig.m_iBufferSize;
            m_RenderVP[index].MaxDepth = 1.0f;
            m_RenderVP[index].MinDepth = 0.0f;
            m_RenderVP[index].TopLeftX = (FLOAT)(m_CopyOfCascadeConfig.m_iBufferSize * index);
            m_RenderVP[index].TopLeftY = 0;
        }

        m_RenderOneTileVP.Height = (FLOAT)m_CopyOfCascadeConfig.m_iBufferSize;
        m_RenderOneTileVP.Width = (FLOAT)m_CopyOfCascadeConfig.m_iBufferSize;
        m_RenderOneTileVP.MaxDepth = 1.0f;
        m_RenderOneTileVP.MinDepth = 0.0f;
        m_RenderOneTileVP.TopLeftX = 0.0f;
        m_RenderOneTileVP.TopLeftY = 0.0f;

        SAFE_RELEASE(m_pCascadedShadowMapSRV);
        SAFE_RELEASE(m_pCascadedShadowMapTexture);
        SAFE_RELEASE(m_pCascadedShadowMapDSV);

        SAFE_RELEASE(m_pVoxelAlbedoTex);
        SAFE_RELEASE(m_pVoxelAlbedoUAV);
        SAFE_RELEASE(m_pVoxelAlbedoSRV);

        SAFE_RELEASE(m_pVoxelMaskTex);
        SAFE_RELEASE(m_pVoxelMaskUAV);
        SAFE_RELEASE(m_pVoxelMaskSRV);

        SAFE_RELEASE(m_pDynamicVoxelAlbedoTex);
        SAFE_RELEASE(m_pDynamicVoxelAlbedoUAV);
        SAFE_RELEASE(m_pDynamicVoxelAlbedoSRV);

        SAFE_RELEASE(m_pDynamicVoxelMaskTex);
        SAFE_RELEASE(m_pDynamicVoxelMaskUAV);
        SAFE_RELEASE(m_pDynamicVoxelMaskSRV);

        DXGI_FORMAT texturefmt = DXGI_FORMAT_R32_TYPELESS;
        DXGI_FORMAT SRVfmt = DXGI_FORMAT_R32_FLOAT;
        DXGI_FORMAT DSVfmt = DXGI_FORMAT_D32_FLOAT;

        switch (m_CopyOfCascadeConfig.m_ShadowBufferFormat)
        {
        case CASCADE_DXGI_FORMAT_R32_TYPELESS:
            texturefmt = DXGI_FORMAT_R32_TYPELESS;
            SRVfmt = DXGI_FORMAT_R32_FLOAT;
            DSVfmt = DXGI_FORMAT_D32_FLOAT;
            break;
        case CASCADE_DXGI_FORMAT_R24G8_TYPELESS:
            texturefmt = DXGI_FORMAT_R24G8_TYPELESS;
            SRVfmt = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
            DSVfmt = DXGI_FORMAT_D24_UNORM_S8_UINT;
            break;
        case CASCADE_DXGI_FORMAT_R16_TYPELESS:
            texturefmt = DXGI_FORMAT_R16_TYPELESS;
            SRVfmt = DXGI_FORMAT_R16_UNORM;
            DSVfmt = DXGI_FORMAT_D16_UNORM;
            break;
        case CASCADE_DXGI_FORMAT_R8_TYPELESS:
            texturefmt = DXGI_FORMAT_R8_TYPELESS;
            SRVfmt = DXGI_FORMAT_R8_UNORM;
            DSVfmt = DXGI_FORMAT_R8_UNORM;
            break;
        }

        D3D11_TEXTURE2D_DESC dtd =
        {
            m_CopyOfCascadeConfig.m_iBufferSize * m_CopyOfCascadeConfig.m_nCascadeLevels,//UINT Width;
            m_CopyOfCascadeConfig.m_iBufferSize,//UINT Height;
            1,//UINT MipLevels;
            1,//UINT ArraySize;
            texturefmt,//DXGI_FORMAT Format;
            1,//DXGI_SAMPLE_DESC SampleDesc;
            0,
            D3D11_USAGE_DEFAULT,//D3D11_USAGE Usage;
            D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,//UINT BindFlags;
            0,//UINT CPUAccessFlags;
            0//UINT MiscFlags;    
        };

        V_RETURN(pd3dDevice->CreateTexture2D(&dtd, NULL, &m_pCascadedShadowMapTexture));
        DXUT_SetDebugName(m_pCascadedShadowMapTexture, "CSM ShadowMap");


        // Create Texture 3D for static voxelization
        D3D11_TEXTURE3D_DESC texDesc = {};
        texDesc.Width = GRID_SIZE_X;
        texDesc.Height = GRID_SIZE_Y;
        texDesc.Depth = GRID_SIZE_Z;
        texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
        texDesc.CPUAccessFlags = 0;
        texDesc.MiscFlags = 0;

         hr = pd3dDevice->CreateTexture3D(&texDesc, nullptr, &m_pVoxelAlbedoTex);

        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = texDesc.Format;
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
        uavDesc.Texture3D.MipSlice = 0;
        uavDesc.Texture3D.FirstWSlice = 0;
        uavDesc.Texture3D.WSize = texDesc.Depth;

        hr = pd3dDevice->CreateUnorderedAccessView(m_pVoxelAlbedoTex, &uavDesc, &m_pVoxelAlbedoUAV);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = texDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
        srvDesc.Texture3D.MostDetailedMip = 0;
        srvDesc.Texture3D.MipLevels = 1;

        hr = pd3dDevice->CreateShaderResourceView(m_pVoxelAlbedoTex, &srvDesc, &m_pVoxelAlbedoSRV);

        D3D11_TEXTURE3D_DESC maskDesc = {};
        maskDesc.Width = GRID_SIZE_X;
        maskDesc.Height = GRID_SIZE_Y;
        maskDesc.Depth = GRID_SIZE_Z;
        maskDesc.MipLevels = 1;
        maskDesc.Format = DXGI_FORMAT_R32_UINT;
        maskDesc.Usage = D3D11_USAGE_DEFAULT;
        maskDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
        maskDesc.CPUAccessFlags = 0;
        maskDesc.MiscFlags = 0;

        hr = pd3dDevice->CreateTexture3D(&maskDesc, nullptr, &m_pVoxelMaskTex);

        D3D11_UNORDERED_ACCESS_VIEW_DESC maskUavDesc = {};
        maskUavDesc.Format = maskDesc.Format;
        maskUavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
        maskUavDesc.Texture3D.MipSlice = 0;
        maskUavDesc.Texture3D.FirstWSlice = 0;
        maskUavDesc.Texture3D.WSize = maskDesc.Depth;

        hr = pd3dDevice->CreateUnorderedAccessView(m_pVoxelMaskTex, &maskUavDesc, &m_pVoxelMaskUAV);

        D3D11_SHADER_RESOURCE_VIEW_DESC maskSrvDesc = {};
        maskSrvDesc.Format = maskDesc.Format;
        maskSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
        maskSrvDesc.Texture3D.MostDetailedMip = 0;
        maskSrvDesc.Texture3D.MipLevels = 1;

        hr = pd3dDevice->CreateShaderResourceView(m_pVoxelMaskTex, &maskSrvDesc, &m_pVoxelMaskSRV);

        // Dynamic voxel resources stay unallocated until that path is enabled again.
        // Keeping them null avoids paying startup cost and large memory allocations.

        m_bStaticVoxelizationDirty = true;


        D3D11_DEPTH_STENCIL_VIEW_DESC  dsvd =
        {
            DSVfmt,
            D3D11_DSV_DIMENSION_TEXTURE2D,
            0
        };
        V_RETURN(pd3dDevice->CreateDepthStencilView(m_pCascadedShadowMapTexture, &dsvd, &m_pCascadedShadowMapDSV));
        DXUT_SetDebugName(m_pCascadedShadowMapDSV, "CSM ShadowMap DSV");

        D3D11_SHADER_RESOURCE_VIEW_DESC dsrvd =
        {
            SRVfmt,
            D3D11_SRV_DIMENSION_TEXTURE2D,
            0,
            0
        };
        dsrvd.Texture2D.MipLevels = 1;

        V_RETURN(pd3dDevice->CreateShaderResourceView(m_pCascadedShadowMapTexture, &dsrvd, &m_pCascadedShadowMapSRV));
        DXUT_SetDebugName(m_pCascadedShadowMapSRV, "CSM ShadowMap SRV");
    }
    return hr;

}

//--------------------------------------------------------------------------------------
// This function takes the camera's projection matrix and returns the 8
// points that make up a view frustum.
// The frustum is scaled to fit within the Begin and End interval paramaters.
//--------------------------------------------------------------------------------------
void CascadedShadowsManager::CreateFrustumPointsFromCascadeInterval(float fCascadeIntervalBegin,
    FLOAT fCascadeIntervalEnd,
    XMMATRIX& vProjection,
    XMVECTOR* pvCornerPointsWorld)
{

    XNA::Frustum vViewFrust;
    ComputeFrustumFromProjection(&vViewFrust, &vProjection);
    vViewFrust.Near = fCascadeIntervalBegin;
    vViewFrust.Far = fCascadeIntervalEnd;

    static const XMVECTORU32 vGrabY = { 0x00000000,0xFFFFFFFF,0x00000000,0x00000000 };
    static const XMVECTORU32 vGrabX = { 0xFFFFFFFF,0x00000000,0x00000000,0x00000000 };

    XMVECTORF32 vRightTop = { vViewFrust.RightSlope,vViewFrust.TopSlope,1.0f,1.0f };
    XMVECTORF32 vLeftBottom = { vViewFrust.LeftSlope,vViewFrust.BottomSlope,1.0f,1.0f };
    XMVECTORF32 vNear = { vViewFrust.Near,vViewFrust.Near,vViewFrust.Near,1.0f };
    XMVECTORF32 vFar = { vViewFrust.Far,vViewFrust.Far,vViewFrust.Far,1.0f };
    XMVECTOR vRightTopNear = XMVectorMultiply(vRightTop, vNear);
    XMVECTOR vRightTopFar = XMVectorMultiply(vRightTop, vFar);
    XMVECTOR vLeftBottomNear = XMVectorMultiply(vLeftBottom, vNear);
    XMVECTOR vLeftBottomFar = XMVectorMultiply(vLeftBottom, vFar);

    pvCornerPointsWorld[0] = vRightTopNear;
    pvCornerPointsWorld[1] = XMVectorSelect(vRightTopNear, vLeftBottomNear, vGrabX);
    pvCornerPointsWorld[2] = vLeftBottomNear;
    pvCornerPointsWorld[3] = XMVectorSelect(vRightTopNear, vLeftBottomNear, vGrabY);

    pvCornerPointsWorld[4] = vRightTopFar;
    pvCornerPointsWorld[5] = XMVectorSelect(vRightTopFar, vLeftBottomFar, vGrabX);
    pvCornerPointsWorld[6] = vLeftBottomFar;
    pvCornerPointsWorld[7] = XMVectorSelect(vRightTopFar, vLeftBottomFar, vGrabY);

}

//--------------------------------------------------------------------------------------
// Used to compute an intersection of the orthographic projection and the Scene AABB
//--------------------------------------------------------------------------------------
struct Triangle
{
    XMVECTOR pt[3];
    BOOL culled;
};


//--------------------------------------------------------------------------------------
// Computing an accurate near and flar plane will decrease surface acne and Peter-panning.
// Surface acne is the term for erroneous self shadowing.  Peter-panning is the effect where
// shadows disappear near the base of an object.
// As offsets are generally used with PCF filtering due self shadowing issues, computing the
// correct near and far planes becomes even more important.
// This concept is not complicated, but the intersection code is.
//--------------------------------------------------------------------------------------
void CascadedShadowsManager::ComputeNearAndFar(FLOAT& fNearPlane,
    FLOAT& fFarPlane,
    FXMVECTOR vLightCameraOrthographicMin,
    FXMVECTOR vLightCameraOrthographicMax,
    XMVECTOR* pvPointsInCameraView)
{

    // Initialize the near and far planes
    fNearPlane = FLT_MAX;
    fFarPlane = -FLT_MAX;

    Triangle triangleList[16];
    INT iTriangleCnt = 1;

    triangleList[0].pt[0] = pvPointsInCameraView[0];
    triangleList[0].pt[1] = pvPointsInCameraView[1];
    triangleList[0].pt[2] = pvPointsInCameraView[2];
    triangleList[0].culled = false;

    // These are the indices used to tesselate an AABB into a list of triangles.
    static const INT iAABBTriIndexes[] =
    {
        0,1,2,  1,2,3,
        4,5,6,  5,6,7,
        0,2,4,  2,4,6,
        1,3,5,  3,5,7,
        0,1,4,  1,4,5,
        2,3,6,  3,6,7
    };

    INT iPointPassesCollision[3];

    // At a high level: 
    // 1. Iterate over all 12 triangles of the AABB.  
    // 2. Clip the triangles against each plane. Create new triangles as needed.
    // 3. Find the min and max z values as the near and far plane.

    //This is easier because the triangles are in camera spacing making the collisions tests simple comparisions.

    float fLightCameraOrthographicMinX = XMVectorGetX(vLightCameraOrthographicMin);
    float fLightCameraOrthographicMaxX = XMVectorGetX(vLightCameraOrthographicMax);
    float fLightCameraOrthographicMinY = XMVectorGetY(vLightCameraOrthographicMin);
    float fLightCameraOrthographicMaxY = XMVectorGetY(vLightCameraOrthographicMax);

    for (INT AABBTriIter = 0; AABBTriIter < 12; ++AABBTriIter)
    {

        triangleList[0].pt[0] = pvPointsInCameraView[iAABBTriIndexes[AABBTriIter * 3 + 0]];
        triangleList[0].pt[1] = pvPointsInCameraView[iAABBTriIndexes[AABBTriIter * 3 + 1]];
        triangleList[0].pt[2] = pvPointsInCameraView[iAABBTriIndexes[AABBTriIter * 3 + 2]];
        iTriangleCnt = 1;
        triangleList[0].culled = FALSE;

        // Clip each invidual triangle against the 4 frustums.  When ever a triangle is clipped into new triangles, 
        //add them to the list.
        for (INT frustumPlaneIter = 0; frustumPlaneIter < 4; ++frustumPlaneIter)
        {

            FLOAT fEdge;
            INT iComponent;

            if (frustumPlaneIter == 0)
            {
                fEdge = fLightCameraOrthographicMinX; // todo make float temp
                iComponent = 0;
            }
            else if (frustumPlaneIter == 1)
            {
                fEdge = fLightCameraOrthographicMaxX;
                iComponent = 0;
            }
            else if (frustumPlaneIter == 2)
            {
                fEdge = fLightCameraOrthographicMinY;
                iComponent = 1;
            }
            else
            {
                fEdge = fLightCameraOrthographicMaxY;
                iComponent = 1;
            }

            for (INT triIter = 0; triIter < iTriangleCnt; ++triIter)
            {
                // We don't delete triangles, so we skip those that have been culled.
                if (!triangleList[triIter].culled)
                {
                    INT iInsideVertCount = 0;
                    XMVECTOR tempOrder;
                    // Test against the correct frustum plane.
                    // This could be written more compactly, but it would be harder to understand.

                    if (frustumPlaneIter == 0)
                    {
                        for (INT triPtIter = 0; triPtIter < 3; ++triPtIter)
                        {
                            if (XMVectorGetX(triangleList[triIter].pt[triPtIter]) >
                                XMVectorGetX(vLightCameraOrthographicMin))
                            {
                                iPointPassesCollision[triPtIter] = 1;
                            }
                            else
                            {
                                iPointPassesCollision[triPtIter] = 0;
                            }
                            iInsideVertCount += iPointPassesCollision[triPtIter];
                        }
                    }
                    else if (frustumPlaneIter == 1)
                    {
                        for (INT triPtIter = 0; triPtIter < 3; ++triPtIter)
                        {
                            if (XMVectorGetX(triangleList[triIter].pt[triPtIter]) <
                                XMVectorGetX(vLightCameraOrthographicMax))
                            {
                                iPointPassesCollision[triPtIter] = 1;
                            }
                            else
                            {
                                iPointPassesCollision[triPtIter] = 0;
                            }
                            iInsideVertCount += iPointPassesCollision[triPtIter];
                        }
                    }
                    else if (frustumPlaneIter == 2)
                    {
                        for (INT triPtIter = 0; triPtIter < 3; ++triPtIter)
                        {
                            if (XMVectorGetY(triangleList[triIter].pt[triPtIter]) >
                                XMVectorGetY(vLightCameraOrthographicMin))
                            {
                                iPointPassesCollision[triPtIter] = 1;
                            }
                            else
                            {
                                iPointPassesCollision[triPtIter] = 0;
                            }
                            iInsideVertCount += iPointPassesCollision[triPtIter];
                        }
                    }
                    else
                    {
                        for (INT triPtIter = 0; triPtIter < 3; ++triPtIter)
                        {
                            if (XMVectorGetY(triangleList[triIter].pt[triPtIter]) <
                                XMVectorGetY(vLightCameraOrthographicMax))
                            {
                                iPointPassesCollision[triPtIter] = 1;
                            }
                            else
                            {
                                iPointPassesCollision[triPtIter] = 0;
                            }
                            iInsideVertCount += iPointPassesCollision[triPtIter];
                        }
                    }

                    // Move the points that pass the frustum test to the begining of the array.
                    if (iPointPassesCollision[1] && !iPointPassesCollision[0])
                    {
                        tempOrder = triangleList[triIter].pt[0];
                        triangleList[triIter].pt[0] = triangleList[triIter].pt[1];
                        triangleList[triIter].pt[1] = tempOrder;
                        iPointPassesCollision[0] = TRUE;
                        iPointPassesCollision[1] = FALSE;
                    }
                    if (iPointPassesCollision[2] && !iPointPassesCollision[1])
                    {
                        tempOrder = triangleList[triIter].pt[1];
                        triangleList[triIter].pt[1] = triangleList[triIter].pt[2];
                        triangleList[triIter].pt[2] = tempOrder;
                        iPointPassesCollision[1] = TRUE;
                        iPointPassesCollision[2] = FALSE;
                    }
                    if (iPointPassesCollision[1] && !iPointPassesCollision[0])
                    {
                        tempOrder = triangleList[triIter].pt[0];
                        triangleList[triIter].pt[0] = triangleList[triIter].pt[1];
                        triangleList[triIter].pt[1] = tempOrder;
                        iPointPassesCollision[0] = TRUE;
                        iPointPassesCollision[1] = FALSE;
                    }

                    if (iInsideVertCount == 0)
                    { // All points failed. We're done,  
                        triangleList[triIter].culled = true;
                    }
                    else if (iInsideVertCount == 1)
                    {// One point passed. Clip the triangle against the Frustum plane
                        triangleList[triIter].culled = false;

                        // 
                        XMVECTOR vVert0ToVert1 = triangleList[triIter].pt[1] - triangleList[triIter].pt[0];
                        XMVECTOR vVert0ToVert2 = triangleList[triIter].pt[2] - triangleList[triIter].pt[0];

                        // Find the collision ratio.
                        FLOAT fHitPointTimeRatio = fEdge - XMVectorGetByIndex(triangleList[triIter].pt[0], iComponent);
                        // Calculate the distance along the vector as ratio of the hit ratio to the component.
                        FLOAT fDistanceAlongVector01 = fHitPointTimeRatio / XMVectorGetByIndex(vVert0ToVert1, iComponent);
                        FLOAT fDistanceAlongVector02 = fHitPointTimeRatio / XMVectorGetByIndex(vVert0ToVert2, iComponent);
                        // Add the point plus a percentage of the vector.
                        vVert0ToVert1 *= fDistanceAlongVector01;
                        vVert0ToVert1 += triangleList[triIter].pt[0];
                        vVert0ToVert2 *= fDistanceAlongVector02;
                        vVert0ToVert2 += triangleList[triIter].pt[0];

                        triangleList[triIter].pt[1] = vVert0ToVert2;
                        triangleList[triIter].pt[2] = vVert0ToVert1;

                    }
                    else if (iInsideVertCount == 2)
                    { // 2 in  // tesselate into 2 triangles


                        // Copy the triangle\(if it exists) after the current triangle out of
                        // the way so we can override it with the new triangle we're inserting.
                        triangleList[iTriangleCnt] = triangleList[triIter + 1];

                        triangleList[triIter].culled = false;
                        triangleList[triIter + 1].culled = false;

                        // Get the vector from the outside point into the 2 inside points.
                        XMVECTOR vVert2ToVert0 = triangleList[triIter].pt[0] - triangleList[triIter].pt[2];
                        XMVECTOR vVert2ToVert1 = triangleList[triIter].pt[1] - triangleList[triIter].pt[2];

                        // Get the hit point ratio.
                        FLOAT fHitPointTime_2_0 = fEdge - XMVectorGetByIndex(triangleList[triIter].pt[2], iComponent);
                        FLOAT fDistanceAlongVector_2_0 = fHitPointTime_2_0 / XMVectorGetByIndex(vVert2ToVert0, iComponent);
                        // Calcaulte the new vert by adding the percentage of the vector plus point 2.
                        vVert2ToVert0 *= fDistanceAlongVector_2_0;
                        vVert2ToVert0 += triangleList[triIter].pt[2];

                        // Add a new triangle.
                        triangleList[triIter + 1].pt[0] = triangleList[triIter].pt[0];
                        triangleList[triIter + 1].pt[1] = triangleList[triIter].pt[1];
                        triangleList[triIter + 1].pt[2] = vVert2ToVert0;

                        //Get the hit point ratio.
                        FLOAT fHitPointTime_2_1 = fEdge - XMVectorGetByIndex(triangleList[triIter].pt[2], iComponent);
                        FLOAT fDistanceAlongVector_2_1 = fHitPointTime_2_1 / XMVectorGetByIndex(vVert2ToVert1, iComponent);
                        vVert2ToVert1 *= fDistanceAlongVector_2_1;
                        vVert2ToVert1 += triangleList[triIter].pt[2];
                        triangleList[triIter].pt[0] = triangleList[triIter + 1].pt[1];
                        triangleList[triIter].pt[1] = triangleList[triIter + 1].pt[2];
                        triangleList[triIter].pt[2] = vVert2ToVert1;
                        // Cncrement triangle count and skip the triangle we just inserted.
                        ++iTriangleCnt;
                        ++triIter;


                    }
                    else
                    { // all in
                        triangleList[triIter].culled = false;

                    }
                }// end if !culled loop            
            }
        }
        for (INT index = 0; index < iTriangleCnt; ++index)
        {
            if (!triangleList[index].culled)
            {
                // Set the near and far plan and the min and max z values respectivly.
                for (int vertind = 0; vertind < 3; ++vertind)
                {
                    float fTriangleCoordZ = XMVectorGetZ(triangleList[index].pt[vertind]);
                    if (fNearPlane > fTriangleCoordZ)
                    {
                        fNearPlane = fTriangleCoordZ;
                    }
                    if (fFarPlane < fTriangleCoordZ)
                    {
                        fFarPlane = fTriangleCoordZ;
                    }
                }
            }
        }
    }

}


//--------------------------------------------------------------------------------------
// This function converts the "center, extents" version of an AABB into 8 points.
//--------------------------------------------------------------------------------------
void CascadedShadowsManager::CreateAABBPoints(XMVECTOR* vAABBPoints, FXMVECTOR vCenter, FXMVECTOR vExtents)
{
    //This map enables us to use a for loop and do vector math.
    static const XMVECTORF32 vExtentsMap[] =
    {
        {1.0f, 1.0f, -1.0f, 1.0f},
        {-1.0f, 1.0f, -1.0f, 1.0f},
        {1.0f, -1.0f, -1.0f, 1.0f},
        {-1.0f, -1.0f, -1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f, 1.0f},
        {-1.0f, 1.0f, 1.0f, 1.0f},
        {1.0f, -1.0f, 1.0f, 1.0f},
        {-1.0f, -1.0f, 1.0f, 1.0f}
    };

    for (INT index = 0; index < 8; ++index)
    {
        vAABBPoints[index] = XMVectorMultiplyAdd(vExtentsMap[index], vExtents, vCenter);
    }

}


//--------------------------------------------------------------------------------------
// This function is where the real work is done. We determin the matricies and constants used in 
// shadow generation and scene generation.
//--------------------------------------------------------------------------------------
HRESULT CascadedShadowsManager::InitFrame(ID3D11Device* pd3dDevice, ISceneMesh* mesh)
{

    ReleaseAndAllocateNewShadowResources(pd3dDevice);

    // Copy D3DX matricies into XNA Math matricies.
    const D3DXMATRIX* mD3DXViewCameraProjection = m_pViewerCamera->GetProjMatrix();
    XMMATRIX matViewCameraProjection = XMLoadFloat4x4((XMFLOAT4X4*)&mD3DXViewCameraProjection->_11);
    const D3DXMATRIX* mD3DXViewCameraView = m_pViewerCamera->GetViewMatrix();
    XMMATRIX matViewCameraView = XMLoadFloat4x4((XMFLOAT4X4*)&mD3DXViewCameraView->_11);
    const D3DXMATRIX* mD3DLightView = m_pLightCamera->GetViewMatrix();
    XMMATRIX matLightCameraView = XMLoadFloat4x4((XMFLOAT4X4*)&mD3DLightView->_11);

    XMVECTOR det;
    XMMATRIX matInverseViewCamera = XMMatrixInverse(&det, matViewCameraView);

    // Convert from min max representation to center extents represnetation.
    // This will make it easier to pull the points out of the transformation.
    XMVECTOR vSceneCenter = m_vSceneAABBMin + m_vSceneAABBMax;
    vSceneCenter *= g_vHalfVector;
    XMVECTOR vSceneExtents = m_vSceneAABBMax - m_vSceneAABBMin;
    vSceneExtents *= g_vHalfVector;

    XMVECTOR vSceneAABBPointsLightSpace[8];
    // This function simply converts the center and extents of an AABB into 8 points
    CreateAABBPoints(vSceneAABBPointsLightSpace, vSceneCenter, vSceneExtents);
    // Transform the scene AABB to Light space.
    for (int index = 0; index < 8; ++index)
    {
        vSceneAABBPointsLightSpace[index] = XMVector4Transform(vSceneAABBPointsLightSpace[index], matLightCameraView);
    }


    FLOAT fFrustumIntervalBegin, fFrustumIntervalEnd;
    XMVECTOR vLightCameraOrthographicMin;  // light space frustrum aabb 
    XMVECTOR vLightCameraOrthographicMax;
    FLOAT fCameraNearFarRange = m_pViewerCamera->GetFarClip() - m_pViewerCamera->GetNearClip();

    XMVECTOR vWorldUnitsPerTexel = g_vZero;

    // We loop over the cascades to calculate the orthographic projection for each cascade.
    for (INT iCascadeIndex = 0; iCascadeIndex < m_CopyOfCascadeConfig.m_nCascadeLevels; ++iCascadeIndex)
    {
        // Calculate the interval of the View Frustum that this cascade covers. We measure the interval 
        // the cascade covers as a Min and Max distance along the Z Axis.
        if (m_eSelectedCascadesFit == FIT_TO_CASCADES)
        {
            // Because we want to fit the orthogrpahic projection tightly around the Cascade, we set the Mimiumum cascade 
            // value to the previous Frustum end Interval
            if (iCascadeIndex == 0) fFrustumIntervalBegin = 0.0f;
            else fFrustumIntervalBegin = (FLOAT)m_iCascadePartitionsZeroToOne[iCascadeIndex - 1];
        }
        else
        {
            // In the FIT_TO_SCENE technique the Cascades overlap eachother.  In other words, interval 1 is coverd by
            // cascades 1 to 8, interval 2 is covered by cascades 2 to 8 and so forth.
            fFrustumIntervalBegin = 0.0f;
        }

        // Scale the intervals between 0 and 1. They are now percentages that we can scale with.
        fFrustumIntervalEnd = (FLOAT)m_iCascadePartitionsZeroToOne[iCascadeIndex];
        fFrustumIntervalBegin /= (FLOAT)m_iCascadePartitionsMax;
        fFrustumIntervalEnd /= (FLOAT)m_iCascadePartitionsMax;
        fFrustumIntervalBegin = fFrustumIntervalBegin * fCameraNearFarRange;
        fFrustumIntervalEnd = fFrustumIntervalEnd * fCameraNearFarRange;
        XMVECTOR vFrustumPoints[8];

        // This function takes the began and end intervals along with the projection matrix and returns the 8
        // points that repreresent the cascade Interval
        CreateFrustumPointsFromCascadeInterval(fFrustumIntervalBegin, fFrustumIntervalEnd,
            matViewCameraProjection, vFrustumPoints);

        vLightCameraOrthographicMin = g_vFLTMAX;
        vLightCameraOrthographicMax = g_vFLTMIN;

        XMVECTOR vTempTranslatedCornerPoint;
        // This next section of code calculates the min and max values for the orthographic projection.
        for (int icpIndex = 0; icpIndex < 8; ++icpIndex)
        {
            // Transform the frustum from camera view space to world space.
            vFrustumPoints[icpIndex] = XMVector4Transform(vFrustumPoints[icpIndex], matInverseViewCamera);
            // Transform the point from world space to Light Camera Space.
            vTempTranslatedCornerPoint = XMVector4Transform(vFrustumPoints[icpIndex], matLightCameraView);
            // Find the closest point.
            vLightCameraOrthographicMin = XMVectorMin(vTempTranslatedCornerPoint, vLightCameraOrthographicMin);
            vLightCameraOrthographicMax = XMVectorMax(vTempTranslatedCornerPoint, vLightCameraOrthographicMax);
        }

        // This code removes the shimmering effect along the edges of shadows due to
        // the light changing to fit the camera.
        if (m_eSelectedCascadesFit == FIT_TO_SCENE)
        {
            // Fit the ortho projection to the cascades far plane and a near plane of zero. 
            // Pad the projection to be the size of the diagonal of the Frustum partition. 
            // 
            // To do this, we pad the ortho transform so that it is always big enough to cover 
            // the entire camera view frustum.
            XMVECTOR vDiagonal = vFrustumPoints[0] - vFrustumPoints[6];
            vDiagonal = XMVector3Length(vDiagonal);

            // The bound is the length of the diagonal of the frustum interval.
            FLOAT fCascadeBound = XMVectorGetX(vDiagonal);

            // The offset calculated will pad the ortho projection so that it is always the same size 
            // and big enough to cover the entire cascade interval.
            XMVECTOR vBoarderOffset = (vDiagonal -
                (vLightCameraOrthographicMax - vLightCameraOrthographicMin))
                * g_vHalfVector;
            // Set the Z and W components to zero.
            vBoarderOffset *= g_vMultiplySetzwToZero;

            // Add the offsets to the projection.
            vLightCameraOrthographicMax += vBoarderOffset;
            vLightCameraOrthographicMin -= vBoarderOffset;

            // The world units per texel are used to snap the shadow the orthographic projection
            // to texel sized increments.  This keeps the edges of the shadows from shimmering.
            FLOAT fWorldUnitsPerTexel = fCascadeBound / (float)m_CopyOfCascadeConfig.m_iBufferSize;
            vWorldUnitsPerTexel = XMVectorSet(fWorldUnitsPerTexel, fWorldUnitsPerTexel, 0.0f, 0.0f);


        }
        else if (m_eSelectedCascadesFit == FIT_TO_CASCADES)
        {

            // We calculate a looser bound based on the size of the PCF blur.  This ensures us that we're 
            // sampling within the correct map.
            float fScaleDuetoBlureAMT = ((float)(m_iPCFBlurSize * 2 + 1)
                / (float)m_CopyOfCascadeConfig.m_iBufferSize);
            XMVECTORF32 vScaleDuetoBlureAMT = { fScaleDuetoBlureAMT, fScaleDuetoBlureAMT, 0.0f, 0.0f };


            float fNormalizeByBufferSize = (1.0f / (float)m_CopyOfCascadeConfig.m_iBufferSize);
            XMVECTOR vNormalizeByBufferSize = XMVectorSet(fNormalizeByBufferSize, fNormalizeByBufferSize, 0.0f, 0.0f);

            // We calculate the offsets as a percentage of the bound.
            XMVECTOR vBoarderOffset = vLightCameraOrthographicMax - vLightCameraOrthographicMin;
            vBoarderOffset *= g_vHalfVector;
            vBoarderOffset *= vScaleDuetoBlureAMT;
            vLightCameraOrthographicMax += vBoarderOffset;
            vLightCameraOrthographicMin -= vBoarderOffset;

            // The world units per texel are used to snap  the orthographic projection
            // to texel sized increments.  
            // Because we're fitting tighly to the cascades, the shimmering shadow edges will still be present when the 
            // camera rotates.  However, when zooming in or strafing the shadow edge will not shimmer.
            vWorldUnitsPerTexel = vLightCameraOrthographicMax - vLightCameraOrthographicMin;
            vWorldUnitsPerTexel *= vNormalizeByBufferSize;

        }
        float fLightCameraOrthographicMinZ = XMVectorGetZ(vLightCameraOrthographicMin);


        if (m_bMoveLightTexelSize)
        {

            // We snape the camera to 1 pixel increments so that moving the camera does not cause the shadows to jitter.
            // This is a matter of integer dividing by the world space size of a texel
            vLightCameraOrthographicMin /= vWorldUnitsPerTexel;
            vLightCameraOrthographicMin = XMVectorFloor(vLightCameraOrthographicMin);
            vLightCameraOrthographicMin *= vWorldUnitsPerTexel;

            vLightCameraOrthographicMax /= vWorldUnitsPerTexel;
            vLightCameraOrthographicMax = XMVectorFloor(vLightCameraOrthographicMax);
            vLightCameraOrthographicMax *= vWorldUnitsPerTexel;

        }

        //These are the unconfigured near and far plane values.  They are purposly awful to show 
        // how important calculating accurate near and far planes is.
        FLOAT fNearPlane = 0.0f;
        FLOAT fFarPlane = 10000.0f;

        if (m_eSelectedNearFarFit == FIT_NEARFAR_AABB)
        {

            XMVECTOR vLightSpaceSceneAABBminValue = g_vFLTMAX;  // world space scene aabb 
            XMVECTOR vLightSpaceSceneAABBmaxValue = g_vFLTMIN;
            // We calculate the min and max vectors of the scene in light space. The min and max "Z" values of the  
            // light space AABB can be used for the near and far plane. This is easier than intersecting the scene with the AABB
            // and in some cases provides similar results.
            for (int index = 0; index < 8; ++index)
            {
                vLightSpaceSceneAABBminValue = XMVectorMin(vSceneAABBPointsLightSpace[index], vLightSpaceSceneAABBminValue);
                vLightSpaceSceneAABBmaxValue = XMVectorMax(vSceneAABBPointsLightSpace[index], vLightSpaceSceneAABBmaxValue);
            }

            // The min and max z values are the near and far planes.
            fNearPlane = XMVectorGetZ(vLightSpaceSceneAABBminValue);
            fFarPlane = XMVectorGetZ(vLightSpaceSceneAABBmaxValue);
        }
        else if (m_eSelectedNearFarFit == FIT_NEARFAR_SCENE_AABB
            || m_eSelectedNearFarFit == FIT_NEARFAR_PANCAKING)
        {
            // By intersecting the light frustum with the scene AABB we can get a tighter bound on the near and far plane.
            ComputeNearAndFar(fNearPlane, fFarPlane, vLightCameraOrthographicMin,
                vLightCameraOrthographicMax, vSceneAABBPointsLightSpace);
            if (m_eSelectedNearFarFit == FIT_NEARFAR_PANCAKING)
            {
                if (fLightCameraOrthographicMinZ > fNearPlane)
                {
                    fNearPlane = fLightCameraOrthographicMinZ;
                }
            }
        }
        else
        {

        }
        // Craete the orthographic projection for this cascade.
        D3DXMatrixOrthoOffCenterLH(&m_matShadowProj[iCascadeIndex],
            XMVectorGetX(vLightCameraOrthographicMin),
            XMVectorGetX(vLightCameraOrthographicMax),
            XMVectorGetY(vLightCameraOrthographicMin),
            XMVectorGetY(vLightCameraOrthographicMax),
            fNearPlane, fFarPlane);

        m_fCascadePartitionsFrustum[iCascadeIndex] = fFrustumIntervalEnd;
    }
    m_matShadowView = *m_pLightCamera->GetViewMatrix();


    return S_OK;
}


//--------------------------------------------------------------------------------------
// Render the cascades into a texture atlas.
//--------------------------------------------------------------------------------------
HRESULT CascadedShadowsManager::RenderShadowsForAllCascades(ID3D11Device* pd3dDevice,
    ID3D11DeviceContext* pd3dDeviceContext,
    ISceneMesh* pMesh
) {

    HRESULT hr = S_OK;

    D3DXMATRIX dxmatWorldViewProjection;
    D3DXMATRIX dxmatWorld;

    pd3dDeviceContext->ClearDepthStencilView(m_pCascadedShadowMapDSV, D3D11_CLEAR_DEPTH, 1.0, 0);
    ID3D11RenderTargetView* pnullView = NULL;
    // Set a null render target so as not to render color.
    pd3dDeviceContext->OMSetRenderTargets(1, &pnullView, m_pCascadedShadowMapDSV);

    if (m_eSelectedNearFarFit == FIT_NEARFAR_PANCAKING)
    {
        pd3dDeviceContext->RSSetState(m_prsShadowPancake);
    }
    else
    {
        pd3dDeviceContext->RSSetState(m_prsShadow);
    }
    // Iterate over cascades and render shadows.
    for (INT currentCascade = 0; currentCascade < m_CopyOfCascadeConfig.m_nCascadeLevels; ++currentCascade)
    {

        // Each cascade has its own viewport because we're storing all the cascades in one large texture.
        pd3dDeviceContext->RSSetViewports(1, &m_RenderVP[currentCascade]);
        dxmatWorld = *m_pLightCamera->GetWorldMatrix();

        // We calculate the matrices in the Init function.
        dxmatWorldViewProjection = m_matShadowView * m_matShadowProj[currentCascade];

        D3D11_MAPPED_SUBRESOURCE MappedResource;
        V(pd3dDeviceContext->Map(m_pcbGlobalConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
        CB_ALL_SHADOW_DATA* pcbAllShadowConstants = (CB_ALL_SHADOW_DATA*)MappedResource.pData;
        D3DXMatrixTranspose(&pcbAllShadowConstants->m_WorldViewProj, &dxmatWorldViewProjection);
        D3DXMATRIX matIdentity;
        D3DXMatrixIdentity(&matIdentity);
        // The model was exported in world space, so we can pass the identity up as the world transform.
        D3DXMatrixTranspose(&pcbAllShadowConstants->m_World, &matIdentity);
        pd3dDeviceContext->Unmap(m_pcbGlobalConstantBuffer, 0);
        pd3dDeviceContext->IASetInputLayout(m_pVertexLayoutMesh);

        // No pixel shader is bound as we're only writing out depth.
        pd3dDeviceContext->VSSetShader(m_pvsRenderOrthoShadow, NULL, 0);
        pd3dDeviceContext->PSSetShader(NULL, NULL, 0);
        pd3dDeviceContext->GSSetShader(NULL, NULL, 0);

        pd3dDeviceContext->VSSetConstantBuffers(0, 1, &m_pcbGlobalConstantBuffer);

        pMesh->Render(pd3dDeviceContext, 0, 1);
    }

    pd3dDeviceContext->RSSetState(NULL);
    pd3dDeviceContext->OMSetRenderTargets(1, &pnullView, NULL);

    return hr;

}


//--------------------------------------------------------------------------------------
// Render the scene.
//--------------------------------------------------------------------------------------
HRESULT CascadedShadowsManager::RenderScene(ID3D11DeviceContext* pd3dDeviceContext,
    ID3D11RenderTargetView* prtvBackBuffer,
    ID3D11DepthStencilView* pdsvBackBuffer,
    ISceneMesh* pMesh,
    CFirstPersonCamera* pActiveCamera,
    D3D11_VIEWPORT* dxutViewPort,
    BOOL bVisualize
) {

    HRESULT hr = S_OK;
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    // We have a seperate render state for the actual rasterization because of different depth biases and Cull modes.
    pd3dDeviceContext->RSSetState(m_prsScene);
    // 
    pd3dDeviceContext->OMSetRenderTargets(1, &prtvBackBuffer, pdsvBackBuffer);
    pd3dDeviceContext->RSSetViewports(1, dxutViewPort);
    pd3dDeviceContext->IASetInputLayout(m_pVertexLayoutMesh);

    D3DXMATRIX dxmatCameraProj = *pActiveCamera->GetProjMatrix();
    D3DXMATRIX dxmatCameraView = *pActiveCamera->GetViewMatrix();

    // The user has the option to view the ortho shadow cameras.
    if (m_eSelectedCamera >= ORTHO_CAMERA1)
    {
        // In the CAMERA_SELECTION enumeration, value 0 is EYE_CAMERA
        // value 1 is LIGHT_CAMERA and 2 to 10 are the ORTHO_CAMERA values.
        // Subtract to so that we can use the enum to index.
        dxmatCameraProj = m_matShadowProj[(int)m_eSelectedCamera - 2];
        dxmatCameraView = m_matShadowView;
    }

    D3DXMATRIX dxmatWorldViewProjection = dxmatCameraView * dxmatCameraProj;

    V(pd3dDeviceContext->Map(m_pcbGlobalConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
    CB_ALL_SHADOW_DATA* pcbAllShadowConstants = (CB_ALL_SHADOW_DATA*)MappedResource.pData;
    D3DXMatrixTranspose(&pcbAllShadowConstants->m_WorldViewProj, &dxmatWorldViewProjection);
    D3DXMatrixTranspose(&pcbAllShadowConstants->m_WorldView, &dxmatCameraView);
    // These are the for loop begin end values. 
    pcbAllShadowConstants->m_iPCFBlurForLoopEnd = m_iPCFBlurSize / 2 + 1;
    pcbAllShadowConstants->m_iPCFBlurForLoopStart = m_iPCFBlurSize / -2;
    // This is a floating point number that is used as the percentage to blur between maps.    
    pcbAllShadowConstants->m_fCascadeBlendArea = m_fBlurBetweenCascadesAmount;
    pcbAllShadowConstants->m_fTexelSize = 1.0f / (float)m_CopyOfCascadeConfig.m_iBufferSize;
    pcbAllShadowConstants->m_fNativeTexelSizeInX = pcbAllShadowConstants->m_fTexelSize / m_CopyOfCascadeConfig.m_nCascadeLevels;
    D3DXMATRIX dxmatIdentity;
    D3DXMatrixIdentity(&dxmatIdentity);
    D3DXMatrixTranspose(&pcbAllShadowConstants->m_World, &dxmatIdentity);
    D3DXMATRIX dxmatTextureScale;
    D3DXMatrixScaling(&dxmatTextureScale,
        0.5f,
        -0.5f,
        1.0f);

    D3DXMATRIX dxmatTextureTranslation;
    D3DXMatrixTranslation(&dxmatTextureTranslation, .5, .5, 0);
    D3DXMATRIX scaleToTile;
    D3DXMatrixScaling(&scaleToTile, 1.0f / (float)m_pCascadeConfig->m_nCascadeLevels, 1.0, 1.0);

    pcbAllShadowConstants->m_fShadowBiasFromGUI = m_fPCFOffset;
    pcbAllShadowConstants->m_fShadowPartitionSize = 1.0f / (float)m_CopyOfCascadeConfig.m_nCascadeLevels;

    D3DXMatrixTranspose(&pcbAllShadowConstants->m_Shadow, &m_matShadowView);
    for (int index = 0; index < m_CopyOfCascadeConfig.m_nCascadeLevels; ++index)
    {
        D3DXMATRIX mShadowTexture = m_matShadowProj[index] * dxmatTextureScale * dxmatTextureTranslation;
        pcbAllShadowConstants->m_vCascadeScale[index].x = mShadowTexture._11;
        pcbAllShadowConstants->m_vCascadeScale[index].y = mShadowTexture._22;
        pcbAllShadowConstants->m_vCascadeScale[index].z = mShadowTexture._33;
        pcbAllShadowConstants->m_vCascadeScale[index].w = 1;

        pcbAllShadowConstants->m_vCascadeOffset[index].x = mShadowTexture._41;
        pcbAllShadowConstants->m_vCascadeOffset[index].y = mShadowTexture._42;
        pcbAllShadowConstants->m_vCascadeOffset[index].z = mShadowTexture._43;
        pcbAllShadowConstants->m_vCascadeOffset[index].w = 0;
    }


    // Copy intervals for the depth interval selection method.
    memcpy(pcbAllShadowConstants->m_fCascadeFrustumsEyeSpaceDepths,
        m_fCascadePartitionsFrustum, MAX_CASCADES * 4);
    for (int index = 0; index < MAX_CASCADES; ++index)
    {
        pcbAllShadowConstants->m_fCascadeFrustumsEyeSpaceDepthsFloat4[index].x = m_fCascadePartitionsFrustum[index];
    }

    // The border padding values keep the pixel shader from reading the borders during PCF filtering.
    pcbAllShadowConstants->m_fMaxBorderPadding = (float)(m_pCascadeConfig->m_iBufferSize - 1.0f) /
        (float)m_pCascadeConfig->m_iBufferSize;
    pcbAllShadowConstants->m_fMinBorderPadding = (float)(1.0f) /
        (float)m_pCascadeConfig->m_iBufferSize;

    D3DXVECTOR3 ep = *m_pLightCamera->GetEyePt();
    D3DXVECTOR3 lp = *m_pLightCamera->GetLookAtPt();
    ep -= lp;
    D3DXVec3Normalize(&ep, &ep);

    pcbAllShadowConstants->m_vLightDir = D3DXVECTOR4(ep.x, ep.y, ep.z, 1.0f);
    pcbAllShadowConstants->m_nCascadeLevels = m_CopyOfCascadeConfig.m_nCascadeLevels;
    pcbAllShadowConstants->m_iVisualizeCascades = bVisualize;
    pd3dDeviceContext->Unmap(m_pcbGlobalConstantBuffer, 0);


    pd3dDeviceContext->PSSetSamplers(0, 1, &m_pSamLinear);
    pd3dDeviceContext->PSSetSamplers(1, 1, &m_pSamLinear);

    pd3dDeviceContext->PSSetSamplers(5, 1, &m_pSamShadowPCF);
    pd3dDeviceContext->GSSetShader(NULL, NULL, 0);

    const INT cascadeShaderIndex = max(0, min((INT)m_CopyOfCascadeConfig.m_nCascadeLevels - 1, MAX_CASCADES - 1));
    ID3D11Device* pd3dDevice = NULL;
    pd3dDeviceContext->GetDevice(&pd3dDevice);
    hr = EnsureRenderSceneVertexShader(pd3dDevice, cascadeShaderIndex);
    if (FAILED(hr))
    {
        SAFE_RELEASE(pd3dDevice);
        return hr;
    }

    hr = EnsureRenderScenePixelShader(pd3dDevice,
        cascadeShaderIndex,
        m_iDerivativeBasedOffset,
        m_iBlurBetweenCascades,
        m_eSelectedCascadeSelection);
    if (FAILED(hr))
    {
        SAFE_RELEASE(pd3dDevice);
        return hr;
    }

    SAFE_RELEASE(pd3dDevice);

    pd3dDeviceContext->VSSetShader(m_pvsRenderScene[cascadeShaderIndex], NULL, 0);

    // There are up to 8 cascades, possible derivative based offsets, blur between cascades, 
    // and two cascade selection maps.  This is a total of 64 permutations of the shader.

    pd3dDeviceContext->PSSetShader(
        m_ppsRenderSceneAllShaders[cascadeShaderIndex]
        [m_iDerivativeBasedOffset]
        [m_iBlurBetweenCascades]
        [m_eSelectedCascadeSelection],
        NULL, 0);

    pd3dDeviceContext->PSSetShaderResources(5, 1, &m_pCascadedShadowMapSRV);

    pd3dDeviceContext->VSSetConstantBuffers(0, 1, &m_pcbGlobalConstantBuffer);
    pd3dDeviceContext->PSSetConstantBuffers(0, 1, &m_pcbGlobalConstantBuffer);

    pMesh->Render(pd3dDeviceContext, 0, 1);

    ID3D11ShaderResourceView* nv[] = { NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };
    pd3dDeviceContext->PSSetShaderResources(5, 8, nv);


    return hr;
}

HRESULT CascadedShadowsManager::RenderDebug(ID3D11DeviceContext* pd3dDeviceContext, ID3D11RenderTargetView* prtvBackBuffer, ID3D11DepthStencilView* pdsvBackBuffer, D3D11_VIEWPORT* dxutViewPort)
{
    if( !m_bRenderDebug )
    {
        return S_OK;
    }

    HRESULT hr = S_OK;

    // 1/4 screen viewport.
    D3D11_VIEWPORT debugVP = {};
    debugVP.Width = dxutViewPort->Width * 0.5f;
    debugVP.Height = dxutViewPort->Height * 0.5f;
    debugVP.TopLeftX = 0.0f;
    debugVP.TopLeftY = dxutViewPort->Height - debugVP.Height;
    debugVP.MinDepth = 0.0f;
    debugVP.MaxDepth = 1.0f;

    pd3dDeviceContext->RSSetState(m_prsDebug);

    // Para overlay no necesitas depth
    pd3dDeviceContext->OMSetRenderTargets(1, &prtvBackBuffer, NULL);

    pd3dDeviceContext->RSSetViewports(1, &debugVP);

    // Fullscreen quad procedural con SV_VertexID
    pd3dDeviceContext->IASetInputLayout(NULL);
    pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    pd3dDeviceContext->GSSetShader(NULL, NULL, 0);

    pd3dDeviceContext->VSSetShader(m_pvsDebug, NULL, 0);
    pd3dDeviceContext->PSSetShader(m_ppsDebug, NULL, 0);

    pd3dDeviceContext->PSSetSamplers(0, 1, &m_pSamLinear);

    // El debug shader lee en t0
    pd3dDeviceContext->PSSetShaderResources(0, 1, &m_pCascadedShadowMapSRV);

    // Dibujar quad
    pd3dDeviceContext->Draw(4, 0);

    // Limpiar el SRV usado por el debug shader
    ID3D11ShaderResourceView* nullSRV[1] = { NULL };
    pd3dDeviceContext->PSSetShaderResources(0, 1, nullSRV);


    return hr;
}

HRESULT CascadedShadowsManager::RenderVoxelizationVolume(ID3D11DeviceContext* pd3dDeviceContext,
    ISceneMesh* pMesh,
    FXMVECTOR vVoxelMin,
    FXMVECTOR vVoxelMax,
    UINT gridSizeX,
    UINT gridSizeY,
    UINT gridSizeZ,
    FLOAT heightWarp,
    ID3D11UnorderedAccessView* pAlbedoUAV,
    ID3D11UnorderedAccessView* pMaskUAV,
    ID3D11UnorderedAccessView* pInstanceUAV,
    ID3D11Buffer* pDrawArgsBuffer)
{
    HRESULT hr = S_OK;

    if (!pd3dDeviceContext || !pMesh ||
        !m_pVertexLayoutMesh ||
        !m_pvsVoxelization || !m_pgsVoxelization || !m_ppsVoxelization ||
        !m_pcbGlobalConstantBuffer || !m_pcbVoxelParams ||
        !pAlbedoUAV || !pMaskUAV)
    {
        return E_FAIL;
    }

    const float clearAlbedo[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const UINT clearMask[4] = { 0u, 0u, 0u, 0u };
    pd3dDeviceContext->ClearUnorderedAccessViewFloat(pAlbedoUAV, clearAlbedo);
    pd3dDeviceContext->ClearUnorderedAccessViewUint(pMaskUAV, clearMask);

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    V_RETURN(pd3dDeviceContext->Map(m_pcbGlobalConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
    CB_ALL_SHADOW_DATA* pcbAllShadowConstants = (CB_ALL_SHADOW_DATA*)MappedResource.pData;

    D3DXMATRIX dxmatIdentity;
    D3DXMatrixIdentity(&dxmatIdentity);
    D3DXMatrixTranspose(&pcbAllShadowConstants->m_WorldViewProj, &dxmatIdentity);
    D3DXMatrixTranspose(&pcbAllShadowConstants->m_World, &dxmatIdentity);
    pd3dDeviceContext->Unmap(m_pcbGlobalConstantBuffer, 0);

    V_RETURN(pd3dDeviceContext->Map(m_pcbVoxelParams, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
    CB_VOXELIZATION_PARAMS* pcbVoxelParams = (CB_VOXELIZATION_PARAMS*)MappedResource.pData;
    XMStoreFloat3((XMFLOAT3*)&pcbVoxelParams->gVoxelMin, vVoxelMin);
    XMStoreFloat3((XMFLOAT3*)&pcbVoxelParams->gVoxelMax, vVoxelMax);
    pcbVoxelParams->gGridSizeX = gridSizeX;
    pcbVoxelParams->gGridSizeY = gridSizeY;
    pcbVoxelParams->gGridSizeZ = gridSizeZ;
    pcbVoxelParams->gHeightWarp = heightWarp;
    pcbVoxelParams->gXYFootprintScale = m_fVoxelXYFootprintScale;
    pcbVoxelParams->gYZFootprintScale = m_fVoxelYZFootprintScale;
    pcbVoxelParams->_padding = 0.0f;
    pd3dDeviceContext->Unmap(m_pcbVoxelParams, 0);

    const UINT voxelViewportSize = (gridSizeX > gridSizeY) ? gridSizeX : gridSizeY;
    D3D11_VIEWPORT voxelViewport = {};
    voxelViewport.TopLeftX = 0.0f;
    voxelViewport.TopLeftY = 0.0f;
    voxelViewport.Width = (FLOAT)voxelViewportSize;
    voxelViewport.Height = (FLOAT)voxelViewportSize;
    voxelViewport.MinDepth = 0.0f;
    voxelViewport.MaxDepth = 1.0f;

    ID3D11RenderTargetView* nullRTV[1] = { NULL };
    ID3D11DepthStencilView* nullDSV = NULL;
    ID3D11UnorderedAccessView* voxelUAVs[3] = { pAlbedoUAV, pMaskUAV, pInstanceUAV };
    UINT initialCounts[3] = { 0xffffffffu, 0xffffffffu, pInstanceUAV ? 0u : 0xffffffffu };
    const UINT voxelUAVCount = pInstanceUAV ? 3u : 2u;

    pd3dDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(
        0, nullRTV, nullDSV, 0, voxelUAVCount, voxelUAVs, initialCounts);

    pd3dDeviceContext->RSSetState(m_prsVoxelization ? m_prsVoxelization : m_prsScene);
    pd3dDeviceContext->RSSetViewports(1, &voxelViewport);
    pd3dDeviceContext->IASetInputLayout(m_pVertexLayoutMesh);

    pd3dDeviceContext->VSSetShader(m_pvsVoxelization, NULL, 0);
    pd3dDeviceContext->GSSetShader(m_pgsVoxelization, NULL, 0);
    pd3dDeviceContext->PSSetShader(m_ppsVoxelization, NULL, 0);

    pd3dDeviceContext->VSSetConstantBuffers(0, 1, &m_pcbGlobalConstantBuffer);
    pd3dDeviceContext->GSSetConstantBuffers(2, 1, &m_pcbVoxelParams);
    pd3dDeviceContext->PSSetConstantBuffers(2, 1, &m_pcbVoxelParams);
    pd3dDeviceContext->PSSetSamplers(0, 1, &m_pSamLinear);

    // Render the mesh with its diffuse textures bound in t0 by SDKMesh.
    pMesh->Render(pd3dDeviceContext, 0, INVALID_SAMPLER_SLOT, INVALID_SAMPLER_SLOT);

    ID3D11ShaderResourceView* nullSRV[1] = { NULL };
    ID3D11UnorderedAccessView* nullUAVs[3] = { NULL, NULL, NULL };

    pd3dDeviceContext->PSSetShaderResources(0, 1, nullSRV);
    pd3dDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(
        0, nullRTV, NULL, 0, voxelUAVCount, nullUAVs, initialCounts);

    if (pInstanceUAV && pDrawArgsBuffer)
    {
        const UINT voxelDrawArgs[4] = { ARRAYSIZE(g_VoxelCubeVertices), 0u, 0u, 0u };
        pd3dDeviceContext->UpdateSubresource(pDrawArgsBuffer, 0, NULL, voxelDrawArgs, 0, 0);
        pd3dDeviceContext->CopyStructureCount(pDrawArgsBuffer, sizeof(UINT), pInstanceUAV);
    }

    pd3dDeviceContext->GSSetShader(NULL, NULL, 0);
    pd3dDeviceContext->PSSetShader(NULL, NULL, 0);
    pd3dDeviceContext->VSSetShader(NULL, NULL, 0);
    pd3dDeviceContext->RSSetState(NULL);

    return hr;
}

HRESULT CascadedShadowsManager::RenderVoxelization(ID3D11DeviceContext* pd3dDeviceContext,
                                                    ISceneMesh* pMesh, 
                                                    CFirstPersonCamera* pActiveCamera)
{
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER(pActiveCamera);

    if (m_bStaticVoxelizationDirty)
    {
        D3DXVECTOR3 sceneMin;
        D3DXVECTOR3 sceneMax;
        XMStoreFloat3((XMFLOAT3*)&sceneMin, m_vSceneAABBMin);
        XMStoreFloat3((XMFLOAT3*)&sceneMax, m_vSceneAABBMax);

        const float sceneSizeX = sceneMax.x - sceneMin.x;
        const float sceneSizeY = sceneMax.y - sceneMin.y;
        const float sceneSizeZ = sceneMax.z - sceneMin.z;
        const float staticSideMarginX = max(1.5f, sceneSizeX * max(m_fStaticVoxelBoundsPaddingXZ, 0.0f));
        const float staticSideMarginZ = max(1.5f, sceneSizeZ * max(m_fStaticVoxelBoundsPaddingXZ, 0.0f));
        const float staticVerticalPadding = max(2.0f, sceneSizeY * max(m_fStaticVoxelBoundsPaddingY, 0.0f));
        const float staticDownMargin = max(2.0f, sceneSizeY * 0.06f);
        const float staticTopCoverage = min(max(m_fStaticVoxelTopCoverage, 0.35f), 1.0f);
        const float staticTopMargin = max(2.0f, sceneSizeY * 0.04f);
        const float staticVoxelMaxY = sceneMin.y + sceneSizeY * staticTopCoverage + staticTopMargin;

        m_vStaticVoxelAABBMin = XMVectorSet(
            sceneMin.x - staticSideMarginX,
            sceneMin.y - (staticDownMargin + staticVerticalPadding),
            sceneMin.z - staticSideMarginZ,
            1.0f);
        m_vStaticVoxelAABBMax = XMVectorSet(
            sceneMax.x + staticSideMarginX,
            min(sceneMax.y + staticTopMargin + staticVerticalPadding, staticVoxelMaxY + staticVerticalPadding),
            sceneMax.z + staticSideMarginZ,
            1.0f);

        V_RETURN(RenderVoxelizationVolume(pd3dDeviceContext, pMesh,
            m_vStaticVoxelAABBMin, m_vStaticVoxelAABBMax,
            GRID_SIZE_X, GRID_SIZE_Y, GRID_SIZE_Z,
            m_fStaticVoxelHeightWarp,
            m_pVoxelAlbedoUAV, m_pVoxelMaskUAV,
            m_pVoxelInstanceUAV, m_pVoxelDrawArgsBuffer));

        m_bStaticVoxelizationDirty = false;
    }

    return hr;
}

HRESULT CascadedShadowsManager::RenderVisualizeVoxelization(ID3D11DeviceContext* pd3dDeviceContext,
    ID3D11RenderTargetView* prtvBackBuffer,
    ID3D11DepthStencilView* pdsvBackBuffer,
    ISceneMesh* pMesh,
    D3D11_VIEWPORT* dxutViewPort,
    CFirstPersonCamera* pActiveCamera,
    bool bVisualizeVoxel)
{
    if(!bVisualizeVoxel)
        return S_OK;

    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER(pMesh);

    if (!pd3dDeviceContext || !prtvBackBuffer || !pdsvBackBuffer || !dxutViewPort || !pActiveCamera ||
        !m_pvsVisualizeVoxelization || !m_ppsVisualizeVoxelization ||
        !m_pVoxelAlbedoSRV || !m_pVoxelMaskSRV ||
        !m_pVoxelInstanceSRV || !m_pVoxelDrawArgsBuffer || !m_pVoxelCubeVertexBuffer ||
        !m_pVertexLayoutVoxelVisualize || !m_pcbVisualizeVoxels)
    {
        return E_FAIL;
    }

    D3DXMATRIX dxmatView = *pActiveCamera->GetViewMatrix();
    D3DXMATRIX dxmatProj = *pActiveCamera->GetProjMatrix();
    D3DXMATRIX dxmatViewProj = dxmatView * dxmatProj;
    D3DXMATRIX dxmatIdentity;
    D3DXMatrixIdentity(&dxmatIdentity);

    D3DXVECTOR3 staticVoxelMin;
    D3DXVECTOR3 staticVoxelMax;
    XMStoreFloat3((XMFLOAT3*)&staticVoxelMin, m_vStaticVoxelAABBMin);
    XMStoreFloat3((XMFLOAT3*)&staticVoxelMax, m_vStaticVoxelAABBMax);

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    V_RETURN(pd3dDeviceContext->Map(m_pcbVisualizeVoxels, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
    CB_VISUALIZE_VOXELS* pcbVisualize = (CB_VISUALIZE_VOXELS*)MappedResource.pData;
    D3DXMatrixTranspose(&pcbVisualize->gWorldViewProj, &dxmatViewProj);
    D3DXMatrixTranspose(&pcbVisualize->gWorld, &dxmatIdentity);
    pcbVisualize->gStaticVoxelMin = staticVoxelMin;
    pcbVisualize->gOpacity = 1.0f;
    pcbVisualize->gStaticVoxelMax = staticVoxelMax;
    pcbVisualize->gStaticHeightWarp = m_fStaticVoxelHeightWarp;
    pcbVisualize->gStaticGridSizeX = GRID_SIZE_X;
    pcbVisualize->gStaticGridSizeY = GRID_SIZE_Y;
    pcbVisualize->gStaticGridSizeZ = GRID_SIZE_Z;
    pcbVisualize->gSurfaceSnap = m_fVoxelVisualizeSurfaceSnap;

    D3DXVECTOR3 ep = *m_pLightCamera->GetEyePt();
    D3DXVECTOR3 lp = *m_pLightCamera->GetLookAtPt();
    ep -= lp;
    D3DXVec3Normalize(&ep, &ep);
    pcbVisualize->gLightDir = ep;
    pcbVisualize->gPadding1 = 0.0f;
    pd3dDeviceContext->Unmap(m_pcbVisualizeVoxels, 0);

    pd3dDeviceContext->OMSetRenderTargets(1, &prtvBackBuffer, pdsvBackBuffer);
    pd3dDeviceContext->RSSetState(m_prsScene);
    pd3dDeviceContext->RSSetViewports(1, dxutViewPort);
    const FLOAT blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    pd3dDeviceContext->OMSetBlendState(NULL, blendFactor, 0xffffffff);

    const UINT stride = sizeof(VOXEL_CUBE_VERTEX);
    const UINT offset = 0;
    pd3dDeviceContext->IASetInputLayout(m_pVertexLayoutVoxelVisualize);
    pd3dDeviceContext->IASetVertexBuffers(0, 1, &m_pVoxelCubeVertexBuffer, &stride, &offset);
    pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    pd3dDeviceContext->VSSetShader(m_pvsVisualizeVoxelization, NULL, 0);
    pd3dDeviceContext->GSSetShader(NULL, NULL, 0);
    pd3dDeviceContext->PSSetShader(m_ppsVisualizeVoxelization, NULL, 0);

    pd3dDeviceContext->VSSetConstantBuffers(3, 1, &m_pcbVisualizeVoxels);
    pd3dDeviceContext->PSSetConstantBuffers(3, 1, &m_pcbVisualizeVoxels);

    ID3D11ShaderResourceView* voxelSRVs[2] =
    {
        m_pVoxelAlbedoSRV,
        m_pVoxelMaskSRV
    };
    pd3dDeviceContext->PSSetShaderResources(8, 2, voxelSRVs);
    pd3dDeviceContext->VSSetShaderResources(10, 1, &m_pVoxelInstanceSRV);

    pd3dDeviceContext->DrawInstancedIndirect(m_pVoxelDrawArgsBuffer, 0);

    ID3D11ShaderResourceView* nullSRVs[2] = { NULL, NULL };
    pd3dDeviceContext->PSSetShaderResources(8, 2, nullSRVs);
    ID3D11ShaderResourceView* nullInstanceSRV[1] = { NULL };
    pd3dDeviceContext->VSSetShaderResources(10, 1, nullInstanceSRV);

    return hr;
}


