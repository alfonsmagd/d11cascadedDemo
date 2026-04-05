#pragma once

#include <vector>

#include "IRenderPass.h"
#include "ShadowSampleMisc.h"
#include "SceneMesh.h"

class CFirstPersonCamera;

class IDebugBase
{
public:
    virtual ~IDebugBase() {}
    virtual HRESULT Create( ID3D11Device* pd3dDevice ) = 0;
    virtual void Destroy() = 0;
    virtual HRESULT Execute( ID3D11DeviceContext* pd3dDeviceContext ) = 0;
};

struct DebugOutput
{
    ID3D11RenderTargetView* pRenderTargetView;
    ID3D11DepthStencilView* pDepthStencilView;
    D3D11_VIEWPORT* pViewport;

    DebugOutput()
        : pRenderTargetView( NULL )
        , pDepthStencilView( NULL )
        , pViewport( NULL )
    {
    }
};

struct DebugSharedState
{
    ISceneMesh* pMeshView;
    std::vector<BoundingBox> sceneBoundingBoxes;
    std::vector<BoundingBox> allBoundingBoxes;
    UINT sceneBoundingBoxCount;
    UINT allBoundingBoxCount;
    INT selectedBoundingBox;
    bool renderBoundingBoxes;
    bool renderAllBoundingBoxes;

    DebugSharedState()
        : pMeshView( NULL )
        , sceneBoundingBoxCount( 0 )
        , allBoundingBoxCount( 0 )
        , selectedBoundingBox( -1 )
        , renderBoundingBoxes( false )
        , renderAllBoundingBoxes( false )
    {
    }
};

struct DebugFrameContext
{
    DebugOutput output;
    CFirstPersonCamera* pViewerCamera;
    CFirstPersonCamera* pLightCamera;
    CAMERA_SELECTION selectedCamera;
    D3DXMATRIX matShadowView;
    D3DXMATRIX matShadowProj[MAX_CASCADES];

    DebugFrameContext()
        : pViewerCamera( NULL )
        , pLightCamera( NULL )
        , selectedCamera( EYE_CAMERA )
    {
        D3DXMatrixIdentity( &matShadowView );
        for( INT cascadeIndex = 0; cascadeIndex < MAX_CASCADES; ++cascadeIndex )
        {
            D3DXMatrixIdentity( &matShadowProj[cascadeIndex] );
        }
    }
};

class DebugSelectionModule : public IDebugBase
{
public:
    explicit DebugSelectionModule( DebugSharedState& sharedState, const DebugFrameContext& frameContext );

    HRESULT Create( ID3D11Device* pd3dDevice ) override;
    void Destroy() override;
    HRESULT Execute( ID3D11DeviceContext* pd3dDeviceContext ) override;

    HRESULT HandleSceneChanged( ID3D11Device* pd3dDevice, ISceneMesh* pMesh );
    HRESULT PickBoundingBox( ID3D11DeviceContext* pd3dDeviceContext, INT mouseX, INT mouseY, const D3D11_VIEWPORT& viewport );

    ID3D11ShaderResourceView* GetSceneBoundingBoxSRV() const;
    ID3D11ShaderResourceView* GetAllBoundingBoxSRV() const;
    UINT GetSceneBoundingBoxCount() const;
    UINT GetAllBoundingBoxCount() const;

private:
    HRESULT UpdateResources( ID3D11Device* pd3dDevice );
    void ReleaseResources();

private:
    DebugSharedState& m_SharedState;
    const DebugFrameContext& m_FrameContext;
    ID3D11Buffer* m_pBoundingBoxBuffer;
    ID3D11ShaderResourceView* m_pBoundingBoxSRV;
    ID3D11Buffer* m_pBoundingAllBoxBuffer;
    ID3D11ShaderResourceView* m_pBoundingAllBoxSRV;
};

class DebugBBoxRendererModule : public IDebugBase
{
public:
    DebugBBoxRendererModule( const DebugSharedState& sharedState,
                             const DebugFrameContext& frameContext,
                             const DebugSelectionModule& selectionModule );

    HRESULT Create( ID3D11Device* pd3dDevice ) override;
    void Destroy() override;
    HRESULT Execute( ID3D11DeviceContext* pd3dDeviceContext ) override;

private:
    const DebugSharedState& m_SharedState;
    const DebugFrameContext& m_FrameContext;
    const DebugSelectionModule& m_SelectionModule;
    ID3D11VertexShader* m_pVS;
    ID3D11PixelShader* m_pPS;
    ID3DBlob* m_pVSBlob;
    ID3DBlob* m_pPSBlob;
    ID3D11Buffer* m_pCBWorldViewProj;
    ID3D11RasterizerState* m_pRasterizerState;
};

class DebugOverlayRendererModule : public IDebugBase
{
public:
    DebugOverlayRendererModule( const DebugSharedState& sharedState, const DebugFrameContext& frameContext );

    HRESULT Create( ID3D11Device* pd3dDevice ) override;
    void Destroy() override;
    HRESULT Execute( ID3D11DeviceContext* pd3dDeviceContext ) override;

private:
    const DebugSharedState& m_SharedState;
    const DebugFrameContext& m_FrameContext;
    ID3D11VertexShader* m_pVS;
    ID3D11PixelShader* m_pPS;
    ID3DBlob* m_pVSBlob;
    ID3DBlob* m_pPSBlob;
    ID3D11Buffer* m_pCBSelectionRing;
    ID3D11RasterizerState* m_pRasterizerState;
    ID3D11BlendState* m_pOverlayBlendState;
    ID3D11DepthStencilState* m_pOverlayDepthState;
};

class DebugRenderPass : public IRenderPass
{
public:
    DebugRenderPass();
    ~DebugRenderPass() override;

    static DebugRenderPass& Get();

    const char* GetPassName() const override;
    HRESULT Create( ID3D11Device* pd3dDevice ) override;
    void Destroy() override;
    HRESULT Execute( ID3D11DeviceContext* pd3dDeviceContext ) override;

    HRESULT HandleSceneChanged( ID3D11Device* pd3dDevice, ISceneMesh* pMesh );
    HRESULT PickBoundingBox( ID3D11DeviceContext* pd3dDeviceContext, INT mouseX, INT mouseY, const D3D11_VIEWPORT& viewport );

    void SetMeshView( ISceneMesh* pMesh );
    void SetOutput( ID3D11RenderTargetView* prtvBackBuffer, ID3D11DepthStencilView* pdsvBackBuffer, D3D11_VIEWPORT* pViewport );
    void SetCameraContext( CFirstPersonCamera* pViewerCamera,
                           CFirstPersonCamera* pLightCamera,
                           CAMERA_SELECTION selectedCamera,
                           const D3DXMATRIX& matShadowView,
                           const D3DXMATRIX* pShadowProj,
                           UINT shadowProjCount );

    void SetRenderBoundingBoxesEnabled( bool enabled );
    void SetRenderAllBoundingBoxesEnabled( bool enabled );
    bool IsRenderBoundingBoxesEnabled() const;
    bool IsRenderAllBoundingBoxesEnabled() const;
    INT GetSelectedBoundingBox() const;

private:
    DebugSharedState m_SharedState;
    DebugFrameContext m_FrameContext;

    DebugSelectionModule* m_pDebugSelectionModule;
    DebugBBoxRendererModule* m_pDebugBBoxRendererModule;
    DebugOverlayRendererModule* m_pDebugOverlayRendererModule;
    IDebugBase* m_pModules[3];

    static DebugRenderPass* s_pInstance;
};
