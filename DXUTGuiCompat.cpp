#include "DXUT.h"
#include "DXUTgui.h"

// SDKmisc still ships text helper entry points that were previously backed by DXUT GUI.
// The runtime now renders all tooling through Dear ImGui, so these compatibility stubs
// keep the sample linkable without reviving the old DXUT dialog system.
void CDXUTDialogResourceManager::StoreD3D11State( ID3D11DeviceContext* pd3dImmediateContext )
{
    UNREFERENCED_PARAMETER( pd3dImmediateContext );
}

void CDXUTDialogResourceManager::RestoreD3D11State( ID3D11DeviceContext* pd3dImmediateContext )
{
    UNREFERENCED_PARAMETER( pd3dImmediateContext );
}

void CDXUTDialogResourceManager::ApplyRenderUI11( ID3D11DeviceContext* pd3dImmediateContext )
{
    UNREFERENCED_PARAMETER( pd3dImmediateContext );
}

void BeginText11()
{
}

void DrawText11DXUT( ID3D11Device* pd3dDevice,
                     ID3D11DeviceContext* pd3d11DeviceContext,
                     LPCWSTR strText,
                     RECT rcScreen,
                     D3DXCOLOR vFontColor,
                     float fBBWidth,
                     float fBBHeight,
                     bool bCenter )
{
    UNREFERENCED_PARAMETER( pd3dDevice );
    UNREFERENCED_PARAMETER( pd3d11DeviceContext );
    UNREFERENCED_PARAMETER( strText );
    UNREFERENCED_PARAMETER( rcScreen );
    UNREFERENCED_PARAMETER( vFontColor );
    UNREFERENCED_PARAMETER( fBBWidth );
    UNREFERENCED_PARAMETER( fBBHeight );
    UNREFERENCED_PARAMETER( bCenter );
}

void EndText11( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3d11DeviceContext )
{
    UNREFERENCED_PARAMETER( pd3dDevice );
    UNREFERENCED_PARAMETER( pd3d11DeviceContext );
}
