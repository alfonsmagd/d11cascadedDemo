#include "DXUT.h"

#include "CpuCullingSystem.h"

#include "DXUTcamera.h"

namespace
{
    const INT kFrustumPlaneCount = 6;
}

CpuCullingSystem::CpuCullingSystem()
    : m_pMeshView( NULL )
    , m_bEnabled( true )
    , m_bSceneVisible( false )
    , m_uTotalSubMeshCount( 0 )
{
}

CpuCullingSystem::~CpuCullingSystem()
{
    Destroy();
}

const char* CpuCullingSystem::GetPassName() const
{
    return "CpuCulling";
}

HRESULT CpuCullingSystem::Create( ID3D11Device* pd3dDevice )
{
    UNREFERENCED_PARAMETER( pd3dDevice );
    return S_OK;
}

void CpuCullingSystem::Destroy()
{
    m_pMeshView = NULL;
    m_bSceneVisible = false;
    m_uTotalSubMeshCount = 0;
    m_VisibleSubMeshIndices.clear();
    m_VisibleBoundingBoxes.clear();
    m_CulledBoundingBoxes.clear();
}

HRESULT CpuCullingSystem::Execute( ID3D11DeviceContext* pd3dDeviceContext )
{
    UNREFERENCED_PARAMETER( pd3dDeviceContext );

    m_bSceneVisible = false;
    m_uTotalSubMeshCount = 0;
    m_VisibleSubMeshIndices.clear();
    m_VisibleBoundingBoxes.clear();
    m_CulledBoundingBoxes.clear();

    if( !m_bEnabled )
    {
        return S_FALSE;
    }

    if( m_pMeshView == NULL || !m_pMeshView->IsLoaded() || m_FrameContext.pViewerCamera == NULL )
    {
        return S_FALSE;
    }

    D3DXMATRIX dxmatCameraView;
    D3DXMATRIX dxmatCameraProj;
    ResolveCameraMatrices( dxmatCameraView, dxmatCameraProj );

    const D3DXMATRIX dxmatViewProjection = dxmatCameraView * dxmatCameraProj;
    FrustumPlaneSet frustumPlanes = {};
    BuildFrustumPlanes( dxmatViewProjection, frustumPlanes );

    std::vector<BoundingBox> allBoundingBoxes;
    m_pMeshView->UpdateAllBoundingBoxes( allBoundingBoxes );
    m_uTotalSubMeshCount = UINT( allBoundingBoxes.size() );

    for( size_t subMeshIndex = 0; subMeshIndex < allBoundingBoxes.size(); ++subMeshIndex )
    {
        const BoundingBox& box = allBoundingBoxes[subMeshIndex];
        if( IntersectsFrustum( box, frustumPlanes ) )
        {
            m_VisibleSubMeshIndices.push_back( INT( subMeshIndex ) );
            m_VisibleBoundingBoxes.push_back( box );
        }
        else
        {
            m_CulledBoundingBoxes.push_back( box );
        }
    }

    m_bSceneVisible = !m_VisibleSubMeshIndices.empty();
    return S_OK;
}

void CpuCullingSystem::SetMeshView( ISceneMesh* pMesh )
{
    m_pMeshView = pMesh;
}

void CpuCullingSystem::SetCameraContext( CFirstPersonCamera* pViewerCamera,
                                         CFirstPersonCamera* pLightCamera,
                                         CAMERA_SELECTION selectedCamera,
                                         const D3DXMATRIX& matShadowView,
                                         const D3DXMATRIX* pShadowProj,
                                         UINT shadowProjCount )
{
    m_FrameContext.pViewerCamera = pViewerCamera;
    m_FrameContext.pLightCamera = pLightCamera;
    m_FrameContext.selectedCamera = selectedCamera;
    m_FrameContext.matShadowView = matShadowView;

    const UINT cascadeCount = min( shadowProjCount, UINT( MAX_CASCADES ) );
    for( UINT cascadeIndex = 0; cascadeIndex < cascadeCount; ++cascadeIndex )
    {
        m_FrameContext.matShadowProj[cascadeIndex] = pShadowProj[cascadeIndex];
    }
}

void CpuCullingSystem::SetEnabled( bool enabled )
{
    m_bEnabled = enabled;
}

bool CpuCullingSystem::IsEnabled() const
{
    return m_bEnabled;
}

bool CpuCullingSystem::IsSceneVisible() const
{
    return m_bSceneVisible;
}

UINT CpuCullingSystem::GetTotalSubMeshCount() const
{
    return m_uTotalSubMeshCount;
}

UINT CpuCullingSystem::GetVisibleSubMeshCount() const
{
    return UINT( m_VisibleSubMeshIndices.size() );
}

const std::vector<INT>& CpuCullingSystem::GetVisibleSubMeshIndices() const
{
    return m_VisibleSubMeshIndices;
}

const std::vector<BoundingBox>& CpuCullingSystem::GetVisibleBoundingBoxes() const
{
    return m_VisibleBoundingBoxes;
}

const std::vector<BoundingBox>& CpuCullingSystem::GetCulledBoundingBoxes() const
{
    return m_CulledBoundingBoxes;
}

void CpuCullingSystem::ResolveCameraMatrices( D3DXMATRIX& dxmatCameraView, D3DXMATRIX& dxmatCameraProj ) const
{
    dxmatCameraProj = *m_FrameContext.pViewerCamera->GetProjMatrix();
    dxmatCameraView = *m_FrameContext.pViewerCamera->GetViewMatrix();

    if( m_FrameContext.selectedCamera == LIGHT_CAMERA && m_FrameContext.pLightCamera != NULL )
    {
        dxmatCameraProj = *m_FrameContext.pLightCamera->GetProjMatrix();
        dxmatCameraView = *m_FrameContext.pLightCamera->GetViewMatrix();
    }
    else if( m_FrameContext.selectedCamera >= ORTHO_CAMERA1 )
    {
        const INT cascadeIndex = INT( m_FrameContext.selectedCamera ) - INT( ORTHO_CAMERA1 );
        if( cascadeIndex >= 0 && cascadeIndex < MAX_CASCADES )
        {
            dxmatCameraProj = m_FrameContext.matShadowProj[cascadeIndex];
            dxmatCameraView = m_FrameContext.matShadowView;
        }
    }
}

void CpuCullingSystem::BuildFrustumPlanes( const D3DXMATRIX& viewProjection, FrustumPlaneSet& frustumPlanes ) const
{
    frustumPlanes.planes[0] = D3DXPLANE(
        viewProjection._14 + viewProjection._11,
        viewProjection._24 + viewProjection._21,
        viewProjection._34 + viewProjection._31,
        viewProjection._44 + viewProjection._41 );

    frustumPlanes.planes[1] = D3DXPLANE(
        viewProjection._14 - viewProjection._11,
        viewProjection._24 - viewProjection._21,
        viewProjection._34 - viewProjection._31,
        viewProjection._44 - viewProjection._41 );

    frustumPlanes.planes[2] = D3DXPLANE(
        viewProjection._14 + viewProjection._12,
        viewProjection._24 + viewProjection._22,
        viewProjection._34 + viewProjection._32,
        viewProjection._44 + viewProjection._42 );

    frustumPlanes.planes[3] = D3DXPLANE(
        viewProjection._14 - viewProjection._12,
        viewProjection._24 - viewProjection._22,
        viewProjection._34 - viewProjection._32,
        viewProjection._44 - viewProjection._42 );

    frustumPlanes.planes[4] = D3DXPLANE(
        viewProjection._13,
        viewProjection._23,
        viewProjection._33,
        viewProjection._43 );

    frustumPlanes.planes[5] = D3DXPLANE(
        viewProjection._14 - viewProjection._13,
        viewProjection._24 - viewProjection._23,
        viewProjection._34 - viewProjection._33,
        viewProjection._44 - viewProjection._43 );

    for( INT planeIndex = 0; planeIndex < kFrustumPlaneCount; ++planeIndex )
    {
        D3DXPlaneNormalize( &frustumPlanes.planes[planeIndex], &frustumPlanes.planes[planeIndex] );
    }
}

bool CpuCullingSystem::IntersectsFrustum( const BoundingBox& box, const FrustumPlaneSet& frustumPlanes ) const
{
    for( INT planeIndex = 0; planeIndex < kFrustumPlaneCount; ++planeIndex )
    {
        bool allCornersOutside = true;
        for( INT cornerIndex = 0; cornerIndex < 8; ++cornerIndex )
        {
            const XMFLOAT3& corner = box.corners[cornerIndex];
            const float distance =
                ( frustumPlanes.planes[planeIndex].a * corner.x ) +
                ( frustumPlanes.planes[planeIndex].b * corner.y ) +
                ( frustumPlanes.planes[planeIndex].c * corner.z ) +
                frustumPlanes.planes[planeIndex].d;

            if( distance >= 0.0f )
            {
                allCornersOutside = false;
                break;
            }
        }

        if( allCornersOutside )
        {
            return false;
        }
    }

    return true;
}
