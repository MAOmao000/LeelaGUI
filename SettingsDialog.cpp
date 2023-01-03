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
        m_radioBoxDefaultRule->Disable();
        m_radioBoxDefaultRule->SetSelection(0);
    } else {
        bool ponderEnabled = wxConfig::Get()->ReadBool(wxT("ponderKataGoEnabled"), true);
        m_checkBoxPondering->SetValue(ponderEnabled);
        int rule = wxConfig::Get()->ReadLong(wxT("DefaultRule"), (long)0);
        m_radioBoxDefaultRule->SetSelection(rule);
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

    int engine_type = wxConfig::Get()->ReadLong(wxT("EngineType"), (long)0);
#ifdef USE_THREAD
    if (engine_type == 2) {
        wxConfig::Get()->Write(wxT("EngineType"), (long)1);
        m_radioBoxEngineType->Enable(2, false);
    }
#endif
    m_radioBoxEngineType->SetSelection(engine_type);
    if (engine_type == 0) {
        m_filePickerEngine->Disable();
        m_filePickerConfigration->Disable();
        m_filePickerModel->Disable();
    } else {
        m_filePickerEngine->Enable();
        m_filePickerConfigration->Enable();
        m_filePickerModel->Enable();
    }

    wxString str1, str2, str3;
    wxConfig::Get()->Read(wxT("EnginePath"), &str1);
    m_filePickerEngine->SetPath(str1);
    wxConfig::Get()->Read(wxT("ConfigPath"), &str2);
    m_filePickerConfigration->SetPath(str2);
    wxConfig::Get()->Read(wxT("ModelPath"), &str3);
    m_filePickerModel->SetPath(str3);

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
        int rule = m_radioBoxDefaultRule->GetSelection();
        wxConfig::Get()->Write(wxT("DefaultRule"), (long)rule);
    }

    bool netsEnabled = m_checkBoxNeuralNet->GetValue();
    wxConfig::Get()->Write(wxT("netsEnabled"), netsEnabled);

    bool soundEnabled = m_checkBoxSound->GetValue();
    wxConfig::Get()->Write(wxT("soundEnabled"), soundEnabled);

    bool dpiEnabled = m_checkBoxDPIScaling->GetValue();
    wxConfig::Get()->Write(wxT("dpiscaleEnabled"), dpiEnabled);

    bool japaneseEnabled = m_checkBoxJapanese->GetValue();
    wxConfig::Get()->Write(wxT("japaneseEnabled"), japaneseEnabled);

    int engine_type = m_radioBoxEngineType->GetSelection();
    wxConfig::Get()->Write(wxT("EngineType"), (long)engine_type);

    wxString path1 = m_filePickerEngine->GetPath();
    wxConfig::Get()->Write(wxT("EnginePath"), path1);

    wxString path2 = m_filePickerConfigration->GetPath();
    wxConfig::Get()->Write(wxT("ConfigPath"), path2);

    wxString path3 = m_filePickerModel->GetPath();
    wxConfig::Get()->Write(wxT("ModelPath"), path3);

    event.Skip();
}

void SettingsDialog::doChangeEngine(wxUpdateUIEvent& event) {
    int engine_type = m_radioBoxEngineType->GetSelection();
    wxConfig::Get()->Write(wxT("EngineType"), (long)engine_type);
    if (engine_type == 0) {
        m_filePickerEngine->Disable();
        m_filePickerConfigration->Disable();
        m_filePickerModel->Disable();
    } else {
        m_filePickerEngine->Enable();
        m_filePickerConfigration->Enable();
        m_filePickerModel->Enable();
    }
    event.Skip();
}
