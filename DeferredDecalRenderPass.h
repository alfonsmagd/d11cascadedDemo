#pragma once

#include "DXBuffer.h"
#include "DXTexture.h"
#include "IRenderPass.h"
#include "SceneMesh.h"

class CFirstPersonCamera;

struct DeferredDecalOutput
{
    ID3D11RenderTargetView* pRenderTargetView;
    D3D11_VIEWPORT* pViewport;

    DeferredDecalOutput()
        : pRenderTargetView( NULL )
        , pViewport( NULL )
    {
    }
};

class DeferredDecalRenderPass : public IRenderPass
{
public:
    DeferredDecalRenderPass();
    ~DeferredDecalRenderPass() override;

    const char* GetPassName() const override;
    HRESULT Create( ID3D11Device* pd3dDevice ) override;
    void Destroy() override;
    HRESULT Execute( ID3D11DeviceContext* pd3dDeviceContext ) override;

    void SetEnabled( bool enabled );
    bool IsEnabled() const;
    void SetMeshView( ISceneMesh* pMesh );
    void SetCameraContext( CFirstPersonCamera* pCamera );
    void SetOutput( ID3D11RenderTargetView* pRenderTargetView, D3D11_VIEWPORT* pViewport );
    void SetGBufferInputs( const DX::Texture::Resource2D* pPositionTexture, const DX::Texture::Resource2D* pNormalTexture );
    void SetPickedDecalPosition( const D3DXVECTOR3& worldPosition );
    void ClearPickedDecalPosition();

private:
    HRESULT CreateExampleDecalTexture( ID3D11Device* pd3dDevice );
    HRESULT CreateWireBoxResources( ID3D11Device* pd3dDevice );

private:
    struct DeferredDecalConstants
    {
        D3DXVECTOR4 vDecalCenter;
        D3DXVECTOR4 vDecalHalfSize;
        D3DXVECTOR4 vProjectionNormalOpacity;
        D3DXVECTOR4 vDecalParams;
    };

    struct DeferredDecalDebugBoxConstants
    {
        D3DXMATRIX m_WorldViewProj;
    };

    DeferredDecalOutput m_Output;
    ISceneMesh* m_pMeshView;
    CFirstPersonCamera* m_pCamera;
    const DX::Texture::Resource2D* m_pPositionTexture;
    const DX::Texture::Resource2D* m_pNormalTexture;
    ID3D11VertexShader* m_pVS;
    ID3D11PixelShader* m_pPS;
    ID3DBlob* m_pVSBlob;
    ID3DBlob* m_pPSBlob;
    ID3D11VertexShader* m_pWireBoxVS;
    ID3D11PixelShader* m_pWireBoxPS;
    ID3DBlob* m_pWireBoxVSBlob;
    ID3DBlob* m_pWireBoxPSBlob;
    DX::Buffer::Resource m_ConstantBuffer;
    DX::Buffer::Resource m_WireBoxConstantBuffer;
    DX::Buffer::Resource m_WireBoxBuffer;
    ID3D11SamplerState* m_pPointSampler;
    ID3D11BlendState* m_pBlendState;
    ID3D11DepthStencilState* m_pDepthStencilState;
    DX::Texture::Resource2D m_ExampleDecalTexture;
    D3DXVECTOR3 m_vPickedDecalPosition;
    bool m_bHasPickedDecalPosition;
    bool m_bEnabled;
};
