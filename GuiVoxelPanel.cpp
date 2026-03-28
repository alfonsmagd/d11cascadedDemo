#include "DXUT.h"
#include "GuiVoxelPanel.h"
#include "CascadedShadowsManager.h"
#include <wchar.h>

Gui_VoxelSliderStrategy::Gui_VoxelSliderStrategy( GUI_VOXEL_SLIDER_KIND kind, Gui_VoxelPanelState& state )
    : m_kind( kind ),
      m_state( state )
{
}

INT Gui_VoxelSliderStrategy::GetMinValue() const
{
    switch( m_kind )
    {
        case GUI_VOXEL_SLIDER_SURFACE_SNAP:
            return 0;
        case GUI_VOXEL_SLIDER_HEIGHT_WARP:
            return 20;
        case GUI_VOXEL_SLIDER_XY_FILL:
        case GUI_VOXEL_SLIDER_YZ_FILL:
            return 50;
        case GUI_VOXEL_SLIDER_TOP_COVERAGE:
            return 50;
    }

    return 0;
}

INT Gui_VoxelSliderStrategy::GetMaxValue() const
{
    switch( m_kind )
    {
        case GUI_VOXEL_SLIDER_SURFACE_SNAP:
            return 100;
        case GUI_VOXEL_SLIDER_HEIGHT_WARP:
            return 250;
        case GUI_VOXEL_SLIDER_XY_FILL:
        case GUI_VOXEL_SLIDER_YZ_FILL:
            return 400;
        case GUI_VOXEL_SLIDER_TOP_COVERAGE:
            return 100;
    }

    return 100;
}

INT Gui_VoxelSliderStrategy::ReadValue() const
{
    switch( m_kind )
    {
        case GUI_VOXEL_SLIDER_SURFACE_SNAP:
            return ( INT )( m_state.surfaceSnap * 100.0f );
        case GUI_VOXEL_SLIDER_HEIGHT_WARP:
            return ( INT )( m_state.heightWarp * 100.0f );
        case GUI_VOXEL_SLIDER_XY_FILL:
            return ( INT )( m_state.xyFill * 100.0f );
        case GUI_VOXEL_SLIDER_YZ_FILL:
            return ( INT )( m_state.yzFill * 100.0f );
        case GUI_VOXEL_SLIDER_TOP_COVERAGE:
            return ( INT )( m_state.topCoverage * 100.0f );
    }

    return 0;
}

void Gui_VoxelSliderStrategy::WriteValue( INT rawValue, UINT nEvent )
{
    const float value = rawValue * 0.01f;

    switch( m_kind )
    {
        case GUI_VOXEL_SLIDER_SURFACE_SNAP:
            m_state.surfaceSnap = value;
        break;
        case GUI_VOXEL_SLIDER_HEIGHT_WARP:
            m_state.heightWarp = value;
        break;
        case GUI_VOXEL_SLIDER_XY_FILL:
            m_state.xyFill = value;
        break;
        case GUI_VOXEL_SLIDER_YZ_FILL:
            m_state.yzFill = value;
        break;
        case GUI_VOXEL_SLIDER_TOP_COVERAGE:
            m_state.topCoverage = value;
        break;
    }

    if( m_kind != GUI_VOXEL_SLIDER_SURFACE_SNAP && nEvent == EVENT_SLIDER_VALUE_CHANGED_UP )
    {
        m_state.requestStaticRevoxelization = true;
    }
}

void Gui_VoxelSliderStrategy::FormatCaption( INT rawValue, WCHAR* text, size_t textCount ) const
{
    const float value = rawValue * 0.01f;

    switch( m_kind )
    {
        case GUI_VOXEL_SLIDER_SURFACE_SNAP:
            swprintf_s( text, textCount, L"Voxel Snap: %.2f", value );
        break;
        case GUI_VOXEL_SLIDER_HEIGHT_WARP:
            swprintf_s( text, textCount, L"Height Warp: %.2f", value );
        break;
        case GUI_VOXEL_SLIDER_XY_FILL:
            swprintf_s( text, textCount, L"XY Fill: %.2f", value );
        break;
        case GUI_VOXEL_SLIDER_YZ_FILL:
            swprintf_s( text, textCount, L"YZ Fill: %.2f", value );
        break;
        case GUI_VOXEL_SLIDER_TOP_COVERAGE:
            swprintf_s( text, textCount, L"Top Coverage: %.2f", value );
        break;
    }
}

void Gui_VoxelSliderStrategy::SyncToRuntime( GuiRuntimeContext& runtime )
{
    if( !runtime.pCascadedShadow )
    {
        m_state.requestStaticRevoxelization = false;
        return;
    }

    switch( m_kind )
    {
        case GUI_VOXEL_SLIDER_SURFACE_SNAP:
            runtime.pCascadedShadow->m_fVoxelVisualizeSurfaceSnap = m_state.surfaceSnap;
        break;

        case GUI_VOXEL_SLIDER_HEIGHT_WARP:
            runtime.pCascadedShadow->m_fStaticVoxelHeightWarp = m_state.heightWarp;
        break;

        case GUI_VOXEL_SLIDER_XY_FILL:
            runtime.pCascadedShadow->m_fVoxelXYFootprintScale = m_state.xyFill;
        break;

        case GUI_VOXEL_SLIDER_YZ_FILL:
            runtime.pCascadedShadow->m_fVoxelYZFootprintScale = m_state.yzFill;
        break;

        case GUI_VOXEL_SLIDER_TOP_COVERAGE:
            runtime.pCascadedShadow->m_fStaticVoxelTopCoverage = m_state.topCoverage;
        break;
    }

    if( m_state.requestStaticRevoxelization )
    {
        runtime.pCascadedShadow->InvalidateStaticVoxelization();
        m_state.requestStaticRevoxelization = false;
    }
}

void Gui_VoxelSliderStrategy::SyncFromRuntime( const GuiRuntimeContext& runtime )
{
    if( !runtime.pCascadedShadow )
    {
        return;
    }

    switch( m_kind )
    {
        case GUI_VOXEL_SLIDER_SURFACE_SNAP:
            m_state.surfaceSnap = runtime.pCascadedShadow->m_fVoxelVisualizeSurfaceSnap;
        break;

        case GUI_VOXEL_SLIDER_HEIGHT_WARP:
            m_state.heightWarp = runtime.pCascadedShadow->m_fStaticVoxelHeightWarp;
        break;

        case GUI_VOXEL_SLIDER_XY_FILL:
            m_state.xyFill = runtime.pCascadedShadow->m_fVoxelXYFootprintScale;
        break;

        case GUI_VOXEL_SLIDER_YZ_FILL:
            m_state.yzFill = runtime.pCascadedShadow->m_fVoxelYZFootprintScale;
        break;

        case GUI_VOXEL_SLIDER_TOP_COVERAGE:
            m_state.topCoverage = runtime.pCascadedShadow->m_fStaticVoxelTopCoverage;
        break;
    }
}

Gui_VoxelToggleStrategy::Gui_VoxelToggleStrategy( GUI_VOXEL_TOGGLE_KIND kind, Gui_VoxelPanelState& state )
    : m_kind( kind ),
      m_state( state )
{
}

const WCHAR* Gui_VoxelToggleStrategy::GetLabelText() const
{
    switch( m_kind )
    {
        case GUI_VOXEL_TOGGLE_VISUALIZE_VOXEL:
            return L"Visualize Voxel";
    }

    return L"Unnamed Voxel Toggle";
}

bool Gui_VoxelToggleStrategy::ReadValue() const
{
    switch( m_kind )
    {
        case GUI_VOXEL_TOGGLE_VISUALIZE_VOXEL:
            return m_state.visualizeVoxel;
    }

    return false;
}

void Gui_VoxelToggleStrategy::WriteValue( bool checked )
{
    switch( m_kind )
    {
        case GUI_VOXEL_TOGGLE_VISUALIZE_VOXEL:
            m_state.visualizeVoxel = checked;
        break;
    }
}

void Gui_VoxelToggleStrategy::SyncToRuntime( GuiRuntimeContext& runtime )
{
    if( !runtime.pVisualizeVoxel )
    {
        return;
    }

    switch( m_kind )
    {
        case GUI_VOXEL_TOGGLE_VISUALIZE_VOXEL:
            *runtime.pVisualizeVoxel = m_state.visualizeVoxel;
        break;
    }
}

void Gui_VoxelToggleStrategy::SyncFromRuntime( const GuiRuntimeContext& runtime )
{
    if( !runtime.pVisualizeVoxel )
    {
        return;
    }

    switch( m_kind )
    {
        case GUI_VOXEL_TOGGLE_VISUALIZE_VOXEL:
            m_state.visualizeVoxel = *runtime.pVisualizeVoxel;
        break;
    }
}

Gui_VoxelPanel::Gui_VoxelPanel( const Gui_VoxelPanelIds& ids, Gui_VoxelPanelState& state )
    : m_ids( ids ),
      m_state( state ),
      m_surfaceSnapStrategy( GUI_VOXEL_SLIDER_SURFACE_SNAP, state ),
      m_heightWarpStrategy( GUI_VOXEL_SLIDER_HEIGHT_WARP, state ),
      m_xyFillStrategy( GUI_VOXEL_SLIDER_XY_FILL, state ),
      m_yzFillStrategy( GUI_VOXEL_SLIDER_YZ_FILL, state ),
      m_topCoverageStrategy( GUI_VOXEL_SLIDER_TOP_COVERAGE, state ),
      m_visualizeVoxelStrategy( GUI_VOXEL_TOGGLE_VISUALIZE_VOXEL, state ),
      m_visualizeVoxelControl( ids.visualizeVoxelCheckId, m_visualizeVoxelStrategy ),
      m_surfaceSnapControl( ids.surfaceSnapTextId, ids.surfaceSnapSliderId, m_surfaceSnapStrategy ),
      m_heightWarpControl( ids.heightWarpTextId, ids.heightWarpSliderId, m_heightWarpStrategy ),
      m_xyFillControl( ids.xyFillTextId, ids.xyFillSliderId, m_xyFillStrategy ),
      m_yzFillControl( ids.yzFillTextId, ids.yzFillSliderId, m_yzFillStrategy ),
      m_topCoverageControl( ids.topCoverageTextId, ids.topCoverageSliderId, m_topCoverageStrategy )
{
}

void Gui_VoxelPanel::BuildControls( GuiControlFactory& factory )
{
    RegisterControl( m_visualizeVoxelControl );
    RegisterControl( m_surfaceSnapControl );
    RegisterControl( m_heightWarpControl );
    RegisterControl( m_xyFillControl );
    RegisterControl( m_yzFillControl );
    RegisterControl( m_topCoverageControl );

    factory.Add( m_visualizeVoxelControl );
    factory.Add( m_surfaceSnapControl );
    factory.Add( m_heightWarpControl );
    factory.Add( m_xyFillControl );
    factory.Add( m_yzFillControl );
    factory.Add( m_topCoverageControl );
}
