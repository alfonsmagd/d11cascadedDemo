#include "DXUT.h"

#include "DeferredDecalRenderPass.h"

#include <algorithm>
#include <vector>

#include "DXUTcamera.h"
#include "SDKmisc.h"
#include "ShadowSampleMisc.h"

DeferredDecalRenderPass::DeferredDecalRenderPass()
    : m_pMeshView( NULL )
    , m_pCamera( NULL )
    , m_pPositionTexture( NULL )
    , m_pNormalTexture( NULL )
    , m_pVS( NULL )
    , m_pPS( NULL )
    , m_pVSBlob( NULL )
    , m_pPSBlob( NULL )
    , m_pWireBoxVS( NULL )
    , m_pWireBoxPS( NULL )
    , m_pWireBoxVSBlob( NULL )
    , m_pWireBoxPSBlob( NULL )
    , m_pPointSampler( NULL )
    , m_pBlendState( NULL )
    , m_pDepthStencilState( NULL )
    , m_vPickedDecalPosition( 0.0f, 0.0f, 0.0f )
    , m_bHasPickedDecalPosition( false )
    , m_bEnabled( true )
{
}

DeferredDecalRenderPass::~DeferredDecalRenderPass()
{
    Destroy();
}

const char* DeferredDecalRenderPass::GetPassName() const
{
    return "DeferredDecal";
}

HRESULT DeferredDecalRenderPass::Create( ID3D11Device* pd3dDevice )
{
    if( pd3dDevice == NULL )
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    WCHAR shaderFile[] = L"DeferredDecal.hlsl";

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
        DXUT_SetDebugName( m_pVS, "DeferredDecalRenderPass_VS" );
    }
    if( m_pPS == NULL )
    {
        V_RETURN( pd3dDevice->CreatePixelShader( m_pPSBlob->GetBufferPointer(), m_pPSBlob->GetBufferSize(), NULL, &m_pPS ) );
        DXUT_SetDebugName( m_pPS, "DeferredDecalRenderPass_PS" );
    }
    if( m_ConstantBuffer.GetBuffer() == NULL )
    {
        V_RETURN( DX::Buffer::CreateConstant(
            pd3dDevice,
            sizeof( DeferredDecalConstants ),
            0,
            "DeferredDecalRenderPass_CB",
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
        DXUT_SetDebugName( m_pPointSampler, "DeferredDecalRenderPass_PointSampler" );
    }
    if( m_pBlendState == NULL )
    {
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
        V_RETURN( pd3dDevice->CreateBlendState( &blendDesc, &m_pBlendState ) );
        DXUT_SetDebugName( m_pBlendState, "DeferredDecalRenderPass_BlendState" );
    }
    if( m_pDepthStencilState == NULL )
    {
        D3D11_DEPTH_STENCIL_DESC depthDesc = {};
        depthDesc.DepthEnable = FALSE;
        depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        depthDesc.StencilEnable = FALSE;
        V_RETURN( pd3dDevice->CreateDepthStencilState( &depthDesc, &m_pDepthStencilState ) );
        DXUT_SetDebugName( m_pDepthStencilState, "DeferredDecalRenderPass_DepthState" );
    }

    V_RETURN( CreateExampleDecalTexture( pd3dDevice ) );
    V_RETURN( CreateWireBoxResources( pd3dDevice ) );

    return hr;
}

void DeferredDecalRenderPass::Destroy()
{
    m_WireBoxBuffer.Destroy();
    m_WireBoxConstantBuffer.Destroy();
    SAFE_RELEASE( m_pWireBoxPS );
    SAFE_RELEASE( m_pWireBoxVS );
    SAFE_RELEASE( m_pWireBoxPSBlob );
    SAFE_RELEASE( m_pWireBoxVSBlob );
    m_ExampleDecalTexture.Destroy();
    SAFE_RELEASE( m_pDepthStencilState );
    SAFE_RELEASE( m_pBlendState );
    SAFE_RELEASE( m_pPointSampler );
    m_ConstantBuffer.Destroy();
    SAFE_RELEASE( m_pPS );
    SAFE_RELEASE( m_pVS );
    SAFE_RELEASE( m_pPSBlob );
    SAFE_RELEASE( m_pVSBlob );

    m_pMeshView = NULL;
    m_pCamera = NULL;
    m_pPositionTexture = NULL;
    m_pNormalTexture = NULL;
    m_bHasPickedDecalPosition = false;
    m_vPickedDecalPosition = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
}

HRESULT DeferredDecalRenderPass::CreateExampleDecalTexture( ID3D11Device* pd3dDevice )
{
    if( m_ExampleDecalTexture.pResource != NULL && m_ExampleDecalTexture.pSRV != NULL )
    {
        return S_OK;
    }

    const UINT tileSize = 18;
    const UINT tileCountX = 8;
    const UINT tileCountY = 8;
    const UINT textureWidth = tileSize * tileCountX;
    const UINT textureHeight = tileSize * tileCountY;
    std::vector<UINT> checkerData( textureWidth * textureHeight, 0 );

    for( UINT y = 0; y < textureHeight; ++y )
    {
        for( UINT x = 0; x < textureWidth; ++x )
        {
            const UINT tileX = x / tileSize;
            const UINT tileY = y / tileSize;
            const bool isWhiteTile = ( ( tileX + tileY ) % 2 ) != 0;
            checkerData[y * textureWidth + x] = isWhiteTile ? 0xFFFFFFFF : 0xFF000000;
        }
    }

    return DX::Texture::CreateImmutableShaderTexture2D(
        pd3dDevice,
        textureWidth,
        textureHeight,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        checkerData.data(),
        textureWidth * sizeof( UINT ),
        "DeferredDecal_ExampleTexture",
        m_ExampleDecalTexture );
}

HRESULT DeferredDecalRenderPass::CreateWireBoxResources( ID3D11Device* pd3dDevice )
{
    HRESULT hr = S_OK;
    WCHAR shaderFile[] = L"DebugBoundingBox.hlsl";

    if( m_pWireBoxVSBlob == NULL )
    {
        V_RETURN( CompileShaderFromFile( shaderFile, NULL, "VSMain", "vs_5_0", &m_pWireBoxVSBlob ) );
    }
    if( m_pWireBoxPSBlob == NULL )
    {
        V_RETURN( CompileShaderFromFile( shaderFile, NULL, "PSMain", "ps_5_0", &m_pWireBoxPSBlob ) );
    }
    if( m_pWireBoxVS == NULL )
    {
        V_RETURN( pd3dDevice->CreateVertexShader( m_pWireBoxVSBlob->GetBufferPointer(), m_pWireBoxVSBlob->GetBufferSize(), NULL, &m_pWireBoxVS ) );
        DXUT_SetDebugName( m_pWireBoxVS, "DeferredDecalRenderPass_WireBoxVS" );
    }
    if( m_pWireBoxPS == NULL )
    {
        V_RETURN( pd3dDevice->CreatePixelShader( m_pWireBoxPSBlob->GetBufferPointer(), m_pWireBoxPSBlob->GetBufferSize(), NULL, &m_pWireBoxPS ) );
        DXUT_SetDebugName( m_pWireBoxPS, "DeferredDecalRenderPass_WireBoxPS" );
    }
    if( m_WireBoxConstantBuffer.GetBuffer() == NULL )
    {
        V_RETURN( DX::Buffer::CreateConstant(
            pd3dDevice,
            sizeof( DeferredDecalDebugBoxConstants ),
            0,
            "DeferredDecalRenderPass_WireBoxCB",
            m_WireBoxConstantBuffer ) );
    }
    if( m_WireBoxBuffer.GetBuffer() == NULL )
    {
        V_RETURN( DX::Buffer::CreateStructuredReadOnly(
            pd3dDevice,
            1,
            sizeof( BoundingBox ),
            "DeferredDecalRenderPass_WireBoxBuffer",
            m_WireBoxBuffer ) );
    }

    return hr;
}

HRESULT DeferredDecalRenderPass::Execute( ID3D11DeviceContext* pd3dDeviceContext )
{
    ID3D11ShaderResourceView* pPositionSRV = m_pPositionTexture ? m_pPositionTexture->GetSRV() : NULL;
    ID3D11ShaderResourceView* pNormalSRV = m_pNormalTexture ? m_pNormalTexture->GetSRV() : NULL;

    if( !m_bEnabled || pd3dDeviceContext == NULL || m_pMeshView == NULL || !m_pMeshView->IsLoaded() ||
        m_Output.pRenderTargetView == NULL || m_Output.pViewport == NULL ||
        pPositionSRV == NULL || pNormalSRV == NULL || m_ExampleDecalTexture.GetSRV() == NULL ||
        m_pVS == NULL || m_pPS == NULL || m_ConstantBuffer.GetBuffer() == NULL || m_pPointSampler == NULL ||
        m_pBlendState == NULL || m_pDepthStencilState == NULL ||
        m_pWireBoxVS == NULL || m_pWireBoxPS == NULL || m_WireBoxConstantBuffer.GetBuffer() == NULL ||
        m_WireBoxBuffer.GetBuffer() == NULL || m_WireBoxBuffer.GetSRV() == NULL )
    {
        return S_OK;
    }

    XMFLOAT3 aabbMin;
    XMFLOAT3 aabbMax;
    XMStoreFloat3( &aabbMin, m_pMeshView->GetAABBMin() );
    XMStoreFloat3( &aabbMax, m_pMeshView->GetAABBMax() );

    D3DXVECTOR3 sceneCenter(
        ( aabbMin.x + aabbMax.x ) * 0.5f,
        aabbMin.y + 0.15f,
        ( aabbMin.z + aabbMax.z ) * 0.5f );

    if( m_bHasPickedDecalPosition )
    {
        sceneCenter = m_vPickedDecalPosition;
    }

    const float sceneExtentX = ( std::max )( aabbMax.x - aabbMin.x, 0.1f );
    const float sceneExtentY = ( std::max )( aabbMax.y - aabbMin.y, 0.1f );
    const float sceneExtentZ = ( std::max )( aabbMax.z - aabbMin.z, 0.1f );
    const D3DXVECTOR3 decalHalfSize(
        ( std::max )( 0.75f, sceneExtentX * 0.12f ),
        ( std::max )( 2.5f, sceneExtentY * 0.50f ),
        ( std::max )( 0.75f, sceneExtentZ * 0.12f ) );
    const BoundingBox decalDebugBox = MakeBoundingBoxFromMinMax(
        sceneCenter - decalHalfSize,
        sceneCenter + decalHalfSize,
        XMFLOAT4( 1.0f, 0.0f, 0.0f, 1.0f ) );

    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
    HRESULT hr = DX::Buffer::MapWriteDiscard( pd3dDeviceContext, m_ConstantBuffer, mappedResource );
    if( FAILED( hr ) )
    {
        return hr;
    }

    DeferredDecalConstants* pConstants = reinterpret_cast<DeferredDecalConstants*>( mappedResource.pData );
    pConstants->vDecalCenter = D3DXVECTOR4( sceneCenter.x, sceneCenter.y, sceneCenter.z, 1.0f );
    pConstants->vDecalHalfSize = D3DXVECTOR4( decalHalfSize.x, decalHalfSize.y, decalHalfSize.z, 1.0f );
    pConstants->vProjectionNormalOpacity = D3DXVECTOR4( 0.0f, 1.0f, 0.0f, 0.65f );
    pConstants->vDecalParams = D3DXVECTOR4( 0.15f, 0.0f, 0.0f, 0.0f );
    DX::Buffer::Unmap( pd3dDeviceContext, m_ConstantBuffer );

    const float blendFactor[4] = { 0, 0, 0, 0 };
    ID3D11RenderTargetView* pRenderTargetView = m_Output.pRenderTargetView;
    ID3D11ShaderResourceView* pSRVs[3] = { pPositionSRV, pNormalSRV, m_ExampleDecalTexture.GetSRV() };

    pd3dDeviceContext->OMSetRenderTargets( 1, &pRenderTargetView, NULL );
    pd3dDeviceContext->OMSetBlendState( m_pBlendState, blendFactor, 0xFFFFFFFF );
    pd3dDeviceContext->OMSetDepthStencilState( m_pDepthStencilState, 0 );
    pd3dDeviceContext->RSSetViewports( 1, m_Output.pViewport );
    pd3dDeviceContext->IASetInputLayout( NULL );
    pd3dDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    pd3dDeviceContext->VSSetShader( m_pVS, NULL, 0 );
    pd3dDeviceContext->PSSetShader( m_pPS, NULL, 0 );
    pd3dDeviceContext->GSSetShader( NULL, NULL, 0 );
    DX::Buffer::BindConstantPS( pd3dDeviceContext, m_ConstantBuffer );
    pd3dDeviceContext->PSSetShaderResources( 0, 3, pSRVs );
    pd3dDeviceContext->PSSetSamplers( 0, 1, &m_pPointSampler );
    pd3dDeviceContext->Draw( 3, 0 );

    ID3D11ShaderResourceView* nullSRVs[3] = { NULL, NULL, NULL };
    pd3dDeviceContext->PSSetShaderResources( 0, 3, nullSRVs );
    pd3dDeviceContext->OMSetBlendState( NULL, blendFactor, 0xFFFFFFFF );
    pd3dDeviceContext->OMSetDepthStencilState( NULL, 0 );

    if( m_pCamera != NULL )
    {
        pd3dDeviceContext->UpdateSubresource( m_WireBoxBuffer.GetBuffer(), 0, NULL, &decalDebugBox, 0, 0 );

        D3DXMATRIX dxmatWorldViewProjection = ( *m_pCamera->GetViewMatrix() ) * ( *m_pCamera->GetProjMatrix() );
        D3D11_MAPPED_SUBRESOURCE debugMappedResource = {};
        hr = DX::Buffer::MapWriteDiscard( pd3dDeviceContext, m_WireBoxConstantBuffer, debugMappedResource );
        if( FAILED( hr ) )
        {
            return hr;
        }

        DeferredDecalDebugBoxConstants* pDebugConstants =
            reinterpret_cast<DeferredDecalDebugBoxConstants*>( debugMappedResource.pData );
        D3DXMatrixTranspose( &pDebugConstants->m_WorldViewProj, &dxmatWorldViewProjection );
        DX::Buffer::Unmap( pd3dDeviceContext, m_WireBoxConstantBuffer );

        ID3D11ShaderResourceView* pWireBoxSRV = m_WireBoxBuffer.GetSRV();
        pd3dDeviceContext->OMSetRenderTargets( 1, &pRenderTargetView, NULL );
        pd3dDeviceContext->RSSetViewports( 1, m_Output.pViewport );
        pd3dDeviceContext->IASetInputLayout( NULL );
        pd3dDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_LINELIST );
        pd3dDeviceContext->VSSetShader( m_pWireBoxVS, NULL, 0 );
        pd3dDeviceContext->PSSetShader( m_pWireBoxPS, NULL, 0 );
        pd3dDeviceContext->GSSetShader( NULL, NULL, 0 );
        DX::Buffer::BindConstantVS( pd3dDeviceContext, m_WireBoxConstantBuffer );
        pd3dDeviceContext->VSSetShaderResources( 0, 1, &pWireBoxSRV );
        pd3dDeviceContext->DrawInstanced( 24, 1, 0, 0 );

        ID3D11ShaderResourceView* pNullWireBoxSRV = NULL;
        pd3dDeviceContext->VSSetShaderResources( 0, 1, &pNullWireBoxSRV );
    }

    return S_OK;
}

void DeferredDecalRenderPass::SetEnabled( bool enabled )
{
    m_bEnabled = enabled;
}

bool DeferredDecalRenderPass::IsEnabled() const
{
    return m_bEnabled;
}

void DeferredDecalRenderPass::SetMeshView( ISceneMesh* pMesh )
{
    m_pMeshView = pMesh;
}

void DeferredDecalRenderPass::SetCameraContext( CFirstPersonCamera* pCamera )
{
    m_pCamera = pCamera;
}

void DeferredDecalRenderPass::SetOutput( ID3D11RenderTargetView* pRenderTargetView, D3D11_VIEWPORT* pViewport )
{
    m_Output.pRenderTargetView = pRenderTargetView;
    m_Output.pViewport = pViewport;
}

void DeferredDecalRenderPass::SetGBufferInputs( const DX::Texture::Resource2D* pPositionTexture, const DX::Texture::Resource2D* pNormalTexture )
{
    m_pPositionTexture = pPositionTexture;
    m_pNormalTexture = pNormalTexture;
}

void DeferredDecalRenderPass::SetPickedDecalPosition( const D3DXVECTOR3& worldPosition )
{
    m_vPickedDecalPosition = worldPosition;
    m_bHasPickedDecalPosition = true;
}

void DeferredDecalRenderPass::ClearPickedDecalPosition()
{
    m_bHasPickedDecalPosition = false;
    m_vPickedDecalPosition = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
}
