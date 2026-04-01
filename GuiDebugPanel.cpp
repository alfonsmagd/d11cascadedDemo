#include "DXUT.h"
#include "GuiDebugPanel.h"
#include "CascadedShadowsManager.h"

Gui_DebugPanel::Gui_DebugPanel( const Gui_DebugPanelIds& ids, Gui_DebugPanelState& state )
    : m_ids( ids ),
      m_state( state )
{
}

void Gui_DebugPanel::BuildControls( GuiControlFactory& factory )
{
    AddCheckBox(
        factory,
        m_ids.renderDebugId,
        L"Render Debug",
        [this]() { return m_state.renderDebug; },
        [this]( bool checked ) { m_state.renderDebug = checked; },
        [this]( GuiRuntimeContext& runtime )
        {
            if( runtime.pCascadedShadow )
            {
                runtime.pCascadedShadow->SetRenderDebugEnabled( m_state.renderDebug );
            }
        },
        [this]( const GuiRuntimeContext& runtime )
        {
            if( runtime.pCascadedShadow )
            {
                m_state.renderDebug = runtime.pCascadedShadow->IsRenderDebugEnabled();
            }
        },
        VK_F9 );

    AddCheckBox(
        factory,
        m_ids.renderDebugBoundingBoxId,
        L"Debug Bounding Box",
        [this]() { return m_state.renderDebugBoundingBox; },
        [this]( bool checked ) { m_state.renderDebugBoundingBox = checked; },
        [this]( GuiRuntimeContext& runtime )
        {
            if( runtime.pCascadedShadow )
            {
                runtime.pCascadedShadow->SetRenderDebugBoundingBoxEnabled( m_state.renderDebugBoundingBox );
            }
        },
        [this]( const GuiRuntimeContext& runtime )
        {
            if( runtime.pCascadedShadow )
            {
                m_state.renderDebugBoundingBox = runtime.pCascadedShadow->IsRenderDebugBoundingBoxEnabled();
            }
        } );

    AddCheckBox(
        factory,
        m_ids.renderAllBoundingBoxesId,
        L"Render All Bounding Boxes",
        [this]() { return m_state.renderAllBoundingBoxes; },
        [this]( bool checked ) { m_state.renderAllBoundingBoxes = checked; },
        [this]( GuiRuntimeContext& runtime )
        {
            if( runtime.pCascadedShadow )
            {
                runtime.pCascadedShadow->SetRenderDebugAllBoundingBoxesEnabled( m_state.renderAllBoundingBoxes );
            }
        },
        [this]( const GuiRuntimeContext& runtime )
        {
            if( runtime.pCascadedShadow )
            {
                m_state.renderAllBoundingBoxes = runtime.pCascadedShadow->IsRenderDebugAllBoundingBoxesEnabled();
            }
        } );
}
