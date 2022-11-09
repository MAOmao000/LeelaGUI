#include "stdafx.h"
#include "MainFrame.h"

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
        //wxCommandEvent event(wxEVT_RECIEVE_KATAGO);//, GetId());
        auto event = new wxCommandEvent(wxEVT_RECIEVE_KATAGO);
        event->SetClientData((void*)new auto(msg));
        wxQueueEvent(m_parent->GetEventHandler(), event);
        //event.SetEventObject(this);
        //event.SetString(msg);
        //::wxPostEvent(GetEventHandler(), event);
        //::ProcessWindowEvent(event);
        /*
        std::string msg_s = msg.ToStdString();
        FILE* outputfile;
        outputfile = fopen("pipe.log", "a");
        if (outputfile != NULL) {
            fprintf(outputfile, "(2) %zd %s\n", msg.length(), (const char*)msg_s.c_str());
            fflush(outputfile);
            fclose(outputfile);
        }
        */
        hasInput = true;
    }
    if ( IsErrorAvailable() ) {
        wxTextInputStream tis(*GetErrorStream());
        wxString msg;
        msg << "(stderr):" << tis.ReadLine();
        //wxCommandEvent event(wxEVT_RECIEVE_KATAGO);//, GetId());
        auto event = new wxCommandEvent(wxEVT_RECIEVE_KATAGO);
        event->SetClientData((void*)new auto(msg));
        wxQueueEvent(m_parent->GetEventHandler(), event);
        //event.SetEventObject(this);
        //event.SetString(msg);
        //::wxPostEvent(GetEventHandler(), event);
        //::ProcessWindowEvent(event);
        hasInput = true;
    }
    return hasInput;
}

void SubProcess::OnTerminate(int pid, int status) {

    while ( HasInput() )
        ;

    m_parent->OnProcessTerminated(this);

    wxLogStatus(m_parent, "Process %u ('%s') terminated with exit code %d.",
                pid, status);

    m_parent->OnAsyncTermination(this);
}
