#include "stdafx.h"
#include "AboutDialog.h"
#include "config.h"
#include "Network.h"

AboutDialog::AboutDialog( wxWindow* parent )
:
TAboutDialog( parent )
{
    
}

void AboutDialog::doInit( wxInitDialogEvent& event ) {
#ifdef JP
    m_staticTextVersion->SetLabel(wxString(wxT("Leela バージョン ")) + wxString(wxT(PROGRAM_VERSION)));
#else
    m_staticTextVersion->SetLabel(wxString(_("Leela version ")) + wxString(wxT(PROGRAM_VERSION)));
#endif
    m_staticTextEngine->SetLabel(wxString(Network::get_Network()->get_backend()));
    Fit();
}