#pragma once

#include "ShadowSampleMisc.h"

class CFirstPersonCamera;

struct FrameContext
{
    CFirstPersonCamera* pViewerCamera;
    CFirstPersonCamera* pLightCamera;
    CAMERA_SELECTION selectedCamera;
    D3DXMATRIX matShadowView;
    D3DXMATRIX matShadowProj[MAX_CASCADES];

    FrameContext()
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
