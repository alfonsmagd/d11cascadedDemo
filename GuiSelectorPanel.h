#pragma once

#include "GuiControlBase.h"

enum GUI_PANEL_CATEGORY
{
    GUI_PANEL_CATEGORY_DEBUG = 0,
    GUI_PANEL_CATEGORY_VOXELIZATION,
    GUI_PANEL_CATEGORY_CASCADES
};

struct Gui_SelectorPanelIds
{
    INT categoryLabelId;
    INT categoryComboId;
};

struct Gui_SelectorPanelState
{
    GUI_PANEL_CATEGORY selectedCategory;
};

class Gui_CategoryComboStrategy : public Gui_ComboBoxStrategy
{
public:
    Gui_CategoryComboStrategy( Gui_SelectorPanelState& state );

    virtual const WCHAR* GetLabelText() const;
    virtual void Populate( CDXUTComboBox& comboBox ) const;
    virtual void RefreshSelection( CDXUTComboBox& comboBox ) const;
    virtual void WriteSelection( CDXUTComboBox& comboBox );
    virtual void SyncToRuntime( GuiRuntimeContext& runtime );
    virtual void SyncFromRuntime( const GuiRuntimeContext& runtime );

private:
    Gui_SelectorPanelState& m_state;
};

class Gui_SelectorPanel : public GuiPanelBase
{
public:
    Gui_SelectorPanel( const Gui_SelectorPanelIds& ids, Gui_SelectorPanelState& state );

protected:
    virtual void BuildControls( GuiControlFactory& factory );

private:
    Gui_SelectorPanelIds m_ids;
    Gui_SelectorPanelState& m_state;
    Gui_CategoryComboStrategy m_categoryStrategy;
    Gui_ComboBoxControl m_categoryControl;
};
