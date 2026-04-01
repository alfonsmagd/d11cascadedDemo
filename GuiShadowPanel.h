#pragma once

#include "GuiControlBase.h"
#include <vector>

struct Gui_ShadowPanelIds
{
    INT visualizeCascadesCheckId;

    INT depthFormatLabelId;
    INT depthFormatComboId;
    INT selectedCameraLabelId;
    INT selectedCameraComboId;
    INT cascadeLevelsLabelId;
    INT cascadeLevelsComboId;
    INT fitToCascadeLabelId;
    INT fitToCascadeComboId;
    INT fitToNearFarLabelId;
    INT fitToNearFarComboId;
    INT cascadeSelectionLabelId;
    INT cascadeSelectionComboId;

    INT bufferSizeTextId;
    INT bufferSizeSliderId;
    INT pcfSizeTextId;
    INT pcfSizeSliderId;
    INT pcfOffsetTextId;
    INT pcfOffsetSliderId;
    INT blendEnabledCheckId;
    INT blendAmountTextId;
    INT blendAmountSliderId;
    INT derivativeOffsetCheckId;
    INT fitLightToTexelsCheckId;

    INT cascadePartitionTextIds[MAX_CASCADES];
    INT cascadePartitionSliderIds[MAX_CASCADES];
};

struct Gui_ShadowPanelState
{
    bool visualizeCascades;
    SHADOW_TEXTURE_FORMAT depthBufferFormat;
    INT selectedCamera;
    INT cascadeLevels;
    FIT_PROJECTION_TO_CASCADES fitToCascades;
    FIT_TO_NEAR_FAR fitToNearFar;
    CASCADE_SELECTION cascadeSelection;

    INT shadowBufferSize;
    INT pcfBlurSize;
    float pcfOffset;
    bool blendBetweenMaps;
    float blendAmount;
    bool derivativeOffset;
    bool fitLightToTexels;

    INT cascadePartitions[MAX_CASCADES];
};

class Gui_ShadowPanel : public GuiPanelBase
{
public:
    Gui_ShadowPanel( const Gui_ShadowPanelIds& ids, Gui_ShadowPanelState& state );

protected:
    virtual void BuildControls( GuiControlFactory& factory );
    virtual void OnUpdate();

private:
    Gui_ShadowPanelIds m_ids;
    Gui_ShadowPanelState& m_state;
    Gui_SliderControl* m_pBlendAmountControl;
    std::vector<Gui_SliderControl*> m_cascadeControls;
};
