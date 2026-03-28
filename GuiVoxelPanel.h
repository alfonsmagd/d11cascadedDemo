#pragma once

#include "GuiControlBase.h"

enum GUI_VOXEL_TOGGLE_KIND
{
    GUI_VOXEL_TOGGLE_VISUALIZE_VOXEL
};

enum GUI_VOXEL_SLIDER_KIND
{
    GUI_VOXEL_SLIDER_SURFACE_SNAP,
    GUI_VOXEL_SLIDER_HEIGHT_WARP,
    GUI_VOXEL_SLIDER_XY_FILL,
    GUI_VOXEL_SLIDER_YZ_FILL,
    GUI_VOXEL_SLIDER_TOP_COVERAGE
};

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

class Gui_VoxelToggleStrategy : public Gui_CheckBoxStrategy
{
public:
    Gui_VoxelToggleStrategy( GUI_VOXEL_TOGGLE_KIND kind, Gui_VoxelPanelState& state );

    virtual const WCHAR* GetLabelText() const;
    virtual bool ReadValue() const;
    virtual void WriteValue( bool checked );
    virtual void SyncToRuntime( GuiRuntimeContext& runtime );
    virtual void SyncFromRuntime( const GuiRuntimeContext& runtime );

private:
    GUI_VOXEL_TOGGLE_KIND m_kind;
    Gui_VoxelPanelState& m_state;
};

class Gui_VoxelSliderStrategy : public Gui_SliderStrategy
{
public:
    Gui_VoxelSliderStrategy( GUI_VOXEL_SLIDER_KIND kind, Gui_VoxelPanelState& state );

    virtual INT GetMinValue() const;
    virtual INT GetMaxValue() const;
    virtual INT ReadValue() const;
    virtual void WriteValue( INT rawValue, UINT nEvent );
    virtual void FormatCaption( INT rawValue, WCHAR* text, size_t textCount ) const;
    virtual void SyncToRuntime( GuiRuntimeContext& runtime );
    virtual void SyncFromRuntime( const GuiRuntimeContext& runtime );

private:
    GUI_VOXEL_SLIDER_KIND m_kind;
    Gui_VoxelPanelState& m_state;
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
    Gui_VoxelSliderStrategy m_surfaceSnapStrategy;
    Gui_VoxelSliderStrategy m_heightWarpStrategy;
    Gui_VoxelSliderStrategy m_xyFillStrategy;
    Gui_VoxelSliderStrategy m_yzFillStrategy;
    Gui_VoxelSliderStrategy m_topCoverageStrategy;
    Gui_VoxelToggleStrategy m_visualizeVoxelStrategy;
    Gui_CheckBoxControl m_visualizeVoxelControl;
    Gui_SliderControl m_surfaceSnapControl;
    Gui_SliderControl m_heightWarpControl;
    Gui_SliderControl m_xyFillControl;
    Gui_SliderControl m_yzFillControl;
    Gui_SliderControl m_topCoverageControl;
};
