#include "stdafx.h"
#include "NewGameDialog.h"
#include "UCTSearch.h"
#include "GTP.h"

NewGameDialog::NewGameDialog( wxWindow* parent )
:
TNewGameDialog( parent )
{
                  
}

void NewGameDialog::doInit( wxInitDialogEvent& event ) {    
    int size = wxConfig::Get()->ReadLong(wxT("DefaultBoardSize"), (long)4);
    m_radioBoxBoardSize->SetSelection(size);
    
    int handicap = wxConfig::Get()->ReadLong(wxT("DefaultHandicap"), (long)0);
    m_spinCtrlHandicap->SetValue(handicap);
    
    int komi = wxConfig::Get()->ReadLong(wxT("DefaultKomi"), (long)7);
    m_spinCtrlKomi->SetValue(komi);
    
    int use_engine = 0;
    if (cfg_use_engine == GTP::ORIGINE_ENGINE) {
        m_radioBoxEngine->SetSelection(0);
        m_radioBoxEngine->Enable(false);
    } else {
        use_engine = wxConfig::Get()->ReadLong(wxT("useEngine"), (long)0);
        m_radioBoxEngine->SetSelection(use_engine);
    }

    int simulations;
    if (use_engine == 0) {
        m_radioBoxRule->SetSelection(0);
        m_radioBoxRule->Enable(1, false);
        
        simulations = wxConfig::Get()->ReadLong(wxT("DefaultSimulations"), (long)6);
        m_radioBoxLevel->SetSelection(simulations);
        
        int simulations_num = wxConfig::Get()->ReadLong(wxT("DefaultSimulationsNum"), (long)250);
        m_spinCtrlLevel->SetValue(simulations_num);
        
        int minutes = wxConfig::Get()->ReadLong(wxT("DefaultMinutes"), (long)20);
        m_spinCtrlTime->SetValue(minutes);
        
        int byo = wxConfig::Get()->ReadLong(wxT("DefaultByo"), (long)2);
        m_spinCtrlByo->SetValue(byo);
    } else {
        int rule = wxConfig::Get()->ReadLong(wxT("NewGameRule"), (long)4);
        m_radioBoxRule->SetSelection(rule);
        
        simulations = wxConfig::Get()->ReadLong(wxT("DefaultSimulationsKataGo"), (long)6);
        m_radioBoxLevel->SetSelection(simulations);
        
        int simulations_num = wxConfig::Get()->ReadLong(wxT("DefaultSimulationsNumKataGo"), (long)250);
        m_spinCtrlLevel->SetValue(simulations_num);
        
        int minutes = wxConfig::Get()->ReadLong(wxT("DefaultMinutesKataGo"), (long)20);
        m_spinCtrlTime->SetValue(minutes);
        
        int byo = wxConfig::Get()->ReadLong(wxT("DefaultByoKataGo"), (long)2);
        m_spinCtrlByo->SetValue(byo);
    }
    
    int color = wxConfig::Get()->ReadLong(wxT("DefaultColor"), (long)0);
    m_radioBoxColor->SetSelection(color);

    bool nets = wxConfig::Get()->Read(wxT("netsEnabled"), true);
    m_checkNeuralNet->SetValue(nets);

    if (cfg_use_engine == GTP::KATAGO_ENGINE) {
        m_checkNeuralNet->Enable(false);
    }

    if (!cfg_board25) {
        wxString sboardsize = m_radioBoxBoardSize->GetStringSelection();
        if (sboardsize == "25 x 25") {
            m_radioBoxBoardSize->SetSelection( 4 );
        }
        m_radioBoxBoardSize->Enable(5, false);
    }

    if (simulations != 7) {
         m_spinCtrlLevel->Enable(false);
    }

    checkNetsEnabled();
    checkHandicapRange();
}

void NewGameDialog::doLevel( wxCommandEvent& WXUNUSED(event) ) {
    int simuls = m_radioBoxLevel->GetSelection();
    if (simuls != 7) {
        m_spinCtrlLevel->Enable(false);
    } else {
        m_spinCtrlLevel->Enable(true);
    }
}

void NewGameDialog::doCancel( wxCommandEvent& event ) {
    event.Skip();
}

void NewGameDialog::doOK( wxCommandEvent& event ) {
    int size = m_radioBoxBoardSize->GetSelection();
    wxConfig::Get()->Write(wxT("DefaultBoardSize"), (long)size);

    int simulations = m_radioBoxLevel->GetSelection();
    int simulations_num = m_spinCtrlLevel->GetValue();
    int use_engine = 0;
    if (cfg_use_engine != GTP::ORIGINE_ENGINE) {
        use_engine = wxConfig::Get()->ReadLong(wxT("useEngine"), (long)0);
        m_radioBoxEngine->SetSelection(use_engine);
        wxConfig::Get()->Write(wxT("useEngine"), (long)use_engine);
    }
    if (use_engine == 0) {
        wxConfig::Get()->Write(wxT("DefaultSimulations"), (long)simulations);
        wxConfig::Get()->Write(wxT("DefaultSimulationsNum"), (long)simulations_num);
    } else {
        int rule = m_radioBoxRule->GetSelection();
        wxConfig::Get()->Write(wxT("NewGameRule"), (long)rule);
        wxConfig::Get()->Write(wxT("DefaultSimulationsKataGo"), (long)simulations);
        wxConfig::Get()->Write(wxT("DefaultSimulationsNumKataGo"), (long)simulations_num);
    }

    int color = m_radioBoxColor->GetSelection();
    wxConfig::Get()->Write(wxT("DefaultColor"), (long)color);

    int handicap = m_spinCtrlHandicap->GetValue();
    wxConfig::Get()->Write(wxT("DefaultHandicap"), (long)handicap);

    int komi = m_spinCtrlKomi->GetValue();
    wxConfig::Get()->Write(wxT("DefaultKomi"), (long)komi);

    int minutes = m_spinCtrlTime->GetValue();
    int byo = m_spinCtrlByo->GetValue();
    if (cfg_use_engine == GTP::ORIGINE_ENGINE) {
        wxConfig::Get()->Write(wxT("DefaultMinutes"), (long)minutes);
        wxConfig::Get()->Write(wxT("DefaultByo"), (long)byo);
    } else {
        wxConfig::Get()->Write(wxT("DefaultMinutesKataGo"), (long)minutes);
        wxConfig::Get()->Write(wxT("DefaultByoKataGo"), (long)byo);
    }

    bool nets = m_checkNeuralNet->GetValue();
    wxConfig::Get()->Write(wxT("netsEnabled"), nets);

    if (simulations != 7) {
         m_spinCtrlLevel->Enable(false);
    }

    event.Skip();
}

void NewGameDialog::doChangeEngine(wxCommandEvent& WXUNUSED(event)) {
    int use_engine = m_radioBoxEngine->GetSelection();
    wxConfig::Get()->Write(wxT("useEngine"), (long)use_engine);

    if (use_engine == 0) {
        m_radioBoxRule->SetSelection(0);
        m_radioBoxRule->Enable(1, false);
    } else {
        m_radioBoxRule->Enable(1, true);
        int rule = wxConfig::Get()->ReadLong(wxT("NewGameRule"), (long)4);
        m_radioBoxRule->SetSelection(rule);
    }
}

void NewGameDialog::doChangeRule(wxCommandEvent& WXUNUSED(event)) {
    if (cfg_use_engine != GTP::ORIGINE_ENGINE) {
        int use_engine = m_radioBoxEngine->GetSelection();
        if (use_engine == 1) {
            int rule = m_radioBoxRule->GetSelection();
            wxConfig::Get()->Write(wxT("NewGameRule"), (long)rule);
        }
    }
}

float NewGameDialog::getKomi() {
    float komi = m_spinCtrlKomi->GetValue();
    return komi + 0.5f;
}

int NewGameDialog::getHandicap() {
    int handicap = m_spinCtrlHandicap->GetValue();
    return handicap;
}

int NewGameDialog::getBoardsize() {
    wxString sboardsize = m_radioBoxBoardSize->GetStringSelection();
    if (sboardsize == "7 x 7") {
        return 7;
    } else if (sboardsize == "9 x 9") {
        return 9;
    } else if (sboardsize == "13 x 13") {
        return 13;
    } else if (sboardsize == "17 x 17") {
        return 17;
    } else if (sboardsize == "19 x 19") {
        return 19;
    } else if (sboardsize == "25 x 25") {
        return 25;
    }

    throw std::runtime_error("Invalid board size selection");
}

int NewGameDialog::simulationsToVisitLimit(int simuls) {
    if (simuls == 0) {
        return 100;
    } else if (simuls == 1) {
        return 500;
    } else if (simuls == 2) {
        return 1000;
    } else if (simuls == 3) {
        return 5000;
    } else if (simuls == 4) {
        return 10000;
    } else if (simuls == 5) {
        return 20000;
    } else if (simuls == 6) {
        return 0;
    } else {
        throw std::runtime_error("Invalid simulations level selection");
    }
}

int NewGameDialog::getSimulations() {
    int simuls = m_radioBoxLevel->GetSelection();
    if (simuls != 7) {
        return simulationsToVisitLimit(simuls);
    } else {
        return m_spinCtrlLevel->GetValue();
    }
}

int NewGameDialog::getPlayerColor() {
    int color = m_radioBoxColor->GetSelection();
    
    return color;
}

int NewGameDialog::getTimeControl() {
    return m_spinCtrlTime->GetValue();
}

int NewGameDialog::getByoControl() {
    return m_spinCtrlByo->GetValue();
}

void NewGameDialog::doHandicapUpdate( wxSpinEvent& event ) {    
    m_spinCtrlKomi->SetValue(0);
}

bool NewGameDialog::getNetsEnabled() {
    return m_checkNeuralNet->GetValue();
}

void NewGameDialog::doRadioBox(wxCommandEvent& WXUNUSED(event)) {
    checkNetsEnabled();
    checkHandicapRange();
}

void NewGameDialog::checkNetsEnabled() {
    wxString sboardsize = m_radioBoxBoardSize->GetStringSelection();

    if (cfg_use_engine != GTP::ORIGINE_ENGINE || sboardsize != "19 x 19") {
        m_checkNeuralNet->Enable(false);
    } else {
        m_checkNeuralNet->Enable(true);
    }
}

void NewGameDialog::checkHandicapRange() {
    wxString sboardsize = m_radioBoxBoardSize->GetStringSelection();

    if (sboardsize == "7 x 7") {
        m_spinCtrlHandicap->SetRange(0, 4);
    } else if (sboardsize == "9 x 9") {
        m_spinCtrlHandicap->SetRange(0, 9);
    } else {
        m_spinCtrlHandicap->SetRange(0, 100);
    }
}
