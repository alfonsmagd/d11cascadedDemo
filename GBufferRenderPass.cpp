#include "DXUT.h"

#include "GBufferRenderPass.h"
#include "SDKmisc.h"
#include "DXUTcamera.h"



GBufferRenderPass::GBufferRenderPass()
    : m_pMeshView(NULL)
    , m_pVisibleSubMeshes(NULL)
    , m_pGeometryVS(NULL)
    , m_pGeometryPS(NULL)
    , m_pGeometryVSBlob(NULL)
    , m_pGeometryPSBlob(NULL)
    , m_pMotionVectorVS(NULL)
    , m_pMotionVectorPS(NULL)
    , m_pMotionVectorVSBlob(NULL)
    , m_pMotionVectorPSBlob(NULL)
    , m_pRasterizerState(NULL)
    , m_pGlobalConstantBuffer(NULL)
    , m_pInputLayout(NULL)
{
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
    m_pGlobalConstantBuffer = NULL;
    m_pInputLayout = NULL;
    m_pVisibleSubMeshes = NULL;

    ReleaseOwnedTargets();
}

void GBufferRenderPass::ReleaseOwnedTargets()
{
    for( INT rtvIndex = 0; rtvIndex < GBUFFER_RTV_COUNT; ++rtvIndex )
    {
        m_GBufferTargets[rtvIndex].Destroy();
    }
}

HRESULT GBufferRenderPass::Resize( ID3D11Device* pd3dDevice, UINT width, UINT height )
{
    if( !pd3dDevice || width == 0 || height == 0 )
    {
        return E_INVALIDARG;
    }

    ReleaseOwnedTargets();

    const DXGI_FORMAT formats[GBUFFER_RTV_COUNT] =
    {
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        DXGI_FORMAT_R16G16B16A16_FLOAT
    };
    const char* debugNames[GBUFFER_RTV_COUNT] =
    {
        "GBuffer_Position",
        "GBuffer_Normals",
        "GBuffer_Tangent",
        "GBuffer_MotionVector"
    };

    for( INT index = 0; index < GBUFFER_RTV_COUNT; ++index )
    {
        HRESULT hr = DX::Texture::CreateColorTarget2D(
            pd3dDevice,
            width,
            height,
            formats[index],
            debugNames[index],
            m_GBufferTargets[index] );
        if( FAILED( hr ) )
        {
            ReleaseOwnedTargets();
            return hr;
        }
    }

    return S_OK;
}

HRESULT GBufferRenderPass::Execute(ID3D11DeviceContext* pd3dDeviceContext)
{
    HRESULT hr = S_OK;
    if (pd3dDeviceContext == NULL)
    {
        return E_INVALIDARG;
    }

    if( m_FrameContext.output.pDepthStencilView == NULL || m_FrameContext.output.pViewport == NULL )
    {
        return S_FALSE;
    }

    for (INT rtvIndex = 0; rtvIndex < GBUFFER_RTV_COUNT; ++rtvIndex)
    {
        if (m_GBufferTargets[rtvIndex].GetRTV() == NULL)
        {
            return S_FALSE;
        }
    }

    if (m_pMeshView == NULL || !m_pMeshView->IsLoaded())
    {
        return S_FALSE;
    }

    if( m_pGeometryVS == NULL || m_pGeometryPS == NULL || m_pGlobalConstantBuffer == NULL || m_pInputLayout == NULL || m_FrameContext.pViewerCamera == NULL )
    {
        return S_FALSE;
    }


    D3DXMATRIX dxmatCameraProj;
    D3DXMATRIX dxmatCameraView;
    ResolveCameraMatrices( dxmatCameraView, dxmatCameraProj );
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
    V_RETURN( pd3dDeviceContext->Map( m_pGlobalConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
    CB_ALL_SHADOW_DATA* pcbAllShadowConstants = (CB_ALL_SHADOW_DATA*)MappedResource.pData;
    D3DXMatrixTranspose(&pcbAllShadowConstants->m_WorldViewProj, &dxmatWorldViewProjection);
    D3DXMatrixTranspose(&pcbAllShadowConstants->m_World, &dxmatWorld);
    D3DXMatrixTranspose(&pcbAllShadowConstants->m_WorldView, &dxmatWorldView);
    pd3dDeviceContext->Unmap(m_pGlobalConstantBuffer, 0);

    //Clear SRV And RTVS
    ID3D11ShaderResourceView* nullSRVs[16] = {};
    pd3dDeviceContext->VSSetShaderResources(0, 16, nullSRVs);
    pd3dDeviceContext->PSSetShaderResources(0, 16, nullSRVs);

    FLOAT clearColor[4] = { 0, 0, 0, 1 };
    for (INT i = 0; i < GBUFFER_RTV_COUNT; ++i)
    {
        pd3dDeviceContext->ClearRenderTargetView(m_GBufferTargets[i].GetRTV(), clearColor);
    }
    pd3dDeviceContext->ClearDepthStencilView(
        m_FrameContext.output.pDepthStencilView,
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
        1.0f,
        0
    );
    //Select CB
    ID3D11RenderTargetView* pRTVs[GBUFFER_RTV_COUNT] =
    {
        m_GBufferTargets[GBUFFER_RTV_POSITION].GetRTV(),
        m_GBufferTargets[GBUFFER_RTV_NORMALS].GetRTV(),
        m_GBufferTargets[GBUFFER_RTV_TANGENT].GetRTV(),
        m_GBufferTargets[GBUFFER_RTV_MOTION_VECTOR].GetRTV()
    };
    pd3dDeviceContext->OMSetRenderTargets(GBUFFER_RTV_COUNT, pRTVs, m_FrameContext.output.pDepthStencilView);
    pd3dDeviceContext->RSSetState(m_pRasterizerState);

    pd3dDeviceContext->RSSetViewports(1, m_FrameContext.output.pViewport);

    pd3dDeviceContext->IASetInputLayout( m_pInputLayout );

    pd3dDeviceContext->VSSetShader(m_pGeometryVS, NULL, 0);
    pd3dDeviceContext->PSSetShader(m_pGeometryPS, NULL, 0);
    pd3dDeviceContext->VSSetConstantBuffers( 0, 1, &m_pGlobalConstantBuffer );
    pd3dDeviceContext->PSSetConstantBuffers( 0, 1, &m_pGlobalConstantBuffer );

    m_pMeshView->RenderSubMeshes( pd3dDeviceContext, m_pVisibleSubMeshes );

    return S_OK;
}

void GBufferRenderPass::SetOutput(ID3D11RenderTargetView* prtvBackBuffer, ID3D11DepthStencilView* pdsvBackBuffer, D3D11_VIEWPORT* pViewport)
{
    UNREFERENCED_PARAMETER( prtvBackBuffer );
    m_FrameContext.output.pDepthStencilView = pdsvBackBuffer;
    m_FrameContext.output.pViewport = pViewport;
}

void GBufferRenderPass::SetMeshView(ISceneMesh* pMesh)
{
    m_pMeshView = pMesh;
}

void GBufferRenderPass::SetVisibleSubMeshes( const std::vector<INT>* pVisibleSubMeshes )
{
    m_pVisibleSubMeshes = pVisibleSubMeshes;
}

const DX::Texture::Resource2D* GBufferRenderPass::GetPositionTexture() const
{
    return &m_GBufferTargets[GBUFFER_RTV_POSITION];
}

const DX::Texture::Resource2D* GBufferRenderPass::GetNormalsTexture() const
{
    return &m_GBufferTargets[GBUFFER_RTV_NORMALS];
}

const DX::Texture::Resource2D* GBufferRenderPass::GetTangentTexture() const
{
    return &m_GBufferTargets[GBUFFER_RTV_TANGENT];
}

const DX::Texture::Resource2D* GBufferRenderPass::GetMotionVectorTexture() const
{
    return &m_GBufferTargets[GBUFFER_RTV_MOTION_VECTOR];
}

ID3D11ShaderResourceView* GBufferRenderPass::GetPositionSRV() const
{
    return GetPositionTexture()->GetSRV();
}

ID3D11ShaderResourceView* GBufferRenderPass::GetNormalsSRV() const
{
    return GetNormalsTexture()->GetSRV();
}

ID3D11ShaderResourceView* GBufferRenderPass::GetTangentSRV() const
{
    return GetTangentTexture()->GetSRV();
}

ID3D11ShaderResourceView* GBufferRenderPass::GetMotionVectorSRV() const
{
    return GetMotionVectorTexture()->GetSRV();
}

void GBufferRenderPass::SetCameraContext(CFirstPersonCamera* pViewerCamera, CFirstPersonCamera* pLightCamera, CAMERA_SELECTION selectedCamera)
{
    m_FrameContext.pViewerCamera = pViewerCamera;
    m_FrameContext.pLightCamera = pLightCamera;
    m_FrameContext.selectedCamera = selectedCamera;
}

void GBufferRenderPass::ResolveCameraMatrices( D3DXMATRIX& dxmatCameraView, D3DXMATRIX& dxmatCameraProj ) const
{
    dxmatCameraProj = *m_FrameContext.pViewerCamera->GetProjMatrix();
    dxmatCameraView = *m_FrameContext.pViewerCamera->GetViewMatrix();

    if( m_FrameContext.selectedCamera == LIGHT_CAMERA && m_FrameContext.pLightCamera != NULL )
    {
        dxmatCameraProj = *m_FrameContext.pLightCamera->GetProjMatrix();
        dxmatCameraView = *m_FrameContext.pLightCamera->GetViewMatrix();
    }
    else if( m_FrameContext.selectedCamera >= ORTHO_CAMERA1 )
    {
        const INT cascadeIndex = INT( m_FrameContext.selectedCamera ) - INT( ORTHO_CAMERA1 );
        if( cascadeIndex >= 0 && cascadeIndex < MAX_CASCADES )
        {
            dxmatCameraProj = m_FrameContext.matShadowProj[cascadeIndex];
            dxmatCameraView = m_FrameContext.matShadowView;
        }
    }
}
