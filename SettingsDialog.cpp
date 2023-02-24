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

    bool resignEnabled1 = wxConfig::Get()->ReadBool(wxT("resignEnabled"), true);
    m_checkBoxResignationsLeela->SetValue(resignEnabled1);

    bool resignEnabled2 = wxConfig::Get()->ReadBool(wxT("resignKataGoEnabled"), true);
    m_checkBoxResignationsKataGo->SetValue(resignEnabled2);

    bool ponderEnabled1 = wxConfig::Get()->ReadBool(wxT("ponderEnabled"), true);
    m_checkBoxPonderingLeela->SetValue(ponderEnabled1);

    bool ponderEnabled2 = wxConfig::Get()->ReadBool(wxT("ponderKataGoEnabled"), true);
    m_checkBoxPonderingKataGo->SetValue(ponderEnabled2);

    int rule = wxConfig::Get()->ReadLong(wxT("KataGoRule"), (long)0);
    m_radioBoxDefaultRule->SetSelection(rule);

    int ratedGameEngine = wxConfig::Get()->ReadBool(wxT("ratedGameEngine"), (long)0);
    m_radioBoxRatedEngine->SetSelection(ratedGameEngine);

    bool netsEnabled = wxConfig::Get()->ReadBool(wxT("netsEnabled"), true);
    m_checkBoxNeuralNet->SetValue(netsEnabled);

    bool soundEnabled = wxConfig::Get()->ReadBool(wxT("soundEnabled"), true);
    m_checkBoxSound->SetValue(soundEnabled);

    bool dpiEnabled = wxConfig::Get()->ReadBool(wxT("dpiscaleEnabled"), false);
    m_checkBoxDPIScaling->SetValue(dpiEnabled);

    bool japaneseEnabled = wxConfig::Get()->ReadBool(wxT("japaneseEnabled"), true);
    m_checkBoxJapanese->SetValue(japaneseEnabled);

    int engine_type = wxConfig::Get()->ReadLong(wxT("EngineType"), (long)0);

#ifdef USE_THREAD
    if (engine_type == 2) {
        engine_type = 1;
        wxConfig::Get()->Write(wxT("EngineType"), (long)1);
    }
    m_radioBoxEngineType->Enable(2, false);
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

    wxString str1 = "";
    wxString str2 = "";
    wxString str3 = "";
    if (engine_type == 1) {
        wxConfig::Get()->Read(wxT("AnalysisEnginePath"), &str1);
        wxConfig::Get()->Read(wxT("AnalysisConfigPath"), &str2);
        wxConfig::Get()->Read(wxT("AnalysisModelPath"), &str3);
    } else if (engine_type == 2) {
        wxConfig::Get()->Read(wxT("GTPEnginePath"), &str1);
        wxConfig::Get()->Read(wxT("GTPConfigPath"), &str2);
        wxConfig::Get()->Read(wxT("GTPModelPath"), &str3);
    }
    m_filePickerEngine->SetPath(str1);
    m_filePickerConfigration->SetPath(str2);
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

    bool resignEnabled = m_checkBoxResignationsLeela->GetValue();
    wxConfig::Get()->Write(wxT("resignEnabled"), resignEnabled);

    bool resignKataGoEnabled = m_checkBoxResignationsKataGo->GetValue();
    wxConfig::Get()->Write(wxT("resignKataGoEnabled"), resignKataGoEnabled);

    bool ponderEnabled = m_checkBoxPonderingLeela->GetValue();
    wxConfig::Get()->Write(wxT("ponderEnabled"), ponderEnabled);

    bool ponderKataGoEnabled = m_checkBoxPonderingKataGo->GetValue();
    wxConfig::Get()->Write(wxT("ponderKataGoEnabled"), ponderKataGoEnabled);

    int rule = m_radioBoxDefaultRule->GetSelection();
    wxConfig::Get()->Write(wxT("KataGoRule"), (long)rule);

    int ratedGameEngine = m_radioBoxRatedEngine->GetSelection();
    wxConfig::Get()->Write(wxT("ratedGameEngine"), (long)ratedGameEngine);

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

    if (engine_type != 0) {
        wxString path1 = m_filePickerEngine->GetPath();
        wxString path2 = m_filePickerConfigration->GetPath();
        wxString path3 = m_filePickerModel->GetPath();
        if (engine_type == 2) {
            wxConfig::Get()->Write(wxT("GTPEnginePath"), path1);
            wxConfig::Get()->Write(wxT("GTPConfigPath"), path2);
            wxConfig::Get()->Write(wxT("GTPModelPath"), path3);
        } else if (engine_type == 1) {
            wxConfig::Get()->Write(wxT("AnalysisEnginePath"), path1);
            wxConfig::Get()->Write(wxT("AnalysisConfigPath"), path2);
            wxConfig::Get()->Write(wxT("AnalysisModelPath"), path3);
        }
    }
    event.Skip();
}

void SettingsDialog::doChangeEngine(wxCommandEvent& WXUNUSED(event)) {
    int engine_type = m_radioBoxEngineType->GetSelection();
    wxConfig::Get()->Write(wxT("EngineType"), (long)engine_type);

    if (engine_type == 0) {
        m_filePickerEngine->SetPath("");
        m_filePickerConfigration->SetPath("");
        m_filePickerModel->SetPath("");
        m_filePickerEngine->Disable();
        m_filePickerConfigration->Disable();
        m_filePickerModel->Disable();
    } else {
        wxString str1, str2, str3;
        if (engine_type == 1) {
            wxConfig::Get()->Read(wxT("AnalysisEnginePath"), &str1);
            wxConfig::Get()->Read(wxT("AnalysisConfigPath"), &str2);
            wxConfig::Get()->Read(wxT("AnalysisModelPath"), &str3);
        } else if (engine_type == 2) {
            wxConfig::Get()->Read(wxT("GTPEnginePath"), &str1);
            wxConfig::Get()->Read(wxT("GTPConfigPath"), &str2);
            wxConfig::Get()->Read(wxT("GTPModelPath"), &str3);
        }
        m_filePickerEngine->SetPath(str1);
        m_filePickerConfigration->SetPath(str2);
        m_filePickerModel->SetPath(str3);
        m_filePickerEngine->Enable();
        m_filePickerConfigration->Enable();
        m_filePickerModel->Enable();
    }
}
