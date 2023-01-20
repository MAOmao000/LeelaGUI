#include "stdafx.h"
#include "MainFrame.h"

#ifndef USE_THREAD
SubProcess::SubProcess(MainFrame *parent) : wxProcess(parent) {
    m_parent = parent;
    Redirect();
}

SubProcess::~SubProcess() {

}

bool SubProcess::HasInput() {
    bool hasInput = false;
    try {
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
    } catch (const std::exception& e) {
        wxLogError(_("Exception at SubProcess::HasInput: %s %s\n"), typeid(e).name(), e.what());
        wxString mess;
        mess.Printf(_("Exception at SubProcess::HasInput: %s %s\n"), typeid(e).name(), e.what());
        auto event = new wxCommandEvent(wxEVT_RECIEVE_KATAGO);
        event->SetClientData((void*)new auto(mess));
        wxQueueEvent(m_parent->GetEventHandler(), event);
    }
    return hasInput;
}

void SubProcess::OnTerminate(int pid, int status) {

    while ( HasInput() )
        ;

    wxLogStatus(m_parent, "Process %u terminated with exit code %d.", pid, status);

    auto event = new wxCommandEvent(wxEVT_TERMINATED_KATAGO);
    wxQueueEvent(m_parent->GetEventHandler(), event);
}
#endif
