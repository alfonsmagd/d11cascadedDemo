#include "DXUT.h"

#include "GBufferRenderPass.h"
#include "SDKmisc.h"
#include "DXUTcamera.h"



GBufferRenderPass::GBufferRenderPass()
    : m_pMeshView(NULL)
    , m_pGeometryVS(NULL)
    , m_pGeometryPS(NULL)
    , m_pGeometryVSBlob(NULL)
    , m_pGeometryPSBlob(NULL)
    , m_pMotionVectorVS(NULL)
    , m_pMotionVectorPS(NULL)
    , m_pMotionVectorVSBlob(NULL)
    , m_pMotionVectorPSBlob(NULL)
{
    for (INT rtvIndex = 0; rtvIndex < GBUFFER_RTV_COUNT; ++rtvIndex)
    {
        m_pRTVs[rtvIndex] = NULL;
        m_pSRVs[rtvIndex] = NULL;
    }
}

GBufferRenderPass::~GBufferRenderPass()
{
    Destroy();
}

const char* GBufferRenderPass::GetPassName() const
{
    return "GBuffer";
}

HRESULT GBufferRenderPass::Create(ID3D11Device* pd3dDevice)
{
    if (pd3dDevice == NULL)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    WCHAR geometryShaderFile[] = L"GBuffer.hlsl";

    if (m_pGeometryVSBlob == NULL)
    {
        V_RETURN(CompileShaderFromFile(geometryShaderFile, NULL, "VSMain", "vs_5_0", &m_pGeometryVSBlob));
    }
    if (m_pGeometryPSBlob == NULL)
    {
        V_RETURN(CompileShaderFromFile(geometryShaderFile, NULL, "PSGBuffer", "ps_5_0", &m_pGeometryPSBlob));
    }
    if (m_pGeometryVS == NULL)
    {
        V_RETURN(pd3dDevice->CreateVertexShader(m_pGeometryVSBlob->GetBufferPointer(), m_pGeometryVSBlob->GetBufferSize(), NULL, &m_pGeometryVS));
        DXUT_SetDebugName(m_pGeometryVS, "GBufferRenderPass_GeometryVS");
    }
    if (m_pGeometryPS == NULL)
    {
        V_RETURN(pd3dDevice->CreatePixelShader(m_pGeometryPSBlob->GetBufferPointer(), m_pGeometryPSBlob->GetBufferSize(), NULL, &m_pGeometryPS));
        DXUT_SetDebugName(m_pGeometryPS, "GBufferRenderPass_GeometryPS");
    }

    if (m_pRasterizerState == NULL)
    {
        D3D11_RASTERIZER_DESC rasterDesc = {};
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.CullMode = D3D11_CULL_NONE;
        rasterDesc.FrontCounterClockwise = FALSE;
        rasterDesc.DepthBias = 0;
        rasterDesc.DepthBiasClamp = 0.0f;
        rasterDesc.SlopeScaledDepthBias = 0.0f;
        rasterDesc.DepthClipEnable = TRUE;
        rasterDesc.ScissorEnable = FALSE;
        rasterDesc.MultisampleEnable = TRUE;
        rasterDesc.AntialiasedLineEnable = FALSE;
        V_RETURN(pd3dDevice->CreateRasterizerState(&rasterDesc, &m_pRasterizerState));
        DXUT_SetDebugName(m_pRasterizerState, "DebugOverlayRendererModule_Rasterizer");
    }


    return S_OK;
}

void GBufferRenderPass::Destroy()
{
    SAFE_RELEASE(m_pMotionVectorPSBlob);
    SAFE_RELEASE(m_pMotionVectorVSBlob);
    SAFE_RELEASE(m_pMotionVectorPS);
    SAFE_RELEASE(m_pMotionVectorVS);

    SAFE_RELEASE(m_pGeometryPSBlob);
    SAFE_RELEASE(m_pGeometryVSBlob);
    SAFE_RELEASE(m_pGeometryPS);
    SAFE_RELEASE(m_pGeometryVS);
    SAFE_RELEASE(m_pRasterizerState);

    for (INT rtvIndex = 0; rtvIndex < GBUFFER_RTV_COUNT; ++rtvIndex)
    {
        SAFE_RELEASE(m_pRTVs[rtvIndex]);
        SAFE_RELEASE(m_pSRVs[rtvIndex]);
        SAFE_RELEASE(m_pGBufferTextures[rtvIndex]);
    }
}

HRESULT GBufferRenderPass::Execute(ID3D11DeviceContext* pd3dDeviceContext)
{
    HRESULT hr = S_OK;
    if (pd3dDeviceContext == NULL)
    {
        return E_INVALIDARG;
    }

    if (m_FrameContext.output.pDepthStencilView == NULL || m_FrameContext.output.pViewport == NULL)
    {
        return S_FALSE;
    }

    for (INT rtvIndex = 0; rtvIndex < GBUFFER_RTV_COUNT; ++rtvIndex)
    {
        if (m_pRTVs[rtvIndex] == NULL)
        {
            return S_FALSE;
        }
    }

    if (m_pMeshView == NULL || !m_pMeshView->IsLoaded())
    {
        return S_FALSE;
    }

    if (m_pGeometryVS == NULL || m_pGeometryPS == NULL)
    {
        return S_FALSE;
    }


  

    auto dxmatCameraProj = *m_FrameContext.pViewerCamera->GetProjMatrix();
    D3DXMATRIX dxmatCameraView = *m_FrameContext.pViewerCamera->GetViewMatrix();
    D3DXMATRIX dxmatWorld;
    if (m_pMeshView)
    {
        dxmatWorld = m_pMeshView->GetWorldMatrix();
    }
    else
    {
        D3DXMatrixIdentity(&dxmatWorld);
    }

    // The user has the option to view the ortho shadow cameras.

    D3DXMATRIX dxmatWorldView = dxmatWorld * dxmatCameraView;
    D3DXMATRIX dxmatWorldViewProjection = dxmatWorldView * dxmatCameraProj;

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    V(pd3dDeviceContext->Map(m_pGlobalConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
    CB_ALL_SHADOW_DATA* pcbAllShadowConstants = (CB_ALL_SHADOW_DATA*)MappedResource.pData;
    D3DXMatrixTranspose(&pcbAllShadowConstants->m_WorldViewProj, &dxmatWorldViewProjection);
    D3DXMatrixTranspose(&pcbAllShadowConstants->m_WorldView, &dxmatWorldView);
    pd3dDeviceContext->Unmap(m_pGlobalConstantBuffer, 0);

    //Clear SRV And RTVS
    ID3D11ShaderResourceView* nullSRVs[16] = {};
    pd3dDeviceContext->VSSetShaderResources(0, 16, nullSRVs);
    pd3dDeviceContext->PSSetShaderResources(0, 16, nullSRVs);

    FLOAT clearColor[4] = { 0, 0, 0, 1 };
    for (INT i = 0; i < GBUFFER_RTV_COUNT; ++i)
    {
        pd3dDeviceContext->ClearRenderTargetView(m_pRTVs[i], clearColor);
    }

    pd3dDeviceContext->ClearDepthStencilView(
        m_FrameContext.output.pDepthStencilView,
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
        1.0f,
        0
    );
    //Select CB
    pd3dDeviceContext->OMSetRenderTargets(GBUFFER_RTV_COUNT, m_pRTVs, m_FrameContext.output.pDepthStencilView);
    pd3dDeviceContext->RSSetState(m_pRasterizerState);

    pd3dDeviceContext->RSSetViewports(1, m_FrameContext.output.pViewport);

    pd3dDeviceContext->IASetInputLayout(m_pInputLayout);

    pd3dDeviceContext->VSSetShader(m_pGeometryVS, NULL, 0);
    pd3dDeviceContext->PSSetShader(m_pGeometryPS, NULL, 0);
    pd3dDeviceContext->PSSetConstantBuffers(0, 1, &m_pGlobalConstantBuffer);

    m_pMeshView->Render(pd3dDeviceContext);

    if (m_pMotionVectorVS != NULL && m_pMotionVectorPS != NULL && m_pRTVs[GBUFFER_RTV_MOTION_VECTOR] != NULL)
    {
        ID3D11RenderTargetView* pMotionVectorRTV = m_pRTVs[GBUFFER_RTV_MOTION_VECTOR];
        pd3dDeviceContext->OMSetRenderTargets(1, &pMotionVectorRTV, m_FrameContext.output.pDepthStencilView);
        pd3dDeviceContext->RSSetViewports(1, m_FrameContext.output.pViewport);
        pd3dDeviceContext->VSSetShader(m_pMotionVectorVS, NULL, 0);
        pd3dDeviceContext->PSSetShader(m_pMotionVectorPS, NULL, 0);

        m_pMeshView->Render(pd3dDeviceContext);
    }

    return S_OK;
}

//void GBufferRenderPass::SetOutput( )
//{
//    /*m_FrameContext.output = output;
//
//    m_pRTVs[GBUFFER_RTV_POSITION] = m_FrameContext.output.pPositionRTV;
//    m_pRTVs[GBUFFER_RTV_NORMALS] = m_FrameContext.output.pNormalsRTV;
//    m_pRTVs[GBUFFER_RTV_TANGENT] = m_FrameContext.output.pTangentRTV;
//    m_pRTVs[GBUFFER_RTV_MOTION_VECTOR] = m_FrameContext.output.pMotionVectorRTV;
//
//    m_pSRVs[GBUFFER_RTV_POSITION] = m_FrameContext.output.pPositionSRV;
//    m_pSRVs[GBUFFER_RTV_NORMALS] = m_FrameContext.output.pNormalsSRV;
//    m_pSRVs[GBUFFER_RTV_TANGENT] = m_FrameContext.output.pTangentSRV;
//    m_pSRVs[GBUFFER_RTV_MOTION_VECTOR] = m_FrameContext.output.pMotionVectorSRV;*/
//}

void GBufferRenderPass::SetOutput(ID3D11RenderTargetView* prtvBackBuffer, ID3D11DepthStencilView* pdsvBackBuffer, D3D11_VIEWPORT* pViewport)
{
    m_FrameContext.output.pDepthStencilView = pdsvBackBuffer;
    m_FrameContext.output.pViewport = pViewport;
}

void GBufferRenderPass::SetMeshView(ISceneMesh* pMesh)
{
    m_pMeshView = pMesh;
}

ID3D11ShaderResourceView* GBufferRenderPass::GetPositionSRV() const
{
    return m_pSRVs[GBUFFER_RTV_POSITION];
}

ID3D11ShaderResourceView* GBufferRenderPass::GetNormalsSRV() const
{
    return m_pSRVs[GBUFFER_RTV_NORMALS];
}

ID3D11ShaderResourceView* GBufferRenderPass::GetTangentSRV() const
{
    return m_pSRVs[GBUFFER_RTV_TANGENT];
}

ID3D11ShaderResourceView* GBufferRenderPass::GetMotionVectorSRV() const
{
    return m_pSRVs[GBUFFER_RTV_MOTION_VECTOR];
}

void GBufferRenderPass::SetCameraContext(CFirstPersonCamera* pViewerCamera, CFirstPersonCamera* pLightCamera, CAMERA_SELECTION selectedCamera)
{
    m_FrameContext.pViewerCamera = pViewerCamera;
    m_FrameContext.pLightCamera = pLightCamera;
    m_FrameContext.selectedCamera = selectedCamera;
}



HRESULT  GBufferRenderPass::ReleaseGBufferResources(
    ID3D11Texture2D** pAuxGBufferTextures,
    ID3D11RenderTargetView** pAuxGBufferRTVs,
    ID3D11ShaderResourceView** pAuxGBufferSRVs)
{
    for (INT index = 0; index < MAX_GBUFFER_RTV; ++index)
    {
        if (pAuxGBufferSRVs[index])
        {
            pAuxGBufferSRVs[index]->Release();
            pAuxGBufferSRVs[index] = nullptr;
        }

        if (pAuxGBufferRTVs[index])
        {
            pAuxGBufferRTVs[index]->Release();
            pAuxGBufferRTVs[index] = nullptr;
        }

        if (pAuxGBufferTextures[index])
        {
            pAuxGBufferTextures[index]->Release();
            pAuxGBufferTextures[index] = nullptr;
        }
    }

    return S_OK;
}

HRESULT GBufferRenderPass::CreateGBufferResources(
    ID3D11Device* pd3dDevice,
    UINT width,
    UINT height,
    ID3D11Texture2D** pAuxGBufferTextures,
    ID3D11RenderTargetView** pAuxGBufferRTVs,
    ID3D11ShaderResourceView** pAuxGBufferSRVs)
{
    if (!pd3dDevice || width == 0 || height == 0 ||
        !pAuxGBufferTextures || !pAuxGBufferRTVs || !pAuxGBufferSRVs)
    {
        return E_INVALIDARG;
    }

    ReleaseGBufferResources(pAuxGBufferTextures, pAuxGBufferRTVs, pAuxGBufferSRVs);

    const DXGI_FORMAT formats[4] =
    {
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        DXGI_FORMAT_R16G16B16A16_FLOAT
    };

    for (INT index = 0; index < MAX_GBUFFER_RTV; ++index)
    {
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = formats[index];
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        HRESULT hr = pd3dDevice->CreateTexture2D(&texDesc, nullptr, &pAuxGBufferTextures[index]);
        if (FAILED(hr))
        {
            ReleaseGBufferResources(pAuxGBufferTextures, pAuxGBufferRTVs, pAuxGBufferSRVs);
            return hr;
        }

        hr = pd3dDevice->CreateRenderTargetView(pAuxGBufferTextures[index], nullptr, &pAuxGBufferRTVs[index]);
        if (FAILED(hr))
        {
            ReleaseGBufferResources(pAuxGBufferTextures, pAuxGBufferRTVs, pAuxGBufferSRVs);
            return hr;
        }

        hr = pd3dDevice->CreateShaderResourceView(pAuxGBufferTextures[index], nullptr, &pAuxGBufferSRVs[index]);
        if (FAILED(hr))
        {
            ReleaseGBufferResources(pAuxGBufferTextures, pAuxGBufferRTVs, pAuxGBufferSRVs);
            return hr;
        }
    }

    return S_OK;
}