#include "DXUT.h"
#include "GuiSelectorPanel.h"

Gui_SelectorPanel::Gui_SelectorPanel( const Gui_SelectorPanelIds& ids, Gui_SelectorPanelState& state )
    : m_ids( ids ),
      m_state( state )
{
}

void Gui_SelectorPanel::BuildControls( GuiControlFactory& factory )
{
    AddComboBox(
        factory,
        m_ids.categoryLabelId,
        m_ids.categoryComboId,
        L"Panel",
        []( CDXUTComboBox& comboBox )
        {
            comboBox.RemoveAllItems();
            comboBox.AddItem( L"Debug", ULongToPtr( GUI_PANEL_CATEGORY_DEBUG ) );
            comboBox.AddItem( L"Voxelization", ULongToPtr( GUI_PANEL_CATEGORY_VOXELIZATION ) );
            comboBox.AddItem( L"Cascades", ULongToPtr( GUI_PANEL_CATEGORY_CASCADES ) );
        },
        [this]( CDXUTComboBox& comboBox )
        {
            comboBox.SetSelectedByData( ULongToPtr( m_state.selectedCategory ) );
        },
        [this]( CDXUTComboBox& comboBox )
        {
            m_state.selectedCategory = (GUI_PANEL_CATEGORY)PtrToUlong( comboBox.GetSelectedData() );
        },
        []( GuiRuntimeContext& )
        {
        },
        []( const GuiRuntimeContext& )
        {
        } );

    AddComboBox(
        factory,
        m_ids.sceneLabelId,
        m_ids.sceneComboId,
        L"Map",
        []( CDXUTComboBox& comboBox )
        {
            comboBox.RemoveAllItems();
            comboBox.AddItem( L"Power Plant", ULongToPtr( POWER_PLANT_SCENE ) );
            comboBox.AddItem( L"Test Scene", ULongToPtr( TEST_SCENE ) );
            comboBox.AddItem( L"Sponza", ULongToPtr( SPONZA_SCENE ) );
            comboBox.AddItem( L"Simple Scene", ULongToPtr( SIMPLE_SCENE ) );
        },
        [this]( CDXUTComboBox& comboBox )
        {
            comboBox.SetSelectedByData( ULongToPtr( m_state.selectedScene ) );
        },
        [this]( CDXUTComboBox& comboBox )
        {
            m_state.selectedScene = (SCENE_SELECTION)PtrToUlong( comboBox.GetSelectedData() );
        },
        [this]( GuiRuntimeContext& runtime )
        {
            if( runtime.pSelectedScene )
            {
                *runtime.pSelectedScene = m_state.selectedScene;
            }
        },
        [this]( const GuiRuntimeContext& runtime )
        {
            if( runtime.pSelectedScene )
            {
                m_state.selectedScene = *runtime.pSelectedScene;
            }
        } );
}
