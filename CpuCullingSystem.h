#pragma once

#include <vector>

#include "IRenderPass.h"
#include "FrameContext.h"
#include "SceneMesh.h"

class CpuCullingSystem : public IRenderPass
{
public:
    CpuCullingSystem();
    ~CpuCullingSystem() override;

    const char* GetPassName() const override;
    HRESULT Create( ID3D11Device* pd3dDevice ) override;
    void Destroy() override;
    HRESULT Execute( ID3D11DeviceContext* pd3dDeviceContext ) override;

    void SetMeshView( ISceneMesh* pMesh );
    void SetCameraContext( CFirstPersonCamera* pViewerCamera,
                           CFirstPersonCamera* pLightCamera,
                           CAMERA_SELECTION selectedCamera,
                           const D3DXMATRIX& matShadowView,
                           const D3DXMATRIX* pShadowProj,
                           UINT shadowProjCount );
    void SetEnabled( bool enabled );

    bool IsEnabled() const;
    bool IsSceneVisible() const;
    UINT GetTotalSubMeshCount() const;
    UINT GetVisibleSubMeshCount() const;
    const std::vector<INT>& GetVisibleSubMeshIndices() const;
    const std::vector<BoundingBox>& GetVisibleBoundingBoxes() const;
    const std::vector<BoundingBox>& GetCulledBoundingBoxes() const;

private:
    struct FrustumPlaneSet
    {
        D3DXPLANE planes[6];
    };

    void ResolveCameraMatrices( D3DXMATRIX& dxmatCameraView, D3DXMATRIX& dxmatCameraProj ) const;
    void BuildFrustumPlanes( const D3DXMATRIX& viewProjection, FrustumPlaneSet& frustumPlanes ) const;
    bool IntersectsFrustum( const BoundingBox& box, const FrustumPlaneSet& frustumPlanes ) const;

    FrameContext m_FrameContext;
    ISceneMesh* m_pMeshView;
    bool m_bEnabled;
    bool m_bSceneVisible;
    UINT m_uTotalSubMeshCount;
    std::vector<INT> m_VisibleSubMeshIndices;
    std::vector<BoundingBox> m_VisibleBoundingBoxes;
    std::vector<BoundingBox> m_CulledBoundingBoxes;
};
