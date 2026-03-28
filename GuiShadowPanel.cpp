#include "DXUT.h"
#include "GuiShadowPanel.h"
#include "CascadedShadowsManager.h"
#include <wchar.h>

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
}

Gui_ShadowSliderStrategy::Gui_ShadowSliderStrategy( GUI_SHADOW_SLIDER_KIND kind, Gui_ShadowPanelState& state )
    : m_kind( kind ),
      m_state( state )
{
}

INT Gui_ShadowSliderStrategy::GetMinValue() const
{
    switch( m_kind )
    {
        case GUI_SHADOW_SLIDER_BUFFER_SIZE:
            return 1;
        case GUI_SHADOW_SLIDER_PCF_SIZE:
            return 1;
        case GUI_SHADOW_SLIDER_PCF_OFFSET:
            return 0;
        case GUI_SHADOW_SLIDER_BLEND_AMOUNT:
            return 0;
    }

    return 0;
}

INT Gui_ShadowSliderStrategy::GetMaxValue() const
{
    switch( m_kind )
    {
        case GUI_SHADOW_SLIDER_BUFFER_SIZE:
            return 128;
        case GUI_SHADOW_SLIDER_PCF_SIZE:
            return 16;
        case GUI_SHADOW_SLIDER_PCF_OFFSET:
            return 50;
        case GUI_SHADOW_SLIDER_BLEND_AMOUNT:
            return 100;
    }

    return 100;
}

INT Gui_ShadowSliderStrategy::ReadValue() const
{
    switch( m_kind )
    {
        case GUI_SHADOW_SLIDER_BUFFER_SIZE:
            return m_state.shadowBufferSize / 32;
        case GUI_SHADOW_SLIDER_PCF_SIZE:
            return ( m_state.pcfBlurSize + 1 ) / 2;
        case GUI_SHADOW_SLIDER_PCF_OFFSET:
            return ( INT )( m_state.pcfOffset * 1000.0f );
        case GUI_SHADOW_SLIDER_BLEND_AMOUNT:
            return ( INT )( m_state.blendAmount * 2000.0f );
    }

    return 0;
}

void Gui_ShadowSliderStrategy::WriteValue( INT rawValue, UINT )
{
    switch( m_kind )
    {
        case GUI_SHADOW_SLIDER_BUFFER_SIZE:
            m_state.shadowBufferSize = rawValue * 32;
            ClampShadowBufferSize( m_state );
        break;

        case GUI_SHADOW_SLIDER_PCF_SIZE:
            m_state.pcfBlurSize = ( rawValue * 2 ) - 1;
        break;

        case GUI_SHADOW_SLIDER_PCF_OFFSET:
            m_state.pcfOffset = rawValue * 0.001f;
        break;

        case GUI_SHADOW_SLIDER_BLEND_AMOUNT:
            m_state.blendAmount = rawValue * 0.0005f;
        break;
    }
}

void Gui_ShadowSliderStrategy::FormatCaption( INT rawValue, WCHAR* text, size_t textCount ) const
{
    switch( m_kind )
    {
        case GUI_SHADOW_SLIDER_BUFFER_SIZE:
            swprintf_s( text, textCount, L"Texture Size: %d", rawValue * 32 );
        break;

        case GUI_SHADOW_SLIDER_PCF_SIZE:
            swprintf_s( text, textCount, L"PCF Blur: %d", ( rawValue * 2 ) - 1 );
        break;

        case GUI_SHADOW_SLIDER_PCF_OFFSET:
            swprintf_s( text, textCount, L"Offset: %.3f", rawValue * 0.001f );
        break;

        case GUI_SHADOW_SLIDER_BLEND_AMOUNT:
            swprintf_s( text, textCount, L"Cascade Blur %.3f", rawValue * 0.0005f );
        break;
    }
}

void Gui_ShadowSliderStrategy::SyncToRuntime( GuiRuntimeContext& runtime )
{
    switch( m_kind )
    {
        case GUI_SHADOW_SLIDER_BUFFER_SIZE:
            if( runtime.pCascadeConfig )
            {
                runtime.pCascadeConfig->m_iBufferSize = m_state.shadowBufferSize;
            }
        break;

        case GUI_SHADOW_SLIDER_PCF_SIZE:
            if( runtime.pCascadedShadow )
            {
                runtime.pCascadedShadow->m_iPCFBlurSize = m_state.pcfBlurSize;
            }
        break;

        case GUI_SHADOW_SLIDER_PCF_OFFSET:
            if( runtime.pCascadedShadow )
            {
                runtime.pCascadedShadow->m_fPCFOffset = m_state.pcfOffset;
            }
        break;

        case GUI_SHADOW_SLIDER_BLEND_AMOUNT:
            if( runtime.pCascadedShadow )
            {
                runtime.pCascadedShadow->m_fBlurBetweenCascadesAmount = m_state.blendAmount;
            }
        break;
    }
}

void Gui_ShadowSliderStrategy::SyncFromRuntime( const GuiRuntimeContext& runtime )
{
    switch( m_kind )
    {
        case GUI_SHADOW_SLIDER_BUFFER_SIZE:
            if( runtime.pCascadeConfig )
            {
                m_state.shadowBufferSize = runtime.pCascadeConfig->m_iBufferSize;
            }
        break;

        case GUI_SHADOW_SLIDER_PCF_SIZE:
            if( runtime.pCascadedShadow )
            {
                m_state.pcfBlurSize = runtime.pCascadedShadow->m_iPCFBlurSize;
            }
        break;

        case GUI_SHADOW_SLIDER_PCF_OFFSET:
            if( runtime.pCascadedShadow )
            {
                m_state.pcfOffset = runtime.pCascadedShadow->m_fPCFOffset;
            }
        break;

        case GUI_SHADOW_SLIDER_BLEND_AMOUNT:
            if( runtime.pCascadedShadow )
            {
                m_state.blendAmount = runtime.pCascadedShadow->m_fBlurBetweenCascadesAmount;
            }
        break;
    }
}

Gui_ShadowToggleStrategy::Gui_ShadowToggleStrategy( GUI_SHADOW_TOGGLE_KIND kind, Gui_ShadowPanelState& state )
    : m_kind( kind ),
      m_state( state )
{
}

const WCHAR* Gui_ShadowToggleStrategy::GetLabelText() const
{
    switch( m_kind )
    {
        case GUI_SHADOW_TOGGLE_VISUALIZE_CASCADES:
            return L"Visualize Cascades";
        case GUI_SHADOW_TOGGLE_BLEND_ENABLED:
            return L"Blend Between Cascades";
        case GUI_SHADOW_TOGGLE_DERIVATIVE_OFFSET:
            return L"DDX, DDY offset";
        case GUI_SHADOW_TOGGLE_FIT_LIGHT_TO_TEXELS:
            return L"Fit Light to Texels";
    }

    return L"Unnamed Shadow Toggle";
}

bool Gui_ShadowToggleStrategy::ReadValue() const
{
    switch( m_kind )
    {
        case GUI_SHADOW_TOGGLE_VISUALIZE_CASCADES:
            return m_state.visualizeCascades;
        case GUI_SHADOW_TOGGLE_BLEND_ENABLED:
            return m_state.blendBetweenMaps;
        case GUI_SHADOW_TOGGLE_DERIVATIVE_OFFSET:
            return m_state.derivativeOffset;
        case GUI_SHADOW_TOGGLE_FIT_LIGHT_TO_TEXELS:
            return m_state.fitLightToTexels;
    }

    return false;
}

void Gui_ShadowToggleStrategy::WriteValue( bool checked )
{
    switch( m_kind )
    {
        case GUI_SHADOW_TOGGLE_VISUALIZE_CASCADES:
            m_state.visualizeCascades = checked;
        break;

        case GUI_SHADOW_TOGGLE_BLEND_ENABLED:
            m_state.blendBetweenMaps = checked;
        break;

        case GUI_SHADOW_TOGGLE_DERIVATIVE_OFFSET:
            m_state.derivativeOffset = checked;
        break;

        case GUI_SHADOW_TOGGLE_FIT_LIGHT_TO_TEXELS:
            m_state.fitLightToTexels = checked;
        break;
    }
}

void Gui_ShadowToggleStrategy::SyncToRuntime( GuiRuntimeContext& runtime )
{
    switch( m_kind )
    {
        case GUI_SHADOW_TOGGLE_VISUALIZE_CASCADES:
            if( runtime.pVisualizeCascades )
            {
                *runtime.pVisualizeCascades = m_state.visualizeCascades;
            }
        break;

        case GUI_SHADOW_TOGGLE_BLEND_ENABLED:
            if( runtime.pCascadedShadow )
            {
                runtime.pCascadedShadow->m_iBlurBetweenCascades = m_state.blendBetweenMaps ? 1 : 0;
            }
        break;

        case GUI_SHADOW_TOGGLE_DERIVATIVE_OFFSET:
            if( runtime.pCascadedShadow )
            {
                runtime.pCascadedShadow->m_iDerivativeBasedOffset = m_state.derivativeOffset ? 1 : 0;
            }
        break;

        case GUI_SHADOW_TOGGLE_FIT_LIGHT_TO_TEXELS:
            if( runtime.pMoveLightTexelSize )
            {
                *runtime.pMoveLightTexelSize = m_state.fitLightToTexels;
            }
            if( runtime.pCascadedShadow )
            {
                runtime.pCascadedShadow->m_bMoveLightTexelSize = m_state.fitLightToTexels;
            }
        break;
    }
}

void Gui_ShadowToggleStrategy::SyncFromRuntime( const GuiRuntimeContext& runtime )
{
    switch( m_kind )
    {
        case GUI_SHADOW_TOGGLE_VISUALIZE_CASCADES:
            if( runtime.pVisualizeCascades )
            {
                m_state.visualizeCascades = *runtime.pVisualizeCascades;
            }
        break;

        case GUI_SHADOW_TOGGLE_BLEND_ENABLED:
            if( runtime.pCascadedShadow )
            {
                m_state.blendBetweenMaps = runtime.pCascadedShadow->m_iBlurBetweenCascades != 0;
            }
        break;

        case GUI_SHADOW_TOGGLE_DERIVATIVE_OFFSET:
            if( runtime.pCascadedShadow )
            {
                m_state.derivativeOffset = runtime.pCascadedShadow->m_iDerivativeBasedOffset != 0;
            }
        break;

        case GUI_SHADOW_TOGGLE_FIT_LIGHT_TO_TEXELS:
            if( runtime.pMoveLightTexelSize )
            {
                m_state.fitLightToTexels = *runtime.pMoveLightTexelSize;
            }
            else if( runtime.pCascadedShadow )
            {
                m_state.fitLightToTexels = runtime.pCascadedShadow->m_bMoveLightTexelSize ? true : false;
            }
        break;
    }
}

Gui_ShadowComboStrategy::Gui_ShadowComboStrategy( GUI_SHADOW_COMBO_KIND kind, Gui_ShadowPanelState& state )
    : m_kind( kind ),
      m_state( state )
{
}

const WCHAR* Gui_ShadowComboStrategy::GetLabelText() const
{
    switch( m_kind )
    {
        case GUI_SHADOW_COMBO_DEPTH_FORMAT:
            return L"Depth Buffer";
        case GUI_SHADOW_COMBO_SELECTED_CAMERA:
            return L"Camera";
        case GUI_SHADOW_COMBO_CASCADE_LEVELS:
            return L"Cascade Levels";
        case GUI_SHADOW_COMBO_FIT_TO_CASCADE:
            return L"Projection Fit";
        case GUI_SHADOW_COMBO_FIT_TO_NEARFAR:
            return L"Near/Far Fit";
        case GUI_SHADOW_COMBO_CASCADE_SELECTION:
            return L"Cascade Selection";
    }

    return L"Shadow Setting";
}

void Gui_ShadowComboStrategy::Populate( CDXUTComboBox& comboBox ) const
{
    comboBox.RemoveAllItems();

    switch( m_kind )
    {
        case GUI_SHADOW_COMBO_DEPTH_FORMAT:
            comboBox.AddItem( L"32 bit Buffer", ULongToPtr( CASCADE_DXGI_FORMAT_R32_TYPELESS ) );
            comboBox.AddItem( L"16 bit Buffer", ULongToPtr( CASCADE_DXGI_FORMAT_R16_TYPELESS ) );
            comboBox.AddItem( L"24 bit Buffer", ULongToPtr( CASCADE_DXGI_FORMAT_R24G8_TYPELESS ) );
        break;

        case GUI_SHADOW_COMBO_SELECTED_CAMERA:
            comboBox.AddItem( L"Eye Camera", ULongToPtr( EYE_CAMERA ) );
            comboBox.AddItem( L"Light Camera", ULongToPtr( LIGHT_CAMERA ) );
            for( INT index = 0; index < m_state.cascadeLevels; ++index )
            {
                WCHAR text[64];
                swprintf_s( text, L"Cascade Cam %d", index + 1 );
                comboBox.AddItem( text, ULongToPtr( ORTHO_CAMERA1 + index ) );
            }
        break;

        case GUI_SHADOW_COMBO_CASCADE_LEVELS:
            for( INT index = 1; index <= MAX_CASCADES; ++index )
            {
                WCHAR text[32];
                swprintf_s( text, L"%d Level", index );
                comboBox.AddItem( text, ULongToPtr( index ) );
            }
        break;

        case GUI_SHADOW_COMBO_FIT_TO_CASCADE:
            comboBox.AddItem( L"Fit Scene", ULongToPtr( FIT_TO_SCENE ) );
            comboBox.AddItem( L"Fit Cascades", ULongToPtr( FIT_TO_CASCADES ) );
        break;

        case GUI_SHADOW_COMBO_FIT_TO_NEARFAR:
            comboBox.AddItem( L"AABB/Scene NearFar", ULongToPtr( FIT_NEARFAR_SCENE_AABB ) );
            comboBox.AddItem( L"Pancaking", ULongToPtr( FIT_NEARFAR_PANCAKING ) );
            comboBox.AddItem( L"0:1 NearFar", ULongToPtr( FIT_NEARFAR_ZERO_ONE ) );
            comboBox.AddItem( L"AABB NearFar", ULongToPtr( FIT_NEARFAR_AABB ) );
        break;

        case GUI_SHADOW_COMBO_CASCADE_SELECTION:
            comboBox.AddItem( L"Map Selection", ULongToPtr( CASCADE_SELECTION_MAP ) );
            comboBox.AddItem( L"Interval Selection", ULongToPtr( CASCADE_SELECTION_INTERVAL ) );
        break;
    }
}

void Gui_ShadowComboStrategy::RefreshSelection( CDXUTComboBox& comboBox ) const
{
    switch( m_kind )
    {
        case GUI_SHADOW_COMBO_DEPTH_FORMAT:
            comboBox.SetSelectedByData( ULongToPtr( m_state.depthBufferFormat ) );
        break;

        case GUI_SHADOW_COMBO_SELECTED_CAMERA:
            comboBox.SetSelectedByData( ULongToPtr( m_state.selectedCamera ) );
        break;

        case GUI_SHADOW_COMBO_CASCADE_LEVELS:
            comboBox.SetSelectedByData( ULongToPtr( m_state.cascadeLevels ) );
        break;

        case GUI_SHADOW_COMBO_FIT_TO_CASCADE:
            comboBox.SetSelectedByData( ULongToPtr( m_state.fitToCascades ) );
        break;

        case GUI_SHADOW_COMBO_FIT_TO_NEARFAR:
            comboBox.SetSelectedByData( ULongToPtr( m_state.fitToNearFar ) );
        break;

        case GUI_SHADOW_COMBO_CASCADE_SELECTION:
            comboBox.SetSelectedByData( ULongToPtr( m_state.cascadeSelection ) );
        break;
    }
}

void Gui_ShadowComboStrategy::WriteSelection( CDXUTComboBox& comboBox )
{
    switch( m_kind )
    {
        case GUI_SHADOW_COMBO_DEPTH_FORMAT:
            m_state.depthBufferFormat = ( SHADOW_TEXTURE_FORMAT )PtrToUlong( comboBox.GetSelectedData() );
        break;

        case GUI_SHADOW_COMBO_SELECTED_CAMERA:
            m_state.selectedCamera = ( INT )PtrToUlong( comboBox.GetSelectedData() );
            ClampSelectedCamera( m_state );
        break;

        case GUI_SHADOW_COMBO_CASCADE_LEVELS:
            m_state.cascadeLevels = ( INT )PtrToUlong( comboBox.GetSelectedData() );
            NormalizeShadowState( m_state );
        break;

        case GUI_SHADOW_COMBO_FIT_TO_CASCADE:
            m_state.fitToCascades = ( FIT_PROJECTION_TO_CASCADES )PtrToUlong( comboBox.GetSelectedData() );
        break;

        case GUI_SHADOW_COMBO_FIT_TO_NEARFAR:
            m_state.fitToNearFar = ( FIT_TO_NEAR_FAR )PtrToUlong( comboBox.GetSelectedData() );
            NormalizeCascadeSelections( m_state );
        break;

        case GUI_SHADOW_COMBO_CASCADE_SELECTION:
            m_state.cascadeSelection = ( CASCADE_SELECTION )PtrToUlong( comboBox.GetSelectedData() );
            if( m_state.cascadeSelection == CASCADE_SELECTION_MAP &&
                m_state.fitToNearFar == FIT_NEARFAR_PANCAKING )
            {
                m_state.fitToNearFar = FIT_NEARFAR_SCENE_AABB;
            }
            NormalizeCascadeSelections( m_state );
        break;
    }
}

void Gui_ShadowComboStrategy::SyncToRuntime( GuiRuntimeContext& runtime )
{
    switch( m_kind )
    {
        case GUI_SHADOW_COMBO_DEPTH_FORMAT:
            if( runtime.pCascadeConfig )
            {
                runtime.pCascadeConfig->m_ShadowBufferFormat = m_state.depthBufferFormat;
            }
        break;

        case GUI_SHADOW_COMBO_SELECTED_CAMERA:
            if( runtime.ppActiveCamera )
            {
                *runtime.ppActiveCamera = ( m_state.selectedCamera < LIGHT_CAMERA ) ? runtime.pViewerCamera : runtime.pLightCamera;
            }
            if( runtime.pCascadedShadow )
            {
                runtime.pCascadedShadow->m_eSelectedCamera = ( CAMERA_SELECTION )m_state.selectedCamera;
            }
        break;

        case GUI_SHADOW_COMBO_CASCADE_LEVELS:
            if( runtime.pCascadeConfig )
            {
                runtime.pCascadeConfig->m_nCascadeLevels = m_state.cascadeLevels;
            }
        break;

        case GUI_SHADOW_COMBO_FIT_TO_CASCADE:
            if( runtime.pCascadedShadow )
            {
                runtime.pCascadedShadow->m_eSelectedCascadesFit = m_state.fitToCascades;
            }
        break;

        case GUI_SHADOW_COMBO_FIT_TO_NEARFAR:
            if( runtime.pCascadedShadow )
            {
                runtime.pCascadedShadow->m_eSelectedNearFarFit = m_state.fitToNearFar;
            }
        break;

        case GUI_SHADOW_COMBO_CASCADE_SELECTION:
            if( runtime.pCascadedShadow )
            {
                runtime.pCascadedShadow->m_eSelectedCascadeSelection = m_state.cascadeSelection;
            }
        break;
    }
}

void Gui_ShadowComboStrategy::SyncFromRuntime( const GuiRuntimeContext& runtime )
{
    switch( m_kind )
    {
        case GUI_SHADOW_COMBO_DEPTH_FORMAT:
            if( runtime.pCascadeConfig )
            {
                m_state.depthBufferFormat = runtime.pCascadeConfig->m_ShadowBufferFormat;
            }
        break;

        case GUI_SHADOW_COMBO_SELECTED_CAMERA:
            if( runtime.pCascadedShadow )
            {
                m_state.selectedCamera = runtime.pCascadedShadow->m_eSelectedCamera;
            }
        break;

        case GUI_SHADOW_COMBO_CASCADE_LEVELS:
            if( runtime.pCascadeConfig )
            {
                m_state.cascadeLevels = runtime.pCascadeConfig->m_nCascadeLevels;
            }
        break;

        case GUI_SHADOW_COMBO_FIT_TO_CASCADE:
            if( runtime.pCascadedShadow )
            {
                m_state.fitToCascades = runtime.pCascadedShadow->m_eSelectedCascadesFit;
            }
        break;

        case GUI_SHADOW_COMBO_FIT_TO_NEARFAR:
            if( runtime.pCascadedShadow )
            {
                m_state.fitToNearFar = runtime.pCascadedShadow->m_eSelectedNearFarFit;
            }
        break;

        case GUI_SHADOW_COMBO_CASCADE_SELECTION:
            if( runtime.pCascadedShadow )
            {
                m_state.cascadeSelection = runtime.pCascadedShadow->m_eSelectedCascadeSelection;
            }
        break;
    }

    NormalizeShadowState( m_state );
}

Gui_ShadowCascadeSliderStrategy::Gui_ShadowCascadeSliderStrategy( INT cascadeIndex, Gui_ShadowPanelState& state )
    : m_cascadeIndex( cascadeIndex ),
      m_state( state )
{
}

INT Gui_ShadowCascadeSliderStrategy::GetMinValue() const
{
    return 0;
}

INT Gui_ShadowCascadeSliderStrategy::GetMaxValue() const
{
    return 100;
}

INT Gui_ShadowCascadeSliderStrategy::ReadValue() const
{
    return m_state.cascadePartitions[m_cascadeIndex];
}

void Gui_ShadowCascadeSliderStrategy::WriteValue( INT rawValue, UINT )
{
    m_state.cascadePartitions[m_cascadeIndex] = rawValue;
    NormalizeCascadePartitions( m_state, m_cascadeIndex );
}

void Gui_ShadowCascadeSliderStrategy::FormatCaption( INT rawValue, WCHAR* text, size_t textCount ) const
{
    swprintf_s( text, textCount, L"L%d: %d", m_cascadeIndex + 1, rawValue );
}

void Gui_ShadowCascadeSliderStrategy::SyncToRuntime( GuiRuntimeContext& runtime )
{
    if( runtime.pCascadedShadow )
    {
        runtime.pCascadedShadow->m_iCascadePartitionsZeroToOne[m_cascadeIndex] = m_state.cascadePartitions[m_cascadeIndex];
    }
}

void Gui_ShadowCascadeSliderStrategy::SyncFromRuntime( const GuiRuntimeContext& runtime )
{
    if( runtime.pCascadedShadow )
    {
        m_state.cascadePartitions[m_cascadeIndex] = runtime.pCascadedShadow->m_iCascadePartitionsZeroToOne[m_cascadeIndex];
    }
}

Gui_ShadowPanel::Gui_ShadowPanel( const Gui_ShadowPanelIds& ids, Gui_ShadowPanelState& state )
    : m_ids( ids ),
      m_state( state ),
      m_depthFormatStrategy( GUI_SHADOW_COMBO_DEPTH_FORMAT, state ),
      m_selectedCameraStrategy( GUI_SHADOW_COMBO_SELECTED_CAMERA, state ),
      m_cascadeLevelsStrategy( GUI_SHADOW_COMBO_CASCADE_LEVELS, state ),
      m_fitToCascadesStrategy( GUI_SHADOW_COMBO_FIT_TO_CASCADE, state ),
      m_fitToNearFarStrategy( GUI_SHADOW_COMBO_FIT_TO_NEARFAR, state ),
      m_cascadeSelectionStrategy( GUI_SHADOW_COMBO_CASCADE_SELECTION, state ),
      m_bufferSizeStrategy( GUI_SHADOW_SLIDER_BUFFER_SIZE, state ),
      m_pcfSizeStrategy( GUI_SHADOW_SLIDER_PCF_SIZE, state ),
      m_pcfOffsetStrategy( GUI_SHADOW_SLIDER_PCF_OFFSET, state ),
      m_blendAmountStrategy( GUI_SHADOW_SLIDER_BLEND_AMOUNT, state ),
      m_visualizeCascadesStrategy( GUI_SHADOW_TOGGLE_VISUALIZE_CASCADES, state ),
      m_blendEnabledStrategy( GUI_SHADOW_TOGGLE_BLEND_ENABLED, state ),
      m_derivativeOffsetStrategy( GUI_SHADOW_TOGGLE_DERIVATIVE_OFFSET, state ),
      m_fitLightToTexelsStrategy( GUI_SHADOW_TOGGLE_FIT_LIGHT_TO_TEXELS, state ),
      m_depthFormatControl( ids.depthFormatLabelId, ids.depthFormatComboId, m_depthFormatStrategy ),
      m_selectedCameraControl( ids.selectedCameraLabelId, ids.selectedCameraComboId, m_selectedCameraStrategy ),
      m_cascadeLevelsControl( ids.cascadeLevelsLabelId, ids.cascadeLevelsComboId, m_cascadeLevelsStrategy ),
      m_fitToCascadesControl( ids.fitToCascadeLabelId, ids.fitToCascadeComboId, m_fitToCascadesStrategy ),
      m_fitToNearFarControl( ids.fitToNearFarLabelId, ids.fitToNearFarComboId, m_fitToNearFarStrategy ),
      m_cascadeSelectionControl( ids.cascadeSelectionLabelId, ids.cascadeSelectionComboId, m_cascadeSelectionStrategy ),
      m_visualizeCascadesControl( ids.visualizeCascadesCheckId, m_visualizeCascadesStrategy ),
      m_bufferSizeControl( ids.bufferSizeTextId, ids.bufferSizeSliderId, m_bufferSizeStrategy ),
      m_pcfSizeControl( ids.pcfSizeTextId, ids.pcfSizeSliderId, m_pcfSizeStrategy ),
      m_pcfOffsetControl( ids.pcfOffsetTextId, ids.pcfOffsetSliderId, m_pcfOffsetStrategy ),
      m_blendEnabledControl( ids.blendEnabledCheckId, m_blendEnabledStrategy ),
      m_blendAmountControl( ids.blendAmountTextId, ids.blendAmountSliderId, m_blendAmountStrategy ),
      m_derivativeOffsetControl( ids.derivativeOffsetCheckId, m_derivativeOffsetStrategy ),
      m_fitLightToTexelsControl( ids.fitLightToTexelsCheckId, m_fitLightToTexelsStrategy )
{
    for( INT index = 0; index < MAX_CASCADES; ++index )
    {
        m_cascadeStrategies.push_back(
            std::unique_ptr<Gui_ShadowCascadeSliderStrategy>(
                new Gui_ShadowCascadeSliderStrategy( index, state ) ) );
        m_cascadeControls.push_back(
            std::unique_ptr<Gui_SliderControl>(
                new Gui_SliderControl( ids.cascadePartitionTextIds[index],
                                       ids.cascadePartitionSliderIds[index],
                                       *m_cascadeStrategies.back() ) ) );
    }
}

void Gui_ShadowPanel::BuildControls( GuiControlFactory& factory )
{
    RegisterControl( m_visualizeCascadesControl );
    RegisterControl( m_depthFormatControl );
    RegisterControl( m_selectedCameraControl );
    RegisterControl( m_cascadeLevelsControl );
    RegisterControl( m_fitToCascadesControl );
    RegisterControl( m_fitToNearFarControl );
    RegisterControl( m_cascadeSelectionControl );
    RegisterControl( m_fitLightToTexelsControl );
    RegisterControl( m_bufferSizeControl );
    RegisterControl( m_pcfSizeControl );
    RegisterControl( m_pcfOffsetControl );
    RegisterControl( m_blendEnabledControl );
    RegisterControl( m_blendAmountControl );
    RegisterControl( m_derivativeOffsetControl );

    factory.Add( m_visualizeCascadesControl );
    factory.Add( m_depthFormatControl );
    factory.Add( m_selectedCameraControl );
    factory.Add( m_cascadeLevelsControl );
    factory.Add( m_fitToCascadesControl );
    factory.Add( m_fitToNearFarControl );
    factory.Add( m_cascadeSelectionControl );
    factory.Add( m_fitLightToTexelsControl );
    factory.Add( m_bufferSizeControl );
    factory.Add( m_pcfSizeControl );
    factory.Add( m_pcfOffsetControl );
    factory.Add( m_blendEnabledControl );
    factory.Add( m_blendAmountControl );
    factory.Add( m_derivativeOffsetControl );

    for( INT index = 0; index < MAX_CASCADES; ++index )
    {
        RegisterControl( *m_cascadeControls[index] );
        factory.Add( *m_cascadeControls[index] );
    }
}

void Gui_ShadowPanel::OnUpdate()
{
    NormalizeShadowState( m_state );
    m_blendAmountControl.SetEnabled( m_state.blendBetweenMaps );

    for( INT index = 0; index < MAX_CASCADES; ++index )
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
