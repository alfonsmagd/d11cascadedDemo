#pragma once

#include "GuiControlBase.h"

struct Gui_VoxelPanelIds
{
    INT visualizeVoxelCheckId;
    INT surfaceSnapTextId;
    INT surfaceSnapSliderId;
    INT heightWarpTextId;
    INT heightWarpSliderId;
    INT xyFillTextId;
    INT xyFillSliderId;
    INT yzFillTextId;
    INT yzFillSliderId;
    INT topCoverageTextId;
    INT topCoverageSliderId;
};

struct Gui_VoxelPanelState
{
    bool visualizeVoxel;
    float surfaceSnap;
    float heightWarp;
    float xyFill;
    float yzFill;
    float topCoverage;
    bool requestStaticRevoxelization;
};

class Gui_VoxelPanel : public GuiPanelBase
{
public:
    Gui_VoxelPanel( const Gui_VoxelPanelIds& ids, Gui_VoxelPanelState& state );

protected:
    virtual void BuildControls( GuiControlFactory& factory );

private:
    Gui_VoxelPanelIds m_ids;
    Gui_VoxelPanelState& m_state;
};
