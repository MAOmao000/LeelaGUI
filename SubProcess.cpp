#include "stdafx.h"
#include "SubProcess.h"

SubProcess::SubProcess(MainFrame *parent,
                       int engine_type,
                       std::vector<wxString>& ini_line) : wxProcess(parent) {
    wxLogDebug(wxT("SubProcess start."));
    m_parent = parent;
    Redirect();
    m_katago_status = KATAGO_STARTING;
    m_engine_type = engine_type;
    m_send_msg.Clear();
    m_stack_msg.Clear();
    m_ini_line = ini_line;
    m_ini_line_size = m_ini_line.size();
}

SubProcess::~SubProcess() {
    wxLogDebug(wxT("SubProcess end."));
}

bool SubProcess::HasInput() {
    bool hasInput = false;
    try {
        if ( IsInputAvailable() ) {
            wxTextInputStream tis(*GetInputStream());
            wxString msg;
            msg << tis.ReadLine();
            if (!msg.IsEmpty()) {
                if (m_katago_status == KATAGO_STARTING) {
                    if (StartUpStdIn(msg)) {
                        auto event = new wxCommandEvent(wxEVT_RECIEVE_KATAGO);
                        event->SetClientData((void*)new auto(msg));
                        wxQueueEvent(m_parent->GetEventHandler(), event);
                    }
                } else if (m_katago_status == KATAGO_IDLE && msg == "= symmetry 0") {
                    m_stack_msg = msg + " ";
                    m_stack_row = 12;
                    m_row_getting = false;
                    m_col_num = 0;
                    m_katago_status = KATA_RAW_NN_RECIEVING;
                } else if (m_katago_status == KATA_RAW_NN_RECIEVING) {
                    if (KataRawNnRecieve(msg)) {
                        m_katago_status = KATAGO_IDLE;
                        auto event = new wxCommandEvent(wxEVT_RECIEVE_KATAGO);
                        event->SetClientData((void*)new auto(m_stack_msg));
                        wxQueueEvent(m_parent->GetEventHandler(), event);
                    }
                } else {
                    if (msg.find(R"("id":"dummy")") == std::string::npos) {
                        auto event = new wxCommandEvent(wxEVT_RECIEVE_KATAGO);
                        event->SetClientData((void*)new auto(msg));
                        wxQueueEvent(m_parent->GetEventHandler(), event);
                    }
                }
            }
            hasInput = true;
        }
        if ( IsErrorAvailable() ) {
            wxTextInputStream tis(*GetErrorStream());
            wxString msg;
            msg << tis.ReadLine();
            if (!msg.IsEmpty()) {
                int offset = 0;
                if (msg.Matches("2???-??-??*")) {
                    offset = 26;
                }
                if (m_katago_status == KATAGO_STARTING) {
                    StartUpStdErr(msg, offset);
                    auto event = new wxCommandEvent(wxEVT_RECIEVE_KATAGO);
                    event->SetClientData((void*)new auto(msg));
                    wxQueueEvent(m_parent->GetEventHandler(), event);
                }
            }
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

bool SubProcess::StartUpStdIn(wxString& msg) {
    if (m_send_msg.find(R"("id":"dummy")") != std::string::npos) {
        if (msg.find(R"("error":)") != std::string::npos) {
            m_board25 = 0;
        }
        msg.Printf(wxT("START OK(board25:%d,japanese_rule:%d)"), m_board25, m_japanese_rule);
        m_katago_status = KATAGO_IDLE;
        return true;
    } else if (m_send_msg == "kata-get-rules\n") {
        if (msg.StartsWith("=")) {
            if (msg.find(R"("scoring":"AREA")") == std::string::npos) {
                m_japanese_rule = 1;
            }
        } else {
            m_parent->SetStatusBarText(msg, 1);
        }
        wxTextOutputStream os(*GetOutputStream());
        m_send_msg = "boardsize 25\n";
        os.WriteString(m_send_msg);
        return false;
    } else if (m_send_msg == "boardsize 25\n") {
        if (msg.StartsWith("?")) {
            m_board25 = 0;
        }
        msg.Printf(wxT("START OK(board25:%d,japanese_rule:%d)"), m_board25, m_japanese_rule);
        m_katago_status = KATAGO_IDLE;
        return true;
    } else if (m_ini_line_idx < m_ini_line_size) {
        wxTextOutputStream os(*GetOutputStream());
        m_send_msg = m_ini_line[m_ini_line_idx] + "\n";
        os.WriteString(m_send_msg);
        m_ini_line_idx++;
        return false;
    } else {
        wxTextOutputStream os(*GetOutputStream());
        m_send_msg = "kata-get-rules\n";
        os.WriteString(m_send_msg);
        return false;
    }
    return false;
}

void SubProcess::StartUpStdErr(wxString& msg, int offset) {
    if (msg.find("Uncaught exception:") != std::string::npos ||
        msg.find("PARSE ERROR:") != std::string::npos ||
        msg.find("failed with error") != std::string::npos ||
        msg.find(": error while loading shared libraries:") != std::string::npos ||
        msg.find("what():") != std::string::npos) {

        m_parent->SetStatusBarText(msg.substr(offset), 1);
        wxLogError(_("Error was returned from KataGo engine: %s"), msg.substr(offset).mb_str());
        m_katago_status = INIT;
    } else if (m_engine_type == GTP::ANALYSIS &&
        msg.find("Started, ready to begin handling requests") != std::string::npos) {

        wxTextOutputStream os(*GetOutputStream());
        m_send_msg = R"({"boardXSize":25,"boardYSize":25,"id":"dummy",)";
        m_send_msg += R"("maxVisits":1,"moves":[],"rules":"chinese"})";
        m_send_msg += "\n";
        os.WriteString(m_send_msg);
    } else if (m_engine_type == GTP::GTP_INTERFACE &&
        msg.find("GTP ready, beginning main protocol loop") != std::string::npos) {

        wxTextOutputStream os(*GetOutputStream());
        if (m_ini_line_size > 1) {
            m_send_msg = m_ini_line[1] + "\n";
        } else {
            m_send_msg = "kata-get-rules\n";
        }
        os.WriteString(m_send_msg);
    }
    m_parent->SetStatusBarText(msg.substr(offset), 1);
}

bool SubProcess::KataRawNnRecieve(wxString& msg) {
    if (msg == "policy" || msg == "whiteOwnership") {
        m_stack_msg += msg + " ";
        m_row_getting = true;
        m_row_num = 0;
    } else if (m_row_getting) {
        if (m_col_num == 0) {
            bool prev_sp = true;
            for (auto i = 0; i < msg.length(); i++) {
                if (msg[i] == (const char)' ') {
                    prev_sp = true;
                } else if (prev_sp) {
                    prev_sp = false;
                    m_col_num++;
                }
            }
            if (m_col_num < 7) {
                m_col_num = 0;
                return (m_stack_row == 0);
            }
        }
        m_stack_msg += msg;
        m_row_num++;
        if (m_row_num >= m_col_num) {
            m_stack_row--;
        }
    } else if (msg.StartsWith("policyPass ")) {
        m_stack_msg += msg;
        m_stack_row--;
    } else {
        m_stack_msg += msg + " ";
        m_stack_row--;
    }
    return (m_stack_row == 0);
}
