#pragma once

#include "DXUT.h"

class IRenderPass
{
public:
    virtual ~IRenderPass()
    {
    }

    virtual const char* GetPassName() const = 0;
    virtual HRESULT Create( ID3D11Device* pd3dDevice ) = 0;
    virtual void Destroy() = 0;
    virtual HRESULT Execute( ID3D11DeviceContext* pd3dDeviceContext ) = 0;
};
