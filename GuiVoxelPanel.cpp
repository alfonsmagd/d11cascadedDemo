#include "DXUT.h"
#include "GuiVoxelPanel.h"
#include "CascadedShadowsManager.h"

namespace
{
    enum VoxelSliderKind
    {
        VOXEL_SLIDER_SURFACE_SNAP,
        VOXEL_SLIDER_HEIGHT_WARP,
        VOXEL_SLIDER_XY_FILL,
        VOXEL_SLIDER_YZ_FILL,
        VOXEL_SLIDER_TOP_COVERAGE
    };

    INT GetVoxelSliderMinValue( VoxelSliderKind kind )
    {
        switch( kind )
        {
            case VOXEL_SLIDER_SURFACE_SNAP:
                return 0;
            case VOXEL_SLIDER_HEIGHT_WARP:
                return 20;
            case VOXEL_SLIDER_XY_FILL:
            case VOXEL_SLIDER_YZ_FILL:
            case VOXEL_SLIDER_TOP_COVERAGE:
                return 50;
        }

        return 0;
    }

    INT GetVoxelSliderMaxValue( VoxelSliderKind kind )
    {
        switch( kind )
        {
            case VOXEL_SLIDER_SURFACE_SNAP:
                return 100;
            case VOXEL_SLIDER_HEIGHT_WARP:
                return 250;
            case VOXEL_SLIDER_XY_FILL:
            case VOXEL_SLIDER_YZ_FILL:
                return 400;
            case VOXEL_SLIDER_TOP_COVERAGE:
                return 100;
        }

        return 100;
    }

    INT ReadVoxelSliderValue( VoxelSliderKind kind, const Gui_VoxelPanelState& state )
    {
        switch( kind )
        {
            case VOXEL_SLIDER_SURFACE_SNAP:
                return (INT)( state.surfaceSnap * 100.0f );
            case VOXEL_SLIDER_HEIGHT_WARP:
                return (INT)( state.heightWarp * 100.0f );
            case VOXEL_SLIDER_XY_FILL:
                return (INT)( state.xyFill * 100.0f );
            case VOXEL_SLIDER_YZ_FILL:
                return (INT)( state.yzFill * 100.0f );
            case VOXEL_SLIDER_TOP_COVERAGE:
                return (INT)( state.topCoverage * 100.0f );
        }

        return 0;
    }

    void WriteVoxelSliderValue( VoxelSliderKind kind, Gui_VoxelPanelState& state, INT rawValue, UINT nEvent )
    {
        const float value = rawValue * 0.01f;

        switch( kind )
        {
            case VOXEL_SLIDER_SURFACE_SNAP:
                state.surfaceSnap = value;
            break;
            case VOXEL_SLIDER_HEIGHT_WARP:
                state.heightWarp = value;
            break;
            case VOXEL_SLIDER_XY_FILL:
                state.xyFill = value;
            break;
            case VOXEL_SLIDER_YZ_FILL:
                state.yzFill = value;
            break;
            case VOXEL_SLIDER_TOP_COVERAGE:
                state.topCoverage = value;
            break;
        }

        if( kind != VOXEL_SLIDER_SURFACE_SNAP && nEvent == EVENT_SLIDER_VALUE_CHANGED_UP )
        {
            state.requestStaticRevoxelization = true;
        }
    }

    std::wstring FormatVoxelSliderCaption( VoxelSliderKind kind, INT rawValue )
    {
        const float value = rawValue * 0.01f;
        WCHAR text[128] = {};

        switch( kind )
        {
            case VOXEL_SLIDER_SURFACE_SNAP:
                swprintf_s( text, L"Voxel Snap: %.2f", value );
            break;
            case VOXEL_SLIDER_HEIGHT_WARP:
                swprintf_s( text, L"Height Warp: %.2f", value );
            break;
            case VOXEL_SLIDER_XY_FILL:
                swprintf_s( text, L"XY Fill: %.2f", value );
            break;
            case VOXEL_SLIDER_YZ_FILL:
                swprintf_s( text, L"YZ Fill: %.2f", value );
            break;
            case VOXEL_SLIDER_TOP_COVERAGE:
                swprintf_s( text, L"Top Coverage: %.2f", value );
            break;
        }

        return text;
    }

    void SyncVoxelSliderToRuntime( VoxelSliderKind kind, Gui_VoxelPanelState& state, GuiRuntimeContext& runtime )
    {
        if( !runtime.pCascadedShadow )
        {
            state.requestStaticRevoxelization = false;
            return;
        }

        switch( kind )
        {
            case VOXEL_SLIDER_SURFACE_SNAP:
                runtime.pCascadedShadow->m_fVoxelVisualizeSurfaceSnap = state.surfaceSnap;
            break;
            case VOXEL_SLIDER_HEIGHT_WARP:
                runtime.pCascadedShadow->m_fStaticVoxelHeightWarp = state.heightWarp;
            break;
            case VOXEL_SLIDER_XY_FILL:
                runtime.pCascadedShadow->m_fVoxelXYFootprintScale = state.xyFill;
            break;
            case VOXEL_SLIDER_YZ_FILL:
                runtime.pCascadedShadow->m_fVoxelYZFootprintScale = state.yzFill;
            break;
            case VOXEL_SLIDER_TOP_COVERAGE:
                runtime.pCascadedShadow->m_fStaticVoxelTopCoverage = state.topCoverage;
            break;
        }

        if( state.requestStaticRevoxelization )
        {
            runtime.pCascadedShadow->InvalidateStaticVoxelization();
            state.requestStaticRevoxelization = false;
        }
    }

    void SyncVoxelSliderFromRuntime( VoxelSliderKind kind, Gui_VoxelPanelState& state, const GuiRuntimeContext& runtime )
    {
        if( !runtime.pCascadedShadow )
        {
            return;
        }

        switch( kind )
        {
            case VOXEL_SLIDER_SURFACE_SNAP:
                state.surfaceSnap = runtime.pCascadedShadow->m_fVoxelVisualizeSurfaceSnap;
            break;
            case VOXEL_SLIDER_HEIGHT_WARP:
                state.heightWarp = runtime.pCascadedShadow->m_fStaticVoxelHeightWarp;
            break;
            case VOXEL_SLIDER_XY_FILL:
                state.xyFill = runtime.pCascadedShadow->m_fVoxelXYFootprintScale;
            break;
            case VOXEL_SLIDER_YZ_FILL:
                state.yzFill = runtime.pCascadedShadow->m_fVoxelYZFootprintScale;
            break;
            case VOXEL_SLIDER_TOP_COVERAGE:
                state.topCoverage = runtime.pCascadedShadow->m_fStaticVoxelTopCoverage;
            break;
        }
    }
}

Gui_VoxelPanel::Gui_VoxelPanel( const Gui_VoxelPanelIds& ids, Gui_VoxelPanelState& state )
    : m_ids( ids ),
      m_state( state )
{
}

void Gui_VoxelPanel::BuildControls( GuiControlFactory& factory )
{
    AddCheckBox(
        factory,
        m_ids.visualizeVoxelCheckId,
        L"Visualize Voxel",
        [this]() { return m_state.visualizeVoxel; },
        [this]( bool checked ) { m_state.visualizeVoxel = checked; },
        [this]( GuiRuntimeContext& runtime )
        {
            if( runtime.pVisualizeVoxel )
            {
                *runtime.pVisualizeVoxel = m_state.visualizeVoxel;
            }
        },
        [this]( const GuiRuntimeContext& runtime )
        {
            if( runtime.pVisualizeVoxel )
            {
                m_state.visualizeVoxel = *runtime.pVisualizeVoxel;
            }
        } );

    const auto addVoxelSlider =
        [this, &factory]( INT textId, INT sliderId, VoxelSliderKind kind )
    {
        AddSlider(
            factory,
            textId,
            sliderId,
            GetVoxelSliderMinValue( kind ),
            GetVoxelSliderMaxValue( kind ),
            [this, kind]() { return ReadVoxelSliderValue( kind, m_state ); },
            [this, kind]( INT rawValue, UINT nEvent ) { WriteVoxelSliderValue( kind, m_state, rawValue, nEvent ); },
            [kind]( INT rawValue ) { return FormatVoxelSliderCaption( kind, rawValue ); },
            [this, kind]( GuiRuntimeContext& runtime ) { SyncVoxelSliderToRuntime( kind, m_state, runtime ); },
            [this, kind]( const GuiRuntimeContext& runtime ) { SyncVoxelSliderFromRuntime( kind, m_state, runtime ); } );
    };

    addVoxelSlider( m_ids.surfaceSnapTextId, m_ids.surfaceSnapSliderId, VOXEL_SLIDER_SURFACE_SNAP );
    addVoxelSlider( m_ids.heightWarpTextId, m_ids.heightWarpSliderId, VOXEL_SLIDER_HEIGHT_WARP );
    addVoxelSlider( m_ids.xyFillTextId, m_ids.xyFillSliderId, VOXEL_SLIDER_XY_FILL );
    addVoxelSlider( m_ids.yzFillTextId, m_ids.yzFillSliderId, VOXEL_SLIDER_YZ_FILL );
    addVoxelSlider( m_ids.topCoverageTextId, m_ids.topCoverageSliderId, VOXEL_SLIDER_TOP_COVERAGE );
}
