#include "stdafx.h"
#include "AboutDialog.h"
#include "config.h"
#include "Network.h"
#include "GTP.h"
#include "Msg.h"

AboutDialog::AboutDialog( wxWindow* parent )
:
TAboutDialog( parent )
{
    
}

void AboutDialog::doInit( wxInitDialogEvent& event ) {
    m_staticTextVersion->SetLabel(wxString(_("Leela ")) + VERSION_WXSTR[cfg_lang] + wxString(_(" ")) + wxString(wxT(PROGRAM_VERSION)));
    m_staticTextEngine->SetLabel(wxString(Network::get_Network()->get_backend()));

    Fit();
}