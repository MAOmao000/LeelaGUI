#include "ScoreDialog.h"

ScoreDialog::ScoreDialog( wxWindow* parent, wxString message,
                          wxString confidence, bool dispute )
:
TScoreDialog( parent )
{
    m_staticTextMessage->SetLabel(message);
    m_staticTextConfidence->SetLabel(confidence);
    if (!dispute) {
        m_buttonDispute->Show(false);
    }
    this->Layout();
}

void ScoreDialog::doAccept( wxCommandEvent& event )
{
    mAccepts = true;
    EndModal(wxID_OK);
}

void ScoreDialog::doDispute( wxCommandEvent& event )
{
    mAccepts = false;
    EndModal(wxID_OK);
}
