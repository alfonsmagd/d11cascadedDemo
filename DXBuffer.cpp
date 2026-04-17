#include <cstdio>

#include "DXUT.h"
#include "DXBuffer.h"

namespace DX
{
namespace Buffer
{
    Desc::Desc()
        : byteWidth( 0 )
        , structureByteStride( 0 )
        , usage( D3D11_USAGE_DEFAULT )
        , bindFlags( 0 )
        , cpuAccessFlags( 0 )
        , miscFlags( 0 )
        , slot( 0 )
        , type( Type::Unknown )
        , lifetime( Lifetime::Persistent )
        , debugName( NULL )
    {
    }

    Resource::Resource()
        : pBuffer( NULL )
        , pSRV( NULL )
        , pUAV( NULL )
    {
    }

    void Resource::Destroy()
    {
        SAFE_RELEASE( pUAV );
        SAFE_RELEASE( pSRV );
        SAFE_RELEASE( pBuffer );
        desc = Desc();
    }

    ID3D11Buffer* Resource::GetBuffer() const
    {
        return pBuffer;
    }

    ID3D11ShaderResourceView* Resource::GetSRV() const
    {
        return pSRV;
    }

    ID3D11UnorderedAccessView* Resource::GetUAV() const
    {
        return pUAV;
    }

    UINT Resource::GetSlot() const
    {
        return desc.slot;
    }

    Lifetime Resource::GetLifetime() const
    {
        return desc.lifetime;
    }

    Type Resource::GetType() const
    {
        return desc.type;
    }

    UINT AlignConstantByteWidth( UINT byteWidth )
    {
        return ( byteWidth + 15u ) & ~15u;
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

    HRESULT Create( ID3D11Device* pd3dDevice,
                    const Desc& desc,
                    const D3D11_SUBRESOURCE_DATA* pInitialData,
                    Resource& outResource )
    {
        if( pd3dDevice == NULL || desc.byteWidth == 0 )
        {
            return E_INVALIDARG;
        }

        outResource.Destroy();
        outResource.desc = desc;

        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.ByteWidth = desc.byteWidth;
        bufferDesc.Usage = desc.usage;
        bufferDesc.BindFlags = desc.bindFlags;
        bufferDesc.CPUAccessFlags = desc.cpuAccessFlags;
        bufferDesc.MiscFlags = desc.miscFlags;
        bufferDesc.StructureByteStride = desc.structureByteStride;

        HRESULT hr = pd3dDevice->CreateBuffer( &bufferDesc, pInitialData, &outResource.pBuffer );
        if( FAILED( hr ) )
        {
            outResource.Destroy();
            return hr;
        }

        if( desc.debugName != NULL )
        {
            DXUT_SetDebugName( outResource.pBuffer, desc.debugName );
        }

        if( ( desc.bindFlags & D3D11_BIND_SHADER_RESOURCE ) != 0 )
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DXGI_FORMAT_UNKNOWN;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
            srvDesc.Buffer.FirstElement = 0;
            srvDesc.Buffer.NumElements = desc.structureByteStride ? ( desc.byteWidth / desc.structureByteStride ) : 0;

            hr = pd3dDevice->CreateShaderResourceView( outResource.pBuffer, &srvDesc, &outResource.pSRV );
            if( FAILED( hr ) )
            {
                outResource.Destroy();
                return hr;
            }

            SetDebugName( outResource.pSRV, desc.debugName, "SRV" );
        }

        if( ( desc.bindFlags & D3D11_BIND_UNORDERED_ACCESS ) != 0 )
        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = DXGI_FORMAT_UNKNOWN;
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
            uavDesc.Buffer.FirstElement = 0;
            uavDesc.Buffer.NumElements = desc.structureByteStride ? ( desc.byteWidth / desc.structureByteStride ) : 0;

            hr = pd3dDevice->CreateUnorderedAccessView( outResource.pBuffer, &uavDesc, &outResource.pUAV );
            if( FAILED( hr ) )
            {
                outResource.Destroy();
                return hr;
            }

            SetDebugName( outResource.pUAV, desc.debugName, "UAV" );
        }

        return S_OK;
    }

    static HRESULT CreateConstantInternal( ID3D11Device* pd3dDevice,
                                           UINT byteWidth,
                                           UINT slot,
                                           Lifetime lifetime,
                                           const char* debugName,
                                           Resource& outResource )
    {
        Desc desc;
        desc.byteWidth = AlignConstantByteWidth( byteWidth );
        desc.usage = D3D11_USAGE_DYNAMIC;
        desc.bindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.cpuAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.slot = slot;
        desc.type = Type::Constant;
        desc.lifetime = lifetime;
        desc.debugName = debugName;
        return Create( pd3dDevice, desc, NULL, outResource );
    }

    HRESULT CreateConstant( ID3D11Device* pd3dDevice,
                            UINT byteWidth,
                            UINT slot,
                            const char* debugName,
                            Resource& outResource )
    {
        return CreateConstantInternal( pd3dDevice, byteWidth, slot, Lifetime::Persistent, debugName, outResource );
    }

    HRESULT CreateTransientConstant( ID3D11Device* pd3dDevice,
                                     UINT byteWidth,
                                     UINT slot,
                                     const char* debugName,
                                     Resource& outResource )
    {
        return CreateConstantInternal( pd3dDevice, byteWidth, slot, Lifetime::Transient, debugName, outResource );
    }

    static HRESULT CreateStructuredInternal( ID3D11Device* pd3dDevice,
                                             UINT elementCount,
                                             UINT elementStride,
                                             Lifetime lifetime,
                                             const char* debugName,
                                             Resource& outResource )
    {
        if( elementCount == 0 || elementStride == 0 )
        {
            return E_INVALIDARG;
        }

        Desc desc;
        desc.byteWidth = elementCount * elementStride;
        desc.structureByteStride = elementStride;
        desc.usage = D3D11_USAGE_DEFAULT;
        desc.bindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.miscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        desc.type = Type::Structured;
        desc.lifetime = lifetime;
        desc.debugName = debugName;
        return Create( pd3dDevice, desc, NULL, outResource );
    }

    HRESULT CreateStructuredReadOnly( ID3D11Device* pd3dDevice,
                                      UINT elementCount,
                                      UINT elementStride,
                                      const char* debugName,
                                      Resource& outResource )
    {
        return CreateStructuredInternal( pd3dDevice, elementCount, elementStride, Lifetime::Persistent, debugName, outResource );
    }

    HRESULT CreateTransientStructuredReadOnly( ID3D11Device* pd3dDevice,
                                               UINT elementCount,
                                               UINT elementStride,
                                               const char* debugName,
                                               Resource& outResource )
    {
        return CreateStructuredInternal( pd3dDevice, elementCount, elementStride, Lifetime::Transient, debugName, outResource );
    }

    HRESULT MapWriteDiscard( ID3D11DeviceContext* pd3dDeviceContext,
                             const Resource& resource,
                             D3D11_MAPPED_SUBRESOURCE& mappedResource )
    {
        if( pd3dDeviceContext == NULL || resource.GetBuffer() == NULL )
        {
            return E_INVALIDARG;
        }

        return pd3dDeviceContext->Map( resource.GetBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
    }

    void Unmap( ID3D11DeviceContext* pd3dDeviceContext, const Resource& resource )
    {
        if( pd3dDeviceContext == NULL || resource.GetBuffer() == NULL )
        {
            return;
        }

        pd3dDeviceContext->Unmap( resource.GetBuffer(), 0 );
    }

    void BindConstantVS( ID3D11DeviceContext* pd3dDeviceContext, const Resource& resource )
    {
        ID3D11Buffer* pBuffer = resource.GetBuffer();
        if( pd3dDeviceContext != NULL && pBuffer != NULL )
        {
            pd3dDeviceContext->VSSetConstantBuffers( resource.GetSlot(), 1, &pBuffer );
        }
    }

    void BindConstantPS( ID3D11DeviceContext* pd3dDeviceContext, const Resource& resource )
    {
        ID3D11Buffer* pBuffer = resource.GetBuffer();
        if( pd3dDeviceContext != NULL && pBuffer != NULL )
        {
            pd3dDeviceContext->PSSetConstantBuffers( resource.GetSlot(), 1, &pBuffer );
        }
    }

    void BindConstantGS( ID3D11DeviceContext* pd3dDeviceContext, const Resource& resource )
    {
        ID3D11Buffer* pBuffer = resource.GetBuffer();
        if( pd3dDeviceContext != NULL && pBuffer != NULL )
        {
            pd3dDeviceContext->GSSetConstantBuffers( resource.GetSlot(), 1, &pBuffer );
        }
    }

    void BindConstantCS( ID3D11DeviceContext* pd3dDeviceContext, const Resource& resource )
    {
        ID3D11Buffer* pBuffer = resource.GetBuffer();
        if( pd3dDeviceContext != NULL && pBuffer != NULL )
        {
            pd3dDeviceContext->CSSetConstantBuffers( resource.GetSlot(), 1, &pBuffer );
        }
    }
}
}
