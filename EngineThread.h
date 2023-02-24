#ifndef ENGINETHREAD_H
#define ENGINETHREAD_H

#include "stdafx.h"
#include "GameState.h"
#include "Utils.h"
#include <atomic>

class MainFrame;
using Utils::ThreadGroup;

class TEngineThread {
    public:
        TEngineThread(GameState& gamestate,
                      MainFrame *frame,
                      wxProcess *process,
                      wxInputStream *std_in,
                      wxInputStream *std_err,
                      wxOutputStream *std_out,
                      std::vector<std::string>& overrideSettings,
                      const std::string& query_id,
                      const std::chrono::time_point<std::chrono::system_clock>& query_start,
                      std::mutex *GTPmutex);
        TEngineThread(const GameState& gamestate, MainFrame *frame);
        void Wait();
        void Run();
        void limit_visits(int visits);
        void set_resigning(bool res);
        void set_analyzing(bool flag);
        void set_pondering(bool flag);
        void set_nopass(bool flag);
        void set_quiet(bool flag);
        void set_nets(bool flag);
        void set_thinking(bool flag);
        bool get_thinking();

        void set_handi(std::vector<int> handi);
        void stop_engine(void);
        void kill_score_update(void);
        GameState& get_state(void) {
            return *m_state;
        }
    private:
        void GTPSend(const wxString& sendCmd, std::string& inmsg, const int& sleep_ms=100);
        TimeControl m_tm;
        wxProcess *m_process;
        wxInputStream *m_in;
        wxInputStream *m_err;
        wxOutputStream *m_out;
        std::string m_query_id;
        std::chrono::time_point<std::chrono::system_clock> m_query_start;
        std::vector<std::string> m_overrideSettings{};
        std::vector<int> m_handi;
        std::atomic<bool> m_thinking;
        std::mutex *m_GTPmutex;
        std::unique_ptr<GameState> m_state;
        MainFrame *m_frame;
        int m_maxvisits=0;
        bool m_nets;
        bool m_resigning;
        bool m_analyseflag;
        bool m_pondering;
        bool m_quiet;
        bool m_nopass;
        bool m_update_score;
        ThreadGroup m_tg{thread_pool};
        std::atomic<bool> m_runflag;
        bool m_katago_engine;
};
#endif