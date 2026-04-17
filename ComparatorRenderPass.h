#pragma once

#include "DXBuffer.h"
#include "IRenderPass.h"

struct ComparatorOutput
{
    ID3D11RenderTargetView* pRenderTargetView;
    D3D11_VIEWPORT* pViewport;

    ComparatorOutput()
        : pRenderTargetView( NULL )
        , pViewport( NULL )
    {
    }
};

class ComparatorRenderPass : public IRenderPass
{
public:
    ComparatorRenderPass();
    ~ComparatorRenderPass() override;

    const char* GetPassName() const override;
    HRESULT Create( ID3D11Device* pd3dDevice ) override;
    void Destroy() override;
    HRESULT Execute( ID3D11DeviceContext* pd3dDeviceContext ) override;

    void SetEnabled( bool enabled );
    bool IsEnabled() const;
    void SetOutput( ID3D11RenderTargetView* pRenderTargetView, D3D11_VIEWPORT* pViewport );
    void SetInputs( ID3D11ShaderResourceView* pLeftSRV, ID3D11ShaderResourceView* pRightSRV );
    void SetSplit( float split );

private:
    struct ComparatorConstants
    {
        D3DXVECTOR4 vViewportSizeAndSplit;
        D3DXVECTOR4 vLineWidthHandleRadius;
        D3DXVECTOR4 vLineColor;
    };

    ComparatorOutput m_Output;
    DX::Buffer::Resource m_ConstantBuffer;
    ID3D11ShaderResourceView* m_pLeftSRV;
    ID3D11ShaderResourceView* m_pRightSRV;
    ID3D11VertexShader* m_pVS;
    ID3D11PixelShader* m_pPS;
    ID3DBlob* m_pVSBlob;
    ID3DBlob* m_pPSBlob;
    ID3D11SamplerState* m_pPointSampler;
    ID3D11DepthStencilState* m_pDepthDisabledState;
    float m_fSplit;
    bool m_bEnabled;
};
