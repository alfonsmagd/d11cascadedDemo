#include "DXUT.h"
#include "GuiSelectorPanel.h"

Gui_CategoryComboStrategy::Gui_CategoryComboStrategy( Gui_SelectorPanelState& state )
    : m_state( state )
{
}

const WCHAR* Gui_CategoryComboStrategy::GetLabelText() const
{
    return L"Panel";
}

void Gui_CategoryComboStrategy::Populate( CDXUTComboBox& comboBox ) const
{
    comboBox.RemoveAllItems();
    comboBox.AddItem( L"Debug", ULongToPtr( GUI_PANEL_CATEGORY_DEBUG ) );
    comboBox.AddItem( L"Voxelization", ULongToPtr( GUI_PANEL_CATEGORY_VOXELIZATION ) );
    comboBox.AddItem( L"Cascades", ULongToPtr( GUI_PANEL_CATEGORY_CASCADES ) );
}

void Gui_CategoryComboStrategy::RefreshSelection( CDXUTComboBox& comboBox ) const
{
    comboBox.SetSelectedByData( ULongToPtr( m_state.selectedCategory ) );
}

void Gui_CategoryComboStrategy::WriteSelection( CDXUTComboBox& comboBox )
{
    m_state.selectedCategory = ( GUI_PANEL_CATEGORY )PtrToUlong( comboBox.GetSelectedData() );
}

void Gui_CategoryComboStrategy::SyncToRuntime( GuiRuntimeContext& )
{
}

void Gui_CategoryComboStrategy::SyncFromRuntime( const GuiRuntimeContext& )
{
}

Gui_SceneComboStrategy::Gui_SceneComboStrategy( Gui_SelectorPanelState& state )
    : m_state( state )
{
}

const WCHAR* Gui_SceneComboStrategy::GetLabelText() const
{
    return L"Map";
}

void Gui_SceneComboStrategy::Populate( CDXUTComboBox& comboBox ) const
{
    comboBox.RemoveAllItems();
    comboBox.AddItem( L"Power Plant", ULongToPtr( POWER_PLANT_SCENE ) );
    comboBox.AddItem( L"Test Scene", ULongToPtr( TEST_SCENE ) );
    comboBox.AddItem( L"Sponza", ULongToPtr( SPONZA_SCENE ) );
}

void Gui_SceneComboStrategy::RefreshSelection( CDXUTComboBox& comboBox ) const
{
    comboBox.SetSelectedByData( ULongToPtr( m_state.selectedScene ) );
}

void Gui_SceneComboStrategy::WriteSelection( CDXUTComboBox& comboBox )
{
    m_state.selectedScene = ( SCENE_SELECTION )PtrToUlong( comboBox.GetSelectedData() );
}

void Gui_SceneComboStrategy::SyncToRuntime( GuiRuntimeContext& runtime )
{
    if( runtime.pSelectedScene )
    {
        *runtime.pSelectedScene = m_state.selectedScene;
    }
}

void Gui_SceneComboStrategy::SyncFromRuntime( const GuiRuntimeContext& runtime )
{
    if( runtime.pSelectedScene )
    {
        m_state.selectedScene = *runtime.pSelectedScene;
    }
}

Gui_SelectorPanel::Gui_SelectorPanel( const Gui_SelectorPanelIds& ids, Gui_SelectorPanelState& state )
    : m_ids( ids ),
      m_state( state ),
      m_categoryStrategy( state ),
      m_sceneStrategy( state ),
      m_categoryControl( ids.categoryLabelId, ids.categoryComboId, m_categoryStrategy ),
      m_sceneControl( ids.sceneLabelId, ids.sceneComboId, m_sceneStrategy )
{
}

void Gui_SelectorPanel::BuildControls( GuiControlFactory& factory )
{
    RegisterControl( m_categoryControl );
    RegisterControl( m_sceneControl );
    factory.Add( m_categoryControl );
    factory.Add( m_sceneControl );
}
