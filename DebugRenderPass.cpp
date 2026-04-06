#include "DXUT.h"

#include "DebugRenderPass.h"

#include <cassert>
#include <cfloat>

#include "DXUTcamera.h"
#include "SDKmisc.h"
#include "FrameContext.h"

DebugRenderPass* DebugRenderPass::s_pInstance = NULL;

namespace
{
    const XMFLOAT4 kSubMeshBoundingBoxColor = XMFLOAT4( 0.0f, 0.0f, 1.0f, 1.0f );
    const XMFLOAT4 kSelectedBoundingBoxColor = XMFLOAT4( 0.0f, 1.0f, 0.0f, 1.0f );
    const XMFLOAT4 kSelectionRingColor = XMFLOAT4( 0.10f, 1.00f, 0.65f, 1.0f );
    const float kSelectionMinObjectRadius = 0.25f;
    const float kSelectionOverlayBlendFactor[4] = { 0, 0, 0, 0 };
    const float kSelectionRingVerticalOffsetMin = 0.02f;
    const float kSelectionRingVerticalOffsetFactor = 0.02f;
    const float kSelectionRingRadiusScale = 1.15f;
    const float kSelectionRingQuadRadiusScale = 1.35f;
    const UINT kSelectionProceduralQuadVertexCount = 4;

    struct CB_DEBUG_BOUNDING_BOX
    {
        D3DXMATRIX m_WorldViewProj;
    };

    struct CB_SELECTION_RING
    {
        D3DXMATRIX m_ViewProj;
        D3DXVECTOR4 m_vRingCenterHalfSize;
        D3DXVECTOR4 m_vRingColorTime;
    };

    void RestoreBoundingBoxColors( std::vector<BoundingBox>& boxes, INT selectedIndex )
    {
        for( size_t index = 0; index < boxes.size(); ++index )
        {
            boxes[index].color = ( INT( index ) == selectedIndex ) ? kSelectedBoundingBoxColor : kSubMeshBoundingBoxColor;
        }
    }

    void ResolveCameraMatrices( const FrameContext& frameContext, D3DXMATRIX& dxmatCameraView, D3DXMATRIX& dxmatCameraProj )
    {
        dxmatCameraProj = *frameContext.pViewerCamera->GetProjMatrix();
        dxmatCameraView = *frameContext.pViewerCamera->GetViewMatrix();

        if( frameContext.selectedCamera == LIGHT_CAMERA )
        {
            dxmatCameraProj = *frameContext.pLightCamera->GetProjMatrix();
            dxmatCameraView = *frameContext.pLightCamera->GetViewMatrix();
        }
        else if( frameContext.selectedCamera >= ORTHO_CAMERA1 )
        {
            dxmatCameraProj = frameContext.matShadowProj[(INT)frameContext.selectedCamera - 2];
            dxmatCameraView = frameContext.matShadowView;
        }
    }
}

DebugSelectionModule::DebugSelectionModule( DebugSharedState& sharedState, const DebugFrameContext& frameContext )
    : m_SharedState( sharedState )
    , m_FrameContext( frameContext )
    , m_pBoundingBoxBuffer( NULL )
    , m_pBoundingBoxSRV( NULL )
    , m_pBoundingAllBoxBuffer( NULL )
    , m_pBoundingAllBoxSRV( NULL )
{
}

HRESULT DebugSelectionModule::Create( ID3D11Device* )
{
    return S_OK;
}

void DebugSelectionModule::Destroy()
{
    ReleaseResources();
    m_SharedState.sceneBoundingBoxes.clear();
    m_SharedState.allBoundingBoxes.clear();
    m_SharedState.sceneBoundingBoxCount = 0;
    m_SharedState.allBoundingBoxCount = 0;
    m_SharedState.selectedBoundingBox = -1;
}

HRESULT DebugSelectionModule::Execute( ID3D11DeviceContext* )
{
    return S_OK;
}

HRESULT DebugSelectionModule::HandleSceneChanged( ID3D11Device* pd3dDevice, ISceneMesh* pMesh )
{
    const bool meshChanged = ( m_SharedState.pMeshView != pMesh );
    m_SharedState.pMeshView = pMesh;
    if( meshChanged )
    {
        m_SharedState.selectedBoundingBox = -1;
    }

    if( !pd3dDevice )
    {
        return S_OK;
    }

    return UpdateResources( pd3dDevice );
}

HRESULT DebugSelectionModule::PickBoundingBox( ID3D11DeviceContext* pd3dDeviceContext, INT mouseX, INT mouseY, const D3D11_VIEWPORT& viewport )
{
    if( !pd3dDeviceContext || !m_SharedState.pMeshView || m_SharedState.allBoundingBoxes.empty() || !m_pBoundingAllBoxBuffer )
    {
        return S_FALSE;
    }

    D3DXMATRIX dxmatProj;
    D3DXMATRIX dxmatView;
    ResolveCameraMatrices( m_FrameContext, dxmatView, dxmatProj );

    const D3DVIEWPORT9 d3dxViewport =
    {
        DWORD( viewport.TopLeftX ),
        DWORD( viewport.TopLeftY ),
        DWORD( viewport.Width ),
        DWORD( viewport.Height ),
        viewport.MinDepth,
        viewport.MaxDepth
    };

    D3DXMATRIX dxmatIdentity;
    D3DXMatrixIdentity( &dxmatIdentity );

    D3DXVECTOR3 nearPoint( (FLOAT)mouseX, (FLOAT)mouseY, 0.0f );
    D3DXVECTOR3 farPoint( (FLOAT)mouseX, (FLOAT)mouseY, 1.0f );
    D3DXVECTOR3 worldNear;
    D3DXVECTOR3 worldFar;
    D3DXVec3Unproject( &worldNear, &nearPoint, &d3dxViewport, &dxmatProj, &dxmatView, &dxmatIdentity );
    D3DXVec3Unproject( &worldFar, &farPoint, &d3dxViewport, &dxmatProj, &dxmatView, &dxmatIdentity );

    D3DXVECTOR3 rayDirection = worldFar - worldNear;
    if( D3DXVec3LengthSq( &rayDirection ) <= 0.0f )
    {
        return S_FALSE;
    }
    D3DXVec3Normalize( &rayDirection, &rayDirection );

    INT pickedIndex = -1;
    float pickedDistance = FLT_MAX;
    m_SharedState.pMeshView->PickSubMesh( worldNear, rayDirection, pickedIndex, pickedDistance );

    if( pickedIndex == m_SharedState.selectedBoundingBox )
    {
        return S_OK;
    }

    m_SharedState.selectedBoundingBox = pickedIndex;
    RestoreBoundingBoxColors( m_SharedState.allBoundingBoxes, m_SharedState.selectedBoundingBox );
    if( !m_SharedState.allBoundingBoxes.empty() )
    {
        pd3dDeviceContext->UpdateSubresource( m_pBoundingAllBoxBuffer, 0, NULL, m_SharedState.allBoundingBoxes.data(), 0, 0 );
    }

    return S_OK;
}

ID3D11ShaderResourceView* DebugSelectionModule::GetSceneBoundingBoxSRV() const
{
    return m_pBoundingBoxSRV;
}

ID3D11ShaderResourceView* DebugSelectionModule::GetAllBoundingBoxSRV() const
{
    return m_pBoundingAllBoxSRV;
}

UINT DebugSelectionModule::GetSceneBoundingBoxCount() const
{
    return m_SharedState.sceneBoundingBoxCount;
}

UINT DebugSelectionModule::GetAllBoundingBoxCount() const
{
    return m_SharedState.allBoundingBoxCount;
}

HRESULT DebugSelectionModule::UpdateResources( ID3D11Device* pd3dDevice )
{
    HRESULT hr = S_OK;
    const INT preservedSelectedBoundingBox = m_SharedState.selectedBoundingBox;

    m_SharedState.sceneBoundingBoxes.clear();
    m_SharedState.allBoundingBoxes.clear();
    m_SharedState.sceneBoundingBoxCount = 0;
    m_SharedState.allBoundingBoxCount = 0;
    m_SharedState.selectedBoundingBox = -1;

    if( m_SharedState.pMeshView )
    {
        m_SharedState.pMeshView->UpdateAllBoundingBoxes( m_SharedState.allBoundingBoxes );
    }

    if( preservedSelectedBoundingBox >= 0 && preservedSelectedBoundingBox < INT( m_SharedState.allBoundingBoxes.size() ) )
    {
        m_SharedState.selectedBoundingBox = preservedSelectedBoundingBox;
    }

    RestoreBoundingBoxColors( m_SharedState.allBoundingBoxes, m_SharedState.selectedBoundingBox );
    ReleaseResources();

    const auto createBoundingBoxResources =
        [&]( const std::vector<BoundingBox>& boundingBoxes,
             ID3D11Buffer** ppBuffer,
             ID3D11ShaderResourceView** ppSRV,
             const char* bufferName,
             const char* srvName ) -> HRESULT
    {
        if( boundingBoxes.empty() )
        {
            return S_OK;
        }

        D3D11_BUFFER_DESC boxBufferDesc = {};
        boxBufferDesc.ByteWidth = UINT( sizeof( BoundingBox ) * boundingBoxes.size() );
        boxBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        boxBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        boxBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        boxBufferDesc.StructureByteStride = sizeof( BoundingBox );

        D3D11_SUBRESOURCE_DATA boxBufferData = {};
        boxBufferData.pSysMem = boundingBoxes.data();

        HRESULT localHr = pd3dDevice->CreateBuffer( &boxBufferDesc, &boxBufferData, ppBuffer );
        if( FAILED( localHr ) )
        {
            return localHr;
        }
        DXUT_SetDebugName( *ppBuffer, bufferName );

        D3D11_SHADER_RESOURCE_VIEW_DESC boxSRVDesc = {};
        boxSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
        boxSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        boxSRVDesc.Buffer.FirstElement = 0;
        boxSRVDesc.Buffer.NumElements = UINT( boundingBoxes.size() );

        localHr = pd3dDevice->CreateShaderResourceView( *ppBuffer, &boxSRVDesc, ppSRV );
        if( FAILED( localHr ) )
        {
            SAFE_RELEASE( *ppBuffer );
            return localHr;
        }
        DXUT_SetDebugName( *ppSRV, srvName );
        return S_OK;
    };

    V_RETURN( createBoundingBoxResources(
        m_SharedState.sceneBoundingBoxes,
        &m_pBoundingBoxBuffer,
        &m_pBoundingBoxSRV,
        "DebugSelection_SceneBoundingBoxBuffer",
        "DebugSelection_SceneBoundingBoxSRV" ) );

    V_RETURN( createBoundingBoxResources(
        m_SharedState.allBoundingBoxes,
        &m_pBoundingAllBoxBuffer,
        &m_pBoundingAllBoxSRV,
        "DebugSelection_AllBoundingBoxBuffer",
        "DebugSelection_AllBoundingBoxSRV" ) );

    m_SharedState.sceneBoundingBoxCount = UINT( m_SharedState.sceneBoundingBoxes.size() );
    m_SharedState.allBoundingBoxCount = UINT( m_SharedState.allBoundingBoxes.size() );
    return hr;
}

void DebugSelectionModule::ReleaseResources()
{
    SAFE_RELEASE( m_pBoundingBoxSRV );
    SAFE_RELEASE( m_pBoundingBoxBuffer );
    SAFE_RELEASE( m_pBoundingAllBoxSRV );
    SAFE_RELEASE( m_pBoundingAllBoxBuffer );
}

DebugBBoxRendererModule::DebugBBoxRendererModule( const DebugSharedState& sharedState,
                                                  const DebugFrameContext& frameContext,
                                                  const DebugSelectionModule& selectionModule )
    : m_SharedState( sharedState )
    , m_FrameContext( frameContext )
    , m_SelectionModule( selectionModule )
    , m_pVS( NULL )
    , m_pPS( NULL )
    , m_pVSBlob( NULL )
    , m_pPSBlob( NULL )
    , m_pCBWorldViewProj( NULL )
    , m_pRasterizerState( NULL )
{
}

HRESULT DebugBBoxRendererModule::Create( ID3D11Device* pd3dDevice )
{
    HRESULT hr = S_OK;
    WCHAR boundingBoxShaderFile[] = L"DebugBoundingBox.hlsl";

    if( m_pVSBlob == NULL )
    {
        V_RETURN( CompileShaderFromFile( boundingBoxShaderFile, NULL, "VSMain", "vs_5_0", &m_pVSBlob ) );
    }
    if( m_pPSBlob == NULL )
    {
        V_RETURN( CompileShaderFromFile( boundingBoxShaderFile, NULL, "PSMain", "ps_5_0", &m_pPSBlob ) );
    }
    if( m_pVS == NULL )
    {
        V_RETURN( pd3dDevice->CreateVertexShader( m_pVSBlob->GetBufferPointer(), m_pVSBlob->GetBufferSize(), NULL, &m_pVS ) );
        DXUT_SetDebugName( m_pVS, "DebugBBoxRendererModule_VS" );
    }
    if( m_pPS == NULL )
    {
        V_RETURN( pd3dDevice->CreatePixelShader( m_pPSBlob->GetBufferPointer(), m_pPSBlob->GetBufferSize(), NULL, &m_pPS ) );
        DXUT_SetDebugName( m_pPS, "DebugBBoxRendererModule_PS" );
    }
    if( m_pCBWorldViewProj == NULL )
    {
        D3D11_BUFFER_DESC desc = {};
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.ByteWidth = sizeof( CB_DEBUG_BOUNDING_BOX );
        V_RETURN( pd3dDevice->CreateBuffer( &desc, NULL, &m_pCBWorldViewProj ) );
        DXUT_SetDebugName( m_pCBWorldViewProj, "DebugBBoxRendererModule_CBWorldViewProj" );
    }
    if( m_pRasterizerState == NULL )
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
        V_RETURN( pd3dDevice->CreateRasterizerState( &rasterDesc, &m_pRasterizerState ) );
        DXUT_SetDebugName( m_pRasterizerState, "DebugBBoxRendererModule_Rasterizer" );
    }

    return hr;
}

void DebugBBoxRendererModule::Destroy()
{
    SAFE_RELEASE( m_pRasterizerState );
    SAFE_RELEASE( m_pCBWorldViewProj );
    SAFE_RELEASE( m_pVS );
    SAFE_RELEASE( m_pPS );
    SAFE_RELEASE( m_pVSBlob );
    SAFE_RELEASE( m_pPSBlob );
}

HRESULT DebugBBoxRendererModule::Execute( ID3D11DeviceContext* pd3dDeviceContext )
{
    HRESULT hr = S_OK;
    const bool drawSceneBoundingBox = m_SharedState.renderBoundingBoxes && m_SelectionModule.GetSceneBoundingBoxSRV() && m_SelectionModule.GetSceneBoundingBoxCount() > 0;
    const bool drawAllBoundingBoxes = m_SharedState.renderAllBoundingBoxes && m_SelectionModule.GetAllBoundingBoxSRV() && m_SelectionModule.GetAllBoundingBoxCount() > 0;
    if( ( !drawSceneBoundingBox && !drawAllBoundingBoxes ) ||
        !m_FrameContext.output.pRenderTargetView || !m_FrameContext.output.pDepthStencilView || !m_FrameContext.output.pViewport ||
        !m_pVS || !m_pPS || !m_pCBWorldViewProj )
    {
        return S_OK;
    }

    D3DXMATRIX dxmatCameraProj;
    D3DXMATRIX dxmatCameraView;
    ResolveCameraMatrices( m_FrameContext, dxmatCameraView, dxmatCameraProj );

    D3DXMATRIX dxmatWorldViewProjection = dxmatCameraView * dxmatCameraProj;

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    V_RETURN( pd3dDeviceContext->Map( m_pCBWorldViewProj, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource ) );
    CB_DEBUG_BOUNDING_BOX* pDebugConstants = (CB_DEBUG_BOUNDING_BOX*)mappedResource.pData;
    D3DXMatrixTranspose( &pDebugConstants->m_WorldViewProj, &dxmatWorldViewProjection );
    pd3dDeviceContext->Unmap( m_pCBWorldViewProj, 0 );

    pd3dDeviceContext->RSSetState( m_pRasterizerState );
    pd3dDeviceContext->OMSetRenderTargets( 1, &m_FrameContext.output.pRenderTargetView, m_FrameContext.output.pDepthStencilView );
    pd3dDeviceContext->RSSetViewports( 1, m_FrameContext.output.pViewport );
    pd3dDeviceContext->IASetInputLayout( NULL );
    pd3dDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_LINELIST );
    pd3dDeviceContext->GSSetShader( NULL, NULL, 0 );
    pd3dDeviceContext->VSSetShader( m_pVS, NULL, 0 );
    pd3dDeviceContext->PSSetShader( m_pPS, NULL, 0 );
    pd3dDeviceContext->VSSetConstantBuffers( 0, 1, &m_pCBWorldViewProj );

    if( drawSceneBoundingBox )
    {
        ID3D11ShaderResourceView* pSceneSRV = m_SelectionModule.GetSceneBoundingBoxSRV();
        pd3dDeviceContext->VSSetShaderResources( 0, 1, &pSceneSRV );
        pd3dDeviceContext->DrawInstanced( 24, m_SelectionModule.GetSceneBoundingBoxCount(), 0, 0 );
    }

    if( drawAllBoundingBoxes )
    {
        ID3D11ShaderResourceView* pAllSRV = m_SelectionModule.GetAllBoundingBoxSRV();
        pd3dDeviceContext->VSSetShaderResources( 0, 1, &pAllSRV );
        pd3dDeviceContext->DrawInstanced( 24, m_SelectionModule.GetAllBoundingBoxCount(), 0, 0 );
    }

    ID3D11ShaderResourceView* nullSRV[1] = { NULL };
    pd3dDeviceContext->VSSetShaderResources( 0, 1, nullSRV );

    return S_OK;
}

DebugOverlayRendererModule::DebugOverlayRendererModule( const DebugSharedState& sharedState, const DebugFrameContext& frameContext )
    : m_SharedState( sharedState )
    , m_FrameContext( frameContext )
    , m_pVS( NULL )
    , m_pPS( NULL )
    , m_pVSBlob( NULL )
    , m_pPSBlob( NULL )
    , m_pCBSelectionRing( NULL )
    , m_pRasterizerState( NULL )
    , m_pOverlayBlendState( NULL )
    , m_pOverlayDepthState( NULL )
{
}

HRESULT DebugOverlayRendererModule::Create( ID3D11Device* pd3dDevice )
{
    HRESULT hr = S_OK;
    WCHAR selectionRingShaderFile[] = L"SelectionRing.hlsl";

    if( m_pVSBlob == NULL )
    {
        V_RETURN( CompileShaderFromFile( selectionRingShaderFile, NULL, "VSMain", "vs_5_0", &m_pVSBlob ) );
    }
    if( m_pPSBlob == NULL )
    {
        V_RETURN( CompileShaderFromFile( selectionRingShaderFile, NULL, "PSMain", "ps_5_0", &m_pPSBlob ) );
    }
    if( m_pVS == NULL )
    {
        V_RETURN( pd3dDevice->CreateVertexShader( m_pVSBlob->GetBufferPointer(), m_pVSBlob->GetBufferSize(), NULL, &m_pVS ) );
        DXUT_SetDebugName( m_pVS, "DebugOverlayRendererModule_VS" );
    }
    if( m_pPS == NULL )
    {
        V_RETURN( pd3dDevice->CreatePixelShader( m_pPSBlob->GetBufferPointer(), m_pPSBlob->GetBufferSize(), NULL, &m_pPS ) );
        DXUT_SetDebugName( m_pPS, "DebugOverlayRendererModule_PS" );
    }
    if( m_pCBSelectionRing == NULL )
    {
        D3D11_BUFFER_DESC desc = {};
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.ByteWidth = sizeof( CB_SELECTION_RING );
        V_RETURN( pd3dDevice->CreateBuffer( &desc, NULL, &m_pCBSelectionRing ) );
        DXUT_SetDebugName( m_pCBSelectionRing, "DebugOverlayRendererModule_CBSelectionRing" );
    }
    if( m_pRasterizerState == NULL )
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
        V_RETURN( pd3dDevice->CreateRasterizerState( &rasterDesc, &m_pRasterizerState ) );
        DXUT_SetDebugName( m_pRasterizerState, "DebugOverlayRendererModule_Rasterizer" );
    }
    if( m_pOverlayBlendState == NULL )
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
        V_RETURN( pd3dDevice->CreateBlendState( &blendDesc, &m_pOverlayBlendState ) );
        DXUT_SetDebugName( m_pOverlayBlendState, "DebugOverlayRendererModule_Blend" );
    }
    if( m_pOverlayDepthState == NULL )
    {
        D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
        depthStencilDesc.DepthEnable = TRUE;
        depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        depthStencilDesc.StencilEnable = FALSE;
        V_RETURN( pd3dDevice->CreateDepthStencilState( &depthStencilDesc, &m_pOverlayDepthState ) );
        DXUT_SetDebugName( m_pOverlayDepthState, "DebugOverlayRendererModule_DepthState" );
    }

    return hr;
}

void DebugOverlayRendererModule::Destroy()
{
    SAFE_RELEASE( m_pOverlayDepthState );
    SAFE_RELEASE( m_pOverlayBlendState );
    SAFE_RELEASE( m_pRasterizerState );
    SAFE_RELEASE( m_pCBSelectionRing );
    SAFE_RELEASE( m_pVS );
    SAFE_RELEASE( m_pPS );
    SAFE_RELEASE( m_pVSBlob );
    SAFE_RELEASE( m_pPSBlob );
}

HRESULT DebugOverlayRendererModule::Execute( ID3D11DeviceContext* pd3dDeviceContext )
{
    HRESULT hr = S_OK;
    if( !pd3dDeviceContext || !m_FrameContext.output.pRenderTargetView || !m_FrameContext.output.pDepthStencilView || !m_FrameContext.output.pViewport ||
        !m_pVS || !m_pPS || !m_pCBSelectionRing ||
        m_SharedState.selectedBoundingBox < 0 || m_SharedState.selectedBoundingBox >= INT( m_SharedState.allBoundingBoxes.size() ) )
    {
        return S_OK;
    }

    D3DXVECTOR3 vMin;
    D3DXVECTOR3 vMax;
    GetBoundingBoxMinMax( m_SharedState.allBoundingBoxes[m_SharedState.selectedBoundingBox], vMin, vMax );

    const D3DXVECTOR3 vCenter( ( vMin.x + vMax.x ) * 0.5f,
        vMin.y + max( kSelectionRingVerticalOffsetMin, ( vMax.y - vMin.y ) * kSelectionRingVerticalOffsetFactor ),
        ( vMin.z + vMax.z ) * 0.5f );
    const float radius = max( kSelectionMinObjectRadius,
        max( ( vMax.x - vMin.x ) * 0.5f, ( vMax.z - vMin.z ) * 0.5f ) * kSelectionRingRadiusScale );
    const float quadHalfSize = radius * kSelectionRingQuadRadiusScale;

    D3DXMATRIX dxmatCameraProj;
    D3DXMATRIX dxmatCameraView;
    ResolveCameraMatrices( m_FrameContext, dxmatCameraView, dxmatCameraProj );
    const D3DXMATRIX dxmatViewProj = dxmatCameraView * dxmatCameraProj;

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    V_RETURN( pd3dDeviceContext->Map( m_pCBSelectionRing, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource ) );
    CB_SELECTION_RING* pSelectionRing = (CB_SELECTION_RING*)mappedResource.pData;
    D3DXMatrixTranspose( &pSelectionRing->m_ViewProj, &dxmatViewProj );
    pSelectionRing->m_vRingCenterHalfSize = D3DXVECTOR4( vCenter.x, vCenter.y, vCenter.z, quadHalfSize );
    pSelectionRing->m_vRingColorTime = D3DXVECTOR4( kSelectionRingColor.x, kSelectionRingColor.y, kSelectionRingColor.z, (FLOAT)DXUTGetTime() );
    pd3dDeviceContext->Unmap( m_pCBSelectionRing, 0 );

    pd3dDeviceContext->RSSetState( m_pRasterizerState );
    pd3dDeviceContext->OMSetRenderTargets( 1, &m_FrameContext.output.pRenderTargetView, m_FrameContext.output.pDepthStencilView );
    pd3dDeviceContext->OMSetBlendState( m_pOverlayBlendState, kSelectionOverlayBlendFactor, 0xFFFFFFFF );
    pd3dDeviceContext->OMSetDepthStencilState( m_pOverlayDepthState, 0 );
    pd3dDeviceContext->RSSetViewports( 1, m_FrameContext.output.pViewport );
    pd3dDeviceContext->IASetInputLayout( NULL );
    pd3dDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );
    pd3dDeviceContext->GSSetShader( NULL, NULL, 0 );
    pd3dDeviceContext->VSSetShader( m_pVS, NULL, 0 );
    pd3dDeviceContext->PSSetShader( m_pPS, NULL, 0 );
    pd3dDeviceContext->VSSetConstantBuffers( 0, 1, &m_pCBSelectionRing );
    pd3dDeviceContext->PSSetConstantBuffers( 0, 1, &m_pCBSelectionRing );
    pd3dDeviceContext->Draw( kSelectionProceduralQuadVertexCount, 0 );
    pd3dDeviceContext->OMSetBlendState( NULL, kSelectionOverlayBlendFactor, 0xFFFFFFFF );
    pd3dDeviceContext->OMSetDepthStencilState( NULL, 0 );

    return S_OK;
}

DebugRenderPass::DebugRenderPass()
    : m_pDebugSelectionModule( NULL )
    , m_pDebugBBoxRendererModule( NULL )
    , m_pDebugOverlayRendererModule( NULL )
{
    assert( s_pInstance == NULL );
    s_pInstance = this;

    m_pDebugSelectionModule = new DebugSelectionModule( m_SharedState, m_FrameContext );
    m_pDebugBBoxRendererModule = new DebugBBoxRendererModule( m_SharedState, m_FrameContext, *m_pDebugSelectionModule );
    m_pDebugOverlayRendererModule = new DebugOverlayRendererModule( m_SharedState, m_FrameContext );

    m_pModules[0] = m_pDebugSelectionModule;
    m_pModules[1] = m_pDebugBBoxRendererModule;
    m_pModules[2] = m_pDebugOverlayRendererModule;
}

DebugRenderPass::~DebugRenderPass()
{
    Destroy();
    delete m_pDebugOverlayRendererModule;
    delete m_pDebugBBoxRendererModule;
    delete m_pDebugSelectionModule;
    m_pDebugOverlayRendererModule = NULL;
    m_pDebugBBoxRendererModule = NULL;
    m_pDebugSelectionModule = NULL;
    m_pModules[0] = NULL;
    m_pModules[1] = NULL;
    m_pModules[2] = NULL;
    assert( s_pInstance == this );
    s_pInstance = NULL;
}

DebugRenderPass& DebugRenderPass::Get()
{
    assert( s_pInstance );
    return *s_pInstance;
}

const char* DebugRenderPass::GetPassName() const
{
    return "Debug";
}

HRESULT DebugRenderPass::Create( ID3D11Device* pd3dDevice )
{
    HRESULT hr = S_OK;
    for( INT moduleIndex = 0; moduleIndex < ARRAYSIZE( m_pModules ); ++moduleIndex )
    {
        V_RETURN( m_pModules[moduleIndex]->Create( pd3dDevice ) );
    }
    return hr;
}

void DebugRenderPass::Destroy()
{
    for( INT moduleIndex = ARRAYSIZE( m_pModules ) - 1; moduleIndex >= 0; --moduleIndex )
    {
        if( m_pModules[moduleIndex] )
        {
            m_pModules[moduleIndex]->Destroy();
        }
    }
}

HRESULT DebugRenderPass::Execute( ID3D11DeviceContext* pd3dDeviceContext )
{
    const bool drawSelectionRing = m_SharedState.selectedBoundingBox >= 0 && m_SharedState.selectedBoundingBox < INT( m_SharedState.allBoundingBoxes.size() );
    if( !m_SharedState.renderBoundingBoxes && !m_SharedState.renderAllBoundingBoxes && !drawSelectionRing )
    {
        return S_OK;
    }

    HRESULT hr = S_OK;
    for( INT moduleIndex = 0; moduleIndex < ARRAYSIZE( m_pModules ); ++moduleIndex )
    {
        V_RETURN( m_pModules[moduleIndex]->Execute( pd3dDeviceContext ) );
    }
    return hr;
}

HRESULT DebugRenderPass::HandleSceneChanged( ID3D11Device* pd3dDevice, ISceneMesh* pMesh )
{
    return m_pDebugSelectionModule->HandleSceneChanged( pd3dDevice, pMesh );
}

HRESULT DebugRenderPass::PickBoundingBox( ID3D11DeviceContext* pd3dDeviceContext, INT mouseX, INT mouseY, const D3D11_VIEWPORT& viewport )
{
    return m_pDebugSelectionModule->PickBoundingBox( pd3dDeviceContext, mouseX, mouseY, viewport );
}

void DebugRenderPass::SetMeshView( ISceneMesh* pMesh )
{
    m_SharedState.pMeshView = pMesh;
}

void DebugRenderPass::SetOutput( ID3D11RenderTargetView* prtvBackBuffer, ID3D11DepthStencilView* pdsvBackBuffer, D3D11_VIEWPORT* pViewport )
{
    m_FrameContext.output.pRenderTargetView = prtvBackBuffer;
    m_FrameContext.output.pDepthStencilView = pdsvBackBuffer;
    m_FrameContext.output.pViewport = pViewport;
}

void DebugRenderPass::SetCameraContext( CFirstPersonCamera* pViewerCamera,
                                        CFirstPersonCamera* pLightCamera,
                                        CAMERA_SELECTION selectedCamera,
                                        const D3DXMATRIX& matShadowView,
                                        const D3DXMATRIX* pShadowProj,
                                        UINT shadowProjCount )
{
    m_FrameContext.pViewerCamera = pViewerCamera;
    m_FrameContext.pLightCamera = pLightCamera;
    m_FrameContext.selectedCamera = selectedCamera;
    m_FrameContext.matShadowView = matShadowView;

    const UINT copyCount = min( shadowProjCount, UINT( MAX_CASCADES ) );
    for( UINT cascadeIndex = 0; cascadeIndex < copyCount; ++cascadeIndex )
    {
        m_FrameContext.matShadowProj[cascadeIndex] = pShadowProj[cascadeIndex];
    }
}

void DebugRenderPass::SetRenderBoundingBoxesEnabled( bool enabled )
{
    m_SharedState.renderBoundingBoxes = enabled;
}

void DebugRenderPass::SetRenderAllBoundingBoxesEnabled( bool enabled )
{
    m_SharedState.renderAllBoundingBoxes = enabled;
}

bool DebugRenderPass::IsRenderBoundingBoxesEnabled() const
{
    return m_SharedState.renderBoundingBoxes;
}

bool DebugRenderPass::IsRenderAllBoundingBoxesEnabled() const
{
    return m_SharedState.renderAllBoundingBoxes;
}

INT DebugRenderPass::GetSelectedBoundingBox() const
{
    return m_SharedState.selectedBoundingBox;
}
