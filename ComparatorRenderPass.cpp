#include "DXUT.h"

#include "ComparatorRenderPass.h"

#include "SDKmisc.h"
#include "ShadowSampleMisc.h"

ComparatorRenderPass::ComparatorRenderPass()
    : m_pLeftSRV( NULL )
    , m_pRightSRV( NULL )
    , m_pVS( NULL )
    , m_pPS( NULL )
    , m_pVSBlob( NULL )
    , m_pPSBlob( NULL )
    , m_pPointSampler( NULL )
    , m_pDepthDisabledState( NULL )
    , m_fSplit( 0.5f )
    , m_bEnabled( false )
{
}

ComparatorRenderPass::~ComparatorRenderPass()
{
    Destroy();
}

const char* ComparatorRenderPass::GetPassName() const
{
    return "Comparator";
}

HRESULT ComparatorRenderPass::Create( ID3D11Device* pd3dDevice )
{
    if( pd3dDevice == NULL )
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    WCHAR shaderFile[] = L"ComparatorFullscreen.hlsl";

    if( m_pVSBlob == NULL )
    {
        V_RETURN( CompileShaderFromFile( shaderFile, NULL, "VSMain", "vs_5_0", &m_pVSBlob ) );
    }
    if( m_pPSBlob == NULL )
    {
        V_RETURN( CompileShaderFromFile( shaderFile, NULL, "PSMain", "ps_5_0", &m_pPSBlob ) );
    }
    if( m_pVS == NULL )
    {
        V_RETURN( pd3dDevice->CreateVertexShader( m_pVSBlob->GetBufferPointer(), m_pVSBlob->GetBufferSize(), NULL, &m_pVS ) );
        DXUT_SetDebugName( m_pVS, "ComparatorRenderPass_VS" );
    }
    if( m_pPS == NULL )
    {
        V_RETURN( pd3dDevice->CreatePixelShader( m_pPSBlob->GetBufferPointer(), m_pPSBlob->GetBufferSize(), NULL, &m_pPS ) );
        DXUT_SetDebugName( m_pPS, "ComparatorRenderPass_PS" );
    }
    if( m_ConstantBuffer.GetBuffer() == NULL )
    {
        V_RETURN( DX::Buffer::CreateConstant(
            pd3dDevice,
            sizeof( ComparatorConstants ),
            0,
            "ComparatorRenderPass_CB",
            m_ConstantBuffer ) );
    }
    if( m_pPointSampler == NULL )
    {
        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        V_RETURN( pd3dDevice->CreateSamplerState( &samplerDesc, &m_pPointSampler ) );
        DXUT_SetDebugName( m_pPointSampler, "ComparatorRenderPass_PointSampler" );
    }
    if( m_pDepthDisabledState == NULL )
    {
        D3D11_DEPTH_STENCIL_DESC depthDesc = {};
        depthDesc.DepthEnable = FALSE;
        depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        depthDesc.StencilEnable = FALSE;
        V_RETURN( pd3dDevice->CreateDepthStencilState( &depthDesc, &m_pDepthDisabledState ) );
        DXUT_SetDebugName( m_pDepthDisabledState, "ComparatorRenderPass_DepthDisabled" );
    }

    return hr;
}

void ComparatorRenderPass::Destroy()
{
    SAFE_RELEASE( m_pDepthDisabledState );
    SAFE_RELEASE( m_pPointSampler );
    m_ConstantBuffer.Destroy();
    SAFE_RELEASE( m_pPS );
    SAFE_RELEASE( m_pVS );
    SAFE_RELEASE( m_pPSBlob );
    SAFE_RELEASE( m_pVSBlob );

    m_pLeftSRV = NULL;
    m_pRightSRV = NULL;
    m_fSplit = 0.5f;
    m_bEnabled = false;
}

HRESULT ComparatorRenderPass::Execute( ID3D11DeviceContext* pd3dDeviceContext )
{
    if( !m_bEnabled || pd3dDeviceContext == NULL || m_Output.pRenderTargetView == NULL || m_Output.pViewport == NULL ||
        m_pLeftSRV == NULL || m_pRightSRV == NULL || m_pVS == NULL || m_pPS == NULL ||
        m_ConstantBuffer.GetBuffer() == NULL || m_pPointSampler == NULL || m_pDepthDisabledState == NULL )
    {
        return S_OK;
    }

    HRESULT hr = S_OK;
    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
    V_RETURN( DX::Buffer::MapWriteDiscard( pd3dDeviceContext, m_ConstantBuffer, mappedResource ) );

    ComparatorConstants* pConstants = reinterpret_cast<ComparatorConstants*>( mappedResource.pData );
    pConstants->vViewportSizeAndSplit = D3DXVECTOR4(
        m_Output.pViewport->Width,
        m_Output.pViewport->Height,
        max( 0.0f, min( 1.0f, m_fSplit ) ),
        0.0f );
    pConstants->vLineWidthHandleRadius = D3DXVECTOR4( 2.0f, 8.0f, 0.0f, 0.0f );
    pConstants->vLineColor = D3DXVECTOR4( 1.0f, 0.12f, 0.12f, 1.0f );
    DX::Buffer::Unmap( pd3dDeviceContext, m_ConstantBuffer );

    ID3D11RenderTargetView* pRTV = m_Output.pRenderTargetView;
    ID3D11ShaderResourceView* pSRVs[2] = { m_pLeftSRV, m_pRightSRV };

    pd3dDeviceContext->OMSetRenderTargets( 1, &pRTV, NULL );
    pd3dDeviceContext->OMSetDepthStencilState( m_pDepthDisabledState, 0 );
    pd3dDeviceContext->RSSetViewports( 1, m_Output.pViewport );
    pd3dDeviceContext->IASetInputLayout( NULL );
    pd3dDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    pd3dDeviceContext->VSSetShader( m_pVS, NULL, 0 );
    pd3dDeviceContext->PSSetShader( m_pPS, NULL, 0 );
    pd3dDeviceContext->GSSetShader( NULL, NULL, 0 );
    DX::Buffer::BindConstantPS( pd3dDeviceContext, m_ConstantBuffer );
    pd3dDeviceContext->PSSetShaderResources( 0, 2, pSRVs );
    pd3dDeviceContext->PSSetSamplers( 0, 1, &m_pPointSampler );
    pd3dDeviceContext->Draw( 3, 0 );

    ID3D11ShaderResourceView* nullSRVs[2] = { NULL, NULL };
    pd3dDeviceContext->PSSetShaderResources( 0, 2, nullSRVs );
    pd3dDeviceContext->OMSetDepthStencilState( NULL, 0 );

    return hr;
}

void ComparatorRenderPass::SetEnabled( bool enabled )
{
    m_bEnabled = enabled;
}

bool ComparatorRenderPass::IsEnabled() const
{
    return m_bEnabled;
}

void ComparatorRenderPass::SetOutput( ID3D11RenderTargetView* pRenderTargetView, D3D11_VIEWPORT* pViewport )
{
    m_Output.pRenderTargetView = pRenderTargetView;
    m_Output.pViewport = pViewport;
}

void ComparatorRenderPass::SetInputs( ID3D11ShaderResourceView* pLeftSRV, ID3D11ShaderResourceView* pRightSRV )
{
    m_pLeftSRV = pLeftSRV;
    m_pRightSRV = pRightSRV;
}

void ComparatorRenderPass::SetSplit( float split )
{
    m_fSplit = split;
}
