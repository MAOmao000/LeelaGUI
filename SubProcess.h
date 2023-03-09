#ifndef SUBPROCESS_H
#define SUBPROCESS_H

#include "MainFrame.h"


class MainFrame;

class SubProcess : public wxProcess {
    public:
         SubProcess(MainFrame *parent, int engine_type, std::vector<wxString>& ini_line);
        ~SubProcess();
        virtual void OnTerminate(int pid, int status) wxOVERRIDE;
        bool HasInput();
    private:
        bool StartUpStdIn(wxString& msg);
        void StartUpStdErr(wxString& msg, int offset);
        bool KataRawNnRecieve(wxString& msg);
        MainFrame *m_parent;
        int m_engine_type;
        static constexpr int INIT = 0;
        static constexpr int KATAGO_STARTING = 1;
        static constexpr int KATAGO_IDLE = 2;
        static constexpr int KATA_RAW_NN_RECIEVING = 3;
        int m_katago_status{INIT};
        wxString m_send_msg;
        wxString m_stack_msg;
        std::vector<wxString> m_ini_line;
        size_t m_ini_line_size{0};
        size_t m_ini_line_idx{1};
        int m_japanese_rule{0};
        int m_board25{1};
        int m_stack_row{12};
        int m_col_num{0};
        int m_row_num{0};
        bool m_row_getting{false};
};

#endif
