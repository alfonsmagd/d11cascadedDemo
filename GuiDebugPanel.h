#pragma once

#include "GuiControlBase.h"

struct Gui_DebugPanelIds
{
    INT renderDebugId;
};

struct Gui_DebugPanelState
{
    bool renderDebug;
};

class Gui_RenderDebugStrategy : public Gui_CheckBoxStrategy
{
public:
    Gui_RenderDebugStrategy( Gui_DebugPanelState& state );

    virtual const WCHAR* GetLabelText() const;
    virtual UINT GetHotkey() const;
    virtual bool ReadValue() const;
    virtual void WriteValue( bool checked );
    virtual void SyncToRuntime( GuiRuntimeContext& runtime );
    virtual void SyncFromRuntime( const GuiRuntimeContext& runtime );

private:
    Gui_DebugPanelState& m_state;
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
    Gui_RenderDebugStrategy m_renderDebugStrategy;
    Gui_CheckBoxControl m_renderDebugControl;
};
