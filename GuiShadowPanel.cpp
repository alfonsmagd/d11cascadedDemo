#include "DXUT.h"
#include "GuiShadowPanel.h"
#include "CascadedShadowsManager.h"

namespace
{
    const D3DCOLOR kCascadeColors[MAX_CASCADES] =
    {
        D3DCOLOR_ARGB( 255, 255,   0,   0 ),
        D3DCOLOR_ARGB( 255,   0, 255,   0 ),
        D3DCOLOR_ARGB( 255,   0,   0, 255 ),
        D3DCOLOR_ARGB( 255, 255, 255,   0 ),
        D3DCOLOR_ARGB( 255,   0, 255, 255 ),
        D3DCOLOR_ARGB( 255, 255,   0, 255 ),
        D3DCOLOR_ARGB( 255, 255, 128,   0 ),
        D3DCOLOR_ARGB( 255, 180, 180, 180 )
    };

    enum ShadowSliderKind
    {
        SHADOW_SLIDER_BUFFER_SIZE,
        SHADOW_SLIDER_PCF_SIZE,
        SHADOW_SLIDER_PCF_OFFSET,
        SHADOW_SLIDER_BLEND_AMOUNT
    };

    enum ShadowToggleKind
    {
        SHADOW_TOGGLE_VISUALIZE_CASCADES,
        SHADOW_TOGGLE_BLEND_ENABLED,
        SHADOW_TOGGLE_DERIVATIVE_OFFSET,
        SHADOW_TOGGLE_FIT_LIGHT_TO_TEXELS
    };

    enum ShadowComboKind
    {
        SHADOW_COMBO_DEPTH_FORMAT,
        SHADOW_COMBO_SELECTED_CAMERA,
        SHADOW_COMBO_CASCADE_LEVELS,
        SHADOW_COMBO_FIT_TO_CASCADE,
        SHADOW_COMBO_FIT_TO_NEARFAR,
        SHADOW_COMBO_CASCADE_SELECTION
    };

    void ClampShadowBufferSize( Gui_ShadowPanelState& state )
    {
        const INT maxBufferSize = 8192 / max( state.cascadeLevels, 1 );
        if( state.shadowBufferSize > maxBufferSize )
        {
            state.shadowBufferSize = maxBufferSize;
        }
        if( state.shadowBufferSize < 32 )
        {
            state.shadowBufferSize = 32;
        }
    }

    void ClampSelectedCamera( Gui_ShadowPanelState& state )
    {
        const INT maxCameraIndex = 1 + max( state.cascadeLevels, 1 );
        if( state.selectedCamera < EYE_CAMERA )
        {
            state.selectedCamera = EYE_CAMERA;
        }
        if( state.selectedCamera > maxCameraIndex )
        {
            state.selectedCamera = maxCameraIndex;
        }
    }

    void NormalizeCascadeSelections( Gui_ShadowPanelState& state )
    {
        if( state.fitToNearFar == FIT_NEARFAR_PANCAKING )
        {
            state.cascadeSelection = CASCADE_SELECTION_INTERVAL;
        }

        if( state.cascadeSelection == CASCADE_SELECTION_INTERVAL )
        {
            state.cascadePartitions[max( state.cascadeLevels, 1 ) - 1] = 100;
        }
    }

    void NormalizeCascadePartitions( Gui_ShadowPanelState& state, INT editedIndex )
    {
        INT clampedValue = state.cascadePartitions[editedIndex];
        if( clampedValue < 0 )
        {
            clampedValue = 0;
        }
        if( clampedValue > 100 )
        {
            clampedValue = 100;
        }

        state.cascadePartitions[editedIndex] = clampedValue;

        for( INT index = 0; index < editedIndex; ++index )
        {
            if( state.cascadePartitions[index] > clampedValue )
            {
                state.cascadePartitions[index] = clampedValue;
            }
        }

        for( INT index = editedIndex + 1; index < MAX_CASCADES; ++index )
        {
            if( state.cascadePartitions[index] < clampedValue )
            {
                state.cascadePartitions[index] = clampedValue;
            }
        }

        NormalizeCascadeSelections( state );
    }

    void NormalizeShadowState( Gui_ShadowPanelState& state )
    {
        if( state.cascadeLevels < 1 )
        {
            state.cascadeLevels = 1;
        }
        if( state.cascadeLevels > MAX_CASCADES )
        {
            state.cascadeLevels = MAX_CASCADES;
        }

        ClampShadowBufferSize( state );
        ClampSelectedCamera( state );

        for( INT index = 0; index < MAX_CASCADES; ++index )
        {
            if( state.cascadePartitions[index] < 0 )
            {
                state.cascadePartitions[index] = 0;
            }
            if( state.cascadePartitions[index] > 100 )
            {
                state.cascadePartitions[index] = 100;
            }
        }

        for( INT index = 1; index < MAX_CASCADES; ++index )
        {
            if( state.cascadePartitions[index] < state.cascadePartitions[index - 1] )
            {
                state.cascadePartitions[index] = state.cascadePartitions[index - 1];
            }
        }

        NormalizeCascadeSelections( state );
    }

    INT GetShadowSliderMinValue( ShadowSliderKind kind )
    {
        switch( kind )
        {
            case SHADOW_SLIDER_BUFFER_SIZE:
                return 1;
            case SHADOW_SLIDER_PCF_SIZE:
                return 1;
            case SHADOW_SLIDER_PCF_OFFSET:
                return 0;
            case SHADOW_SLIDER_BLEND_AMOUNT:
                return 0;
        }

        return 0;
    }

    INT GetShadowSliderMaxValue( ShadowSliderKind kind )
    {
        switch( kind )
        {
            case SHADOW_SLIDER_BUFFER_SIZE:
                return 128;
            case SHADOW_SLIDER_PCF_SIZE:
                return 16;
            case SHADOW_SLIDER_PCF_OFFSET:
                return 50;
            case SHADOW_SLIDER_BLEND_AMOUNT:
                return 100;
        }

        return 100;
    }

    INT ReadShadowSliderValue( ShadowSliderKind kind, const Gui_ShadowPanelState& state )
    {
        switch( kind )
        {
            case SHADOW_SLIDER_BUFFER_SIZE:
                return state.shadowBufferSize / 32;
            case SHADOW_SLIDER_PCF_SIZE:
                return ( state.pcfBlurSize + 1 ) / 2;
            case SHADOW_SLIDER_PCF_OFFSET:
                return (INT)( state.pcfOffset * 1000.0f );
            case SHADOW_SLIDER_BLEND_AMOUNT:
                return (INT)( state.blendAmount * 2000.0f );
        }

        return 0;
    }

    void WriteShadowSliderValue( ShadowSliderKind kind, Gui_ShadowPanelState& state, INT rawValue )
    {
        switch( kind )
        {
            case SHADOW_SLIDER_BUFFER_SIZE:
                state.shadowBufferSize = rawValue * 32;
                ClampShadowBufferSize( state );
            break;

            case SHADOW_SLIDER_PCF_SIZE:
                state.pcfBlurSize = ( rawValue * 2 ) - 1;
            break;

            case SHADOW_SLIDER_PCF_OFFSET:
                state.pcfOffset = rawValue * 0.001f;
            break;

            case SHADOW_SLIDER_BLEND_AMOUNT:
                state.blendAmount = rawValue * 0.0005f;
            break;
        }
    }

    std::wstring FormatShadowSliderCaption( ShadowSliderKind kind, INT rawValue )
    {
        WCHAR text[128] = {};

        switch( kind )
        {
            case SHADOW_SLIDER_BUFFER_SIZE:
                swprintf_s( text, L"Texture Size: %d", rawValue * 32 );
            break;

            case SHADOW_SLIDER_PCF_SIZE:
                swprintf_s( text, L"PCF Blur: %d", ( rawValue * 2 ) - 1 );
            break;

            case SHADOW_SLIDER_PCF_OFFSET:
                swprintf_s( text, L"Offset: %.3f", rawValue * 0.001f );
            break;

            case SHADOW_SLIDER_BLEND_AMOUNT:
                swprintf_s( text, L"Cascade Blur %.3f", rawValue * 0.0005f );
            break;
        }

        return text;
    }

    void SyncShadowSliderToRuntime( ShadowSliderKind kind, Gui_ShadowPanelState& state, GuiRuntimeContext& runtime )
    {
        switch( kind )
        {
            case SHADOW_SLIDER_BUFFER_SIZE:
                if( runtime.pCascadeConfig )
                {
                    runtime.pCascadeConfig->m_iBufferSize = state.shadowBufferSize;
                }
            break;

            case SHADOW_SLIDER_PCF_SIZE:
                if( runtime.pCascadedShadow )
                {
                    runtime.pCascadedShadow->m_iPCFBlurSize = state.pcfBlurSize;
                }
            break;

            case SHADOW_SLIDER_PCF_OFFSET:
                if( runtime.pCascadedShadow )
                {
                    runtime.pCascadedShadow->m_fPCFOffset = state.pcfOffset;
                }
            break;

            case SHADOW_SLIDER_BLEND_AMOUNT:
                if( runtime.pCascadedShadow )
                {
                    runtime.pCascadedShadow->m_fBlurBetweenCascadesAmount = state.blendAmount;
                }
            break;
        }
    }

    void SyncShadowSliderFromRuntime( ShadowSliderKind kind, Gui_ShadowPanelState& state, const GuiRuntimeContext& runtime )
    {
        switch( kind )
        {
            case SHADOW_SLIDER_BUFFER_SIZE:
                if( runtime.pCascadeConfig )
                {
                    state.shadowBufferSize = runtime.pCascadeConfig->m_iBufferSize;
                }
            break;

            case SHADOW_SLIDER_PCF_SIZE:
                if( runtime.pCascadedShadow )
                {
                    state.pcfBlurSize = runtime.pCascadedShadow->m_iPCFBlurSize;
                }
            break;

            case SHADOW_SLIDER_PCF_OFFSET:
                if( runtime.pCascadedShadow )
                {
                    state.pcfOffset = runtime.pCascadedShadow->m_fPCFOffset;
                }
            break;

            case SHADOW_SLIDER_BLEND_AMOUNT:
                if( runtime.pCascadedShadow )
                {
                    state.blendAmount = runtime.pCascadedShadow->m_fBlurBetweenCascadesAmount;
                }
            break;
        }
    }

    const WCHAR* GetShadowToggleLabel( ShadowToggleKind kind )
    {
        switch( kind )
        {
            case SHADOW_TOGGLE_VISUALIZE_CASCADES:
                return L"Visualize Cascades";
            case SHADOW_TOGGLE_BLEND_ENABLED:
                return L"Blend Between Cascades";
            case SHADOW_TOGGLE_DERIVATIVE_OFFSET:
                return L"DDX, DDY offset";
            case SHADOW_TOGGLE_FIT_LIGHT_TO_TEXELS:
                return L"Fit Light to Texels";
        }

        return L"Unnamed Shadow Toggle";
    }

    bool ReadShadowToggleValue( ShadowToggleKind kind, const Gui_ShadowPanelState& state )
    {
        switch( kind )
        {
            case SHADOW_TOGGLE_VISUALIZE_CASCADES:
                return state.visualizeCascades;
            case SHADOW_TOGGLE_BLEND_ENABLED:
                return state.blendBetweenMaps;
            case SHADOW_TOGGLE_DERIVATIVE_OFFSET:
                return state.derivativeOffset;
            case SHADOW_TOGGLE_FIT_LIGHT_TO_TEXELS:
                return state.fitLightToTexels;
        }

        return false;
    }

    void WriteShadowToggleValue( ShadowToggleKind kind, Gui_ShadowPanelState& state, bool checked )
    {
        switch( kind )
        {
            case SHADOW_TOGGLE_VISUALIZE_CASCADES:
                state.visualizeCascades = checked;
            break;

            case SHADOW_TOGGLE_BLEND_ENABLED:
                state.blendBetweenMaps = checked;
            break;

            case SHADOW_TOGGLE_DERIVATIVE_OFFSET:
                state.derivativeOffset = checked;
            break;

            case SHADOW_TOGGLE_FIT_LIGHT_TO_TEXELS:
                state.fitLightToTexels = checked;
            break;
        }
    }

    void SyncShadowToggleToRuntime( ShadowToggleKind kind, Gui_ShadowPanelState& state, GuiRuntimeContext& runtime )
    {
        switch( kind )
        {
            case SHADOW_TOGGLE_VISUALIZE_CASCADES:
                if( runtime.pVisualizeCascades )
                {
                    *runtime.pVisualizeCascades = state.visualizeCascades;
                }
            break;

            case SHADOW_TOGGLE_BLEND_ENABLED:
                if( runtime.pCascadedShadow )
                {
                    runtime.pCascadedShadow->m_iBlurBetweenCascades = state.blendBetweenMaps ? 1 : 0;
                }
            break;

            case SHADOW_TOGGLE_DERIVATIVE_OFFSET:
                if( runtime.pCascadedShadow )
                {
                    runtime.pCascadedShadow->m_iDerivativeBasedOffset = state.derivativeOffset ? 1 : 0;
                }
            break;

            case SHADOW_TOGGLE_FIT_LIGHT_TO_TEXELS:
                if( runtime.pMoveLightTexelSize )
                {
                    *runtime.pMoveLightTexelSize = state.fitLightToTexels;
                }
                if( runtime.pCascadedShadow )
                {
                    runtime.pCascadedShadow->m_bMoveLightTexelSize = state.fitLightToTexels;
                }
            break;
        }
    }

    void SyncShadowToggleFromRuntime( ShadowToggleKind kind, Gui_ShadowPanelState& state, const GuiRuntimeContext& runtime )
    {
        switch( kind )
        {
            case SHADOW_TOGGLE_VISUALIZE_CASCADES:
                if( runtime.pVisualizeCascades )
                {
                    state.visualizeCascades = *runtime.pVisualizeCascades;
                }
            break;

            case SHADOW_TOGGLE_BLEND_ENABLED:
                if( runtime.pCascadedShadow )
                {
                    state.blendBetweenMaps = runtime.pCascadedShadow->m_iBlurBetweenCascades != 0;
                }
            break;

            case SHADOW_TOGGLE_DERIVATIVE_OFFSET:
                if( runtime.pCascadedShadow )
                {
                    state.derivativeOffset = runtime.pCascadedShadow->m_iDerivativeBasedOffset != 0;
                }
            break;

            case SHADOW_TOGGLE_FIT_LIGHT_TO_TEXELS:
                if( runtime.pMoveLightTexelSize )
                {
                    state.fitLightToTexels = *runtime.pMoveLightTexelSize;
                }
                else if( runtime.pCascadedShadow )
                {
                    state.fitLightToTexels = runtime.pCascadedShadow->m_bMoveLightTexelSize ? true : false;
                }
            break;
        }
    }

    const WCHAR* GetShadowComboLabel( ShadowComboKind kind )
    {
        switch( kind )
        {
            case SHADOW_COMBO_DEPTH_FORMAT:
                return L"Depth Buffer";
            case SHADOW_COMBO_SELECTED_CAMERA:
                return L"Camera";
            case SHADOW_COMBO_CASCADE_LEVELS:
                return L"Cascade Levels";
            case SHADOW_COMBO_FIT_TO_CASCADE:
                return L"Projection Fit";
            case SHADOW_COMBO_FIT_TO_NEARFAR:
                return L"Near/Far Fit";
            case SHADOW_COMBO_CASCADE_SELECTION:
                return L"Cascade Selection";
        }

        return L"Shadow Setting";
    }

    void PopulateShadowCombo( ShadowComboKind kind, const Gui_ShadowPanelState& state, CDXUTComboBox& comboBox )
    {
        comboBox.RemoveAllItems();

        switch( kind )
        {
            case SHADOW_COMBO_DEPTH_FORMAT:
                comboBox.AddItem( L"32 bit Buffer", ULongToPtr( CASCADE_DXGI_FORMAT_R32_TYPELESS ) );
                comboBox.AddItem( L"16 bit Buffer", ULongToPtr( CASCADE_DXGI_FORMAT_R16_TYPELESS ) );
                comboBox.AddItem( L"24 bit Buffer", ULongToPtr( CASCADE_DXGI_FORMAT_R24G8_TYPELESS ) );
            break;

            case SHADOW_COMBO_SELECTED_CAMERA:
                comboBox.AddItem( L"Eye Camera", ULongToPtr( EYE_CAMERA ) );
                comboBox.AddItem( L"Light Camera", ULongToPtr( LIGHT_CAMERA ) );
                for( INT index = 0; index < state.cascadeLevels; ++index )
                {
                    WCHAR text[64];
                    swprintf_s( text, L"Cascade Cam %d", index + 1 );
                    comboBox.AddItem( text, ULongToPtr( ORTHO_CAMERA1 + index ) );
                }
            break;

            case SHADOW_COMBO_CASCADE_LEVELS:
                for( INT index = 1; index <= MAX_CASCADES; ++index )
                {
                    WCHAR text[32];
                    swprintf_s( text, L"%d Level", index );
                    comboBox.AddItem( text, ULongToPtr( index ) );
                }
            break;

            case SHADOW_COMBO_FIT_TO_CASCADE:
                comboBox.AddItem( L"Fit Scene", ULongToPtr( FIT_TO_SCENE ) );
                comboBox.AddItem( L"Fit Cascades", ULongToPtr( FIT_TO_CASCADES ) );
            break;

            case SHADOW_COMBO_FIT_TO_NEARFAR:
                comboBox.AddItem( L"AABB/Scene NearFar", ULongToPtr( FIT_NEARFAR_SCENE_AABB ) );
                comboBox.AddItem( L"Pancaking", ULongToPtr( FIT_NEARFAR_PANCAKING ) );
                comboBox.AddItem( L"0:1 NearFar", ULongToPtr( FIT_NEARFAR_ZERO_ONE ) );
                comboBox.AddItem( L"AABB NearFar", ULongToPtr( FIT_NEARFAR_AABB ) );
            break;

            case SHADOW_COMBO_CASCADE_SELECTION:
                comboBox.AddItem( L"Map Selection", ULongToPtr( CASCADE_SELECTION_MAP ) );
                comboBox.AddItem( L"Interval Selection", ULongToPtr( CASCADE_SELECTION_INTERVAL ) );
            break;
        }
    }

    void RefreshShadowComboSelection( ShadowComboKind kind, const Gui_ShadowPanelState& state, CDXUTComboBox& comboBox )
    {
        switch( kind )
        {
            case SHADOW_COMBO_DEPTH_FORMAT:
                comboBox.SetSelectedByData( ULongToPtr( state.depthBufferFormat ) );
            break;

            case SHADOW_COMBO_SELECTED_CAMERA:
                comboBox.SetSelectedByData( ULongToPtr( state.selectedCamera ) );
            break;

            case SHADOW_COMBO_CASCADE_LEVELS:
                comboBox.SetSelectedByData( ULongToPtr( state.cascadeLevels ) );
            break;

            case SHADOW_COMBO_FIT_TO_CASCADE:
                comboBox.SetSelectedByData( ULongToPtr( state.fitToCascades ) );
            break;

            case SHADOW_COMBO_FIT_TO_NEARFAR:
                comboBox.SetSelectedByData( ULongToPtr( state.fitToNearFar ) );
            break;

            case SHADOW_COMBO_CASCADE_SELECTION:
                comboBox.SetSelectedByData( ULongToPtr( state.cascadeSelection ) );
            break;
        }
    }

    void WriteShadowComboSelection( ShadowComboKind kind, Gui_ShadowPanelState& state, CDXUTComboBox& comboBox )
    {
        switch( kind )
        {
            case SHADOW_COMBO_DEPTH_FORMAT:
                state.depthBufferFormat = (SHADOW_TEXTURE_FORMAT)PtrToUlong( comboBox.GetSelectedData() );
            break;

            case SHADOW_COMBO_SELECTED_CAMERA:
                state.selectedCamera = (INT)PtrToUlong( comboBox.GetSelectedData() );
                ClampSelectedCamera( state );
            break;

            case SHADOW_COMBO_CASCADE_LEVELS:
                state.cascadeLevels = (INT)PtrToUlong( comboBox.GetSelectedData() );
                NormalizeShadowState( state );
            break;

            case SHADOW_COMBO_FIT_TO_CASCADE:
                state.fitToCascades = (FIT_PROJECTION_TO_CASCADES)PtrToUlong( comboBox.GetSelectedData() );
            break;

            case SHADOW_COMBO_FIT_TO_NEARFAR:
                state.fitToNearFar = (FIT_TO_NEAR_FAR)PtrToUlong( comboBox.GetSelectedData() );
                NormalizeCascadeSelections( state );
            break;

            case SHADOW_COMBO_CASCADE_SELECTION:
                state.cascadeSelection = (CASCADE_SELECTION)PtrToUlong( comboBox.GetSelectedData() );
                if( state.cascadeSelection == CASCADE_SELECTION_MAP &&
                    state.fitToNearFar == FIT_NEARFAR_PANCAKING )
                {
                    state.fitToNearFar = FIT_NEARFAR_SCENE_AABB;
                }
                NormalizeCascadeSelections( state );
            break;
        }
    }

    void SyncShadowComboToRuntime( ShadowComboKind kind, Gui_ShadowPanelState& state, GuiRuntimeContext& runtime )
    {
        switch( kind )
        {
            case SHADOW_COMBO_DEPTH_FORMAT:
                if( runtime.pCascadeConfig )
                {
                    runtime.pCascadeConfig->m_ShadowBufferFormat = state.depthBufferFormat;
                }
            break;

            case SHADOW_COMBO_SELECTED_CAMERA:
                if( runtime.ppActiveCamera )
                {
                    *runtime.ppActiveCamera = ( state.selectedCamera < LIGHT_CAMERA ) ? runtime.pViewerCamera : runtime.pLightCamera;
                }
                if( runtime.pCascadedShadow )
                {
                    runtime.pCascadedShadow->m_eSelectedCamera = (CAMERA_SELECTION)state.selectedCamera;
                }
            break;

            case SHADOW_COMBO_CASCADE_LEVELS:
                if( runtime.pCascadeConfig )
                {
                    runtime.pCascadeConfig->m_nCascadeLevels = state.cascadeLevels;
                }
            break;

            case SHADOW_COMBO_FIT_TO_CASCADE:
                if( runtime.pCascadedShadow )
                {
                    runtime.pCascadedShadow->m_eSelectedCascadesFit = state.fitToCascades;
                }
            break;

            case SHADOW_COMBO_FIT_TO_NEARFAR:
                if( runtime.pCascadedShadow )
                {
                    runtime.pCascadedShadow->m_eSelectedNearFarFit = state.fitToNearFar;
                }
            break;

            case SHADOW_COMBO_CASCADE_SELECTION:
                if( runtime.pCascadedShadow )
                {
                    runtime.pCascadedShadow->m_eSelectedCascadeSelection = state.cascadeSelection;
                }
            break;
        }
    }

    void SyncShadowComboFromRuntime( ShadowComboKind kind, Gui_ShadowPanelState& state, const GuiRuntimeContext& runtime )
    {
        switch( kind )
        {
            case SHADOW_COMBO_DEPTH_FORMAT:
                if( runtime.pCascadeConfig )
                {
                    state.depthBufferFormat = runtime.pCascadeConfig->m_ShadowBufferFormat;
                }
            break;

            case SHADOW_COMBO_SELECTED_CAMERA:
                if( runtime.pCascadedShadow )
                {
                    state.selectedCamera = runtime.pCascadedShadow->m_eSelectedCamera;
                }
            break;

            case SHADOW_COMBO_CASCADE_LEVELS:
                if( runtime.pCascadeConfig )
                {
                    state.cascadeLevels = runtime.pCascadeConfig->m_nCascadeLevels;
                }
            break;

            case SHADOW_COMBO_FIT_TO_CASCADE:
                if( runtime.pCascadedShadow )
                {
                    state.fitToCascades = runtime.pCascadedShadow->m_eSelectedCascadesFit;
                }
            break;

            case SHADOW_COMBO_FIT_TO_NEARFAR:
                if( runtime.pCascadedShadow )
                {
                    state.fitToNearFar = runtime.pCascadedShadow->m_eSelectedNearFarFit;
                }
            break;

            case SHADOW_COMBO_CASCADE_SELECTION:
                if( runtime.pCascadedShadow )
                {
                    state.cascadeSelection = runtime.pCascadedShadow->m_eSelectedCascadeSelection;
                }
            break;
        }

        NormalizeShadowState( state );
    }

    INT ReadCascadePartitionValue( INT cascadeIndex, const Gui_ShadowPanelState& state )
    {
        return state.cascadePartitions[cascadeIndex];
    }

    void WriteCascadePartitionValue( INT cascadeIndex, Gui_ShadowPanelState& state, INT rawValue )
    {
        state.cascadePartitions[cascadeIndex] = rawValue;
        NormalizeCascadePartitions( state, cascadeIndex );
    }

    std::wstring FormatCascadePartitionCaption( INT cascadeIndex, INT rawValue )
    {
        WCHAR text[64] = {};
        swprintf_s( text, L"L%d: %d", cascadeIndex + 1, rawValue );
        return text;
    }

    void SyncCascadePartitionToRuntime( INT cascadeIndex, Gui_ShadowPanelState& state, GuiRuntimeContext& runtime )
    {
        if( runtime.pCascadedShadow )
        {
            runtime.pCascadedShadow->m_iCascadePartitionsZeroToOne[cascadeIndex] = state.cascadePartitions[cascadeIndex];
        }
    }

    void SyncCascadePartitionFromRuntime( INT cascadeIndex, Gui_ShadowPanelState& state, const GuiRuntimeContext& runtime )
    {
        if( runtime.pCascadedShadow )
        {
            state.cascadePartitions[cascadeIndex] = runtime.pCascadedShadow->m_iCascadePartitionsZeroToOne[cascadeIndex];
        }
    }
}

Gui_ShadowPanel::Gui_ShadowPanel( const Gui_ShadowPanelIds& ids, Gui_ShadowPanelState& state )
    : m_ids( ids ),
      m_state( state ),
      m_pBlendAmountControl( NULL )
{
}

void Gui_ShadowPanel::BuildControls( GuiControlFactory& factory )
{
    m_cascadeControls.clear();

    AddCheckBox(
        factory,
        m_ids.visualizeCascadesCheckId,
        GetShadowToggleLabel( SHADOW_TOGGLE_VISUALIZE_CASCADES ),
        [this]() { return ReadShadowToggleValue( SHADOW_TOGGLE_VISUALIZE_CASCADES, m_state ); },
        [this]( bool checked ) { WriteShadowToggleValue( SHADOW_TOGGLE_VISUALIZE_CASCADES, m_state, checked ); },
        [this]( GuiRuntimeContext& runtime ) { SyncShadowToggleToRuntime( SHADOW_TOGGLE_VISUALIZE_CASCADES, m_state, runtime ); },
        [this]( const GuiRuntimeContext& runtime ) { SyncShadowToggleFromRuntime( SHADOW_TOGGLE_VISUALIZE_CASCADES, m_state, runtime ); } );

    const auto addShadowCombo =
        [this, &factory]( INT labelId, INT comboId, ShadowComboKind kind )
    {
        AddComboBox(
            factory,
            labelId,
            comboId,
            GetShadowComboLabel( kind ),
            [this, kind]( CDXUTComboBox& comboBox ) { PopulateShadowCombo( kind, m_state, comboBox ); },
            [this, kind]( CDXUTComboBox& comboBox ) { RefreshShadowComboSelection( kind, m_state, comboBox ); },
            [this, kind]( CDXUTComboBox& comboBox ) { WriteShadowComboSelection( kind, m_state, comboBox ); },
            [this, kind]( GuiRuntimeContext& runtime ) { SyncShadowComboToRuntime( kind, m_state, runtime ); },
            [this, kind]( const GuiRuntimeContext& runtime ) { SyncShadowComboFromRuntime( kind, m_state, runtime ); } );
    };

    addShadowCombo( m_ids.depthFormatLabelId, m_ids.depthFormatComboId, SHADOW_COMBO_DEPTH_FORMAT );
    addShadowCombo( m_ids.selectedCameraLabelId, m_ids.selectedCameraComboId, SHADOW_COMBO_SELECTED_CAMERA );
    addShadowCombo( m_ids.cascadeLevelsLabelId, m_ids.cascadeLevelsComboId, SHADOW_COMBO_CASCADE_LEVELS );
    addShadowCombo( m_ids.fitToCascadeLabelId, m_ids.fitToCascadeComboId, SHADOW_COMBO_FIT_TO_CASCADE );
    addShadowCombo( m_ids.fitToNearFarLabelId, m_ids.fitToNearFarComboId, SHADOW_COMBO_FIT_TO_NEARFAR );
    addShadowCombo( m_ids.cascadeSelectionLabelId, m_ids.cascadeSelectionComboId, SHADOW_COMBO_CASCADE_SELECTION );

    AddCheckBox(
        factory,
        m_ids.fitLightToTexelsCheckId,
        GetShadowToggleLabel( SHADOW_TOGGLE_FIT_LIGHT_TO_TEXELS ),
        [this]() { return ReadShadowToggleValue( SHADOW_TOGGLE_FIT_LIGHT_TO_TEXELS, m_state ); },
        [this]( bool checked ) { WriteShadowToggleValue( SHADOW_TOGGLE_FIT_LIGHT_TO_TEXELS, m_state, checked ); },
        [this]( GuiRuntimeContext& runtime ) { SyncShadowToggleToRuntime( SHADOW_TOGGLE_FIT_LIGHT_TO_TEXELS, m_state, runtime ); },
        [this]( const GuiRuntimeContext& runtime ) { SyncShadowToggleFromRuntime( SHADOW_TOGGLE_FIT_LIGHT_TO_TEXELS, m_state, runtime ); } );

    const auto addShadowSlider =
        [this, &factory]( INT textId, INT sliderId, ShadowSliderKind kind ) -> Gui_SliderControl&
    {
        return AddSlider(
            factory,
            textId,
            sliderId,
            GetShadowSliderMinValue( kind ),
            GetShadowSliderMaxValue( kind ),
            [this, kind]() { return ReadShadowSliderValue( kind, m_state ); },
            [this, kind]( INT rawValue, UINT ) { WriteShadowSliderValue( kind, m_state, rawValue ); },
            [kind]( INT rawValue ) { return FormatShadowSliderCaption( kind, rawValue ); },
            [this, kind]( GuiRuntimeContext& runtime ) { SyncShadowSliderToRuntime( kind, m_state, runtime ); },
            [this, kind]( const GuiRuntimeContext& runtime ) { SyncShadowSliderFromRuntime( kind, m_state, runtime ); } );
    };

    addShadowSlider( m_ids.bufferSizeTextId, m_ids.bufferSizeSliderId, SHADOW_SLIDER_BUFFER_SIZE );
    addShadowSlider( m_ids.pcfSizeTextId, m_ids.pcfSizeSliderId, SHADOW_SLIDER_PCF_SIZE );
    addShadowSlider( m_ids.pcfOffsetTextId, m_ids.pcfOffsetSliderId, SHADOW_SLIDER_PCF_OFFSET );

    AddCheckBox(
        factory,
        m_ids.blendEnabledCheckId,
        GetShadowToggleLabel( SHADOW_TOGGLE_BLEND_ENABLED ),
        [this]() { return ReadShadowToggleValue( SHADOW_TOGGLE_BLEND_ENABLED, m_state ); },
        [this]( bool checked ) { WriteShadowToggleValue( SHADOW_TOGGLE_BLEND_ENABLED, m_state, checked ); },
        [this]( GuiRuntimeContext& runtime ) { SyncShadowToggleToRuntime( SHADOW_TOGGLE_BLEND_ENABLED, m_state, runtime ); },
        [this]( const GuiRuntimeContext& runtime ) { SyncShadowToggleFromRuntime( SHADOW_TOGGLE_BLEND_ENABLED, m_state, runtime ); } );

    m_pBlendAmountControl = &addShadowSlider( m_ids.blendAmountTextId, m_ids.blendAmountSliderId, SHADOW_SLIDER_BLEND_AMOUNT );

    AddCheckBox(
        factory,
        m_ids.derivativeOffsetCheckId,
        GetShadowToggleLabel( SHADOW_TOGGLE_DERIVATIVE_OFFSET ),
        [this]() { return ReadShadowToggleValue( SHADOW_TOGGLE_DERIVATIVE_OFFSET, m_state ); },
        [this]( bool checked ) { WriteShadowToggleValue( SHADOW_TOGGLE_DERIVATIVE_OFFSET, m_state, checked ); },
        [this]( GuiRuntimeContext& runtime ) { SyncShadowToggleToRuntime( SHADOW_TOGGLE_DERIVATIVE_OFFSET, m_state, runtime ); },
        [this]( const GuiRuntimeContext& runtime ) { SyncShadowToggleFromRuntime( SHADOW_TOGGLE_DERIVATIVE_OFFSET, m_state, runtime ); } );

    for( INT index = 0; index < MAX_CASCADES; ++index )
    {
        Gui_SliderControl& control = AddSlider(
            factory,
            m_ids.cascadePartitionTextIds[index],
            m_ids.cascadePartitionSliderIds[index],
            0,
            100,
            [this, index]() { return ReadCascadePartitionValue( index, m_state ); },
            [this, index]( INT rawValue, UINT ) { WriteCascadePartitionValue( index, m_state, rawValue ); },
            [index]( INT rawValue ) { return FormatCascadePartitionCaption( index, rawValue ); },
            [this, index]( GuiRuntimeContext& runtime ) { SyncCascadePartitionToRuntime( index, m_state, runtime ); },
            [this, index]( const GuiRuntimeContext& runtime ) { SyncCascadePartitionFromRuntime( index, m_state, runtime ); } );
        m_cascadeControls.push_back( &control );
    }
}

void Gui_ShadowPanel::OnUpdate()
{
    NormalizeShadowState( m_state );

    if( m_pBlendAmountControl )
    {
        m_pBlendAmountControl->SetEnabled( m_state.blendBetweenMaps );
    }

    for( INT index = 0; index < MAX_CASCADES && index < (INT)m_cascadeControls.size(); ++index )
    {
        const bool isVisible = index < m_state.cascadeLevels;
        const bool isEnabled = isVisible &&
                               !( m_state.cascadeSelection == CASCADE_SELECTION_INTERVAL &&
                                  index == ( m_state.cascadeLevels - 1 ) );

        m_cascadeControls[index]->SetVisible( isVisible );
        m_cascadeControls[index]->SetEnabled( isEnabled );

        CDXUTStatic* pLabel = Dialog().GetStatic( m_ids.cascadePartitionTextIds[index] );
        if( pLabel )
        {
            pLabel->SetTextColor( kCascadeColors[index] );
        }
    }
}
