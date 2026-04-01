// Base GUI abstraction layer shared by the selector/debug/voxel/shadow panels.
#pragma once

#include "DXUT.h"
#include "DXUTgui.h"
#include "GuiRuntimeContext.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <stddef.h>

struct GuiPanelLayout
{
    GuiPanelLayout();

    INT x;
    INT y;
    INT width;
    INT labelHeight;
    INT sliderHeight;
    INT checkBoxHeight;
    INT sliderSpacing;
    INT checkBoxSpacing;
};

class Gui_ControlBase
{
public:
    Gui_ControlBase();
    virtual ~Gui_ControlBase();

    virtual void Create( CDXUTDialog* pDialog, const GuiPanelLayout& layout, INT& y ) = 0;
    virtual bool Handles( INT controlId ) const = 0;
    virtual void HandleEvent( UINT nEvent ) = 0;
    virtual void Refresh() = 0;
    virtual void SetVisible( bool visible ) = 0;
    virtual void SetEnabled( bool enabled ) = 0;
    void ApplyPendingChanges( GuiRuntimeContext& runtime );
    void UpdateFromRuntime( const GuiRuntimeContext& runtime );
    bool HasPendingChanges() const;

protected:
    void MarkDirty();
    virtual void SyncToRuntime( GuiRuntimeContext& runtime ) = 0;
    virtual void SyncFromRuntime( const GuiRuntimeContext& runtime ) = 0;

private:
    bool m_hasPendingChanges;
};

class Gui_SliderStrategy
{
public:
    virtual ~Gui_SliderStrategy();

    virtual INT GetMinValue() const = 0;
    virtual INT GetMaxValue() const = 0;
    virtual INT ReadValue() const = 0;
    virtual void WriteValue( INT rawValue, UINT nEvent ) = 0;
    virtual void FormatCaption( INT rawValue, WCHAR* text, size_t textCount ) const = 0;
    virtual void SyncToRuntime( GuiRuntimeContext& runtime ) = 0;
    virtual void SyncFromRuntime( const GuiRuntimeContext& runtime ) = 0;
};

class Gui_CheckBoxStrategy
{
public:
    virtual ~Gui_CheckBoxStrategy();

    virtual const WCHAR* GetLabelText() const = 0;
    virtual UINT GetHotkey() const;
    virtual bool ReadValue() const = 0;
    virtual void WriteValue( bool checked ) = 0;
    virtual void SyncToRuntime( GuiRuntimeContext& runtime ) = 0;
    virtual void SyncFromRuntime( const GuiRuntimeContext& runtime ) = 0;
};

class Gui_ComboBoxStrategy
{
public:
    virtual ~Gui_ComboBoxStrategy();

    virtual const WCHAR* GetLabelText() const = 0;
    virtual UINT GetHotkey() const;
    virtual void Populate( CDXUTComboBox& comboBox ) const = 0;
    virtual void RefreshSelection( CDXUTComboBox& comboBox ) const = 0;
    virtual void WriteSelection( CDXUTComboBox& comboBox ) = 0;
    virtual void SyncToRuntime( GuiRuntimeContext& runtime ) = 0;
    virtual void SyncFromRuntime( const GuiRuntimeContext& runtime ) = 0;
};

class Gui_SliderControl : public Gui_ControlBase
{
public:
    Gui_SliderControl( INT textId, INT sliderId, Gui_SliderStrategy& strategy );

    virtual void Create( CDXUTDialog* pDialog, const GuiPanelLayout& layout, INT& y );
    virtual bool Handles( INT controlId ) const;
    virtual void HandleEvent( UINT nEvent );
    virtual void Refresh();
    virtual void SetVisible( bool visible );
    virtual void SetEnabled( bool enabled );

protected:
    CDXUTStatic* GetLabel() const;
    CDXUTSlider* GetSlider() const;
    virtual void SyncToRuntime( GuiRuntimeContext& runtime );
    virtual void SyncFromRuntime( const GuiRuntimeContext& runtime );

private:
    CDXUTDialog* m_pDialog;
    INT m_textId;
    INT m_sliderId;
    Gui_SliderStrategy& m_strategy;
};

class Gui_CheckBoxControl : public Gui_ControlBase
{
public:
    Gui_CheckBoxControl( INT checkBoxId, Gui_CheckBoxStrategy& strategy );

    virtual void Create( CDXUTDialog* pDialog, const GuiPanelLayout& layout, INT& y );
    virtual bool Handles( INT controlId ) const;
    virtual void HandleEvent( UINT nEvent );
    virtual void Refresh();
    virtual void SetVisible( bool visible );
    virtual void SetEnabled( bool enabled );

protected:
    CDXUTCheckBox* GetCheckBox() const;
    virtual void SyncToRuntime( GuiRuntimeContext& runtime );
    virtual void SyncFromRuntime( const GuiRuntimeContext& runtime );

private:
    CDXUTDialog* m_pDialog;
    INT m_checkBoxId;
    Gui_CheckBoxStrategy& m_strategy;
};

class Gui_ComboBoxControl : public Gui_ControlBase
{
public:
    Gui_ComboBoxControl( INT labelId, INT comboId, Gui_ComboBoxStrategy& strategy );

    virtual void Create( CDXUTDialog* pDialog, const GuiPanelLayout& layout, INT& y );
    virtual bool Handles( INT controlId ) const;
    virtual void HandleEvent( UINT nEvent );
    virtual void Refresh();
    virtual void SetVisible( bool visible );
    virtual void SetEnabled( bool enabled );

protected:
    CDXUTStatic* GetLabel() const;
    CDXUTComboBox* GetComboBox() const;
    virtual void SyncToRuntime( GuiRuntimeContext& runtime );
    virtual void SyncFromRuntime( const GuiRuntimeContext& runtime );

private:
    CDXUTDialog* m_pDialog;
    INT m_labelId;
    INT m_comboId;
    Gui_ComboBoxStrategy& m_strategy;
};

class GuiControlFactory
{
public:
    GuiControlFactory( CDXUTDialog* pDialog, const GuiPanelLayout& layout );

    void Add( Gui_ControlBase& control );
    INT GetCurrentY() const;

private:
    CDXUTDialog* m_pDialog;
    GuiPanelLayout m_layout;
    INT m_currentY;
};

class GuiPanelBase
{
    using ptr_VectorSliderStrategies = std::vector<std::unique_ptr<Gui_SliderStrategy>>;
    using ptr_VectorCheckBoxStrategies = std::vector<std::unique_ptr<Gui_CheckBoxStrategy>>;
    using ptr_VectorComboBoxStrategies = std::vector<std::unique_ptr<Gui_ComboBoxStrategy>>;
    using ptr_VectorControls = std::vector<std::unique_ptr<Gui_ControlBase>>;
public:
    GuiPanelBase();
    virtual ~GuiPanelBase();

    void Initialize( CDXUTDialogResourceManager* pResourceManager,
                     PCALLBACKDXUTGUIEVENT pCallback,
                     const GuiPanelLayout& layout = GuiPanelLayout() );
    void Update();
    void UpdateFromRuntime( const GuiRuntimeContext& runtime );
    void ApplyPendingChanges( GuiRuntimeContext& runtime );
    bool HasPendingChanges() const;

    void Open();
    void Close();
    void ToggleOpen();
    bool IsOpen() const;

    void SetLocation( INT x, INT y );
    void SetSize( INT width, INT height );
    void SetEnabled( bool enabled );

    void OnRender( FLOAT elapsedTime );
    bool MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    bool HandleEvent( UINT nEvent, INT controlId );

    CDXUTDialog& Dialog();
    const CDXUTDialog& Dialog() const;

protected:
    Gui_CheckBoxControl& AddCheckBox(
        GuiControlFactory& factory,
        INT checkBoxId,
        const WCHAR* labelText,
        const std::function<bool()>& readValue,
        const std::function<void( bool )>& writeValue,
        const std::function<void( GuiRuntimeContext& )>& syncToRuntime,
        const std::function<void( const GuiRuntimeContext& )>& syncFromRuntime,
        UINT hotkey = 0 );
    Gui_SliderControl& AddSlider(
        GuiControlFactory& factory,
        INT textId,
        INT sliderId,
        INT minValue,
        INT maxValue,
        const std::function<INT()>& readValue,
        const std::function<void( INT, UINT )>& writeValue,
        const std::function<std::wstring( INT )>& formatCaption,
        const std::function<void( GuiRuntimeContext& )>& syncToRuntime,
        const std::function<void( const GuiRuntimeContext& )>& syncFromRuntime );
    Gui_ComboBoxControl& AddComboBox(
        GuiControlFactory& factory,
        INT labelId,
        INT comboId,
        const WCHAR* labelText,
        const std::function<void( CDXUTComboBox& )>& populate,
        const std::function<void( CDXUTComboBox& )>& refreshSelection,
        const std::function<void( CDXUTComboBox& )>& writeSelection,
        const std::function<void( GuiRuntimeContext& )>& syncToRuntime,
        const std::function<void( const GuiRuntimeContext& )>& syncFromRuntime,
        UINT hotkey = 0 );

    virtual void BuildControls( GuiControlFactory& factory ) = 0;
    virtual void OnUpdate();
    virtual void OnOpenStateChanged( bool isOpen );
    virtual bool OnUnhandledEvent( UINT nEvent, INT controlId );

private:
    template<typename TStrategyBase>
    TStrategyBase& OwnStrategy( std::vector<std::unique_ptr<TStrategyBase>>& storage,
                                std::unique_ptr<TStrategyBase> strategy )
    {
        TStrategyBase* pStrategy = strategy.get();
        storage.push_back( std::move( strategy ) );
        return *pStrategy;
    }

    template<typename TControl>
    TControl& OwnControl( GuiControlFactory& factory, std::unique_ptr<TControl> control )
    {
        TControl* pControl = control.get();
        factory.Add( *pControl );
        m_ownedControls.push_back( std::move( control ) );
        return *pControl;
    }

    void RefreshControls();

    CDXUTDialog m_dialog;
    ptr_VectorSliderStrategies   m_ownedSliderStrategies;
    ptr_VectorCheckBoxStrategies m_ownedCheckBoxStrategies;
    ptr_VectorComboBoxStrategies m_ownedComboBoxStrategies;
    ptr_VectorControls           m_ownedControls;
    bool m_isInitialized;
    bool m_isOpen;
};
