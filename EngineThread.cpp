#include "stdafx.h"

#include <thread>
#include "EngineThread.h"
#include "UCTSearch.h"
#include "MainFrame.h"
#include "Utils.h"
#include "MCOTable.h"

using namespace std;

TEngineThread::TEngineThread(const GameState& state, GTPKata * gtpKata, MainFrame * frame)
    :m_state(std::make_unique<GameState>(state)),
     m_frame(frame),
     m_maxvisits(0),
     m_nets(true),
     m_resigning(true),
     m_analyseflag(false),
     m_pondering(false),
     m_quiet(false),
     m_nopass(false),
     m_update_score(true),
     m_runflag(true),
     m_gtpKata(gtpKata)
{
}

void TEngineThread::Run() {
    auto Func = [this] {
        int who = m_state->get_to_move();

        if (!cfg_use_gtp) {
            auto search = make_unique<UCTSearch>(*m_state);

            if (m_analyseflag && !m_pondering) {
                search->set_playout_limit(0);
            } else {
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
            } else {
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
                auto scoretuple = make_tuple(movenum, get<0>(scores), get<1>(scores), get<2>(scores));
                event->SetClientData((void*)new auto(scoretuple));

                wxQueueEvent(m_frame->GetEventHandler(), event);
            }
            if (!m_analyseflag) {
                wxQueueEvent(m_frame->GetEventHandler(), new wxCommandEvent(wxEVT_NEW_MOVE));
            }
        } else if (m_pondering) {
            m_gtpKata->ponder();
        } else {
            // do some preprocessing for move ordering
            // Note: Playouts are required to display the "Moyo"
            MCOwnerTable::get_MCO()->clear();
            GameState& rootstate(*m_state);
            float mc_score = Playout::mc_owner(rootstate);

            double winrate;
            double scoreLead;
            Player pla = who == FastBoard::BLACK ? P_BLACK : P_WHITE;
            string response = m_gtpKata->gen_move(pla, winrate, scoreLead);

            if (response == "pass") {
                m_state->play_move(who, FastBoard::PASS);
            } else if (response == "resign") {
                m_state->play_move(who, FastBoard::RESIGN);
            } else {
                transform(response.begin(), response.end(), response.begin(), ::tolower);
                m_state->play_move(who, m_state->board.text_to_move(response));
            }

            int boardsize = m_state->board.get_boardsize();
            auto policy = m_gtpKata->get_policy();

            std::vector<float> conv_policy((boardsize + 2) * (boardsize + 2), 0.0);
            float maxProbability = 0.0f;
            float policyPass = 0.0f;
            int x, y, pos;
            for (const auto& pair : policy) {
                if (pair.second == boardsize * boardsize) {
                    policyPass = pair.first;
                    continue;
                }
                x = pair.second % boardsize;
                y = pair.second / boardsize;
                y = -1 * (y - boardsize) - 1;
                pos = m_state->board.get_vertex(x, y);
                conv_policy[pos] = pair.first;
                if (pair.first > maxProbability) {
                    maxProbability = pair.first;
                }
            }
            conv_policy[0] = maxProbability;
            conv_policy[1] = policyPass;
            m_state->m_policy.clear();
            for (auto itr = conv_policy.begin(); itr != conv_policy.end(); ++itr) {
                m_state->m_policy.emplace_back(*itr);
            }

            auto owners = m_gtpKata->get_owner();

            int vertex = 0;
            m_state->m_owner.clear();
            for (const auto& owner : owners) {
                x = vertex % boardsize;
                y = vertex / boardsize;
                y = -1 * (y - boardsize) - 1;
                pos = m_state->board.get_vertex(x, y);
                m_state->m_owner.emplace_back(-1 * (owner / 2) + 0.5);
                vertex++;
            }

            if (m_update_score) {
                // Broadcast result from search
                auto event = new wxCommandEvent(wxEVT_EVALUATION_UPDATE);
                auto movenum = m_state->get_movenum();
                float black_winrate;
                if (who == FastBoard::BLACK) {
                    black_winrate = winrate;
                } else {
                    black_winrate = -1.0 * (winrate - 1.0);
                }
                auto scoretuple = make_tuple(movenum, black_winrate, black_winrate, black_winrate);
                event->SetClientData((void*)new auto(scoretuple));

                wxQueueEvent(m_frame->GetEventHandler(), event);
            }

            Utils::GUIprintf(cfg_lang, _("Win rate:%3.1f%% Score:%.1f").utf8_str(), 100 - winrate * 100, -1 * scoreLead);

            if (!m_analyseflag) {
                wxQueueEvent(m_frame->GetEventHandler(), new wxCommandEvent(wxEVT_NEW_MOVE));
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