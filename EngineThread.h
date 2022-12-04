#ifndef ENGINETHREAD_H
#define ENGINETHREAD_H

#include "stdafx.h"
#include "GameState.h"
#include "Utils.h"
#include <atomic>
#ifdef USE_THREAD
#include "json.hpp"
#include <vector>
#endif

class MainFrame;
using Utils::ThreadGroup;

class TEngineThread {
    public:
#ifdef USE_THREAD
        TEngineThread(GameState& gamestate,
                      MainFrame *frame,
                      wxProcess *process,
                      wxInputStream *std_in,
                      wxInputStream *std_err,
                      wxOutputStream *std_out,
                      std::vector<std::string>& overrideSettings,
                      const std::string& query_id,
                      std::mutex *GTPmutex);
#else
        TEngineThread(const GameState& gamestate, MainFrame *frame);
#endif
        void Wait();
        void Run();
        void limit_visits(int visits);
        void set_resigning(bool res);
        void set_analyzing(bool flag);
        void set_pondering(bool flag);
        void set_nopass(bool flag);
        void set_quiet(bool flag);
        void set_nets(bool flag);
#ifdef USE_THREAD
        void set_handi(std::vector<int> handi);
#endif
        void stop_engine(void);
        void kill_score_update(void);
        GameState& get_state(void) {
            return *m_state;
        }
    private:
#ifdef USE_THREAD
        void GTPSend(const wxString& sendCmd, std::string& inmsg, const int& sleep_ms=100);
        TimeControl m_tm;
#endif
        std::unique_ptr<GameState> m_state;
        MainFrame *m_frame;
#ifdef USE_THREAD
        wxProcess *m_process;
        wxInputStream *m_in;
        wxInputStream *m_err;
        wxOutputStream *m_out;
        std::string m_query_id;
#endif
        int m_maxvisits=0;
        bool m_nets;
        bool m_resigning;
        bool m_analyseflag;
        bool m_pondering;
        bool m_quiet;
        bool m_nopass;
        bool m_update_score;
#ifdef USE_THREAD
        std::vector<std::string> m_overrideSettings{};
        std::vector<int> m_handi;
#endif
        ThreadGroup m_tg{thread_pool};
        std::atomic<bool> m_runflag;
#ifdef USE_THREAD
        std::mutex *m_GTPmutex;
#endif
};

#endif