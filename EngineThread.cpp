#include "stdafx.h"

#include <thread>
#include "EngineThread.h"
#include "UCTSearch.h"
#include "MainFrame.h"
#include "Utils.h"

TEngineThread::TEngineThread(const GameState& state, GTPKata * gtpKata, MainFrame * frame) {
    m_state = std::make_unique<GameState>(state);
    m_frame = frame;
    m_maxvisits = 0;
    m_runflag = true;
    m_nopass = false;
    m_quiet = false;
    m_nets = true;
    m_update_score = true;
    m_gtpKata = gtpKata;
}

void TEngineThread::Run() {
    auto Func = [this] {
        double winRate;
        double scoreLead;
        auto search = std::make_unique<UCTSearch>(*m_state);

        int who = m_state->get_to_move();

        if (m_gtpKata == nullptr || (m_analyseflag && !m_pondering)) {
            if (m_analyseflag && !m_pondering) {
                search->set_playout_limit(0);
            }
            else {
                search->set_playout_limit(m_maxvisits);
            }
            search->set_runflag(&m_runflag);
            search->set_analyzing(m_analyseflag);
            search->set_quiet(m_quiet);
            search->set_use_nets(m_nets);

            int mode = UCTSearch::NORMAL;
            if (m_nopass) {
                mode = UCTSearch::NOPASS;
            }

            int move;
            if (m_resigning) {
                move = search->think(who, mode);
            }
            else {
                move = search->think(who, mode | UCTSearch::NORESIGN);
            }

            if (!m_analyseflag) {
                m_state->play_move(who, move);
            }
            if (m_update_score) {
                // Broadcast result from search
                auto event = new wxCommandEvent(wxEVT_EVALUATION_UPDATE);
                auto scores = search->get_scores();
                auto movenum = m_state->get_movenum();
                auto scoretuple = std::make_tuple(movenum,
                    std::get<0>(scores),
                    std::get<1>(scores),
                    std::get<2>(scores));
                event->SetClientData((void*)new auto(scoretuple));

                wxQueueEvent(m_frame->GetEventHandler(), event);
            }
            if (!m_analyseflag) {
                wxQueueEvent(m_frame->GetEventHandler(),
                    new wxCommandEvent(wxEVT_NEW_MOVE));
            }
        } else if (m_pondering) {
            m_gtpKata->ponder();
        } else {
            Player pla = who == FastBoard::BLACK ? P_BLACK : P_WHITE;
            string response = m_gtpKata->gen_move(pla, winRate, scoreLead);
            Utils::GUIprintf(cfg_lang, _("Win rate:%3.1f%% Score:%.1f").utf8_str(), 100 - winRate*100, -1 * scoreLead);

            if (response == "pass") {
                m_state->play_move(who, FastBoard::PASS);
            } else if (response == "resign") {
                m_state->play_move(who, FastBoard::RESIGN);
            } else {
                using namespace std;
                transform(response.begin(), response.end(), response.begin(), ::tolower);
                string str_who = who == FastBoard::BLACK ? "b" : "w";
                m_state->play_textmove(str_who, response);
            }

            if (m_update_score) {
                // Broadcast result from search
                auto event = new wxCommandEvent(wxEVT_EVALUATION_UPDATE);
                auto movenum = m_state->get_movenum() + 1;
                float black_winrate;
                if (who == FastBoard::BLACK) {
                    black_winrate = winRate;
                } else {
                    black_winrate = -1.0 * (winRate - 1.0);
                }
                auto scoretuple = std::make_tuple(movenum,
                                                  black_winrate,
                                                  black_winrate,
                                                  black_winrate);
                event->SetClientData((void*)new auto(scoretuple));

                wxQueueEvent(m_frame->GetEventHandler(), event);
            }
            if (!m_analyseflag) {
                wxQueueEvent(m_frame->GetEventHandler(),
                    new wxCommandEvent(wxEVT_NEW_MOVE));
            }
        }
    };

    m_tg.add_task(Func);
}

void TEngineThread::Wait() {
    m_tg.wait_all();
}

void TEngineThread::limit_visits(int visits) {
    m_maxvisits = visits;
}

void TEngineThread::stop_engine() {
    m_runflag = false;
}

void TEngineThread::set_resigning(bool res) {
    m_resigning = res;
}

void TEngineThread::set_analyzing(bool flag) {
    m_analyseflag = flag;
}

void TEngineThread::set_pondering(bool flag) {
    m_pondering = flag;
}

void TEngineThread::set_nopass(bool flag) {
    m_nopass = flag;
}

void TEngineThread::set_quiet(bool flag) {
    m_quiet = flag;
}

void TEngineThread::set_nets(bool flag) {
    m_nets = flag;
}

void TEngineThread::kill_score_update(void) {
    m_update_score = false;
}