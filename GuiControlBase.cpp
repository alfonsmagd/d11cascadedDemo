#include "DXUT.h"
#include "GuiControlBase.h"
#include <wchar.h>

namespace
{
    class Gui_LambdaSliderStrategy : public Gui_SliderStrategy
    {
    public:
        Gui_LambdaSliderStrategy(
            INT minValue,
            INT maxValue,
            const std::function<INT()>& readValue,
            const std::function<void( INT, UINT )>& writeValue,
            const std::function<std::wstring( INT )>& formatCaption,
            const std::function<void( GuiRuntimeContext& )>& syncToRuntime,
            const std::function<void( const GuiRuntimeContext& )>& syncFromRuntime )
            : m_minValue( minValue ),
              m_maxValue( maxValue ),
              m_readValue( readValue ),
              m_writeValue( writeValue ),
              m_formatCaption( formatCaption ),
              m_syncToRuntime( syncToRuntime ),
              m_syncFromRuntime( syncFromRuntime )
        {
        }

        virtual INT GetMinValue() const
        {
            return m_minValue;
        }

        virtual INT GetMaxValue() const
        {
            return m_maxValue;
        }

        virtual INT ReadValue() const
        {
            return m_readValue();
        }

        virtual void WriteValue( INT rawValue, UINT nEvent )
        {
            m_writeValue( rawValue, nEvent );
        }

        virtual void FormatCaption( INT rawValue, WCHAR* text, size_t textCount ) const
        {
            const std::wstring caption = m_formatCaption( rawValue );
            wcsncpy_s( text, textCount, caption.c_str(), _TRUNCATE );
        }

        virtual void SyncToRuntime( GuiRuntimeContext& runtime )
        {
            m_syncToRuntime( runtime );
        }

        virtual void SyncFromRuntime( const GuiRuntimeContext& runtime )
        {
            m_syncFromRuntime( runtime );
        }

    private:
        INT m_minValue;
        INT m_maxValue;
        std::function<INT()> m_readValue;
        std::function<void( INT, UINT )> m_writeValue;
        std::function<std::wstring( INT )> m_formatCaption;
        std::function<void( GuiRuntimeContext& )> m_syncToRuntime;
        std::function<void( const GuiRuntimeContext& )> m_syncFromRuntime;
    };

    class Gui_LambdaCheckBoxStrategy : public Gui_CheckBoxStrategy
    {
    public:
        Gui_LambdaCheckBoxStrategy(
            const WCHAR* labelText,
            const std::function<bool()>& readValue,
            const std::function<void( bool )>& writeValue,
            const std::function<void( GuiRuntimeContext& )>& syncToRuntime,
            const std::function<void( const GuiRuntimeContext& )>& syncFromRuntime,
            UINT hotkey )
            : m_labelText( labelText ? labelText : L"" ),
              m_readValue( readValue ),
              m_writeValue( writeValue ),
              m_syncToRuntime( syncToRuntime ),
              m_syncFromRuntime( syncFromRuntime ),
              m_hotkey( hotkey )
        {
        }

        virtual const WCHAR* GetLabelText() const
        {
            return m_labelText.c_str();
        }

        virtual UINT GetHotkey() const
        {
            return m_hotkey;
        }

        virtual bool ReadValue() const
        {
            return m_readValue();
        }

        virtual void WriteValue( bool checked )
        {
            m_writeValue( checked );
        }

        virtual void SyncToRuntime( GuiRuntimeContext& runtime )
        {
            m_syncToRuntime( runtime );
        }

        virtual void SyncFromRuntime( const GuiRuntimeContext& runtime )
        {
            m_syncFromRuntime( runtime );
        }

    private:
        std::wstring m_labelText;
        std::function<bool()> m_readValue;
        std::function<void( bool )> m_writeValue;
        std::function<void( GuiRuntimeContext& )> m_syncToRuntime;
        std::function<void( const GuiRuntimeContext& )> m_syncFromRuntime;
        UINT m_hotkey;
    };

    class Gui_LambdaComboBoxStrategy : public Gui_ComboBoxStrategy
    {
    public:
        Gui_LambdaComboBoxStrategy(
            const WCHAR* labelText,
            const std::function<void( CDXUTComboBox& )>& populate,
            const std::function<void( CDXUTComboBox& )>& refreshSelection,
            const std::function<void( CDXUTComboBox& )>& writeSelection,
            const std::function<void( GuiRuntimeContext& )>& syncToRuntime,
            const std::function<void( const GuiRuntimeContext& )>& syncFromRuntime,
            UINT hotkey )
            : m_labelText( labelText ? labelText : L"" ),
              m_populate( populate ),
              m_refreshSelection( refreshSelection ),
              m_writeSelection( writeSelection ),
              m_syncToRuntime( syncToRuntime ),
              m_syncFromRuntime( syncFromRuntime ),
              m_hotkey( hotkey )
        {
        }

        virtual const WCHAR* GetLabelText() const
        {
            return m_labelText.c_str();
        }

        virtual UINT GetHotkey() const
        {
            return m_hotkey;
        }

        virtual void Populate( CDXUTComboBox& comboBox ) const
        {
            m_populate( comboBox );
        }

        virtual void RefreshSelection( CDXUTComboBox& comboBox ) const
        {
            m_refreshSelection( comboBox );
        }

        virtual void WriteSelection( CDXUTComboBox& comboBox )
        {
            m_writeSelection( comboBox );
        }

        virtual void SyncToRuntime( GuiRuntimeContext& runtime )
        {
            m_syncToRuntime( runtime );
        }

        virtual void SyncFromRuntime( const GuiRuntimeContext& runtime )
        {
            m_syncFromRuntime( runtime );
        }

    private:
        std::wstring m_labelText;
        std::function<void( CDXUTComboBox& )> m_populate;
        std::function<void( CDXUTComboBox& )> m_refreshSelection;
        std::function<void( CDXUTComboBox& )> m_writeSelection;
        std::function<void( GuiRuntimeContext& )> m_syncToRuntime;
        std::function<void( const GuiRuntimeContext& )> m_syncFromRuntime;
        UINT m_hotkey;
    };
}

GuiPanelLayout::GuiPanelLayout()
    : x( 0 ),
      y( 10 ),
      width( 190 ),
      labelHeight( 16 ),
      sliderHeight( 16 ),
      checkBoxHeight( 23 ),
      sliderSpacing( 24 ),
      checkBoxSpacing( 26 )
{
}

Gui_ControlBase::Gui_ControlBase()
    : m_hasPendingChanges( false )
{
}

Gui_ControlBase::~Gui_ControlBase()
{
}

void Gui_ControlBase::ApplyPendingChanges( GuiRuntimeContext& runtime )
{
    if( !m_hasPendingChanges )
    {
        return;
    }

    SyncToRuntime( runtime );
    m_hasPendingChanges = false;
    Refresh();
}

void Gui_ControlBase::UpdateFromRuntime( const GuiRuntimeContext& runtime )
{
    SyncFromRuntime( runtime );
    Refresh();
}

bool Gui_ControlBase::HasPendingChanges() const
{
    return m_hasPendingChanges;
}

void Gui_ControlBase::MarkDirty()
{
    m_hasPendingChanges = true;
}

Gui_SliderStrategy::~Gui_SliderStrategy()
{
}

Gui_CheckBoxStrategy::~Gui_CheckBoxStrategy()
{
}

UINT Gui_CheckBoxStrategy::GetHotkey() const
{
    return 0;
}

Gui_ComboBoxStrategy::~Gui_ComboBoxStrategy()
{
}

UINT Gui_ComboBoxStrategy::GetHotkey() const
{
    return 0;
}

Gui_SliderControl::Gui_SliderControl( INT textId, INT sliderId, Gui_SliderStrategy& strategy )
    : m_pDialog( NULL ),
      m_textId( textId ),
      m_sliderId( sliderId ),
      m_strategy( strategy )
{
}

void Gui_SliderControl::Create( CDXUTDialog* pDialog, const GuiPanelLayout& layout, INT& y )
{
    m_pDialog = pDialog;
    pDialog->AddStatic( m_textId, L"", layout.x, y, layout.width, layout.labelHeight );
    y += layout.labelHeight + 2;
    pDialog->AddSlider( m_sliderId, layout.x, y, layout.width, layout.sliderHeight,
                        m_strategy.GetMinValue(), m_strategy.GetMaxValue(), m_strategy.ReadValue() );
    y += layout.sliderSpacing;
    Refresh();
}

bool Gui_SliderControl::Handles( INT controlId ) const
{
    return controlId == m_sliderId;
}

void Gui_SliderControl::HandleEvent( UINT nEvent )
{
    CDXUTSlider* pSlider = GetSlider();
    if( !pSlider )
    {
        return;
    }

    m_strategy.WriteValue( pSlider->GetValue(), nEvent );
    MarkDirty();
    Refresh();
}

void Gui_SliderControl::Refresh()
{
    CDXUTStatic* pLabel = GetLabel();
    CDXUTSlider* pSlider = GetSlider();
    if( !pLabel || !pSlider )
    {
        return;
    }

    const INT value = m_strategy.ReadValue();
    WCHAR text[128];
    m_strategy.FormatCaption( value, text, ARRAYSIZE( text ) );
    pLabel->SetText( text );
    pSlider->SetValue( value );
}

void Gui_SliderControl::SetVisible( bool visible )
{
    CDXUTStatic* pLabel = GetLabel();
    CDXUTSlider* pSlider = GetSlider();
    if( pLabel )
    {
        pLabel->SetVisible( visible );
    }
    if( pSlider )
    {
        pSlider->SetVisible( visible );
    }
}

void Gui_SliderControl::SetEnabled( bool enabled )
{
    CDXUTStatic* pLabel = GetLabel();
    CDXUTSlider* pSlider = GetSlider();
    if( pLabel )
    {
        pLabel->SetEnabled( enabled );
    }
    if( pSlider )
    {
        pSlider->SetEnabled( enabled );
    }
}

CDXUTStatic* Gui_SliderControl::GetLabel() const
{
    return m_pDialog ? m_pDialog->GetStatic( m_textId ) : NULL;
}

CDXUTSlider* Gui_SliderControl::GetSlider() const
{
    return m_pDialog ? m_pDialog->GetSlider( m_sliderId ) : NULL;
}

void Gui_SliderControl::SyncToRuntime( GuiRuntimeContext& runtime )
{
    m_strategy.SyncToRuntime( runtime );
}

void Gui_SliderControl::SyncFromRuntime( const GuiRuntimeContext& runtime )
{
    m_strategy.SyncFromRuntime( runtime );
}

Gui_CheckBoxControl::Gui_CheckBoxControl( INT checkBoxId, Gui_CheckBoxStrategy& strategy )
    : m_pDialog( NULL ),
      m_checkBoxId( checkBoxId ),
      m_strategy( strategy )
{
}

void Gui_CheckBoxControl::Create( CDXUTDialog* pDialog, const GuiPanelLayout& layout, INT& y )
{
    m_pDialog = pDialog;
    pDialog->AddCheckBox( m_checkBoxId, m_strategy.GetLabelText(), layout.x, y, layout.width,
                          layout.checkBoxHeight, m_strategy.ReadValue(), m_strategy.GetHotkey() );
    y += layout.checkBoxSpacing;
    Refresh();
}

bool Gui_CheckBoxControl::Handles( INT controlId ) const
{
    return controlId == m_checkBoxId;
}

void Gui_CheckBoxControl::HandleEvent( UINT )
{
    CDXUTCheckBox* pCheckBox = GetCheckBox();
    if( !pCheckBox )
    {
        return;
    }

    m_strategy.WriteValue( pCheckBox->GetChecked() );
    MarkDirty();
    Refresh();
}

void Gui_CheckBoxControl::Refresh()
{
    CDXUTCheckBox* pCheckBox = GetCheckBox();
    if( pCheckBox )
    {
        pCheckBox->SetChecked( m_strategy.ReadValue() );
    }
}

void Gui_CheckBoxControl::SetVisible( bool visible )
{
    CDXUTCheckBox* pCheckBox = GetCheckBox();
    if( pCheckBox )
    {
        pCheckBox->SetVisible( visible );
    }
}

void Gui_CheckBoxControl::SetEnabled( bool enabled )
{
    CDXUTCheckBox* pCheckBox = GetCheckBox();
    if( pCheckBox )
    {
        pCheckBox->SetEnabled( enabled );
    }
}

CDXUTCheckBox* Gui_CheckBoxControl::GetCheckBox() const
{
    return m_pDialog ? m_pDialog->GetCheckBox( m_checkBoxId ) : NULL;
}

void Gui_CheckBoxControl::SyncToRuntime( GuiRuntimeContext& runtime )
{
    m_strategy.SyncToRuntime( runtime );
}

void Gui_CheckBoxControl::SyncFromRuntime( const GuiRuntimeContext& runtime )
{
    m_strategy.SyncFromRuntime( runtime );
}

Gui_ComboBoxControl::Gui_ComboBoxControl( INT labelId, INT comboId, Gui_ComboBoxStrategy& strategy )
    : m_pDialog( NULL ),
      m_labelId( labelId ),
      m_comboId( comboId ),
      m_strategy( strategy )
{
}

void Gui_ComboBoxControl::Create( CDXUTDialog* pDialog, const GuiPanelLayout& layout, INT& y )
{
    m_pDialog = pDialog;
    pDialog->AddStatic( m_labelId, m_strategy.GetLabelText(), layout.x, y, layout.width, layout.labelHeight );
    y += layout.labelHeight + 2;

    CDXUTComboBox* pComboBox = NULL;
    pDialog->AddComboBox( m_comboId, layout.x, y, layout.width, layout.checkBoxHeight,
                          m_strategy.GetHotkey(), false, &pComboBox );
    if( pComboBox )
    {
        m_strategy.Populate( *pComboBox );
        m_strategy.RefreshSelection( *pComboBox );
    }

    y += layout.checkBoxSpacing;
    Refresh();
}

bool Gui_ComboBoxControl::Handles( INT controlId ) const
{
    return controlId == m_comboId;
}

void Gui_ComboBoxControl::HandleEvent( UINT )
{
    CDXUTComboBox* pComboBox = GetComboBox();
    if( !pComboBox )
    {
        return;
    }

    m_strategy.WriteSelection( *pComboBox );
    MarkDirty();
    Refresh();
}

void Gui_ComboBoxControl::Refresh()
{
    CDXUTStatic* pLabel = GetLabel();
    CDXUTComboBox* pComboBox = GetComboBox();
    if( pLabel )
    {
        pLabel->SetText( m_strategy.GetLabelText() );
    }
    if( pComboBox )
    {
        m_strategy.Populate( *pComboBox );
        m_strategy.RefreshSelection( *pComboBox );
    }
}

void Gui_ComboBoxControl::SetVisible( bool visible )
{
    CDXUTStatic* pLabel = GetLabel();
    CDXUTComboBox* pComboBox = GetComboBox();
    if( pLabel )
    {
        pLabel->SetVisible( visible );
    }
    if( pComboBox )
    {
        pComboBox->SetVisible( visible );
    }
}

void Gui_ComboBoxControl::SetEnabled( bool enabled )
{
    CDXUTStatic* pLabel = GetLabel();
    CDXUTComboBox* pComboBox = GetComboBox();
    if( pLabel )
    {
        pLabel->SetEnabled( enabled );
    }
    if( pComboBox )
    {
        pComboBox->SetEnabled( enabled );
    }
}

CDXUTStatic* Gui_ComboBoxControl::GetLabel() const
{
    return m_pDialog ? m_pDialog->GetStatic( m_labelId ) : NULL;
}

CDXUTComboBox* Gui_ComboBoxControl::GetComboBox() const
{
    return m_pDialog ? m_pDialog->GetComboBox( m_comboId ) : NULL;
}

void Gui_ComboBoxControl::SyncToRuntime( GuiRuntimeContext& runtime )
{
    m_strategy.SyncToRuntime( runtime );
}

void Gui_ComboBoxControl::SyncFromRuntime( const GuiRuntimeContext& runtime )
{
    m_strategy.SyncFromRuntime( runtime );
}

GuiControlFactory::GuiControlFactory( CDXUTDialog* pDialog, const GuiPanelLayout& layout )
    : m_pDialog( pDialog ),
      m_layout( layout ),
      m_currentY( layout.y )
{
}

void GuiControlFactory::Add( Gui_ControlBase& control )
{
    control.Create( m_pDialog, m_layout, m_currentY );
}

INT GuiControlFactory::GetCurrentY() const
{
    return m_currentY;
}

GuiPanelBase::GuiPanelBase()
    : m_isInitialized( false ),
      m_isOpen( true )
{
}

GuiPanelBase::~GuiPanelBase()
{
}

void GuiPanelBase::Initialize( CDXUTDialogResourceManager* pResourceManager,
                               PCALLBACKDXUTGUIEVENT pCallback,
                               const GuiPanelLayout& layout )
{
    m_ownedSliderStrategies.clear();
    m_ownedCheckBoxStrategies.clear();
    m_ownedComboBoxStrategies.clear();
    m_ownedControls.clear();
    m_dialog.Init( pResourceManager );
    m_dialog.SetCallback( pCallback );

    GuiControlFactory factory( &m_dialog, layout );
    BuildControls( factory );
    m_isInitialized = true;
    Update();
    OnOpenStateChanged( m_isOpen );
}

void GuiPanelBase::Update()
{
    OnUpdate();
    RefreshControls();
}

void GuiPanelBase::UpdateFromRuntime( const GuiRuntimeContext& runtime )
{
    for( size_t i = 0; i < m_ownedControls.size(); ++i )
    {
        m_ownedControls[i]->UpdateFromRuntime( runtime );
    }
    Update();
}

void GuiPanelBase::ApplyPendingChanges( GuiRuntimeContext& runtime )
{
    bool didApplyChanges = false;
    for( size_t i = 0; i < m_ownedControls.size(); ++i )
    {
        if( !m_ownedControls[i]->HasPendingChanges() )
        {
            continue;
        }

        m_ownedControls[i]->ApplyPendingChanges( runtime );
        didApplyChanges = true;
    }

    if( didApplyChanges )
    {
        Update();
    }
}

bool GuiPanelBase::HasPendingChanges() const
{
    for( size_t i = 0; i < m_ownedControls.size(); ++i )
    {
        if( m_ownedControls[i]->HasPendingChanges() )
        {
            return true;
        }
    }

    return false;
}

void GuiPanelBase::Open()
{
    if( !m_isOpen )
    {
        m_isOpen = true;
        OnOpenStateChanged( true );
    }
}

void GuiPanelBase::Close()
{
    if( m_isOpen )
    {
        m_isOpen = false;
        OnOpenStateChanged( false );
    }
}

void GuiPanelBase::ToggleOpen()
{
    if( m_isOpen )
    {
        Close();
    }
    else
    {
        Open();
    }
}

bool GuiPanelBase::IsOpen() const
{
    return m_isOpen;
}

void GuiPanelBase::SetLocation( INT x, INT y )
{
    m_dialog.SetLocation( x, y );
}

void GuiPanelBase::SetSize( INT width, INT height )
{
    m_dialog.SetSize( width, height );
}

void GuiPanelBase::SetEnabled( bool enabled )
{
    for( size_t i = 0; i < m_ownedControls.size(); ++i )
    {
        m_ownedControls[i]->SetEnabled( enabled );
    }
}

void GuiPanelBase::OnRender( FLOAT elapsedTime )
{
    if( m_isInitialized && m_isOpen )
    {
        m_dialog.OnRender( elapsedTime );
    }
}

bool GuiPanelBase::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    if( !m_isInitialized || !m_isOpen )
    {
        return false;
    }

    return m_dialog.MsgProc( hWnd, uMsg, wParam, lParam );
}

bool GuiPanelBase::HandleEvent( UINT nEvent, INT controlId )
{
    if( !m_isInitialized || !m_isOpen )
    {
        return false;
    }

    for( size_t i = 0; i < m_ownedControls.size(); ++i )
    {
        if( m_ownedControls[i]->Handles( controlId ) )
        {
            m_ownedControls[i]->HandleEvent( nEvent );
            return true;
        }
    }

    return OnUnhandledEvent( nEvent, controlId );
}

CDXUTDialog& GuiPanelBase::Dialog()
{
    return m_dialog;
}

const CDXUTDialog& GuiPanelBase::Dialog() const
{
    return m_dialog;
}

Gui_CheckBoxControl& GuiPanelBase::AddCheckBox(
    GuiControlFactory& factory,
    INT checkBoxId,
    const WCHAR* labelText,
    const std::function<bool()>& readValue,
    const std::function<void( bool )>& writeValue,
    const std::function<void( GuiRuntimeContext& )>& syncToRuntime,
    const std::function<void( const GuiRuntimeContext& )>& syncFromRuntime,
    UINT hotkey )
{
    std::unique_ptr<Gui_CheckBoxStrategy> strategy(
        new Gui_LambdaCheckBoxStrategy(
            labelText,
            readValue,
            writeValue,
            syncToRuntime,
            syncFromRuntime,
            hotkey ) );
    Gui_CheckBoxStrategy& strategyRef = OwnStrategy( m_ownedCheckBoxStrategies, std::move( strategy ) );
    return OwnControl(
        factory,
        std::unique_ptr<Gui_CheckBoxControl>(
            new Gui_CheckBoxControl( checkBoxId, strategyRef ) ) );
}

Gui_SliderControl& GuiPanelBase::AddSlider(
    GuiControlFactory& factory,
    INT textId,
    INT sliderId,
    INT minValue,
    INT maxValue,
    const std::function<INT()>& readValue,
    const std::function<void( INT, UINT )>& writeValue,
    const std::function<std::wstring( INT )>& formatCaption,
    const std::function<void( GuiRuntimeContext& )>& syncToRuntime,
    const std::function<void( const GuiRuntimeContext& )>& syncFromRuntime )
{
    std::unique_ptr<Gui_SliderStrategy> strategy(
        new Gui_LambdaSliderStrategy(
            minValue,
            maxValue,
            readValue,
            writeValue,
            formatCaption,
            syncToRuntime,
            syncFromRuntime ) );
    Gui_SliderStrategy& strategyRef = OwnStrategy( m_ownedSliderStrategies, std::move( strategy ) );
    return OwnControl(
        factory,
        std::unique_ptr<Gui_SliderControl>(
            new Gui_SliderControl( textId, sliderId, strategyRef ) ) );
}

Gui_ComboBoxControl& GuiPanelBase::AddComboBox(
    GuiControlFactory& factory,
    INT labelId,
    INT comboId,
    const WCHAR* labelText,
    const std::function<void( CDXUTComboBox& )>& populate,
    const std::function<void( CDXUTComboBox& )>& refreshSelection,
    const std::function<void( CDXUTComboBox& )>& writeSelection,
    const std::function<void( GuiRuntimeContext& )>& syncToRuntime,
    const std::function<void( const GuiRuntimeContext& )>& syncFromRuntime,
    UINT hotkey )
{
    std::unique_ptr<Gui_ComboBoxStrategy> strategy(
        new Gui_LambdaComboBoxStrategy(
            labelText,
            populate,
            refreshSelection,
            writeSelection,
            syncToRuntime,
            syncFromRuntime,
            hotkey ) );
    Gui_ComboBoxStrategy& strategyRef = OwnStrategy( m_ownedComboBoxStrategies, std::move( strategy ) );
    return OwnControl(
        factory,
        std::unique_ptr<Gui_ComboBoxControl>(
            new Gui_ComboBoxControl( labelId, comboId, strategyRef ) ) );
}

void GuiPanelBase::OnUpdate()
{
}

void GuiPanelBase::OnOpenStateChanged( bool isOpen )
{
    m_dialog.SetVisible( isOpen );

    for( size_t i = 0; i < m_ownedControls.size(); ++i )
    {
        m_ownedControls[i]->SetVisible( isOpen );
    }
}

bool GuiPanelBase::OnUnhandledEvent( UINT, INT )
{
    return false;
}

void GuiPanelBase::RefreshControls()
{
    for( size_t i = 0; i < m_ownedControls.size(); ++i )
    {
        m_ownedControls[i]->Refresh();
    }
}
