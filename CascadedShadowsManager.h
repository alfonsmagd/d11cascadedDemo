//--------------------------------------------------------------------------------------
// File: CascadedShadowManager.h
//
// This is where the shadows are calcaulted and rendered.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


#pragma once

#ifndef _CASCADE_SHADOWS_H_
#define _CASCADE_SHADOWS_H_

#include "ShadowSampleMisc.h"
#include "SceneMesh.h"
#include "DebugRenderPass.h"
#include "GBufferRenderPass.h"
#include <vector>

class CFirstPersonCamera;
class ISceneMesh;

#pragma warning(push)
#pragma warning(disable: 4324)

_DECLSPEC_ALIGN_16_ class CascadedShadowsManager 
{
public:
    CascadedShadowsManager();
    ~CascadedShadowsManager();
    
    // This runs when the application is initialized.
    HRESULT Init( ID3D11Device* pd3dDevice, 
                  ID3D11DeviceContext* pd3dImmediateContext, 
                  ISceneMesh* pMesh, 
                  CFirstPersonCamera* pViewerCamera,
                  CFirstPersonCamera* pLightCamera,
                  CascadeConfig* pCascadeConfig
                );
    
    HRESULT DestroyAndDeallocateShadowResources();

    // This runs per frame.  This data could be cached when the cameras do not move.
    HRESULT InitFrame( ID3D11Device* pd3dDevice, ISceneMesh* mesh ) ;

    HRESULT RenderShadowsForAllCascades( ID3D11Device* pd3dDevice, 
                                         ID3D11DeviceContext* pd3dDeviceContext, 
                                         ISceneMesh* pMesh 
                                       );

    HRESULT RenderScene ( ID3D11DeviceContext* pd3dDeviceContext, 
                          ID3D11RenderTargetView* prtvBackBuffer, 
                          ID3D11DepthStencilView* pdsvBackBuffer, 
                          ISceneMesh* pMesh,  
                          CFirstPersonCamera* pActiveCamera,
                          D3D11_VIEWPORT* dxutViewPort,
                          BOOL bVisualize
                        );

    HRESULT RenderDebug(ID3D11DeviceContext* pd3dDeviceContext,
        ID3D11RenderTargetView* prtvBackBuffer,
        ID3D11DepthStencilView* pdsvBackBuffer,
        D3D11_VIEWPORT* dxutViewPort);

    HRESULT RenderVoxelization(ID3D11DeviceContext* pd3dDeviceContext, 
                               ISceneMesh* pMesh, CFirstPersonCamera* pActiveCamera = NULL);
    HRESULT RenderVisualizeVoxelization(ID3D11DeviceContext* pd3dDeviceContext,
        ID3D11RenderTargetView* prtvBackBuffer,
        ID3D11DepthStencilView* pdsvBackBuffer,
        ISceneMesh* pMesh,
        D3D11_VIEWPORT* dxutViewPort,
        CFirstPersonCamera* pActiveCamera,
        bool bVisualize = false);
    HRESULT HandleSceneChanged( ID3D11Device* pd3dDevice, ISceneMesh* pMesh );
    INT GetCurrentSelectedBoundingBox() const { return m_DebugRenderPass.GetSelectedBoundingBox(); }
    void InvalidateStaticVoxelization() { m_bStaticVoxelizationDirty = true; }
    void SetRenderDebugEnabled( bool enabled ) { m_bRenderDebug = enabled; }
    bool IsRenderDebugEnabled() const { return m_bRenderDebug; }
    ID3D11ShaderResourceView* GetDebugShadowMapSRV() const { return m_pCascadedShadowMapSRV; }
    INT GetDebugShadowMapAtlasWidth() const { return max( 1, m_CopyOfCascadeConfig.m_iBufferSize * max( m_CopyOfCascadeConfig.m_nCascadeLevels, 1 ) ); }
    INT GetDebugShadowMapAtlasHeight() const { return max( 1, m_CopyOfCascadeConfig.m_iBufferSize ); }
    INT GetDebugShadowMapCascadeCount() const { return max( 1, m_CopyOfCascadeConfig.m_nCascadeLevels ); }
    void SetRenderDebugBoundingBoxEnabled( bool enabled ) { m_DebugRenderPass.SetRenderBoundingBoxesEnabled( enabled ); }
    void SetRenderDebugAllBoundingBoxesEnabled(bool enabled) { m_DebugRenderPass.SetRenderAllBoundingBoxesEnabled( enabled ); }
    bool IsRenderDebugBoundingBoxEnabled() const { return m_DebugRenderPass.IsRenderBoundingBoxesEnabled(); }
    bool IsRenderDebugAllBoundingBoxesEnabled() const { return m_DebugRenderPass.IsRenderAllBoundingBoxesEnabled(); }
    HRESULT PickDebugBoundingBox( ID3D11DeviceContext* pd3dDeviceContext, const ISceneMesh* pMesh, INT mouseX, INT mouseY, const D3D11_VIEWPORT& viewport );
    HRESULT TranslateSelectedSubMesh( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dDeviceContext, ISceneMesh* pMesh, const D3DXVECTOR3& delta );

    FLOAT                               m_fStaticVoxelHeightWarp = 0.55f;
    FLOAT                               m_fVoxelVisualizeSurfaceSnap = 0.22f;
    FLOAT                               m_fVoxelXYFootprintScale = 1.65f;
    FLOAT                               m_fVoxelYZFootprintScale = 1.35f;
    FLOAT                               m_fStaticVoxelTopCoverage = 1.00f;
    FLOAT                               m_fStaticVoxelBoundsPaddingXZ = 0.04f;
    FLOAT                               m_fStaticVoxelBoundsPaddingY = 0.08f;



    XMVECTOR GetSceneAABBMin() { return m_vSceneAABBMin; };
    XMVECTOR GetSceneAABBMax() { return m_vSceneAABBMax; };

    
    INT                                 m_iCascadePartitionsMax;
    FLOAT                               m_fCascadePartitionsFrustum[MAX_CASCADES]; // Values are  between near and far
    INT                                 m_iCascadePartitionsZeroToOne[MAX_CASCADES]; // Values are 0 to 100 and represent a percent of the frstum
    INT                                 m_iPCFBlurSize;
    FLOAT                               m_fPCFOffset;
    INT                                 m_iDerivativeBasedOffset;
    INT                                 m_iBlurBetweenCascades;
    FLOAT                               m_fBlurBetweenCascadesAmount;

    BOOL                                m_bMoveLightTexelSize;
    CAMERA_SELECTION                    m_eSelectedCamera;
    FIT_PROJECTION_TO_CASCADES          m_eSelectedCascadesFit;
    FIT_TO_NEAR_FAR                     m_eSelectedNearFarFit;
    CASCADE_SELECTION                   m_eSelectedCascadeSelection;
    

private:

    HRESULT InitializeSceneContext( ID3D11Device* pd3dDevice,
                                    ISceneMesh* pMesh,
                                    CFirstPersonCamera* pViewerCamera,
                                    CFirstPersonCamera* pLightCamera,
                                    CascadeConfig* pCascadeConfig );
    HRESULT CreateShaders( ID3D11Device* pd3dDevice );
    HRESULT CreateGeometryResources( ID3D11Device* pd3dDevice );
    HRESULT CreateRenderStateResources( ID3D11Device* pd3dDevice );
    HRESULT CreateConstantBufferResources( ID3D11Device* pd3dDevice );
    HRESULT CreateDebugResources( ID3D11Device* pd3dDevice, const ISceneMesh* pMesh );
    HRESULT CreateGBufferResources(ID3D11Device* pd3dDevice, const ISceneMesh* pMesh);
    HRESULT CreateShadowMapResources( ID3D11Device* pd3dDevice );

    // Compute the near and far plane by intersecting an Ortho Projection with the Scenes AABB.
    void ComputeNearAndFar( FLOAT& fNearPlane, 
                            FLOAT& fFarPlane, 
                            FXMVECTOR vLightCameraOrthographicMin, 
                            FXMVECTOR vLightCameraOrthographicMax, 
                            XMVECTOR* pvPointsInCameraView 
                          );
    

    void CreateFrustumPointsFromCascadeInterval ( FLOAT fCascadeIntervalBegin, 
                                                  FLOAT fCascadeIntervalEnd, 
                                                  XMMATRIX& vProjection,
                                                  XMVECTOR* pvCornerPointsWorld
                                                );


    void CreateAABBPoints( XMVECTOR* vAABBPoints, FXMVECTOR vCenter, FXMVECTOR vExtents );


    HRESULT ReleaseAndAllocateNewShadowResources( ID3D11Device* pd3dDevice );  // This is called when cascade config changes. 
    HRESULT EnsureRenderSceneVertexShader(ID3D11Device* pd3dDevice, INT cascadeIndex);
    HRESULT EnsureRenderScenePixelShader(ID3D11Device* pd3dDevice, INT cascadeIndex, INT derivativeIndex, INT blendIndex, INT intervalIndex);
    HRESULT RenderVoxelizationVolume(ID3D11DeviceContext* pd3dDeviceContext,
        ISceneMesh* pMesh,
        FXMVECTOR vVoxelMin,
        FXMVECTOR vVoxelMax,
        UINT gridSizeX,
        UINT gridSizeY,
        UINT gridSizeZ,
        FLOAT heightWarp,
        ID3D11UnorderedAccessView* pAlbedoUAV,
        ID3D11UnorderedAccessView* pMaskUAV,
        ID3D11UnorderedAccessView* pInstanceUAV = NULL,
        ID3D11Buffer* pDrawArgsBuffer = NULL);

    XMVECTOR                            m_vSceneAABBMin;
    XMVECTOR                            m_vSceneAABBMax;
    XMVECTOR                            m_vStaticVoxelAABBMin;
    XMVECTOR                            m_vStaticVoxelAABBMax;
    XMVECTOR                            m_vDynamicVoxelAABBMin;
    XMVECTOR                            m_vDynamicVoxelAABBMax;
    bool                                m_bStaticVoxelizationDirty = true;
    bool                                m_bRenderDebug = false;
                                                                               // For example: when the shadow buffer size changes.
    char                                m_cvsModel[31];
    char                                m_cpsModel[31];
    char                                m_cgsModel[31];
    D3DXMATRIX                          m_matShadowProj[MAX_CASCADES]; 
    D3DXMATRIX                          m_matShadowView;
    CascadeConfig                       m_CopyOfCascadeConfig;      // This copy is used to determine when settings change. 
                                                                    //Some of these settings require new buffer allocations.
    CascadeConfig*                      m_pCascadeConfig;           // Pointer to the most recent setting.

// D3D11 variables
    ID3D11InputLayout*                  m_pVertexLayoutMesh = nullptr;
    ID3D11InputLayout*                  m_pVertexLayoutVoxelVisualize = nullptr;

    ID3D11VertexShader*                 m_pvsRenderOrthoShadow = nullptr;
    ID3D11VertexShader*                 m_pvsVoxelization = nullptr;

    ID3DBlob*                           m_pgsVoxelizationBlob = nullptr;

    ID3DBlob*                           m_pvsRenderOrthoShadowBlob = nullptr;
    ID3DBlob*                           m_pvsVoxelizationBlob = nullptr;
    ID3D11VertexShader*                 m_pvsRenderScene[MAX_CASCADES];
    ID3D11VertexShader*                 m_pvsVisualizeVoxelization = nullptr;
    ID3DBlob*                           m_pvsVisualizeVoxelizationBlob = nullptr;
    ID3DBlob*                           m_pvsRenderSceneBlob[MAX_CASCADES];
    ID3D11PixelShader*                  m_ppsRenderSceneAllShaders[MAX_CASCADES][2][2][2];
    ID3DBlob*                           m_ppsRenderSceneAllShadersBlob[MAX_CASCADES][2][2][2];
    ID3D11PixelShader*                  m_ppsVisualizeVoxelization = nullptr;
    ID3D11PixelShader*                  m_ppsVoxelization = nullptr;
    ID3D11GeometryShader*               m_pgsVoxelization = nullptr;
    ID3DBlob*                           m_ppsVoxelizationBlob = nullptr;
    ID3DBlob*                           m_ppsVisualizeVoxelizationBlob = nullptr;

    ID3D11Texture2D*                    m_pCascadedShadowMapTexture = nullptr;
    ID3D11DepthStencilView*             m_pCascadedShadowMapDSV = nullptr;
    ID3D11ShaderResourceView*           m_pCascadedShadowMapSRV = nullptr;
    ID3D11Texture3D*                    m_pVoxelAlbedoTex = nullptr;
    ID3D11UnorderedAccessView*          m_pVoxelAlbedoUAV = nullptr;
    ID3D11ShaderResourceView*           m_pVoxelAlbedoSRV = nullptr;

    ID3D11Texture3D*                    m_pVoxelMaskTex = nullptr;
    ID3D11UnorderedAccessView*          m_pVoxelMaskUAV = nullptr;
    ID3D11ShaderResourceView*           m_pVoxelMaskSRV = nullptr;

    ID3D11Buffer*                       m_pVoxelInstanceBuffer = nullptr;
    ID3D11ShaderResourceView*           m_pVoxelInstanceSRV = nullptr;
    ID3D11UnorderedAccessView*          m_pVoxelInstanceUAV = nullptr;
    ID3D11Buffer*                       m_pVoxelDrawArgsBuffer = nullptr;
    ID3D11Buffer*                       m_pVoxelCubeVertexBuffer = nullptr;

    ID3D11Texture3D*                    m_pDynamicVoxelAlbedoTex = nullptr;
    ID3D11UnorderedAccessView*          m_pDynamicVoxelAlbedoUAV = nullptr;
    ID3D11ShaderResourceView*           m_pDynamicVoxelAlbedoSRV = nullptr;

    ID3D11Texture3D*                    m_pDynamicVoxelMaskTex = nullptr;
    ID3D11UnorderedAccessView*          m_pDynamicVoxelMaskUAV = nullptr;
    ID3D11ShaderResourceView*           m_pDynamicVoxelMaskSRV = nullptr;


#if  TEXTURE_2D_SHADOWMAP 
	ID3D11Texture2D*                    m_pCascadedShadowMapTextureArray = nullptr;
#endif

    ID3D11Buffer*                       m_pcbVoxelParams = nullptr;
    ID3D11Buffer*                       m_pcbVisualizeVoxels = nullptr;
    ID3D11Buffer*                       m_pcbGlobalConstantBuffer = nullptr; // All VS and PS constants are in the same buffer.  
                                                          // An actual title would break this up into multiple 
                                                          // buffers updated based on frequency of variable changes

    ID3D11RasterizerState*              m_prsScene = nullptr;
    ID3D11RasterizerState*              m_prsVoxelization = nullptr;
    ID3D11RasterizerState*              m_prsShadow = nullptr;
    ID3D11RasterizerState*              m_prsShadowPancake = nullptr;
    ID3D11BlendState*                   m_pbsVoxelVisualize = nullptr;
    
    D3D11_VIEWPORT                      m_RenderVP[MAX_CASCADES];
    D3D11_VIEWPORT                      m_RenderOneTileVP;

    CFirstPersonCamera*                 m_pViewerCamera = nullptr;         
    CFirstPersonCamera*                 m_pLightCamera = nullptr;         

    ID3D11SamplerState*                 m_pSamLinear = nullptr;
    ID3D11SamplerState*                 m_pSamShadowPCF = nullptr;
    ID3D11SamplerState*                 m_pSamShadowPoint = nullptr;
    DebugRenderPass                     m_DebugRenderPass;
    GBufferRenderPass                   m_GbufferRenderPass;
};

#pragma warning(pop)

#endif
