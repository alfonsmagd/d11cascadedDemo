#pragma once

#include "IRenderPass.h"
#include "FrameContext.h"
#include "ShadowSampleMisc.h"
#include "SceneMesh.h"


#define MAX_GBUFFER_RTV 4
struct GBufferOutput
{
    ID3D11RenderTargetView* pPositionRTV;
    ID3D11RenderTargetView* pNormalsRTV;
    ID3D11RenderTargetView* pTangentRTV;
    ID3D11RenderTargetView* pMotionVectorRTV;
    ID3D11ShaderResourceView* pPositionSRV;
    ID3D11ShaderResourceView* pNormalsSRV;
    ID3D11ShaderResourceView* pTangentSRV;
    ID3D11ShaderResourceView* pMotionVectorSRV;
    ID3D11DepthStencilView* pDepthStencilView;
    D3D11_VIEWPORT* pViewport;

    GBufferOutput()
        : pPositionRTV( NULL )
        , pNormalsRTV( NULL )
        , pTangentRTV( NULL )
        , pMotionVectorRTV( NULL )
        , pPositionSRV( NULL )
        , pNormalsSRV( NULL )
        , pTangentSRV( NULL )
        , pMotionVectorSRV( NULL )
        , pDepthStencilView( NULL )
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

    void SetOutput( const GBufferOutput& output );
    void SetMeshView( ISceneMesh* pMesh );
    ID3D11ShaderResourceView* GetPositionSRV() const;
    ID3D11ShaderResourceView* GetNormalsSRV() const;
    ID3D11ShaderResourceView* GetTangentSRV() const;
    ID3D11ShaderResourceView* GetMotionVectorSRV() const;

    HRESULT static GBufferRenderPass::ReleaseGBufferResources(
        ID3D11Texture2D** pAuxGBufferTextures,
        ID3D11RenderTargetView** pAuxGBufferRTVs,
        ID3D11ShaderResourceView** pAuxGBufferSRVs);

    HRESULT static GBufferRenderPass::CreateGBufferResources(
        ID3D11Device* pd3dDevice,
        UINT width,
        UINT height,
        ID3D11Texture2D** pAuxGBufferTextures,
        ID3D11RenderTargetView** pAuxGBufferRTVs,
        ID3D11ShaderResourceView** pAuxGBufferSRVs);

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

    ID3D11RenderTargetView* m_pRTVs[GBUFFER_RTV_COUNT];
    ID3D11ShaderResourceView* m_pSRVs[GBUFFER_RTV_COUNT];

    ID3D11VertexShader* m_pGeometryVS;
    ID3D11PixelShader* m_pGeometryPS;
    ID3DBlob* m_pGeometryVSBlob;
    ID3DBlob* m_pGeometryPSBlob;

    ID3D11VertexShader* m_pMotionVectorVS;
    ID3D11PixelShader* m_pMotionVectorPS;
    ID3DBlob* m_pMotionVectorVSBlob;
    ID3DBlob* m_pMotionVectorPSBlob;
};
