#include "DXUT.h"
#include "GuiDebugPanel.h"
#include "CascadedShadowsManager.h"

Gui_RenderDebugStrategy::Gui_RenderDebugStrategy( Gui_DebugPanelState& state )
    : m_state( state )
{
}

const WCHAR* Gui_RenderDebugStrategy::GetLabelText() const
{
    return L"Render Debug";
}

UINT Gui_RenderDebugStrategy::GetHotkey() const
{
    return VK_F9;
}

bool Gui_RenderDebugStrategy::ReadValue() const
{
    return m_state.renderDebug;
}

void Gui_RenderDebugStrategy::WriteValue( bool checked )
{
    m_state.renderDebug = checked;
}

void Gui_RenderDebugStrategy::SyncToRuntime( GuiRuntimeContext& runtime )
{
    if( runtime.pCascadedShadow )
    {
        runtime.pCascadedShadow->SetRenderDebugEnabled( m_state.renderDebug );
    }
}

void Gui_RenderDebugStrategy::SyncFromRuntime( const GuiRuntimeContext& runtime )
{
    if( runtime.pCascadedShadow )
    {
        m_state.renderDebug = runtime.pCascadedShadow->IsRenderDebugEnabled();
    }
}

Gui_RenderDebugBoundingBoxStrategy::Gui_RenderDebugBoundingBoxStrategy( Gui_DebugPanelState& state )
    : m_state( state )
{
}

const WCHAR* Gui_RenderDebugBoundingBoxStrategy::GetLabelText() const
{
    return L"Debug Bounding Box";
}

bool Gui_RenderDebugBoundingBoxStrategy::ReadValue() const
{
    return m_state.renderDebugBoundingBox;
}

void Gui_RenderDebugBoundingBoxStrategy::WriteValue( bool checked )
{
    m_state.renderDebugBoundingBox = checked;
}

void Gui_RenderDebugBoundingBoxStrategy::SyncToRuntime( GuiRuntimeContext& runtime )
{
    if( runtime.pCascadedShadow )
    {
        runtime.pCascadedShadow->SetRenderDebugBoundingBoxEnabled( m_state.renderDebugBoundingBox );
    }
}

void Gui_RenderDebugBoundingBoxStrategy::SyncFromRuntime( const GuiRuntimeContext& runtime )
{
    if( runtime.pCascadedShadow )
    {
        m_state.renderDebugBoundingBox = runtime.pCascadedShadow->IsRenderDebugBoundingBoxEnabled();
    }
}

Gui_DebugPanel::Gui_DebugPanel( const Gui_DebugPanelIds& ids, Gui_DebugPanelState& state )
    : m_ids( ids ),
      m_state( state ),
      m_renderDebugStrategy( state ),
      m_renderDebugBoundingBoxStrategy( state ),
      m_renderDebugControl( ids.renderDebugId, m_renderDebugStrategy ),
      m_renderDebugBoundingBoxControl( ids.renderDebugBoundingBoxId, m_renderDebugBoundingBoxStrategy )
{
}

void Gui_DebugPanel::BuildControls( GuiControlFactory& factory )
{
    RegisterControl( m_renderDebugControl );
    RegisterControl( m_renderDebugBoundingBoxControl );
    factory.Add( m_renderDebugControl );
    factory.Add( m_renderDebugBoundingBoxControl );
}
