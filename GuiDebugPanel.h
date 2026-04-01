#pragma once

#include "GuiControlBase.h"

struct Gui_DebugPanelIds
{
    INT renderDebugId;
    INT renderDebugBoundingBoxId;
    INT renderAllBoundingBoxesId;
};

struct Gui_DebugPanelState
{
    bool renderDebug;
    bool renderDebugBoundingBox;
    bool renderAllBoundingBoxes;
};

class Gui_DebugPanel : public GuiPanelBase
{
public:
    Gui_DebugPanel( const Gui_DebugPanelIds& ids, Gui_DebugPanelState& state );

protected:
    virtual void BuildControls( GuiControlFactory& factory );

private:
    Gui_DebugPanelIds m_ids;
    Gui_DebugPanelState& m_state;
};
