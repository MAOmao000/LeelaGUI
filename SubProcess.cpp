#include "stdafx.h"
#include "MainFrame.h"

#ifndef USE_THREAD
SubProcess::SubProcess(MainFrame *parent) : wxProcess(parent) {
    m_parent = parent;
    Redirect();
}

bool SubProcess::HasInput() {
    bool hasInput = false;
    if ( IsInputAvailable() ) {
        wxTextInputStream tis(*GetInputStream());
        wxString msg;
        msg << tis.ReadLine();
        auto event = new wxCommandEvent(wxEVT_RECIEVE_KATAGO);
        event->SetClientData((void*)new auto(msg));
        wxQueueEvent(m_parent->GetEventHandler(), event);
        hasInput = true;
    }
    if ( IsErrorAvailable() ) {
        wxTextInputStream tis(*GetErrorStream());
        wxString msg;
        msg << "(stderr):" << tis.ReadLine();
        auto event = new wxCommandEvent(wxEVT_RECIEVE_KATAGO);
        event->SetClientData((void*)new auto(msg));
        wxQueueEvent(m_parent->GetEventHandler(), event);
        hasInput = true;
    }
    return hasInput;
}

void SubProcess::OnTerminate(int pid, int status) {

    while ( HasInput() )
        ;

    m_parent->OnProcessTerminated(this);

    wxLogStatus(m_parent, "Process %u terminated with exit code %d.", pid, status);

    m_parent->OnAsyncTermination(this);
}
#endif
