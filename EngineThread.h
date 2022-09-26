#ifndef ENGINETHREAD_H
#define ENGINETHREAD_H

#include "stdafx.h"
#include "GameState.h"
#include "Utils.h"
#include <atomic>
#include <vector>

class MainFrame;
using Utils::ThreadGroup;

class TEngineThread {
    public:
        TEngineThread(const GameState& gamestate,
                      MainFrame * frame,
                      wxInputStream *std_in,
                      wxOutputStream *std_out);
        void Wait();
        void Run();
        void limit_visits(int visits);
        void set_resigning(bool res);
        void set_analyzing(bool flag);
        void set_pondering(bool flag);
        void set_nopass(bool flag);
        void set_quiet(bool flag);
        void set_nets(bool flag);
        void set_handi(std::vector<int> handi);
        void stop_engine(void);
        void force_stop_engine(void);
        void kill_score_update(void);
        GameState& get_state(void) {
            return *m_state;
        }
    private:
        void kata_raw_nn(void);
        void GTPSend(const wxString& sendCmd, std::string& inmsg, const int& sleep_ms=500);
        std::unique_ptr<GameState> m_state;
        MainFrame * m_frame;
        wxInputStream *m_in;
        wxOutputStream *m_out;
        int m_maxvisits;
        bool m_nets;
        bool m_resigning;
        bool m_analyseflag;
        bool m_pondering;
        bool m_quiet;
        bool m_nopass;
        bool m_update_score;
        std::vector<int> m_handi;
        ThreadGroup m_tg{thread_pool};
        std::atomic<bool> m_runflag;
        std::atomic<bool> m_stopflag;
};

#endif