#pragma once

#include "DXUT.h"

namespace DX
{
namespace Texture
{
    enum class Type
    {
        Unknown,
        Color2D,
        Depth2D,
        ShaderOnly2D
    };

    struct Desc2D
    {
        UINT width;
        UINT height;
        UINT mipLevels;
        UINT arraySize;
        DXGI_FORMAT resourceFormat;
        DXGI_FORMAT srvFormat;
        DXGI_FORMAT rtvFormat;
        DXGI_FORMAT dsvFormat;
        DXGI_FORMAT uavFormat;
        D3D11_USAGE usage;
        UINT bindFlags;
        UINT cpuAccessFlags;
        UINT miscFlags;
        UINT sampleCount;
        UINT sampleQuality;
        Type type;
        const char* debugName;

        Desc2D();
    };

    struct Resource2D
    {
        Desc2D desc;
        ID3D11Texture2D* pResource;
        ID3D11ShaderResourceView* pSRV;
        ID3D11RenderTargetView* pRTV;
        ID3D11DepthStencilView* pDSV;
        ID3D11UnorderedAccessView* pUAV;

        Resource2D();

        void Destroy();
        ID3D11Texture2D* GetResource() const;
        ID3D11ShaderResourceView* GetSRV() const;
        ID3D11RenderTargetView* GetRTV() const;
        ID3D11DepthStencilView* GetDSV() const;
        ID3D11UnorderedAccessView* GetUAV() const;
    };

    void SetDebugName( ID3D11DeviceChild* pObject, const char* baseName, const char* suffix );

    HRESULT Create2D( ID3D11Device* pd3dDevice,
                      const Desc2D& desc,
                      const D3D11_SUBRESOURCE_DATA* pInitialData,
                      Resource2D& outResource );

    HRESULT CreateColorTarget2D( ID3D11Device* pd3dDevice,
                                 UINT width,
                                 UINT height,
                                 DXGI_FORMAT format,
                                 const char* debugName,
                                 Resource2D& outResource );

    HRESULT CreateDepthTarget2D( ID3D11Device* pd3dDevice,
                                 UINT width,
                                 UINT height,
                                 DXGI_FORMAT resourceFormat,
                                 DXGI_FORMAT dsvFormat,
                                 DXGI_FORMAT srvFormat,
                                 const char* debugName,
                                 Resource2D& outResource );

    HRESULT CreateImmutableShaderTexture2D( ID3D11Device* pd3dDevice,
                                            UINT width,
                                            UINT height,
                                            DXGI_FORMAT format,
                                            const void* pData,
                                            UINT rowPitch,
                                            const char* debugName,
                                            Resource2D& outResource );
}
}
