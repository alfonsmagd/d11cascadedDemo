#pragma once

#include "DXUT.h"

namespace DX
{
namespace Buffer
{
    enum class Type
    {
        Unknown,
        Constant,
        Vertex,
        Index,
        Structured,
        IndirectArgs
    };

    enum class Lifetime
    {
        Persistent,
        Transient
    };

    struct Desc
    {
        UINT byteWidth;
        UINT structureByteStride;
        D3D11_USAGE usage;
        UINT bindFlags;
        UINT cpuAccessFlags;
        UINT miscFlags;
        UINT slot;
        Type type;
        Lifetime lifetime;
        const char* debugName;

        Desc();
    };

    struct Resource
    {
        Desc desc;
        ID3D11Buffer* pBuffer;
        ID3D11ShaderResourceView* pSRV;
        ID3D11UnorderedAccessView* pUAV;

        Resource();

        void Destroy();
        ID3D11Buffer* GetBuffer() const;
        ID3D11ShaderResourceView* GetSRV() const;
        ID3D11UnorderedAccessView* GetUAV() const;
        UINT GetSlot() const;
        Lifetime GetLifetime() const;
        Type GetType() const;
    };

    UINT AlignConstantByteWidth( UINT byteWidth );
    void SetDebugName( ID3D11DeviceChild* pObject, const char* baseName, const char* suffix );

    HRESULT Create( ID3D11Device* pd3dDevice,
                    const Desc& desc,
                    const D3D11_SUBRESOURCE_DATA* pInitialData,
                    Resource& outResource );

    HRESULT CreateConstant( ID3D11Device* pd3dDevice,
                            UINT byteWidth,
                            UINT slot,
                            const char* debugName,
                            Resource& outResource );

    HRESULT CreateTransientConstant( ID3D11Device* pd3dDevice,
                                     UINT byteWidth,
                                     UINT slot,
                                     const char* debugName,
                                     Resource& outResource );

    HRESULT CreateStructuredReadOnly( ID3D11Device* pd3dDevice,
                                      UINT elementCount,
                                      UINT elementStride,
                                      const char* debugName,
                                      Resource& outResource );

    HRESULT CreateTransientStructuredReadOnly( ID3D11Device* pd3dDevice,
                                               UINT elementCount,
                                               UINT elementStride,
                                               const char* debugName,
                                               Resource& outResource );

    HRESULT MapWriteDiscard( ID3D11DeviceContext* pd3dDeviceContext,
                             const Resource& resource,
                             D3D11_MAPPED_SUBRESOURCE& mappedResource );

    void Unmap( ID3D11DeviceContext* pd3dDeviceContext, const Resource& resource );

    void BindConstantVS( ID3D11DeviceContext* pd3dDeviceContext, const Resource& resource );
    void BindConstantPS( ID3D11DeviceContext* pd3dDeviceContext, const Resource& resource );
    void BindConstantGS( ID3D11DeviceContext* pd3dDeviceContext, const Resource& resource );
    void BindConstantCS( ID3D11DeviceContext* pd3dDeviceContext, const Resource& resource );
}
}
