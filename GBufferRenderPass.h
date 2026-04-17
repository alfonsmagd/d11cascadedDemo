#pragma once

#include "IRenderPass.h"
#include "DXTexture.h"
#include "FrameContext.h"
#include "ShadowSampleMisc.h"
#include "SceneMesh.h"


#define MAX_GBUFFER_RTV 4
struct GBufferOutput
{
    ID3D11DepthStencilView* pDepthStencilView;
    D3D11_VIEWPORT* pViewport;

    GBufferOutput()
        : pDepthStencilView( NULL )
        , pViewport( NULL )
    {
    }
};

struct GBufferFrameContext : public FrameContext
{
    GBufferOutput output;

    GBufferFrameContext()
        : FrameContext()
    {
    }
};

class GBufferRenderPass : public IRenderPass
{
public:
    GBufferRenderPass();
    ~GBufferRenderPass() override;

    const char* GetPassName() const override;
    HRESULT Create( ID3D11Device* pd3dDevice ) override;
    void Destroy() override;
    HRESULT Execute( ID3D11DeviceContext* pd3dDeviceContext ) override;
    HRESULT Resize( ID3D11Device* pd3dDevice, UINT width, UINT height );

    void SetOutput(ID3D11RenderTargetView* prtvBackBuffer, ID3D11DepthStencilView* pdsvBackBuffer, D3D11_VIEWPORT* pViewport);
    void SetMeshView( ISceneMesh* pMesh );
    void SetVisibleSubMeshes( const std::vector<INT>* pVisibleSubMeshes );
    const DX::Texture::Resource2D* GetPositionTexture() const;
    const DX::Texture::Resource2D* GetNormalsTexture() const;
    const DX::Texture::Resource2D* GetTangentTexture() const;
    const DX::Texture::Resource2D* GetMotionVectorTexture() const;
    ID3D11ShaderResourceView* GetPositionSRV() const;
    ID3D11ShaderResourceView* GetNormalsSRV() const;
    ID3D11ShaderResourceView* GetTangentSRV() const;
    ID3D11ShaderResourceView* GetMotionVectorSRV() const;

    void SetCameraContext(CFirstPersonCamera* pViewerCamera,
                          CFirstPersonCamera* pLightCamera, 
                          CAMERA_SELECTION selectedCamera);

    void SetGlobalConstantBuffer(ID3D11Buffer* pGlobalConstantBuffer) { m_pGlobalConstantBuffer = pGlobalConstantBuffer; }
    void SetInputVertexLayout(ID3D11InputLayout* pInputLayout) { m_pInputLayout = pInputLayout; }

private:
    enum
    {
        GBUFFER_RTV_POSITION = 0,
        GBUFFER_RTV_NORMALS = 1,
        GBUFFER_RTV_TANGENT = 2,
        GBUFFER_RTV_MOTION_VECTOR = 3,
        GBUFFER_RTV_COUNT = MAX_GBUFFER_RTV
    };

    GBufferFrameContext m_FrameContext;
    ISceneMesh* m_pMeshView;
    const std::vector<INT>* m_pVisibleSubMeshes;

    DX::Texture::Resource2D m_GBufferTargets[GBUFFER_RTV_COUNT];

    ID3D11VertexShader* m_pGeometryVS;
    ID3D11PixelShader* m_pGeometryPS;
    ID3DBlob* m_pGeometryVSBlob;
    ID3DBlob* m_pGeometryPSBlob;

    ID3D11VertexShader* m_pMotionVectorVS;
    ID3D11PixelShader* m_pMotionVectorPS;
    ID3DBlob* m_pMotionVectorVSBlob;
    ID3DBlob* m_pMotionVectorPSBlob;

    ID3D11RasterizerState* m_pRasterizerState;
    ID3D11Buffer* m_pGlobalConstantBuffer;
    ID3D11InputLayout* m_pInputLayout;

    void ResolveCameraMatrices( D3DXMATRIX& dxmatCameraView, D3DXMATRIX& dxmatCameraProj ) const;
    void ReleaseOwnedTargets();
};
