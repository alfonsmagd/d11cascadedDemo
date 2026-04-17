#include <cstdio>

#include "DXUT.h"
#include "DXTexture.h"

namespace DX
{
namespace Texture
{
    Desc2D::Desc2D()
        : width( 0 )
        , height( 0 )
        , mipLevels( 1 )
        , arraySize( 1 )
        , resourceFormat( DXGI_FORMAT_UNKNOWN )
        , srvFormat( DXGI_FORMAT_UNKNOWN )
        , rtvFormat( DXGI_FORMAT_UNKNOWN )
        , dsvFormat( DXGI_FORMAT_UNKNOWN )
        , uavFormat( DXGI_FORMAT_UNKNOWN )
        , usage( D3D11_USAGE_DEFAULT )
        , bindFlags( 0 )
        , cpuAccessFlags( 0 )
        , miscFlags( 0 )
        , sampleCount( 1 )
        , sampleQuality( 0 )
        , type( Type::Unknown )
        , debugName( NULL )
    {
    }

    Resource2D::Resource2D()
        : pResource( NULL )
        , pSRV( NULL )
        , pRTV( NULL )
        , pDSV( NULL )
        , pUAV( NULL )
    {
    }

    void Resource2D::Destroy()
    {
        SAFE_RELEASE( pUAV );
        SAFE_RELEASE( pDSV );
        SAFE_RELEASE( pRTV );
        SAFE_RELEASE( pSRV );
        SAFE_RELEASE( pResource );
        desc = Desc2D();
    }

    ID3D11Texture2D* Resource2D::GetResource() const
    {
        return pResource;
    }

    ID3D11ShaderResourceView* Resource2D::GetSRV() const
    {
        return pSRV;
    }

    ID3D11RenderTargetView* Resource2D::GetRTV() const
    {
        return pRTV;
    }

    ID3D11DepthStencilView* Resource2D::GetDSV() const
    {
        return pDSV;
    }

    ID3D11UnorderedAccessView* Resource2D::GetUAV() const
    {
        return pUAV;
    }

    void SetDebugName( ID3D11DeviceChild* pObject, const char* baseName, const char* suffix )
    {
        if( pObject == NULL || baseName == NULL || suffix == NULL )
        {
            return;
        }

        char debugName[128] = {};
        sprintf_s( debugName, "%s_%s", baseName, suffix );
        DXUT_SetDebugName( pObject, debugName );
    }

    HRESULT Create2D( ID3D11Device* pd3dDevice,
                      const Desc2D& desc,
                      const D3D11_SUBRESOURCE_DATA* pInitialData,
                      Resource2D& outResource )
    {
        if( pd3dDevice == NULL || desc.width == 0 || desc.height == 0 || desc.resourceFormat == DXGI_FORMAT_UNKNOWN )
        {
            return E_INVALIDARG;
        }

        outResource.Destroy();
        outResource.desc = desc;

        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = desc.width;
        textureDesc.Height = desc.height;
        textureDesc.MipLevels = desc.mipLevels;
        textureDesc.ArraySize = desc.arraySize;
        textureDesc.Format = desc.resourceFormat;
        textureDesc.SampleDesc.Count = desc.sampleCount;
        textureDesc.SampleDesc.Quality = desc.sampleQuality;
        textureDesc.Usage = desc.usage;
        textureDesc.BindFlags = desc.bindFlags;
        textureDesc.CPUAccessFlags = desc.cpuAccessFlags;
        textureDesc.MiscFlags = desc.miscFlags;

        HRESULT hr = pd3dDevice->CreateTexture2D( &textureDesc, pInitialData, &outResource.pResource );
        if( FAILED( hr ) )
        {
            outResource.Destroy();
            return hr;
        }

        if( desc.debugName != NULL )
        {
            DXUT_SetDebugName( outResource.pResource, desc.debugName );
        }

        if( desc.srvFormat != DXGI_FORMAT_UNKNOWN )
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = desc.srvFormat;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = desc.mipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;

            hr = pd3dDevice->CreateShaderResourceView( outResource.pResource, &srvDesc, &outResource.pSRV );
            if( FAILED( hr ) )
            {
                outResource.Destroy();
                return hr;
            }

            SetDebugName( outResource.pSRV, desc.debugName, "SRV" );
        }

        if( desc.rtvFormat != DXGI_FORMAT_UNKNOWN )
        {
            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = desc.rtvFormat;
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D.MipSlice = 0;

            hr = pd3dDevice->CreateRenderTargetView( outResource.pResource, &rtvDesc, &outResource.pRTV );
            if( FAILED( hr ) )
            {
                outResource.Destroy();
                return hr;
            }

            SetDebugName( outResource.pRTV, desc.debugName, "RTV" );
        }

        if( desc.dsvFormat != DXGI_FORMAT_UNKNOWN )
        {
            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = desc.dsvFormat;
            dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Texture2D.MipSlice = 0;

            hr = pd3dDevice->CreateDepthStencilView( outResource.pResource, &dsvDesc, &outResource.pDSV );
            if( FAILED( hr ) )
            {
                outResource.Destroy();
                return hr;
            }

            SetDebugName( outResource.pDSV, desc.debugName, "DSV" );
        }

        if( desc.uavFormat != DXGI_FORMAT_UNKNOWN )
        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = desc.uavFormat;
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = 0;

            hr = pd3dDevice->CreateUnorderedAccessView( outResource.pResource, &uavDesc, &outResource.pUAV );
            if( FAILED( hr ) )
            {
                outResource.Destroy();
                return hr;
            }

            SetDebugName( outResource.pUAV, desc.debugName, "UAV" );
        }

        return S_OK;
    }

    HRESULT CreateColorTarget2D( ID3D11Device* pd3dDevice,
                                 UINT width,
                                 UINT height,
                                 DXGI_FORMAT format,
                                 const char* debugName,
                                 Resource2D& outResource )
    {
        Desc2D desc;
        desc.width = width;
        desc.height = height;
        desc.resourceFormat = format;
        desc.srvFormat = format;
        desc.rtvFormat = format;
        desc.bindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        desc.type = Type::Color2D;
        desc.debugName = debugName;
        return Create2D( pd3dDevice, desc, NULL, outResource );
    }

    HRESULT CreateDepthTarget2D( ID3D11Device* pd3dDevice,
                                 UINT width,
                                 UINT height,
                                 DXGI_FORMAT resourceFormat,
                                 DXGI_FORMAT dsvFormat,
                                 DXGI_FORMAT srvFormat,
                                 const char* debugName,
                                 Resource2D& outResource )
    {
        Desc2D desc;
        desc.width = width;
        desc.height = height;
        desc.resourceFormat = resourceFormat;
        desc.srvFormat = srvFormat;
        desc.dsvFormat = dsvFormat;
        desc.bindFlags = D3D11_BIND_DEPTH_STENCIL | ( srvFormat != DXGI_FORMAT_UNKNOWN ? D3D11_BIND_SHADER_RESOURCE : 0 );
        desc.type = Type::Depth2D;
        desc.debugName = debugName;
        return Create2D( pd3dDevice, desc, NULL, outResource );
    }

    HRESULT CreateImmutableShaderTexture2D( ID3D11Device* pd3dDevice,
                                            UINT width,
                                            UINT height,
                                            DXGI_FORMAT format,
                                            const void* pData,
                                            UINT rowPitch,
                                            const char* debugName,
                                            Resource2D& outResource )
    {
        if( pData == NULL || rowPitch == 0 )
        {
            return E_INVALIDARG;
        }

        Desc2D desc;
        desc.width = width;
        desc.height = height;
        desc.resourceFormat = format;
        desc.srvFormat = format;
        desc.usage = D3D11_USAGE_IMMUTABLE;
        desc.bindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.type = Type::ShaderOnly2D;
        desc.debugName = debugName;

        D3D11_SUBRESOURCE_DATA initialData = {};
        initialData.pSysMem = pData;
        initialData.SysMemPitch = rowPitch;

        return Create2D( pd3dDevice, desc, &initialData, outResource );
    }
}
}
