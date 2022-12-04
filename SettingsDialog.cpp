#include "SettingsDialog.h"
#include "GTP.h"

SettingsDialog::SettingsDialog( wxWindow* parent )
:
TSettingsDialog( parent )
{

}

void SettingsDialog::doInit(wxInitDialogEvent& event) {
    bool passEnabled = wxConfig::Get()->ReadBool(wxT("passEnabled"), true);
    m_checkBoxPasses->SetValue(passEnabled);

    bool resignEnabled = wxConfig::Get()->ReadBool(wxT("resignEnabled"), true);
    m_checkBoxResignations->SetValue(resignEnabled);

    if (cfg_use_engine == GTP::ORIGINE_ENGINE) {
        bool ponderEnabled = wxConfig::Get()->ReadBool(wxT("ponderEnabled"), true);
        m_checkBoxPondering->SetValue(ponderEnabled);
    } else {
        bool ponderEnabled = wxConfig::Get()->ReadBool(wxT("ponderKataGoEnabled"), true);
        m_checkBoxPondering->SetValue(ponderEnabled);
    }

    bool netsEnabled = wxConfig::Get()->ReadBool(wxT("netsEnabled"), true);
    m_checkBoxNeuralNet->SetValue(netsEnabled);

    if (cfg_use_engine == GTP::KATAGO_ENGINE) {
        m_checkBoxNeuralNet->Enable(false);
    }

    bool soundEnabled = wxConfig::Get()->ReadBool(wxT("soundEnabled"), true);
    m_checkBoxSound->SetValue(soundEnabled);

    bool dpiEnabled = wxConfig::Get()->ReadBool(wxT("dpiscaleEnabled"), false);
    m_checkBoxDPIScaling->SetValue(dpiEnabled);

    bool japaneseEnabled = wxConfig::Get()->ReadBool(wxT("japaneseEnabled"), true);
    m_checkBoxJapanese->SetValue(japaneseEnabled);

    bool katagoEnabled = wxConfig::Get()->ReadBool(wxT("katagoEnabled"), true);
    m_checkBoxKataGo->SetValue(katagoEnabled);

#ifdef __WXGTK__
    m_checkBoxDPIScaling->Disable();
    m_checkBoxDPIScaling->SetValue(false);
#elif defined(__WXMAC__)
    m_checkBoxDPIScaling->Disable();
    m_checkBoxSound->Disable();
    m_checkBoxSound->SetValue(false);
#endif
}

void SettingsDialog::doCancel(wxCommandEvent& event) {
    event.Skip();
}

void SettingsDialog::doOK(wxCommandEvent& event) {
    bool passEnabled = m_checkBoxPasses->GetValue();
    wxConfig::Get()->Write(wxT("passEnabled"), passEnabled);

    bool resignEnabled = m_checkBoxResignations->GetValue();
    wxConfig::Get()->Write(wxT("resignEnabled"), resignEnabled);

    bool ponderEnabled = m_checkBoxPondering->GetValue();
    if (cfg_use_engine == GTP::ORIGINE_ENGINE) {
        wxConfig::Get()->Write(wxT("ponderEnabled"), ponderEnabled);
    } else {
        wxConfig::Get()->Write(wxT("ponderKataGoEnabled"), ponderEnabled);
    }

    bool netsEnabled = m_checkBoxNeuralNet->GetValue();
    wxConfig::Get()->Write(wxT("netsEnabled"), netsEnabled);

    bool soundEnabled = m_checkBoxSound->GetValue();
    wxConfig::Get()->Write(wxT("soundEnabled"), soundEnabled);

    bool dpiEnabled = m_checkBoxDPIScaling->GetValue();
    wxConfig::Get()->Write(wxT("dpiscaleEnabled"), dpiEnabled);

    bool japaneseEnabled = m_checkBoxJapanese->GetValue();
    wxConfig::Get()->Write(wxT("japaneseEnabled"), japaneseEnabled);

    bool katagoEnabled = m_checkBoxKataGo->GetValue();
    wxConfig::Get()->Write(wxT("katagoEnabled"), katagoEnabled);

    event.Skip();
}
