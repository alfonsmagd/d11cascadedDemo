#pragma once

#include "GuiControlBase.h"
#include <memory>
#include <vector>

enum GUI_SHADOW_SLIDER_KIND
{
    GUI_SHADOW_SLIDER_BUFFER_SIZE,
    GUI_SHADOW_SLIDER_PCF_SIZE,
    GUI_SHADOW_SLIDER_PCF_OFFSET,
    GUI_SHADOW_SLIDER_BLEND_AMOUNT
};

enum GUI_SHADOW_TOGGLE_KIND
{
    GUI_SHADOW_TOGGLE_VISUALIZE_CASCADES,
    GUI_SHADOW_TOGGLE_BLEND_ENABLED,
    GUI_SHADOW_TOGGLE_DERIVATIVE_OFFSET,
    GUI_SHADOW_TOGGLE_FIT_LIGHT_TO_TEXELS
};

enum GUI_SHADOW_COMBO_KIND
{
    GUI_SHADOW_COMBO_DEPTH_FORMAT,
    GUI_SHADOW_COMBO_SELECTED_CAMERA,
    GUI_SHADOW_COMBO_CASCADE_LEVELS,
    GUI_SHADOW_COMBO_FIT_TO_CASCADE,
    GUI_SHADOW_COMBO_FIT_TO_NEARFAR,
    GUI_SHADOW_COMBO_CASCADE_SELECTION
};

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

class Gui_ShadowSliderStrategy : public Gui_SliderStrategy
{
public:
    Gui_ShadowSliderStrategy( GUI_SHADOW_SLIDER_KIND kind, Gui_ShadowPanelState& state );

    virtual INT GetMinValue() const;
    virtual INT GetMaxValue() const;
    virtual INT ReadValue() const;
    virtual void WriteValue( INT rawValue, UINT nEvent );
    virtual void FormatCaption( INT rawValue, WCHAR* text, size_t textCount ) const;
    virtual void SyncToRuntime( GuiRuntimeContext& runtime );
    virtual void SyncFromRuntime( const GuiRuntimeContext& runtime );

private:
    GUI_SHADOW_SLIDER_KIND m_kind;
    Gui_ShadowPanelState& m_state;
};

class Gui_ShadowToggleStrategy : public Gui_CheckBoxStrategy
{
public:
    Gui_ShadowToggleStrategy( GUI_SHADOW_TOGGLE_KIND kind, Gui_ShadowPanelState& state );

    virtual const WCHAR* GetLabelText() const;
    virtual bool ReadValue() const;
    virtual void WriteValue( bool checked );
    virtual void SyncToRuntime( GuiRuntimeContext& runtime );
    virtual void SyncFromRuntime( const GuiRuntimeContext& runtime );

private:
    GUI_SHADOW_TOGGLE_KIND m_kind;
    Gui_ShadowPanelState& m_state;
};

class Gui_ShadowComboStrategy : public Gui_ComboBoxStrategy
{
public:
    Gui_ShadowComboStrategy( GUI_SHADOW_COMBO_KIND kind, Gui_ShadowPanelState& state );

    virtual const WCHAR* GetLabelText() const;
    virtual void Populate( CDXUTComboBox& comboBox ) const;
    virtual void RefreshSelection( CDXUTComboBox& comboBox ) const;
    virtual void WriteSelection( CDXUTComboBox& comboBox );
    virtual void SyncToRuntime( GuiRuntimeContext& runtime );
    virtual void SyncFromRuntime( const GuiRuntimeContext& runtime );

private:
    GUI_SHADOW_COMBO_KIND m_kind;
    Gui_ShadowPanelState& m_state;
};

class Gui_ShadowCascadeSliderStrategy : public Gui_SliderStrategy
{
public:
    Gui_ShadowCascadeSliderStrategy( INT cascadeIndex, Gui_ShadowPanelState& state );

    virtual INT GetMinValue() const;
    virtual INT GetMaxValue() const;
    virtual INT ReadValue() const;
    virtual void WriteValue( INT rawValue, UINT nEvent );
    virtual void FormatCaption( INT rawValue, WCHAR* text, size_t textCount ) const;
    virtual void SyncToRuntime( GuiRuntimeContext& runtime );
    virtual void SyncFromRuntime( const GuiRuntimeContext& runtime );

private:
    INT m_cascadeIndex;
    Gui_ShadowPanelState& m_state;
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

    Gui_ShadowComboStrategy m_depthFormatStrategy;
    Gui_ShadowComboStrategy m_selectedCameraStrategy;
    Gui_ShadowComboStrategy m_cascadeLevelsStrategy;
    Gui_ShadowComboStrategy m_fitToCascadesStrategy;
    Gui_ShadowComboStrategy m_fitToNearFarStrategy;
    Gui_ShadowComboStrategy m_cascadeSelectionStrategy;
    Gui_ShadowSliderStrategy m_bufferSizeStrategy;
    Gui_ShadowSliderStrategy m_pcfSizeStrategy;
    Gui_ShadowSliderStrategy m_pcfOffsetStrategy;
    Gui_ShadowSliderStrategy m_blendAmountStrategy;
    Gui_ShadowToggleStrategy m_visualizeCascadesStrategy;
    Gui_ShadowToggleStrategy m_blendEnabledStrategy;
    Gui_ShadowToggleStrategy m_derivativeOffsetStrategy;
    Gui_ShadowToggleStrategy m_fitLightToTexelsStrategy;

    Gui_ComboBoxControl m_depthFormatControl;
    Gui_ComboBoxControl m_selectedCameraControl;
    Gui_ComboBoxControl m_cascadeLevelsControl;
    Gui_ComboBoxControl m_fitToCascadesControl;
    Gui_ComboBoxControl m_fitToNearFarControl;
    Gui_ComboBoxControl m_cascadeSelectionControl;
    Gui_CheckBoxControl m_visualizeCascadesControl;
    Gui_SliderControl m_bufferSizeControl;
    Gui_SliderControl m_pcfSizeControl;
    Gui_SliderControl m_pcfOffsetControl;
    Gui_CheckBoxControl m_blendEnabledControl;
    Gui_SliderControl m_blendAmountControl;
    Gui_CheckBoxControl m_derivativeOffsetControl;
    Gui_CheckBoxControl m_fitLightToTexelsControl;

    std::vector<std::unique_ptr<Gui_ShadowCascadeSliderStrategy> > m_cascadeStrategies;
    std::vector<std::unique_ptr<Gui_SliderControl> > m_cascadeControls;
};
