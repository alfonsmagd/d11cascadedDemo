//--------------------------------------------------------------------------------------
// File: Cascaded11.cpp
//
// This sample demonstrates cascaded shadow maps.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTcamera.h"
#include "SDKmisc.h"
#include "resource.h"

#include "ShadowSampleMisc.h"
#include "CascadedShadowsManager.h"
#include "SceneMesh.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include <commdlg.h>
#include <vector>
#include "WaitDlg.h"

#pragma comment(lib, "legacy_stdio_definitions.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CascadedShadowsManager      g_CascadedShadow;

CFirstPersonCamera          g_ViewerCamera;          
CFirstPersonCamera          g_LightCamera;         
CFirstPersonCamera*         g_pActiveCamera = &g_ViewerCamera;

CascadeConfig               g_CascadeConfig;
SDKSceneMesh                g_MeshPowerPlant( L"powerplant\\powerplant.sdkmesh" );
SDKSceneMesh                g_MeshTestScene( L"ShadowColumns\\testscene.sdkmesh" );
OBJSceneMesh                g_MeshSponza( L"sponza\\sponza.obj", 0.05f );
SimpleSceneMesh             g_MeshSimpleScene;
ISceneMesh*                 g_pSelectedMesh = &g_MeshPowerPlant;                
SCENE_SELECTION             g_eSelectedScene = POWER_PLANT_SCENE;

D3DXMATRIX                  g_mCenterMesh;
INT                         g_nNumActiveLights;
INT                         g_nActiveLight;
BOOL                        g_bShowHelp = FALSE;    // If true, it renders the UI control text
bool                        g_bVisualizeCascades = FALSE;
bool                        g_bVisualizeVoxel = FALSE;
bool                        g_bMoveLightTexelSize = TRUE;
FLOAT                       g_fAspectRatio = 1.0f;
float                       g_fDepthMin;
float                       g_fDepthMax;
float                       g_fDepthScale;
bool                        g_bImGuiInitialized = false;
bool                        g_bShowImGuiOverlay = true;
bool                        g_bShowImGuiDemoWindow = false;
bool                        g_bShowImGuiMetricsWindow = false;

enum IMGUI_PANEL_TAB
{
    IMGUI_PANEL_TAB_SELECTOR = 0,
    IMGUI_PANEL_TAB_DEBUG,
    IMGUI_PANEL_TAB_VOXELIZATION,
    IMGUI_PANEL_TAB_CASCADES
};

IMGUI_PANEL_TAB             g_eSelectedImGuiTab = IMGUI_PANEL_TAB_SELECTOR;

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, FLOAT fElapsedTime, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext );
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
void CALLBACK OnD3D11DestroyDevice( void* pUserContext );
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                  FLOAT fElapsedTime, void* pUserContext );

void InitApp();
HRESULT DestroyD3DComponents();
HRESULT CreateD3DComponents( ID3D11Device* pd3dDevice );
void UpdateViewerCameraNearFar();
void ResetSceneCameras();
HRESULT EnsureSceneMeshLoaded( ID3D11Device* pd3dDevice, ISceneMesh* pMesh );
HRESULT ApplySceneSelectionChange();
HRESULT InitializeImGui( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext );
void ShutdownImGui();
void RenderImGuiOverlay( ID3D11DeviceContext* pd3dImmediateContext, double fTime, FLOAT fElapsedTime,
                         ID3D11RenderTargetView* pRTV, ID3D11DepthStencilView* pDSV );
bool ImGuiWantsToCaptureMessage( UINT uMsg, WPARAM wParam );
const char* GetSceneSelectionName( SCENE_SELECTION sceneSelection );
const char* GetShadowTextureFormatName( SHADOW_TEXTURE_FORMAT format );
const char* GetCameraSelectionName( CAMERA_SELECTION cameraSelection );
const char* GetProjectionFitName( FIT_PROJECTION_TO_CASCADES fit );
const char* GetNearFarFitName( FIT_TO_NEAR_FAR fit );
const char* GetCascadeSelectionName( CASCADE_SELECTION selection );
void NormalizeShadowSettings( INT editedCascadePartitionIndex = -1 );
void RenderImGuiSelectorTab();
void RenderImGuiDebugTab();
void RenderImGuiVoxelizationTab();
void RenderImGuiCascadesTab();

const char* GetSceneSelectionName( SCENE_SELECTION sceneSelection )
{
    switch( sceneSelection )
    {
        case POWER_PLANT_SCENE:
            return "Power Plant";
        case TEST_SCENE:
            return "Test Scene";
        case SPONZA_SCENE:
            return "Sponza";
        case SIMPLE_SCENE:
            return "Simple Scene";
        default:
            return "Unknown";
    }
}

const char* GetShadowTextureFormatName( SHADOW_TEXTURE_FORMAT format )
{
    switch( format )
    {
        case CASCADE_DXGI_FORMAT_R32_TYPELESS:
            return "32 bit Buffer";
        case CASCADE_DXGI_FORMAT_R24G8_TYPELESS:
            return "24 bit Buffer";
        case CASCADE_DXGI_FORMAT_R16_TYPELESS:
            return "16 bit Buffer";
        case CASCADE_DXGI_FORMAT_R8_TYPELESS:
            return "8 bit Buffer";
        default:
            return "Unknown";
    }
}

const char* GetCameraSelectionName( CAMERA_SELECTION cameraSelection )
{
    static char cascadeCameraText[32] = {};

    switch( cameraSelection )
    {
        case EYE_CAMERA:
            return "Eye Camera";
        case LIGHT_CAMERA:
            return "Light Camera";
        default:
            if( cameraSelection >= ORTHO_CAMERA1 && cameraSelection <= ORTHO_CAMERA8 )
            {
                sprintf_s( cascadeCameraText, "Cascade Cam %d", ( cameraSelection - ORTHO_CAMERA1 ) + 1 );
                return cascadeCameraText;
            }
            return "Unknown";
    }
}

const char* GetProjectionFitName( FIT_PROJECTION_TO_CASCADES fit )
{
    switch( fit )
    {
        case FIT_TO_SCENE:
            return "Fit Scene";
        case FIT_TO_CASCADES:
            return "Fit Cascades";
        default:
            return "Unknown";
    }
}

const char* GetNearFarFitName( FIT_TO_NEAR_FAR fit )
{
    switch( fit )
    {
        case FIT_NEARFAR_SCENE_AABB:
            return "AABB/Scene NearFar";
        case FIT_NEARFAR_PANCAKING:
            return "Pancaking";
        case FIT_NEARFAR_ZERO_ONE:
            return "0:1 NearFar";
        case FIT_NEARFAR_AABB:
            return "AABB NearFar";
        default:
            return "Unknown";
    }
}

const char* GetCascadeSelectionName( CASCADE_SELECTION selection )
{
    switch( selection )
    {
        case CASCADE_SELECTION_MAP:
            return "Map Selection";
        case CASCADE_SELECTION_INTERVAL:
            return "Interval Selection";
        default:
            return "Unknown";
    }
}

void NormalizeShadowSettings( INT editedCascadePartitionIndex )
{
    if( g_CascadeConfig.m_nCascadeLevels < 1 )
    {
        g_CascadeConfig.m_nCascadeLevels = 1;
    }
    if( g_CascadeConfig.m_nCascadeLevels > MAX_CASCADES )
    {
        g_CascadeConfig.m_nCascadeLevels = MAX_CASCADES;
    }

    const INT maxBufferSize = 8192 / max( g_CascadeConfig.m_nCascadeLevels, 1 );
    if( g_CascadeConfig.m_iBufferSize < 32 )
    {
        g_CascadeConfig.m_iBufferSize = 32;
    }
    if( g_CascadeConfig.m_iBufferSize > maxBufferSize )
    {
        g_CascadeConfig.m_iBufferSize = maxBufferSize;
    }
    g_CascadeConfig.m_iBufferSize = max( 32, ( g_CascadeConfig.m_iBufferSize / 32 ) * 32 );

    if( g_CascadedShadow.m_eSelectedCamera < EYE_CAMERA )
    {
        g_CascadedShadow.m_eSelectedCamera = EYE_CAMERA;
    }
    const INT maxCameraIndex = 1 + max( g_CascadeConfig.m_nCascadeLevels, 1 );
    if( g_CascadedShadow.m_eSelectedCamera > maxCameraIndex )
    {
        g_CascadedShadow.m_eSelectedCamera = static_cast<CAMERA_SELECTION>( maxCameraIndex );
    }

    if( editedCascadePartitionIndex >= 0 && editedCascadePartitionIndex < MAX_CASCADES )
    {
        INT clampedValue = g_CascadedShadow.m_iCascadePartitionsZeroToOne[editedCascadePartitionIndex];
        clampedValue = max( 0, min( 100, clampedValue ) );
        g_CascadedShadow.m_iCascadePartitionsZeroToOne[editedCascadePartitionIndex] = clampedValue;

        for( INT index = 0; index < editedCascadePartitionIndex; ++index )
        {
            if( g_CascadedShadow.m_iCascadePartitionsZeroToOne[index] > clampedValue )
            {
                g_CascadedShadow.m_iCascadePartitionsZeroToOne[index] = clampedValue;
            }
        }

        for( INT index = editedCascadePartitionIndex + 1; index < MAX_CASCADES; ++index )
        {
            if( g_CascadedShadow.m_iCascadePartitionsZeroToOne[index] < clampedValue )
            {
                g_CascadedShadow.m_iCascadePartitionsZeroToOne[index] = clampedValue;
            }
        }
    }

    for( INT index = 0; index < MAX_CASCADES; ++index )
    {
        g_CascadedShadow.m_iCascadePartitionsZeroToOne[index] =
            max( 0, min( 100, g_CascadedShadow.m_iCascadePartitionsZeroToOne[index] ) );
    }

    for( INT index = 1; index < MAX_CASCADES; ++index )
    {
        if( g_CascadedShadow.m_iCascadePartitionsZeroToOne[index] < g_CascadedShadow.m_iCascadePartitionsZeroToOne[index - 1] )
        {
            g_CascadedShadow.m_iCascadePartitionsZeroToOne[index] = g_CascadedShadow.m_iCascadePartitionsZeroToOne[index - 1];
        }
    }

    if( g_CascadedShadow.m_eSelectedNearFarFit == FIT_NEARFAR_PANCAKING )
    {
        g_CascadedShadow.m_eSelectedCascadeSelection = CASCADE_SELECTION_INTERVAL;
    }

    if( g_CascadedShadow.m_eSelectedCascadeSelection == CASCADE_SELECTION_INTERVAL )
    {
        g_CascadedShadow.m_iCascadePartitionsZeroToOne[max( g_CascadeConfig.m_nCascadeLevels, 1 ) - 1] = 100;
    }

    if( g_CascadedShadow.m_iPCFBlurSize < 1 )
    {
        g_CascadedShadow.m_iPCFBlurSize = 1;
    }
    if( ( g_CascadedShadow.m_iPCFBlurSize % 2 ) == 0 )
    {
        ++g_CascadedShadow.m_iPCFBlurSize;
    }
}

HRESULT InitializeImGui( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext )
{
    if( g_bImGuiInitialized )
    {
        return S_OK;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = NULL;

    ImGui::StyleColorsDark();

    if( !ImGui_ImplWin32_Init( DXUTGetHWND() ) )
    {
        ImGui::DestroyContext();
        return E_FAIL;
    }

    if( !ImGui_ImplDX11_Init( pd3dDevice, pd3dImmediateContext ) )
    {
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        return E_FAIL;
    }

    g_bImGuiInitialized = true;
    return S_OK;
}

void ShutdownImGui()
{
    if( !g_bImGuiInitialized )
    {
        return;
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    g_bImGuiInitialized = false;
}

bool ImGuiWantsToCaptureMessage( UINT uMsg, WPARAM wParam )
{
    if( !g_bImGuiInitialized )
    {
        return false;
    }

    // Keep the overlay hotkeys responsive even when Dear ImGui owns the keyboard.
    if( ( uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN ) && ( wParam == VK_F2 || wParam == VK_F3 ) )
    {
        return false;
    }

    const ImGuiIO& io = ImGui::GetIO();
    switch( uMsg )
    {
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MBUTTONDBLCLK:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_XBUTTONDBLCLK:
            return io.WantCaptureMouse;

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
            return io.WantCaptureKeyboard;

        case WM_CHAR:
            return io.WantTextInput;

        default:
            return false;
    }
}

void RenderImGuiSelectorTab()
{
    ImGui::Text( "%ls", DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    ImGui::Text( "%ls", DXUTGetDeviceStats() );
    ImGui::Separator();

    if( ImGui::BeginCombo( "Scene", GetSceneSelectionName( g_eSelectedScene ) ) )
    {
        for( INT index = POWER_PLANT_SCENE; index <= SIMPLE_SCENE; ++index )
        {
            const SCENE_SELECTION sceneSelection = static_cast<SCENE_SELECTION>( index );
            const bool selected = ( g_eSelectedScene == sceneSelection );
            if( ImGui::Selectable( GetSceneSelectionName( sceneSelection ), selected ) )
            {
                g_eSelectedScene = sceneSelection;
                if( SUCCEEDED( ApplySceneSelectionChange() ) )
                {
                    UpdateViewerCameraNearFar();
                }
            }
            if( selected )
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    const char* activeCameraName = ( g_pActiveCamera == &g_LightCamera ) ? "Light Camera" : "Viewer Camera";
    if( ImGui::BeginCombo( "Active Camera", activeCameraName ) )
    {
        const bool viewerSelected = ( g_pActiveCamera == &g_ViewerCamera );
        if( ImGui::Selectable( "Viewer Camera", viewerSelected ) )
        {
            g_pActiveCamera = &g_ViewerCamera;
        }
        if( viewerSelected )
        {
            ImGui::SetItemDefaultFocus();
        }

        const bool lightSelected = ( g_pActiveCamera == &g_LightCamera );
        if( ImGui::Selectable( "Light Camera", lightSelected ) )
        {
            g_pActiveCamera = &g_LightCamera;
        }
        ImGui::EndCombo();
    }

    if( ImGui::Button( "Reset Cameras" ) )
    {
        ResetSceneCameras();
        UpdateViewerCameraNearFar();
    }

    ImGui::Checkbox( "Show Help", reinterpret_cast<bool*>( &g_bShowHelp ) );
    ImGui::Checkbox( "Show Demo Window", &g_bShowImGuiDemoWindow );
    ImGui::Checkbox( "Show Metrics Window", &g_bShowImGuiMetricsWindow );

    if( g_bShowHelp )
    {
        ImGui::Separator();
        ImGui::TextWrapped( "WASD/EQ move the camera. Mouse drag rotates. Left click picks a debug bounding box. J/K/I/M/O/P move the selected submesh." );
        ImGui::Text( "F1 Help | F2 Controls | F3 Demo" );
    }
}

void RenderImGuiDebugTab()
{
    bool renderDebug = g_CascadedShadow.IsRenderDebugEnabled();
    if( ImGui::Checkbox( "Render Debug", &renderDebug ) )
    {
        g_CascadedShadow.SetRenderDebugEnabled( renderDebug );
    }

    bool renderDebugBoundingBox = g_CascadedShadow.IsRenderDebugBoundingBoxEnabled();
    if( ImGui::Checkbox( "Debug Bounding Box", &renderDebugBoundingBox ) )
    {
        g_CascadedShadow.SetRenderDebugBoundingBoxEnabled( renderDebugBoundingBox );
    }

    bool renderAllBoundingBoxes = g_CascadedShadow.IsRenderDebugAllBoundingBoxesEnabled();
    if( ImGui::Checkbox( "Render All Bounding Boxes", &renderAllBoundingBoxes ) )
    {
        g_CascadedShadow.SetRenderDebugAllBoundingBoxesEnabled( renderAllBoundingBoxes );
    }

    ImGui::Separator();
    if( renderDebug )
    {
        ID3D11ShaderResourceView* pShadowAtlasSRV = g_CascadedShadow.GetDebugShadowMapSRV();
        if( pShadowAtlasSRV )
        {
            const INT cascadeCount = g_CascadedShadow.GetDebugShadowMapCascadeCount();
            const float atlasWidth = (FLOAT)g_CascadedShadow.GetDebugShadowMapAtlasWidth();
            const float atlasHeight = (FLOAT)g_CascadedShadow.GetDebugShadowMapAtlasHeight();
            const float availableWidth = ImGui::GetContentRegionAvail().x;
            const float imageWidth = max( 260.0f, min( availableWidth, 760.0f ) );
            const float imageHeight = imageWidth * ( atlasHeight / max( atlasWidth, 1.0f ) );

            ImGui::Text( "Cascade Shadow Atlas" );
            ImGui::TextDisabled( "%d cascades | %dx%d", cascadeCount,
                g_CascadedShadow.GetDebugShadowMapAtlasWidth(),
                g_CascadedShadow.GetDebugShadowMapAtlasHeight() );

            ImGui::Image( ImTextureRef( (ImTextureID)(UINT_PTR)pShadowAtlasSRV ), ImVec2( imageWidth, imageHeight ) );

            ImDrawList* pDrawList = ImGui::GetWindowDrawList();
            const ImVec2 imageMin = ImGui::GetItemRectMin();
            const ImVec2 imageMax = ImGui::GetItemRectMax();
            pDrawList->AddRect( imageMin, imageMax, IM_COL32( 255, 255, 255, 110 ), 4.0f, 0, 1.5f );

            for( INT cascadeIndex = 0; cascadeIndex < cascadeCount; ++cascadeIndex )
            {
                const float tileMinX = imageMin.x + ( imageMax.x - imageMin.x ) * ( (FLOAT)cascadeIndex / (FLOAT)cascadeCount );
                const float tileMaxX = imageMin.x + ( imageMax.x - imageMin.x ) * ( (FLOAT)( cascadeIndex + 1 ) / (FLOAT)cascadeCount );

                if( cascadeIndex > 0 )
                {
                    pDrawList->AddLine( ImVec2( tileMinX, imageMin.y ), ImVec2( tileMinX, imageMax.y ),
                        IM_COL32( 80, 255, 200, 180 ), 1.0f );
                }

                char cascadeLabel[16] = {};
                sprintf_s( cascadeLabel, "C%d", cascadeIndex + 1 );
                const ImVec2 labelMin( tileMinX + 6.0f, imageMin.y + 6.0f );
                const ImVec2 labelMax( min( tileMaxX - 6.0f, tileMinX + 38.0f ), imageMin.y + 24.0f );
                pDrawList->AddRectFilled( labelMin, labelMax, IM_COL32( 8, 12, 18, 200 ), 4.0f );
                pDrawList->AddText( ImVec2( labelMin.x + 7.0f, labelMin.y + 3.0f ), IM_COL32( 180, 255, 230, 255 ), cascadeLabel );
            }

            //Put Draw Text debug albedo 
            if (g_pSelectedMesh && g_CascadedShadow.GetCurrentSelectedBoundingBox() >= 0)
            {
                const INT selectedIndex = g_CascadedShadow.GetCurrentSelectedBoundingBox();
                SelectedAlbedoInfo info;
                if (g_pSelectedMesh->GetSubMeshAlbedoInfo(selectedIndex, info))
                {
                    ImGui::Separator();
                    ImGui::Text("Picked Material");
                    ImGui::Text("Material: %s", info.materialName.c_str());
                    ImGui::TextWrapped("Texture: %s", info.textureName.c_str());

                    if (info.pDiffuseSRV)
                    {
                        ImGui::Image(
                            ImTextureRef((ImTextureID)(UINT_PTR)info.pDiffuseSRV),
                            ImVec2(192.0f, 192.0f));
                    }
                }
            }
        }
        else
        {
            ImGui::TextDisabled( "The cascade shadow atlas is not allocated yet." );
        }

        ImGui::Separator();
    }

    ImGui::TextWrapped( "The red fog, ring, beam and picking all follow the currently selected debug bounding box." );
}

void RenderImGuiVoxelizationTab()
{
    bool voxelizationNeedsRefresh = false;

    ImGui::Checkbox( "Visualize Voxel", &g_bVisualizeVoxel );

    if( ImGui::SliderFloat( "Voxel Snap", &g_CascadedShadow.m_fVoxelVisualizeSurfaceSnap, 0.0f, 1.0f, "%.2f" ) )
    {
        g_CascadedShadow.m_fVoxelVisualizeSurfaceSnap = max( 0.0f, min( 1.0f, g_CascadedShadow.m_fVoxelVisualizeSurfaceSnap ) );
    }
    if( ImGui::SliderFloat( "Height Warp", &g_CascadedShadow.m_fStaticVoxelHeightWarp, 1.0f, 4.0f, "%.2f" ) )
    {
        voxelizationNeedsRefresh = true;
    }
    if( ImGui::SliderFloat( "XY Fill", &g_CascadedShadow.m_fVoxelXYFootprintScale, 4.0f, 8.0f, "%.2f" ) )
    {
        voxelizationNeedsRefresh = true;
    }
    if( ImGui::SliderFloat( "YZ Fill", &g_CascadedShadow.m_fVoxelYZFootprintScale, 4.0f, 8.0f, "%.2f" ) )
    {
        voxelizationNeedsRefresh = true;
    }
    if( ImGui::SliderFloat( "Top Coverage", &g_CascadedShadow.m_fStaticVoxelTopCoverage, 1.0f, 4.0f, "%.2f" ) )
    {
        voxelizationNeedsRefresh = true;
    }

    if( voxelizationNeedsRefresh )
    {
        g_CascadedShadow.InvalidateStaticVoxelization();
    }
}

void RenderImGuiCascadesTab()
{
    NormalizeShadowSettings();

    ImGui::Checkbox( "Visualize Cascades", &g_bVisualizeCascades );

    if( ImGui::BeginCombo( "Depth Buffer", GetShadowTextureFormatName( g_CascadeConfig.m_ShadowBufferFormat ) ) )
    {
        const SHADOW_TEXTURE_FORMAT formats[] =
        {
            CASCADE_DXGI_FORMAT_R32_TYPELESS,
            CASCADE_DXGI_FORMAT_R16_TYPELESS,
            CASCADE_DXGI_FORMAT_R24G8_TYPELESS
        };

        for( INT index = 0; index < ARRAYSIZE( formats ); ++index )
        {
            const SHADOW_TEXTURE_FORMAT format = formats[index];
            const bool selected = ( g_CascadeConfig.m_ShadowBufferFormat == format );
            if( ImGui::Selectable( GetShadowTextureFormatName( format ), selected ) )
            {
                g_CascadeConfig.m_ShadowBufferFormat = format;
            }
            if( selected )
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    INT cascadeLevels = g_CascadeConfig.m_nCascadeLevels;
    if( ImGui::SliderInt( "Cascade Levels", &cascadeLevels, 1, MAX_CASCADES ) )
    {
        g_CascadeConfig.m_nCascadeLevels = cascadeLevels;
        NormalizeShadowSettings();
    }

    if( ImGui::BeginCombo( "Camera", GetCameraSelectionName( g_CascadedShadow.m_eSelectedCamera ) ) )
    {
        const INT maxCameraIndex = 1 + max( g_CascadeConfig.m_nCascadeLevels, 1 );
        for( INT cameraIndex = EYE_CAMERA; cameraIndex <= maxCameraIndex; ++cameraIndex )
        {
            const CAMERA_SELECTION selection = static_cast<CAMERA_SELECTION>( cameraIndex );
            const bool selected = ( g_CascadedShadow.m_eSelectedCamera == selection );
            if( ImGui::Selectable( GetCameraSelectionName( selection ), selected ) )
            {
                g_CascadedShadow.m_eSelectedCamera = selection;
            }
            if( selected )
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if( ImGui::BeginCombo( "Projection Fit", GetProjectionFitName( g_CascadedShadow.m_eSelectedCascadesFit ) ) )
    {
        const FIT_PROJECTION_TO_CASCADES items[] = { FIT_TO_SCENE, FIT_TO_CASCADES };
        for( INT index = 0; index < ARRAYSIZE( items ); ++index )
        {
            const FIT_PROJECTION_TO_CASCADES item = items[index];
            const bool selected = ( g_CascadedShadow.m_eSelectedCascadesFit == item );
            if( ImGui::Selectable( GetProjectionFitName( item ), selected ) )
            {
                g_CascadedShadow.m_eSelectedCascadesFit = item;
            }
            if( selected )
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if( ImGui::BeginCombo( "Near/Far Fit", GetNearFarFitName( g_CascadedShadow.m_eSelectedNearFarFit ) ) )
    {
        const FIT_TO_NEAR_FAR items[] =
        {
            FIT_NEARFAR_SCENE_AABB,
            FIT_NEARFAR_PANCAKING,
            FIT_NEARFAR_ZERO_ONE,
            FIT_NEARFAR_AABB
        };

        for( INT index = 0; index < ARRAYSIZE( items ); ++index )
        {
            const FIT_TO_NEAR_FAR item = items[index];
            const bool selected = ( g_CascadedShadow.m_eSelectedNearFarFit == item );
            if( ImGui::Selectable( GetNearFarFitName( item ), selected ) )
            {
                g_CascadedShadow.m_eSelectedNearFarFit = item;
                NormalizeShadowSettings();
            }
            if( selected )
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if( ImGui::BeginCombo( "Cascade Selection", GetCascadeSelectionName( g_CascadedShadow.m_eSelectedCascadeSelection ) ) )
    {
        const CASCADE_SELECTION items[] = { CASCADE_SELECTION_MAP, CASCADE_SELECTION_INTERVAL };
        for( INT index = 0; index < ARRAYSIZE( items ); ++index )
        {
            const CASCADE_SELECTION item = items[index];
            const bool selected = ( g_CascadedShadow.m_eSelectedCascadeSelection == item );
            if( ImGui::Selectable( GetCascadeSelectionName( item ), selected ) )
            {
                g_CascadedShadow.m_eSelectedCascadeSelection = item;
                NormalizeShadowSettings();
            }
            if( selected )
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    const INT maxBufferSize = 8192 / max( g_CascadeConfig.m_nCascadeLevels, 1 );
    if( ImGui::SliderInt( "Texture Size", &g_CascadeConfig.m_iBufferSize, 32, maxBufferSize, "%d" ) )
    {
        NormalizeShadowSettings();
    }

    if( ImGui::SliderInt( "PCF Blur", &g_CascadedShadow.m_iPCFBlurSize, 1, 31, "%d" ) )
    {
        NormalizeShadowSettings();
    }

    ImGui::SliderFloat( "Offset", &g_CascadedShadow.m_fPCFOffset, 0.0f, 0.05f, "%.3f" );

    bool blurBetweenCascades = ( g_CascadedShadow.m_iBlurBetweenCascades != 0 );
    if( ImGui::Checkbox( "Blend Between Cascades", &blurBetweenCascades ) )
    {
        g_CascadedShadow.m_iBlurBetweenCascades = blurBetweenCascades ? 1 : 0;
    }
    if( blurBetweenCascades )
    {
        ImGui::SliderFloat( "Cascade Blur", &g_CascadedShadow.m_fBlurBetweenCascadesAmount, 0.0f, 0.05f, "%.3f" );
    }

    bool derivativeOffset = ( g_CascadedShadow.m_iDerivativeBasedOffset != 0 );
    if( ImGui::Checkbox( "DDX, DDY Offset", &derivativeOffset ) )
    {
        g_CascadedShadow.m_iDerivativeBasedOffset = derivativeOffset ? 1 : 0;
    }

    ImGui::Checkbox( "Fit Light to Texels", &g_bMoveLightTexelSize );

    ImGui::Separator();
    ImGui::Text( "Cascade Partitions" );
    for( INT index = 0; index < g_CascadeConfig.m_nCascadeLevels; ++index )
    {
        char label[32] = {};
        sprintf_s( label, "L%d", index + 1 );
        if( ImGui::SliderInt( label, &g_CascadedShadow.m_iCascadePartitionsZeroToOne[index], 0, 100, "%d" ) )
        {
            NormalizeShadowSettings( index );
        }
    }
}

void RenderImGuiOverlay( ID3D11DeviceContext* pd3dImmediateContext, double fTime, FLOAT fElapsedTime,
                         ID3D11RenderTargetView* pRTV, ID3D11DepthStencilView* pDSV )
{
    UNREFERENCED_PARAMETER( fTime );
    UNREFERENCED_PARAMETER( fElapsedTime );

    if( !g_bImGuiInitialized )
    {
        return;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if( g_bShowImGuiOverlay )
    {
        ImGui::SetNextWindowBgAlpha( 0.88f );
        ImGui::SetNextWindowPos( ImVec2( 18.0f, 18.0f ), ImGuiCond_FirstUseEver );
        ImGui::SetNextWindowSize( ImVec2( 390.0f, 520.0f ), ImGuiCond_FirstUseEver );


        RenderImGuiDebugTab();

        if( ImGui::Begin( "Renderer Controls", &g_bShowImGuiOverlay ) )
        {
            ImGui::Text( "DX11 + Win32 backend mounted on DXUT core only" );
            ImGui::Text( "FPS: %.1f", ImGui::GetIO().Framerate );
            ImGui::Separator();

            if( ImGui::BeginTabBar( "RendererTabs" ) )
            {
                if( ImGui::BeginTabItem( "Selector" ) )
                {
                    g_eSelectedImGuiTab = IMGUI_PANEL_TAB_SELECTOR;
                    RenderImGuiSelectorTab();
                    ImGui::EndTabItem();
                }

                if( ImGui::BeginTabItem( "Debug" ) )
                {
                    g_eSelectedImGuiTab = IMGUI_PANEL_TAB_DEBUG;
                    
                    ImGui::EndTabItem();
                }

                if( ImGui::BeginTabItem( "Voxelization" ) )
                {
                    g_eSelectedImGuiTab = IMGUI_PANEL_TAB_VOXELIZATION;
                    RenderImGuiVoxelizationTab();
                    ImGui::EndTabItem();
                }

                if( ImGui::BeginTabItem( "Cascades" ) )
                {
                    g_eSelectedImGuiTab = IMGUI_PANEL_TAB_CASCADES;
                    RenderImGuiCascadesTab();
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }

    if( g_bShowImGuiDemoWindow )
    {
        ImGui::ShowDemoWindow( &g_bShowImGuiDemoWindow );
    }

    if( g_bShowImGuiMetricsWindow )
    {
        ImGui::ShowMetricsWindow( &g_bShowImGuiMetricsWindow );
    }

    ImGui::Render();

    pd3dImmediateContext->OMSetRenderTargets( 1, &pRTV, pDSV );
    ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData() );
}

static SCENE_SELECTION GetCurrentSceneSelection()
{
    return g_eSelectedScene;
}

HRESULT EnsureSceneMeshLoaded( ID3D11Device* pd3dDevice, ISceneMesh* pMesh )
{
    if( !pMesh || pMesh->IsLoaded() )
    {
        return S_OK;
    }

    return pMesh->Create( pd3dDevice, DXUTGetD3D11DeviceContext() );
}

HRESULT ApplySceneSelectionChange()
{
    HRESULT hr = S_OK;
    ISceneMesh* pNewMesh = &g_MeshPowerPlant;

    switch( g_eSelectedScene )
    {
        case TEST_SCENE:
            pNewMesh = &g_MeshTestScene;
        break;

        case SPONZA_SCENE:
            pNewMesh = &g_MeshSponza;
        break;

        case SIMPLE_SCENE:
            pNewMesh = &g_MeshSimpleScene;
        break;

        case POWER_PLANT_SCENE:
        default:
            pNewMesh = &g_MeshPowerPlant;
        break;
    }

    if( g_pSelectedMesh == pNewMesh )
    {
        return S_OK;
    }

    g_pSelectedMesh = pNewMesh;
    ID3D11Device* pd3dDevice = DXUTGetD3D11Device();
    V_RETURN( EnsureSceneMeshLoaded( pd3dDevice, g_pSelectedMesh ) );
    g_pActiveCamera = &g_ViewerCamera;
    ResetSceneCameras();
    V_RETURN( g_CascadedShadow.HandleSceneChanged( pd3dDevice, g_pSelectedMesh ) );
    UpdateViewerCameraNearFar();
    return S_OK;
}

void ResetSceneCameras()
{
    const SCENE_SELECTION ss = GetCurrentSceneSelection();

    D3DXVECTOR3 vecEye( 100.0f, 5.0f, 5.0f );
    D3DXVECTOR3 vecAt( 0.0f, 0.0f, 0.0f );

    if( ss == SPONZA_SCENE )
    {
        vecEye = D3DXVECTOR3( -2.0f, 15.5f, -64.0f );
        vecAt = D3DXVECTOR3( -2.0f, 14.0f, 0.0f );
    }
    else if( ss == SIMPLE_SCENE )
    {
        vecEye = D3DXVECTOR3( -9.0f, 6.0f, -12.0f );
        vecAt = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
    }

    D3DXVECTOR3 vMin = D3DXVECTOR3( -2500.0f, -2500.0f, -2500.0f );
    D3DXVECTOR3 vMax = D3DXVECTOR3( 2500.0f, 2500.0f, 2500.0f );
    if( ss == SIMPLE_SCENE )
    {
        vMin = D3DXVECTOR3( -40.0f, -10.0f, -40.0f );
        vMax = D3DXVECTOR3( 40.0f, 25.0f, 40.0f );
    }
    g_ViewerCamera.SetViewParams( &vecEye, &vecAt );
    g_ViewerCamera.SetRotateButtons(TRUE, FALSE, FALSE);
    g_ViewerCamera.SetScalers( 0.01f, 10.0f );
    g_ViewerCamera.SetDrag( true );
    g_ViewerCamera.SetEnableYAxisMovement( true );
    g_ViewerCamera.SetClipToBoundary( TRUE, &vMin, &vMax );
    g_ViewerCamera.FrameMove( 0 );

    D3DXVECTOR3 lightEye( -320.0f, 300.0f, -220.3f );
    D3DXVECTOR3 lightAt( 0.0f, 0.0f, 0.0f );
    if( ss == SPONZA_SCENE )
    {
        lightEye = D3DXVECTOR3( -45.0f, 72.5f, -42.5f );
        lightAt = D3DXVECTOR3( 0.0f, 12.5f, 0.0f );
    }
    else if( ss == SIMPLE_SCENE )
    {
        lightEye = D3DXVECTOR3( -8.0f, 12.0f, -8.0f );
        lightAt = D3DXVECTOR3( 0.0f, 0.5f, 0.0f );
    }

    g_LightCamera.SetViewParams( &lightEye, &lightAt );
    g_LightCamera.SetRotateButtons( TRUE, FALSE, FALSE );
    g_LightCamera.SetScalers( 0.01f, 50.0f );
    g_LightCamera.SetDrag( true );
    g_LightCamera.SetEnableYAxisMovement( true );
    g_LightCamera.SetClipToBoundary( TRUE, &vMin, &vMax );
    g_LightCamera.SetProjParams( D3DX_PI / 4, 1.0f, 0.1f , 4000.0f);
    g_LightCamera.FrameMove( 0 );
}

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // Set DXUT callbacks
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackFrameMove( OnFrameMove );

    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );
    InitApp();

    DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params

    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"CascadedShadowDepthMap" );
    CWaitDlg CompilingShadersDlg;
    if ( DXUT_EnsureD3D11APIs() )
        CompilingShadersDlg.ShowDialog( L"Compiling Shaders and loading models." );
    DXUTCreateDevice (D3D_FEATURE_LEVEL_10_0, true, 800, 600 );
    CompilingShadersDlg.DestroyDialog();
    DXUTMainLoop(); // Enter into the DXUT render loop

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{

    g_CascadeConfig.m_nCascadeLevels = 3;
    g_CascadeConfig.m_iBufferSize = 1024;


    g_CascadedShadow.m_iCascadePartitionsZeroToOne[0] = 5;
    g_CascadedShadow.m_iCascadePartitionsZeroToOne[1] = 15;
    g_CascadedShadow.m_iCascadePartitionsZeroToOne[2] = 60;
    g_CascadedShadow.m_iCascadePartitionsZeroToOne[3] = 100;
    g_CascadedShadow.m_iCascadePartitionsZeroToOne[4] = 100;
    g_CascadedShadow.m_iCascadePartitionsZeroToOne[5] = 100;
    g_CascadedShadow.m_iCascadePartitionsZeroToOne[6] = 100;
    g_CascadedShadow.m_iCascadePartitionsZeroToOne[7] = 100;

    g_CascadedShadow.m_iCascadePartitionsMax = 100;
    NormalizeShadowSettings();
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D11 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    // For the first device created if its a REF device, optionally display a warning dialog box
    static BOOL s_bFirstTime = true;
    
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( ( DXUT_D3D11_DEVICE == pDeviceSettings->ver &&
              pDeviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE ) )
        {
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
        }
    }

    return true;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, FLOAT fElapsedTime, void* pUserContext )
{
    // Update the camera's position based on user input 
    g_LightCamera.FrameMove( fElapsedTime );
    g_ViewerCamera.FrameMove( fElapsedTime );
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
    UNREFERENCED_PARAMETER( pUserContext );
    *pbNoFurtherProcessing = false;

    if( g_bImGuiInitialized )
    {
        ImGui_ImplWin32_WndProcHandler( hWnd, uMsg, wParam, lParam );
        if( ImGuiWantsToCaptureMessage( uMsg, wParam ) )
        {
            *pbNoFurtherProcessing = true;
            return 0;
        }
    }

    if( uMsg == WM_LBUTTONDOWN && g_CascadedShadow.IsRenderDebugAllBoundingBoxesEnabled() )
    {
        ID3D11DeviceContext* pd3dDeviceContext = DXUTGetD3D11DeviceContext();
        if( pd3dDeviceContext )
        {
            RECT clientRect = {};
            GetClientRect( hWnd, &clientRect );

            D3D11_VIEWPORT viewport = {};
            viewport.TopLeftX = 0.0f;
            viewport.TopLeftY = 0.0f;
            viewport.Width = FLOAT( clientRect.right - clientRect.left );
            viewport.Height = FLOAT( clientRect.bottom - clientRect.top );
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;

            const INT mouseX = (INT)(short)LOWORD( lParam );
            const INT mouseY = (INT)(short)HIWORD( lParam );
            g_CascadedShadow.PickDebugBoundingBox( pd3dDeviceContext, g_pSelectedMesh, mouseX, mouseY, viewport );
        }
    }

    // Pass all remaining windows messages to camera so it can respond to user input
    g_pActiveCamera->HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    UNREFERENCED_PARAMETER( bAltDown );
    UNREFERENCED_PARAMETER( pUserContext );

    if( bKeyDown )
    {
        if( nChar == VK_F2 )
        {
            g_bShowImGuiOverlay = !g_bShowImGuiOverlay;
            return;
        }

        if( nChar == VK_F3 )
        {
            g_bShowImGuiDemoWindow = !g_bShowImGuiDemoWindow;
            return;
        }

        if( g_bImGuiInitialized )
        {
            const ImGuiIO& io = ImGui::GetIO();
            if( io.WantCaptureKeyboard || io.WantTextInput )
            {
                return;
            }
        }

        switch( nChar )
        {
            case VK_F1:
                g_bShowHelp = !g_bShowHelp; break;

            case 'J':
                if( g_pSelectedMesh )
                {
                    ID3D11Device* pd3dDevice = DXUTGetD3D11Device();
                    ID3D11DeviceContext* pd3dDeviceContext = DXUTGetD3D11DeviceContext();
                    g_CascadedShadow.TranslateSelectedSubMesh( pd3dDevice, pd3dDeviceContext, g_pSelectedMesh, D3DXVECTOR3( -1.0f, 0.0f, 0.0f ) );
                }
            break;

            case 'K':
                if( g_pSelectedMesh )
                {
                    ID3D11Device* pd3dDevice = DXUTGetD3D11Device();
                    ID3D11DeviceContext* pd3dDeviceContext = DXUTGetD3D11DeviceContext();
                    g_CascadedShadow.TranslateSelectedSubMesh( pd3dDevice, pd3dDeviceContext, g_pSelectedMesh, D3DXVECTOR3( 1.0f, 0.0f, 0.0f ) );
                }
            break;
            case 'M':
                if (g_pSelectedMesh)
                {
                    ID3D11Device* pd3dDevice = DXUTGetD3D11Device();
                    ID3D11DeviceContext* pd3dDeviceContext = DXUTGetD3D11DeviceContext();
                    g_CascadedShadow.TranslateSelectedSubMesh(pd3dDevice, pd3dDeviceContext, g_pSelectedMesh, D3DXVECTOR3(0.0f, 1.0f, 0.0f));
                }
                break;
            case 'I':
                if (g_pSelectedMesh)
                {
                    ID3D11Device* pd3dDevice = DXUTGetD3D11Device();
                    ID3D11DeviceContext* pd3dDeviceContext = DXUTGetD3D11DeviceContext();
                    g_CascadedShadow.TranslateSelectedSubMesh(pd3dDevice, pd3dDeviceContext, g_pSelectedMesh, D3DXVECTOR3(0.0f, -1.0f, 0.0f));
                }
                break;
            case 'O':
                if (g_pSelectedMesh)
                {
                    ID3D11Device* pd3dDevice = DXUTGetD3D11Device();
                    ID3D11DeviceContext* pd3dDeviceContext = DXUTGetD3D11DeviceContext();
                    g_CascadedShadow.TranslateSelectedSubMesh(pd3dDevice, pd3dDeviceContext, g_pSelectedMesh, D3DXVECTOR3(0.0f, 0.0f, 1.0f));
                }
                break;
            case 'P':
                if (g_pSelectedMesh)
                {
                    ID3D11Device* pd3dDevice = DXUTGetD3D11Device();
                    ID3D11DeviceContext* pd3dDeviceContext = DXUTGetD3D11DeviceContext();
                    g_CascadedShadow.TranslateSelectedSubMesh(pd3dDevice, pd3dDeviceContext, g_pSelectedMesh, D3DXVECTOR3(0.0f, 0.0f, -1.0f));
                }

        }
    }
}


//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo* AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo* DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// When the user changes scene, recreate these components as they are scene 
// dependent.
//--------------------------------------------------------------------------------------
HRESULT CreateD3DComponents( ID3D11Device* pd3dDevice ) 
{
    HRESULT hr;
    
    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    ResetSceneCameras();

    g_CascadedShadow.Init( pd3dDevice, pd3dImmediateContext, 
        g_pSelectedMesh, &g_ViewerCamera, &g_LightCamera, &g_CascadeConfig );

    V_RETURN( InitializeImGui( pd3dDevice, pd3dImmediateContext ) );
    
    return S_OK;
}



//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    g_MeshPowerPlant.Destroy();
    g_MeshTestScene.Destroy();
    g_MeshSponza.Destroy();
    g_MeshSimpleScene.Destroy();
    DestroyD3DComponents();
}

HRESULT DestroyD3DComponents() 
{
    ShutdownImGui();
    DXUTGetGlobalResourceCache().OnDestroyDevice();

    g_CascadedShadow.DestroyAndDeallocateShadowResources();
    return S_OK;

}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext )
{
    HRESULT hr = S_OK;

    if ( !g_pSelectedMesh )
    {
        g_pSelectedMesh = &g_MeshPowerPlant;
    }

    V_RETURN( EnsureSceneMeshLoaded( pd3dDevice, g_pSelectedMesh ) );
    return CreateD3DComponents( pd3dDevice );
}


//--------------------------------------------------------------------------------------
// Calcaulte the camera based on size of the current scene
//--------------------------------------------------------------------------------------
void UpdateViewerCameraNearFar () 
{
    XMVECTOR vMeshExtents = g_CascadedShadow.GetSceneAABBMax() - g_CascadedShadow.GetSceneAABBMin();
    XMVECTOR vMeshLength = XMVector3Length( vMeshExtents );
    FLOAT fMeshLength = XMVectorGetByIndex( vMeshLength, 0);
    g_ViewerCamera.SetProjParams( D3DX_PI / 4, g_fAspectRatio, 0.05f, fMeshLength );
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    UNREFERENCED_PARAMETER( pd3dDevice );
    UNREFERENCED_PARAMETER( pSwapChain );
    UNREFERENCED_PARAMETER( pUserContext );

    g_fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT ) pBackBufferSurfaceDesc->Height;

    UpdateViewerCameraNearFar();

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    UNREFERENCED_PARAMETER( pUserContext );
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                  FLOAT fElapsedTime, void* pUserContext )
{
    UNREFERENCED_PARAMETER( pUserContext );

    FLOAT ClearColor[4] = { 0.0f, 0.25f, 0.25f, 0.55f };
    ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
    ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
    pd3dImmediateContext->ClearRenderTargetView( pRTV, ClearColor );
    pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

    g_CascadedShadow.InitFrame( pd3dDevice, g_pSelectedMesh);

    g_CascadedShadow.RenderShadowsForAllCascades( pd3dDevice, pd3dImmediateContext, g_pSelectedMesh);
    
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)DXUTGetDXGIBackBufferSurfaceDesc()->Width;
    vp.Height = (FLOAT)DXUTGetDXGIBackBufferSurfaceDesc()->Height;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;


    if(!g_bVisualizeVoxel)
        g_CascadedShadow.RenderScene(pd3dImmediateContext, pRTV, pDSV, g_pSelectedMesh, g_pActiveCamera, &vp, g_bVisualizeCascades);

    g_CascadedShadow.RenderVoxelization(pd3dImmediateContext, g_pSelectedMesh, g_pActiveCamera);
    g_CascadedShadow.RenderVisualizeVoxelization(pd3dImmediateContext, pRTV, pDSV, g_pSelectedMesh, &vp, g_pActiveCamera,g_bVisualizeVoxel);
    g_CascadedShadow.RenderDebug(pd3dImmediateContext, pRTV, pDSV, &vp);

    pd3dImmediateContext->RSSetViewports( 1, &vp);            
    pd3dImmediateContext->OMSetRenderTargets( 1, &pRTV, pDSV );

    DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
    RenderImGuiOverlay( pd3dImmediateContext, fTime, fElapsedTime, pRTV, pDSV );
    DXUT_EndPerfEvent();
}

