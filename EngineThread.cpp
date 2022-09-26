#include "stdafx.h"

#include <thread>
#include <chrono>
#include <regex>
#include "EngineThread.h"
#include "UCTSearch.h"
#include "MainFrame.h"
#include "Utils.h"
#include "MCOTable.h"

using namespace std;
using std::this_thread::sleep_for;

TEngineThread::TEngineThread(const GameState& state,
                             MainFrame * frame,
                             wxInputStream *std_in,
                             wxOutputStream *std_out,
                             nlohmann::json& overrideSettings)
    :m_state(std::make_unique<GameState>(state)),
     m_frame(frame),
     m_in(std_in),
     m_out(std_out),
     m_overrideSettings(overrideSettings),
     m_maxvisits(0),
     m_nets(true),
     m_resigning(true),
     m_analyseflag(false),
     m_pondering(false),
     m_quiet(false),
     m_nopass(false),
     m_update_score(true),
     m_runflag(true),
     m_stopflag(false)
{
}

void TEngineThread::Run() {
    auto Func = [this] {
        int who = m_state->get_to_move();

        if (cfg_use_engine == GTP::ORIGINE_ENGINE) {
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
            return;
        } else if (cfg_use_engine == GTP::USE_KATAGO_GTP) {
            // do some preprocessing for move ordering
            // Note: Playouts are required to display the "Moyo"
            MCOwnerTable::get_MCO()->clear();
            GameState& rootstate(*m_state);
            float mc_score = Playout::mc_owner(rootstate);

            wxString sendCmd;
            if (who == FastBoard::BLACK) {
                sendCmd = wxString("genmove_analyze b\n\n");
            } else {
                sendCmd = wxString("genmove_analyze w\n\n");
            }
            string inmsg;
            GTPSend(sendCmd, inmsg);
            if (!inmsg.length()) {
                return;
            }
            string move_str = "";
            string winrate_str = "";
            string scoreMean_str = "";
            string::size_type pos = inmsg.rfind("play ");
            if (pos == string::npos) {
                return;
            }
            for (int i = pos + sizeof("play ") - 1; i < inmsg.length(); i++) {
                if (inmsg[i] != ' ' && inmsg[i] != '\r' && inmsg[i] != '\n') {
                    move_str += inmsg[i];
                } else {
                    break;
                }
            }
            if (move_str.length() > 0) {
                string find_str = "move " + move_str;
                if (move_str != "pass" && move_str != "resign") {
                    pos = inmsg.find(find_str);
                    if (pos == string::npos) {
                        pos = 0;
                    }
                } else {
                    pos = 0;
                }
            } else {
                return;
            }
            pos = inmsg.find("winrate ", pos);
            if (pos == string::npos) {
                pos = 0;
                winrate_str = "0.0";
            } else {
                for (int i = pos + sizeof("winrate ") - 1; i < inmsg.length(); i++) {
                    if (inmsg[i] != ' ' && inmsg[i] != '\r' && inmsg[i] != '\n') {
                        winrate_str += inmsg[i];
                    } else {
                        break;
                    }
                }
            }
            pos = inmsg.find("scoreMean ", pos);
            if (pos == string::npos) {
                scoreMean_str = "0.0";
            } else {
                for (int i = pos + sizeof("scoreMean ") - 1; i < inmsg.length(); i++) {
                    if (inmsg[i] != ' ' && inmsg[i] != '\r' && inmsg[i] != '\n') {
                        scoreMean_str += inmsg[i];
                    } else {
                        break;
                    }
                }
            }
            if (move_str.length() > 0) {
                if (move_str == "pass") {
                    m_state->play_move(who, FastBoard::PASS);
                } else if (move_str == "resign") {
                    m_state->play_move(who, FastBoard::RESIGN);
                } else {
                    m_state->play_move(who, m_state->board.text_to_move(move_str));
                }
            }

            kata_raw_nn();

            float winrate = stof(winrate_str);
            float scoreMean = stof(scoreMean_str);
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

            Utils::GUIprintf(cfg_lang, _("Win rate:%3.1f%% Score:%.1f").utf8_str(), 100 - winrate * 100, -1 * scoreMean);

            if (!m_analyseflag) {
                wxQueueEvent(m_frame->GetEventHandler(), new wxCommandEvent(wxEVT_NEW_MOVE));
            }
        } else {
            // do some preprocessing for move ordering
            // Note: Playouts are required to display the "Moyo"
            MCOwnerTable::get_MCO()->clear();
            GameState& rootstate(*m_state);
            float mc_score = Playout::mc_owner(rootstate);

            int board_size = m_state->board.get_boardsize();
            nlohmann::json send_1_json = nlohmann::json::parse(R"(
                {
                    "id":"node_0",
                    "rules":"Chinese",
                    "komi":7.5,
                    "whiteHandicapBonus":"N",
                    "boardXSize":19,
                    "boardYSize":19,
                    "initialStones":[],
                    "initialPlayer":"B",
                    "moves":[]
                }
            )");
            send_1_json["id"] = "node_" + std::to_string(m_state->get_movenum());
            send_1_json["komi"] = m_state->get_komi();
            send_1_json["boardXSize"] = board_size;
            send_1_json["boardYSize"] = board_size;
            if (m_overrideSettings != NULL) {
                send_1_json["overrideSettings"] = m_overrideSettings;
            }
            int i = 0;
            if (m_handi.size() > 0) {
                for (auto itr = m_handi.begin(); itr != m_handi.end(); ++itr) {
                    std::string handi_move = m_state->move_to_text(*itr);
                    send_1_json["initialStones"][i][0] = "B";
                    send_1_json["initialStones"][i][1] = handi_move;
                    i++;
                }
            }
            std::unique_ptr<GameState> state(new GameState);
            *state = *m_state;
            state->rewind();
            i = 0;
            if (m_state->get_movenum() > 0) {
                while (state->forward_move()) {
                    int move = state->get_last_move();
                    if (move == FastBoard::RESIGN) {
                        break;
                    }
                    std::string movestr = state->board.move_to_text(move);
                    if (state->get_to_move() == FastBoard::BLACK) {
                        send_1_json["moves"][i][0] = "W";
                    } else {
                        send_1_json["moves"][i][0] = "B";
                    }
                    send_1_json["moves"][i][1] = movestr;
                    i++;
                }
            }
            string req_query = send_1_json.dump();
            string res_query;
            GTPSend(req_query + "\n", res_query);
            if (!res_query.length()) {
                return;
            }
            nlohmann::json res_1_json = nlohmann::json::parse(res_query);
            string move_str = res_1_json["moveInfos"][0]["move"];
            if (move_str.length() > 0) {
                if (move_str == "pass") {
                    m_state->play_move(who, FastBoard::PASS);
                } else if (move_str == "resign") {
                    m_state->play_move(who, FastBoard::RESIGN);
                } else {
                    m_state->play_move(who, m_state->board.text_to_move(move_str));
                }
            }

            nlohmann::json send_2_json = nlohmann::json::parse(R"(
                {
                    "id":"node_0",
                    "rules":"Chinese",
                    "komi":7.5,
                    "whiteHandicapBonus":"N",
                    "boardXSize":19,
                    "boardYSize":19,
                    "includeOwnership":true,
                    "includePolicy":true,
                    "overrideSettings":
                        {
                            "maxVisits":1
                        },
                    "initialStones":[],
                    "initialPlayer":"B",
                    "moves":[]
                }
            )");
            send_2_json["id"] = "node_" + std::to_string(m_state->get_movenum());
            send_2_json["komi"] = m_state->get_komi();
            send_2_json["boardXSize"] = board_size;
            send_2_json["boardYSize"] = board_size;
            send_2_json["initialStones"] = send_1_json["initialStones"];
            send_2_json["moves"] = send_1_json["moves"];
            if (m_state->get_to_move() == FastBoard::BLACK) {
                send_2_json["moves"][m_state->get_movenum() - 1][0] = "W";
            } else {
                send_2_json["moves"][m_state->get_movenum() - 1][0] = "B";
            }
            send_2_json["moves"][m_state->get_movenum() - 1][1] = move_str;
            req_query = send_2_json.dump();
            res_query.erase(0);
            GTPSend(req_query + "\n", res_query);
            if (!res_query.length()) {
                return;
            }
            nlohmann::json res_2_json = nlohmann::json::parse(res_query);
            float winrate = res_2_json["rootInfo"]["winrate"];
            float scoreMean = res_2_json["rootInfo"]["scoreLead"];
            std::vector<float> conv_owner((board_size + 2) * (board_size + 2), 0.0);
            for (int vertex = 0; vertex < board_size * board_size; vertex++) {
                int x = vertex % board_size;
                int y = vertex / board_size;
                y = -1 * (y - board_size) - 1;
                int pos = m_state->board.get_vertex(x, y);
                float owner = res_2_json["ownership"][vertex];
                conv_owner[pos] = (owner / 2) + 0.5;
            }
            m_state->m_owner.clear();
            for (auto itr = conv_owner.begin(); itr != conv_owner.end(); ++itr) {
                m_state->m_owner.emplace_back(*itr);
            }
            std::vector<float> conv_policy((board_size + 2) * (board_size + 2), 0.0);
            float maxProbability = 0.0f;
            for (int vertex = 0; vertex < board_size * board_size + 1; vertex++) {
                int x = vertex % board_size;
                int y = vertex / board_size;
                y = -1 * (y - board_size) - 1;
                int pos = m_state->board.get_vertex(x, y);
                float policy = res_2_json["policy"][vertex];
                conv_policy[pos] = policy;
                if (policy > maxProbability) {
                    maxProbability = policy;
                }
            }
            float policy = res_2_json["policy"][board_size * board_size];
            conv_policy[0] = maxProbability;
            conv_policy[1] = policy;
            m_state->m_policy.clear();
            for (auto itr = conv_policy.begin(); itr != conv_policy.end(); ++itr) {
                m_state->m_policy.emplace_back(*itr);
            }

            if (m_update_score) {
                // Broadcast result from search
                auto event = new wxCommandEvent(wxEVT_EVALUATION_UPDATE);
                auto movenum = m_state->get_movenum();
                auto scoretuple = make_tuple(movenum, winrate, winrate, winrate);
                event->SetClientData((void*)new auto(scoretuple));

                wxQueueEvent(m_frame->GetEventHandler(), event);
            }

            if (m_state->get_to_move() == FastBoard::BLACK) {
                Utils::GUIprintf(cfg_lang, _("Win rate:%3.1f%% Score:%.1f").utf8_str(), winrate * 100, scoreMean);
            } else {
                Utils::GUIprintf(cfg_lang, _("Win rate:%3.1f%% Score:%.1f").utf8_str(), 100 - winrate * 100, -1 * scoreMean);
            }

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

void TEngineThread::force_stop_engine() {
    m_stopflag = true;
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

void TEngineThread::set_handi(std::vector<int> handi) {
    m_handi = handi;
}

void TEngineThread::kill_score_update(void) {
    m_update_score = false;
}

void TEngineThread::kata_raw_nn() {
    string inmsg;
    wxString sendCmd = "kata-raw-nn 0\n\n";
    GTPSend(sendCmd, inmsg);
    if (!inmsg.length()) {
        return;
    }
    string plain_result = std::regex_replace(inmsg, std::regex("\r\n|\n|\r"), " ");
    istringstream iss(plain_result);
    string s;
    bool pick_policy = false;
    bool pick_policyPass = false;
    bool pick_whiteOwnership = false;
    int i = 0;
    int board_size = m_state->board.get_boardsize();
    int vertex_size = board_size * board_size;
    vector<float> policy(vertex_size, 0.0);
    float policyPass = 0.0;
    vector<float> whiteOwnership(vertex_size, 0.0);
    while (iss >> s) {
        if (s == "policy") {
            i = 0;
            pick_policy = true;
            pick_policyPass = false;
            pick_whiteOwnership = false;
            continue;
        } else if (s == "policyPass") {
            pick_policy = false;
            pick_policyPass = true;
            pick_whiteOwnership = false;
            continue;
        } else if (s == "whiteOwnership") {
            i = 0;
            pick_policy = false;
            pick_policyPass = false;
            pick_whiteOwnership = true;
            continue;
        }
        else if (s == "symmetry" ||
                 s == "whiteWin" ||
                 s == "noResult" ||
                 s == "whiteLead" ||
                 s == "whiteScoreSelfplay" ||
                 s == "whiteScoreSelfplaySq" ||
                 s == "varTimeLeft" ||
                 s == "shorttermWinlossError" ||
                 s == "shorttermScoreError" ||
                 s == "shorttermScoreError") {
            pick_policy = false;
            pick_policyPass = false;
            pick_whiteOwnership = false;
            continue;
        }
        if (pick_policy) {
            policy[i] += strtof(s.c_str(), nullptr); // / 8;
            i++;
            if (i >= vertex_size)
                pick_policy = false;
        } else if (pick_policyPass) {
            policyPass += strtof(s.c_str(), nullptr); // / 8;
            pick_policyPass = false;
        } else if (pick_whiteOwnership) {
            whiteOwnership[i] += strtof(s.c_str(), nullptr); // / 8;
            i++;
            if (i >= vertex_size)
                pick_whiteOwnership = false;
        }
    }
    std::vector<float> conv_policy((board_size + 2) * (board_size + 2), 0.0);
    float maxProbability = 0.0f;
    int vertex = 0;
    for (const auto& policy_ent : policy) {
        int x = vertex % board_size;
        int y = vertex / board_size;
        y = -1 * (y - board_size) - 1;
        int pos = m_state->board.get_vertex(x, y);
        conv_policy[pos] = policy_ent;
        if (policy_ent > maxProbability) {
            maxProbability = policy_ent;
        }
        vertex++;
    }
    conv_policy[0] = maxProbability;
    conv_policy[1] = policyPass;
    m_state->m_policy.clear();
    for (auto itr = conv_policy.begin(); itr != conv_policy.end(); ++itr) {
        m_state->m_policy.emplace_back(*itr);
    }

    std::vector<float> conv_owner((board_size + 2) * (board_size + 2), 0.0);
    vertex = 0;
    for (const auto& owner : whiteOwnership) {
        int x = vertex % board_size;
        int y = vertex / board_size;
        y = -1 * (y - board_size) - 1;
        int pos = m_state->board.get_vertex(x, y);
        conv_owner[pos] = -1 * (owner / 2) + 0.5;
        vertex++;
    }
    m_state->m_owner.clear();
    for (auto itr = conv_owner.begin(); itr != conv_owner.end(); ++itr) {
        m_state->m_owner.emplace_back(*itr);
    }
}

void TEngineThread::GTPSend(const wxString& sendCmd, string &res_msg, const int &sleep_ms) {
    res_msg = "";
    char buffer[2048];
    buffer[0] = 0;

    m_out->Write(sendCmd.c_str(), sendCmd.length());
    while ( true ) {
        if (m_stopflag) {
            res_msg = "";
            return;
        }
        sleep_for(chrono::milliseconds(sleep_ms));
        if (m_stopflag) {
            res_msg = "";
            return;
        }
        if ( m_in->CanRead() ) {
            m_in->Read(buffer, WXSIZEOF(buffer) - 1);
            buffer[m_in->LastRead()] = '\0';
            res_msg += buffer;
            buffer[0] = 0;
            while (true) {
                std::string::size_type pos;
                if (cfg_use_engine == GTP::USE_KATAGO_GTP) {
                    if (sendCmd.find("genmove_analyze ") != string::npos) {
                        pos = res_msg.rfind("\nplay ");
                        if (pos != string::npos) {
                            if ((pos = res_msg.rfind("\r\n\r\n")) != string::npos) {
                                res_msg.erase(pos);
                                return;
                            }
                            if ((pos = res_msg.rfind("\n\n")) != string::npos) {
                                res_msg.erase(pos);
                                return;
                            }
                        }
                    } else {
                        if ((pos = res_msg.rfind("\r\n\r\n")) != string::npos) {
                            res_msg.erase(pos);
                            return;
                        }
                        if ((pos = res_msg.rfind("\n\n")) != string::npos) {
                            res_msg.erase(pos);
                            return;
                        }
                    }
                } else {
                    if ((pos = res_msg.rfind("\r\n")) != string::npos) {
                        res_msg.erase(pos);
                        return;
                    }
                    if ((pos = res_msg.rfind("\n")) != string::npos) {
                        res_msg.erase(pos);
                        return;
                    }
                }
                sleep_for(chrono::milliseconds(sleep_ms));
                if (m_stopflag) {
                    res_msg = "";
                    return;
                }
                if (m_in->CanRead()) {
                    m_in->Read(buffer, WXSIZEOF(buffer) - 1);
                    buffer[m_in->LastRead()] = '\0';
                    res_msg += buffer;
                    buffer[0] = 0;
                }
            }
            break;
        }
        if (m_stopflag) {
            res_msg = "";
            return;
        }
    }
    return;
}
