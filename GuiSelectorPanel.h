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
    INT sceneLabelId;
    INT sceneComboId;
};

struct Gui_SelectorPanelState
{
    GUI_PANEL_CATEGORY selectedCategory;
    SCENE_SELECTION selectedScene;
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
};
