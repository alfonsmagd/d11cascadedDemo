//--------------------------------------------------------------------------------------
// File: Cascaded11.cpp
//
// This sample demonstrates cascaded shadow maps.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "resource.h"

#include "ShadowSampleMisc.h"
#include "CascadedShadowsManager.h"
#include "GuiDebugPanel.h"
#include "GuiSelectorPanel.h"
#include "GuiShadowPanel.h"
#include "GuiVoxelPanel.h"
#include "SceneMesh.h"
#include <commdlg.h>
#include <vector>
#include "WaitDlg.h"

#pragma comment(lib, "legacy_stdio_definitions.lib")

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CascadedShadowsManager      g_CascadedShadow;

CDXUTDialogResourceManager  g_DialogResourceManager; // manager for shared resources of dialogs
CFirstPersonCamera          g_ViewerCamera;          
CFirstPersonCamera          g_LightCamera;         
CFirstPersonCamera*         g_pActiveCamera = &g_ViewerCamera;

CascadeConfig               g_CascadeConfig;
SDKSceneMesh                g_MeshPowerPlant( L"powerplant\\powerplant.sdkmesh" );
SDKSceneMesh                g_MeshTestScene( L"ShadowColumns\\testscene.sdkmesh" );
OBJSceneMesh                g_MeshSponza( L"sponza\\sponza.obj", 0.05f );
ISceneMesh*                 g_pSelectedMesh = &g_MeshPowerPlant;                
SCENE_SELECTION             g_eSelectedScene = POWER_PLANT_SCENE;

// DXUT GUI stuff
CD3DSettingsDlg             g_D3DSettingsDlg;       // Device settings dialog
CDXUTTextHelper*            g_pTxtHelper = NULL;


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


//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN         1
#define IDC_TOGGLEWARP               2
#define IDC_CHANGEDEVICE             3

#define IDC_TOGGLEVISUALIZECASCADES  4
#define IDC_DEPTHBUFFERFORMAT        5

#define IDC_BUFFER_SIZE              6
#define IDC_BUFFER_SIZETEXT          7
#define IDC_SELECTED_CAMERA          8

#define IDC_SELECTED_SCENE           9

#define IDC_CASCADELEVELS            10

#define IDC_CASCADELEVEL1            11
#define IDC_CASCADELEVEL2            12
#define IDC_CASCADELEVEL3            13
#define IDC_CASCADELEVEL4            14
#define IDC_CASCADELEVEL5            15
#define IDC_CASCADELEVEL6            16
#define IDC_CASCADELEVEL7            17
#define IDC_CASCADELEVEL8            18

#define IDC_CASCADELEVEL1TEXT        19
#define IDC_CASCADELEVEL2TEXT        20
#define IDC_CASCADELEVEL3TEXT        21
#define IDC_CASCADELEVEL4TEXT        22
#define IDC_CASCADELEVEL5TEXT        23
#define IDC_CASCADELEVEL6TEXT        24
#define IDC_CASCADELEVEL7TEXT        25
#define IDC_CASCADELEVEL8TEXT        26

#define IDC_MOVE_LIGHT_IN_TEXEL_INC  27

#define IDC_FIT_TO_CASCADE           28
#define IDC_FIT_TO_NEARFAR           29
#define IDC_CASCADE_SELECT           30
#define IDC_PCF_SIZE                 31
#define IDC_PCF_SIZETEXT             32
#define IDC_TOGGLE_DERIVATIVE_OFFSET 33
#define IDC_PCF_OFFSET_SIZE          34
#define IDC_PCF_OFFSET_SIZETEXT      35

#define IDC_BLEND_BETWEEN_MAPS_CHECK 36
#define IDC_BLEND_MAPS_SLIDER        37

#define IDC_TOGGLE_DEBUG_CASCADES     38
#define IDC_TOGGLE_VISUALIZE_VOXEL    39
#define IDC_VOXEL_SURFACE_SNAP_TEXT   40
#define IDC_VOXEL_SURFACE_SNAP        41
#define IDC_VOXEL_HEIGHT_WARP_TEXT    42
#define IDC_VOXEL_HEIGHT_WARP         43
#define IDC_VOXEL_XY_FOOTPRINT_TEXT   44
#define IDC_VOXEL_XY_FOOTPRINT        45
#define IDC_VOXEL_YZ_FOOTPRINT_TEXT   46
#define IDC_VOXEL_YZ_FOOTPRINT        47
#define IDC_VOXEL_TOP_COVERAGE_TEXT   48
#define IDC_VOXEL_TOP_COVERAGE        49
#define IDC_GUI_CATEGORY_TEXT         50
#define IDC_GUI_CATEGORY_COMBO        51
#define IDC_GUI_VISUALIZE_CASCADES    52
#define IDC_GUI_BLEND_AMOUNT_TEXT     53
#define IDC_GUI_DEPTHFORMAT_TEXT      54
#define IDC_GUI_SELECTED_CAMERA_TEXT  55
#define IDC_GUI_CASCADELEVELS_TEXT    56
#define IDC_GUI_FIT_TO_CASCADE_TEXT   57
#define IDC_GUI_FIT_TO_NEARFAR_TEXT   58
#define IDC_GUI_CASCADE_SELECT_TEXT   59
#define IDC_GUI_SCENE_TEXT            60
#define IDC_GUI_SCENE_COMBO           61
#define IDC_TOGGLE_DEBUG_BOUNDING_BOX 62

Gui_SelectorPanelState      g_SelectorGuiState = { GUI_PANEL_CATEGORY_DEBUG, POWER_PLANT_SCENE };
Gui_SelectorPanelIds        g_SelectorPanelIds = { IDC_GUI_CATEGORY_TEXT, IDC_GUI_CATEGORY_COMBO, IDC_GUI_SCENE_TEXT, IDC_GUI_SCENE_COMBO };
Gui_SelectorPanel           g_SelectorPanel( g_SelectorPanelIds, g_SelectorGuiState );
Gui_DebugPanelState         g_DebugGuiState = {};
Gui_DebugPanelIds           g_DebugPanelIds = { IDC_TOGGLE_DEBUG_CASCADES, IDC_TOGGLE_DEBUG_BOUNDING_BOX };
Gui_DebugPanel              g_DebugPanel( g_DebugPanelIds, g_DebugGuiState );
Gui_ShadowPanelState        g_ShadowGuiState = {};
Gui_ShadowPanelIds          g_ShadowPanelIds =
{
    IDC_GUI_VISUALIZE_CASCADES,
    IDC_GUI_DEPTHFORMAT_TEXT,
    IDC_DEPTHBUFFERFORMAT,
    IDC_GUI_SELECTED_CAMERA_TEXT,
    IDC_SELECTED_CAMERA,
    IDC_GUI_CASCADELEVELS_TEXT,
    IDC_CASCADELEVELS,
    IDC_GUI_FIT_TO_CASCADE_TEXT,
    IDC_FIT_TO_CASCADE,
    IDC_GUI_FIT_TO_NEARFAR_TEXT,
    IDC_FIT_TO_NEARFAR,
    IDC_GUI_CASCADE_SELECT_TEXT,
    IDC_CASCADE_SELECT,
    IDC_BUFFER_SIZETEXT,
    IDC_BUFFER_SIZE,
    IDC_PCF_SIZETEXT,
    IDC_PCF_SIZE,
    IDC_PCF_OFFSET_SIZETEXT,
    IDC_PCF_OFFSET_SIZE,
    IDC_BLEND_BETWEEN_MAPS_CHECK,
    IDC_GUI_BLEND_AMOUNT_TEXT,
    IDC_BLEND_MAPS_SLIDER,
    IDC_TOGGLE_DERIVATIVE_OFFSET,
    IDC_MOVE_LIGHT_IN_TEXEL_INC,
    { IDC_CASCADELEVEL1TEXT, IDC_CASCADELEVEL2TEXT, IDC_CASCADELEVEL3TEXT, IDC_CASCADELEVEL4TEXT,
      IDC_CASCADELEVEL5TEXT, IDC_CASCADELEVEL6TEXT, IDC_CASCADELEVEL7TEXT, IDC_CASCADELEVEL8TEXT },
    { IDC_CASCADELEVEL1, IDC_CASCADELEVEL2, IDC_CASCADELEVEL3, IDC_CASCADELEVEL4,
      IDC_CASCADELEVEL5, IDC_CASCADELEVEL6, IDC_CASCADELEVEL7, IDC_CASCADELEVEL8 }
};
Gui_ShadowPanel             g_ShadowPanel( g_ShadowPanelIds, g_ShadowGuiState );
Gui_VoxelPanelState         g_VoxelGuiState = {};
Gui_VoxelPanelIds           g_VoxelPanelIds =
{
    IDC_TOGGLE_VISUALIZE_VOXEL,
    IDC_VOXEL_SURFACE_SNAP_TEXT,
    IDC_VOXEL_SURFACE_SNAP,
    IDC_VOXEL_HEIGHT_WARP_TEXT,
    IDC_VOXEL_HEIGHT_WARP,
    IDC_VOXEL_XY_FOOTPRINT_TEXT,
    IDC_VOXEL_XY_FOOTPRINT,
    IDC_VOXEL_YZ_FOOTPRINT_TEXT,
    IDC_VOXEL_YZ_FOOTPRINT,
    IDC_VOXEL_TOP_COVERAGE_TEXT,
    IDC_VOXEL_TOP_COVERAGE
};
Gui_VoxelPanel              g_VoxelPanel( g_VoxelPanelIds, g_VoxelGuiState );
GuiRuntimeContext           g_GuiRuntimeContext = { &g_CascadedShadow, &g_CascadeConfig, &g_bVisualizeCascades, &g_bVisualizeVoxel, &g_bMoveLightTexelSize, &g_eSelectedScene, &g_pActiveCamera, &g_ViewerCamera, &g_LightCamera };
std::vector<GuiPanelBase*>  g_AllGuiPanels;

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, FLOAT fElapsedTime, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, INT nControlID, CDXUTControl* pControl, void* pUserContext );
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
void RenderText();
HRESULT DestroyD3DComponents();
HRESULT CreateD3DComponents( ID3D11Device* pd3dDevice );
void UpdateViewerCameraNearFar();
void RegisterAllGuiPanels();
void SyncGuiPanelsToRuntime();
void SyncGuiPanelsFromRuntime();
bool HandleRuntimeGuiPanelEvent( UINT nEvent, INT nControlID );
bool DispatchRuntimeGuiPanelMsg( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
void RenderRuntimeGuiPanels( FLOAT fElapsedTime );
void UpdateGuiPanelVisibility();
void ResetSceneCameras();
HRESULT EnsureSceneMeshLoaded( ID3D11Device* pd3dDevice, ISceneMesh* pMesh );
HRESULT ApplySceneSelectionChange();

void RegisterAllGuiPanels()
{
    g_AllGuiPanels.clear();
    g_AllGuiPanels.push_back( &g_SelectorPanel );
    g_AllGuiPanels.push_back( &g_DebugPanel );
    g_AllGuiPanels.push_back( &g_VoxelPanel );
    g_AllGuiPanels.push_back( &g_ShadowPanel );
}

void SyncGuiPanelsToRuntime()
{
    for( size_t i = 0; i < g_AllGuiPanels.size(); ++i )
    {
        g_AllGuiPanels[i]->ApplyPendingChanges( g_GuiRuntimeContext );
    }
}

void SyncGuiPanelsFromRuntime()
{
    for( size_t i = 0; i < g_AllGuiPanels.size(); ++i )
    {
        g_AllGuiPanels[i]->UpdateFromRuntime( g_GuiRuntimeContext );
    }
}

bool HandleRuntimeGuiPanelEvent( UINT nEvent, INT nControlID )
{
    for( size_t i = 0; i < g_AllGuiPanels.size(); ++i )
    {
        if( g_AllGuiPanels[i]->HandleEvent( nEvent, nControlID ) )
        {
            return true;
        }
    }

    return false;
}

bool DispatchRuntimeGuiPanelMsg( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    for( size_t i = 0; i < g_AllGuiPanels.size(); ++i )
    {
        if( g_AllGuiPanels[i]->MsgProc( hWnd, uMsg, wParam, lParam ) )
        {
            return true;
        }
    }

    return false;
}

void RenderRuntimeGuiPanels( FLOAT fElapsedTime )
{
    for( size_t i = 0; i < g_AllGuiPanels.size(); ++i )
    {
        g_AllGuiPanels[i]->OnRender( fElapsedTime );
    }
}

void UpdateGuiPanelVisibility()
{
    g_SelectorPanel.Open();

    switch( g_SelectorGuiState.selectedCategory )
    {
        case GUI_PANEL_CATEGORY_DEBUG:
            g_DebugPanel.Open();
            g_VoxelPanel.Close();
            g_ShadowPanel.Close();
        break;

        case GUI_PANEL_CATEGORY_VOXELIZATION:
            g_DebugPanel.Close();
            g_VoxelPanel.Open();
            g_ShadowPanel.Close();
        break;

        case GUI_PANEL_CATEGORY_CASCADES:
        default:
            g_DebugPanel.Close();
            g_VoxelPanel.Close();
            g_ShadowPanel.Open();
        break;
    }
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
    V_RETURN( EnsureSceneMeshLoaded( DXUTGetD3D11Device(), g_pSelectedMesh ) );
    g_pActiveCamera = &g_ViewerCamera;
    ResetSceneCameras();
    g_CascadedShadow.InvalidateStaticVoxelization();
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

    D3DXVECTOR3 vMin = D3DXVECTOR3( -2500.0f, -2500.0f, -2500.0f );
    D3DXVECTOR3 vMax = D3DXVECTOR3( 2500.0f, 2500.0f, 2500.0f );
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


    // Pick some arbitrary intervals for the Cascade Maps
    //g_CascadedShadow.m_iCascadePartitionsZeroToOne[0] = 2;
    //g_CascadedShadow.m_iCascadePartitionsZeroToOne[1] = 4;
    //g_CascadedShadow.m_iCascadePartitionsZeroToOne[2] = 6;
    //g_CascadedShadow.m_iCascadePartitionsZeroToOne[3] = 9;
    //g_CascadedShadow.m_iCascadePartitionsZeroToOne[4] = 13;
    //g_CascadedShadow.m_iCascadePartitionsZeroToOne[5] = 26;
    //g_CascadedShadow.m_iCascadePartitionsZeroToOne[6] = 36;
    //g_CascadedShadow.m_iCascadePartitionsZeroToOne[7] = 70;

    g_CascadedShadow.m_iCascadePartitionsMax = 100;
    g_D3DSettingsDlg.Init( &g_DialogResourceManager );
    g_SelectorPanel.Initialize( &g_DialogResourceManager, OnGUIEvent );
    g_DebugPanel.Initialize( &g_DialogResourceManager, OnGUIEvent );
    g_VoxelPanel.Initialize( &g_DialogResourceManager, OnGUIEvent );
    g_ShadowPanel.Initialize( &g_DialogResourceManager, OnGUIEvent );
    RegisterAllGuiPanels();
    SyncGuiPanelsFromRuntime();
    UpdateGuiPanelVisibility();
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
// Render the help and statistics text
//--------------------------------------------------------------------------------------
void RenderText()
{
    UINT nBackBufferHeight = ( DXUTIsAppRenderingWithD3D9() ) ? DXUTGetD3D9BackBufferSurfaceDesc()->Height :
            DXUTGetDXGIBackBufferSurfaceDesc()->Height;

    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 2, 0 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );

    // Draw help
    if( g_bShowHelp )
    {
        g_pTxtHelper->SetInsertionPos( 2, nBackBufferHeight - 20 * 6 );
        g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
        g_pTxtHelper->DrawTextLine( L"Controls:" );

        g_pTxtHelper->SetInsertionPos( 20, nBackBufferHeight - 20 * 5 );
        g_pTxtHelper->DrawTextLine( L"Move forward and backward with 'E' and 'D'\n"
                                    L"Move left and right with 'S' and 'D' \n"
                                    L"Click the mouse button to roate the camera\n");

        g_pTxtHelper->SetInsertionPos( 350, nBackBufferHeight - 20 * 5 );
        g_pTxtHelper->DrawTextLine( L"Hide help: F1\n"
                                    L"Quit: ESC\n" );
    }
    else
    {
        g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
        g_pTxtHelper->DrawTextLine( L"Press F1 for help" );
    }

    g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    *pbNoFurtherProcessing = DispatchRuntimeGuiPanelMsg( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass all remaining windows messages to camera so it can respond to user input
    g_pActiveCamera->HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    if( bKeyDown )
    {
        switch( nChar )
        {
            case VK_F1:
                g_bShowHelp = !g_bShowHelp; break;

        }
    }
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, INT nControlID, CDXUTControl* pControl, void* pUserContext )
{
    UNREFERENCED_PARAMETER( pControl );
    UNREFERENCED_PARAMETER( pUserContext );

    if( HandleRuntimeGuiPanelEvent( nEvent, nControlID ) )
    {
        SyncGuiPanelsToRuntime();
        ApplySceneSelectionChange();
        UpdateGuiPanelVisibility();
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
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
    g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

    ResetSceneCameras();

    g_CascadedShadow.Init( pd3dDevice, pd3dImmediateContext, 
        g_pSelectedMesh, &g_ViewerCamera, &g_LightCamera, &g_CascadeConfig );
    
    return S_OK;
}



//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    g_MeshPowerPlant.Destroy();
    g_MeshTestScene.Destroy();
    //g_MeshSponza.Destroy();
    DestroyD3DComponents();
}

HRESULT DestroyD3DComponents() 
{
    g_DialogResourceManager.OnD3D11DestroyDevice();
    g_D3DSettingsDlg.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_DELETE( g_pTxtHelper );

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
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    g_fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT ) pBackBufferSurfaceDesc->Height;

    UpdateViewerCameraNearFar();
        
    g_SelectorPanel.SetLocation( 10, 10 );
    g_SelectorPanel.SetSize( 210, 110 );
    g_DebugPanel.SetLocation( 10, 125 );
    g_DebugPanel.SetSize( 210, 28 );
    g_VoxelPanel.SetLocation( 10, 125 );
    g_VoxelPanel.SetSize( 210, 190 );
    g_ShadowPanel.SetLocation( 10, 125 );
    g_ShadowPanel.SetSize( 210, 245 );
    UpdateGuiPanelVisibility();

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11ReleasingSwapChain();

}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                  FLOAT fElapsedTime, void* pUserContext )
{

    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        return;
    }

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

    SyncGuiPanelsFromRuntime();
    UpdateGuiPanelVisibility();


    pd3dImmediateContext->RSSetViewports( 1, &vp);            
    pd3dImmediateContext->OMSetRenderTargets( 1, &pRTV, pDSV );

    DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );

    RenderRuntimeGuiPanels( fElapsedTime );
    RenderText();
    DXUT_EndPerfEvent();
}

