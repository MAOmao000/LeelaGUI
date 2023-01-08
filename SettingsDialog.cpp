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

#ifdef USE_GPU
    int engine_type = wxConfig::Get()->ReadLong(wxT("EngineTypeGPU"), (long)0);
#else
    int engine_type = wxConfig::Get()->ReadLong(wxT("EngineTypeCPU"), (long)0);
#endif
#ifdef USE_THREAD
    if (engine_type == 2) {
        engine_type = 1;
#ifdef USE_GPU
        wxConfig::Get()->Write(wxT("EngineTypeGPU"), (long)1);
#else
        wxConfig::Get()->Write(wxT("EngineTypeCPU"), (long)1);
#endif
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
#ifdef USE_GPU
    if (engine_type == 2) {
        wxConfig::Get()->Read(wxT("GTPEnginePathGPU"), &str1);
        wxConfig::Get()->Read(wxT("GTPConfigPathGPU"), &str2);
        wxConfig::Get()->Read(wxT("GTPModelPathGPU"), &str3);
    } else if (engine_type == 1) {
        wxConfig::Get()->Read(wxT("AnalysisEnginePathGPU"), &str1);
        wxConfig::Get()->Read(wxT("AnalysisConfigPathGPU"), &str2);
        wxConfig::Get()->Read(wxT("AnalysisModelPathGPU"), &str3);
    } else {
        str1 = "";
        str2 = "";
        str3 = "";
    }
#else
    if (engine_type == 2) {
        wxConfig::Get()->Read(wxT("GTPEnginePathCPU"), &str1);
        wxConfig::Get()->Read(wxT("GTPConfigPathCPU"), &str2);
        wxConfig::Get()->Read(wxT("GTPModelPathCPU"), &str3);
    } else if (engine_type == 1) {
        wxConfig::Get()->Read(wxT("AnalysisEnginePathCPU"), &str1);
        wxConfig::Get()->Read(wxT("AnalysisConfigPathCPU"), &str2);
        wxConfig::Get()->Read(wxT("AnalysisModelPathCPU"), &str3);
    } else {
        str1 = "";
        str2 = "";
        str3 = "";
    }
#endif
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

#ifdef USE_GPU
    int engine_type = m_radioBoxEngineType->GetSelection();
    wxConfig::Get()->Write(wxT("EngineTypeGPU"), (long)engine_type);

    if (engine_type != 0) {
        wxString path1 = m_filePickerEngine->GetPath();
        wxString path2 = m_filePickerConfigration->GetPath();
        wxString path3 = m_filePickerModel->GetPath();
        if (engine_type == 2) {
            wxConfig::Get()->Write(wxT("GTPEnginePathGPU"), path1);
            wxConfig::Get()->Write(wxT("GTPConfigPathGPU"), path2);
            wxConfig::Get()->Write(wxT("GTPModelPathGPU"), path3);
        } else if (engine_type == 1) {
            wxConfig::Get()->Write(wxT("AnalysisEnginePathGPU"), path1);
            wxConfig::Get()->Write(wxT("AnalysisConfigPathGPU"), path2);
            wxConfig::Get()->Write(wxT("AnalysisModelPathGPU"), path3);
        }
    }
#else
    int engine_type = m_radioBoxEngineType->GetSelection();
    wxConfig::Get()->Write(wxT("EngineTypeCPU"), (long)engine_type);

    if (engine_type != 0) {
        wxString path1 = m_filePickerEngine->GetPath();
        wxString path2 = m_filePickerConfigration->GetPath();
        wxString path3 = m_filePickerModel->GetPath();
        if (engine_type == 2) {
            wxConfig::Get()->Write(wxT("GTPEnginePathCPU"), path1);
            wxConfig::Get()->Write(wxT("GTPConfigPathCPU"), path2);
            wxConfig::Get()->Write(wxT("GTPModelPathCPU"), path3);
        } else if (engine_type == 1) {
            wxConfig::Get()->Write(wxT("AnalysisEnginePathCPU"), path1);
            wxConfig::Get()->Write(wxT("AnalysisConfigPathCPU"), path2);
            wxConfig::Get()->Write(wxT("AnalysisModelPathCPU"), path3);
        }
    }
#endif

    event.Skip();
}

void SettingsDialog::doChangeEngine(wxUpdateUIEvent& event) {
    int engine_type = m_radioBoxEngineType->GetSelection();
#ifdef USE_GPU
    wxConfig::Get()->Write(wxT("EngineTypeGPU"), (long)engine_type);
#else
    wxConfig::Get()->Write(wxT("EngineTypeCPU"), (long)engine_type);
#endif
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
            wxConfig::Get()->Read(wxT("AnalysisEnginePathCPU"), &str1);
            wxConfig::Get()->Read(wxT("AnalysisConfigPathCPU"), &str2);
            wxConfig::Get()->Read(wxT("AnalysisModelPathCPU"), &str3);
        } else if (engine_type == 2) {
            wxConfig::Get()->Read(wxT("GTPEnginePathCPU"), &str1);
            wxConfig::Get()->Read(wxT("GTPConfigPathCPU"), &str2);
            wxConfig::Get()->Read(wxT("GTPModelPathCPU"), &str3);
        }
        m_filePickerEngine->SetPath(str1);
        m_filePickerConfigration->SetPath(str2);
        m_filePickerModel->SetPath(str3);
        m_filePickerEngine->Enable();
        m_filePickerConfigration->Enable();
        m_filePickerModel->Enable();
    }
    event.Skip();
}
