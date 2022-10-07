#include "stdafx.h"

#include <thread>
#include <chrono>
#include <regex>
#include "EngineThread.h"
#include "UCTSearch.h"
#include "MainFrame.h"
#include "Utils.h"
#include "MCOTable.h"

#define RESIGN_THRESHOLD 0.05
#define RESIGN_MINSCORE_DIFFERENCE -1e10

using namespace std;
using std::this_thread::sleep_for;

TEngineThread::TEngineThread(const GameState& state,
                             MainFrame *frame,
                             wxInputStream *std_in,
                             wxOutputStream *std_out,
                             std::mutex *GTPmutex,
                             std::vector<std::string>& overrideSettings)
    :m_state(std::make_unique<GameState>(state)),
     m_frame(frame),
     m_in(std_in),
     m_out(std_out),
     m_GTPmutex(GTPmutex),
     m_overrideSettings(overrideSettings),
     m_maxvisits(0),
     m_nets(true),
     m_resigning(true),
     m_analyseflag(false),
     m_pondering(false),
     m_quiet(false),
     m_nopass(false),
     m_update_score(true),
     m_runflag(true)
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
            return;
        } else if (cfg_use_engine == GTP::USE_KATAGO_GTP) {
            if (m_analyseflag) {
                return;
            }
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
            if (move_str == "pass") {
                m_state->play_move(who, FastBoard::PASS);
            } else if (move_str == "resign") {
                m_state->play_move(who, FastBoard::RESIGN);
            } else {
                m_state->play_move(who, m_state->board.text_to_move(move_str));
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
                std::tuple<int, float, float, float> scoretuple = make_tuple(movenum, black_winrate, black_winrate, black_winrate);
                event->SetClientData((void*)new auto(scoretuple));

                wxQueueEvent(m_frame->GetEventHandler(), event);
            }

            Utils::GUIprintf(cfg_lang, _("Win rate:%3.1f%% Score:%.1f").utf8_str(), 100 - winrate * 100, -1 * scoreMean);

            //if (!m_analyseflag) {
                wxQueueEvent(m_frame->GetEventHandler(), new wxCommandEvent(wxEVT_NEW_MOVE));
            //}
            return;
        }
        using namespace std::chrono;
        uint64_t ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        std::string query_id = std::to_string(ms);
        // do some preprocessing for move ordering
        // Note: Playouts are required to display the "Moyo"
        MCOwnerTable::get_MCO()->clear();
        GameState& rootstate(*m_state);
        float mc_score = Playout::mc_owner(rootstate);

        int board_size = m_state->board.get_boardsize();
        string tmp_query = "";
        for (auto it = m_overrideSettings.begin(); it != m_overrideSettings.end(); ++it) {
            tmp_query += *it;
        }
        nlohmann::json send_json = nlohmann::json::parse(tmp_query);
        send_json["id"] = "analysis_" + query_id;
        send_json["komi"] = m_state->get_komi();
        send_json["boardXSize"] = board_size;
        send_json["boardYSize"] = board_size;
        int i = 0;
        if (m_handi.size() > 0) {
            for (auto itr = m_handi.begin(); itr != m_handi.end(); ++itr) {
                std::string handi_move = m_state->move_to_text(*itr);
                send_json["initialStones"][i][0] = "B";
                send_json["initialStones"][i][1] = handi_move;
                i++;
            }
        }
        std::unique_ptr<GameState> state(new GameState);
        *state = *m_state;
        state->rewind();
        i = 0;
        send_json["moves"] = nlohmann::json::array();
        if (m_state->get_movenum() > 0) {
            while (state->forward_move()) {
                int move = state->get_last_move();
                if (move == FastBoard::RESIGN) {
                    break;
                }
                std::string movestr = state->board.move_to_text(move);
                if (state->get_to_move() == FastBoard::BLACK) {
                    send_json["moves"][i][0] = "W";
                } else {
                    send_json["moves"][i][0] = "B";
                }
                send_json["moves"][i][1] = movestr;
                i++;
            }
        } else {
            if (m_handi.size() > 0) {
                send_json["initialPlayer"] = "W";
            } else {
                send_json["initialPlayer"] = "B";
            }
        }
        send_json["reportDuringSearchEvery"] = 1.0;
        if (m_analyseflag) {
            send_json["maxVisits"] = 100000;
            send_json["overrideSettings"]["maxTime"] = 3600;
        }
        string req_query = send_json.dump();
        string move_str;
        float winrate = std::nanf("");
        float scoreMean;
        string res_query;
        string last_query = "";
        bool isDuringSearch = true;
        std::string::size_type pos;
        using TRowVector = std::vector<std::pair<std::string, std::string>>;
        using TDataVector = std::tuple<int, float, std::vector<TRowVector>>;
        using TMoveData = std::vector<std::pair<std::string, float>>;
        if (m_analyseflag) {
            while (isDuringSearch && m_runflag) {
                res_query = "";
                GTPSend(req_query + "\n", res_query);
                if (!res_query.length()) {
                    break;
                }
                if (isDuringSearch && res_query.find("\"isDuringSearch\":false") != string::npos) {
                    isDuringSearch = false;
                }
                pos = res_query.rfind("\r\n");
                if (pos != string::npos) {
                    last_query = res_query.substr(pos + 2);
                    res_query = last_query;
                } else {
                    pos = res_query.find("\n");
                    if (pos != string::npos) {
                        last_query = res_query.substr(pos + 1);
                        res_query = last_query;
                    }
                }
                req_query = "";
                nlohmann::json res_1_json = nlohmann::json::parse(res_query);
                move_str = res_1_json["moveInfos"][0]["move"];
                winrate = 1.0 - res_1_json["rootInfo"]["winrate"].get<float>();
                scoreMean = -1.0 * res_1_json["rootInfo"]["scoreLead"].get<float>();
                nlohmann::json j1 = res_1_json["moveInfos"];
                std::unique_ptr<TDataVector> analysis_packet(new TDataVector);
                std::unique_ptr<TMoveData> move_data(new TMoveData);
                std::get<0>(*analysis_packet) = who;
                std::get<1>(*analysis_packet) = scoreMean;
                auto & analysis_data = std::get<2>(*analysis_packet);
                for (nlohmann::json::iterator it1 = j1.begin(); it1 != j1.end(); ++it1) {
                    nlohmann::json j2 = it1.value();
                    if (j2.contains("isSymmetryOf")) {
                        continue;
                    }
                    TRowVector row;
                    row.emplace_back(_("Move").utf8_str(), j2["move"]);
                    row.emplace_back(_("Effort%").utf8_str(),
                        std::to_string(100.0 * j2["visits"].get<int>() / (double)res_1_json["rootInfo"]["visits"].get<int>()));
                    row.emplace_back(_("Simulations").utf8_str(), std::to_string(j2["visits"].get<int>()));
                    if (who == FastBoard::BLACK) {
                        //row.emplace_back(_("Win%").utf8_str(), std::to_string(100.0 - 100.0 * j2["winrate"].get<float>()));
                        //row.emplace_back(_("Lead").utf8_str(), std::to_string(-1.0 * j2["scoreLead"].get<float>()));
                        row.emplace_back(_("Win%").utf8_str(), std::to_string(100.0 * j2["winrate"].get<float>()));
                        row.emplace_back(_("Lead").utf8_str(), std::to_string(j2["scoreLead"].get<float>()));
                    } else {
                        //row.emplace_back(_("Win%").utf8_str(), std::to_string(100.0 * j2["winrate"].get<float>()));
                        //row.emplace_back(_("Lead").utf8_str(), std::to_string(j2["scoreLead"].get<float>()));
                        row.emplace_back(_("Win%").utf8_str(), std::to_string(100.0 - 100.0 * j2["winrate"].get<float>()));
                        row.emplace_back(_("Lead").utf8_str(), std::to_string(-1.0 * j2["scoreLead"].get<float>()));
                    }
                    std::string pvstring;
                    nlohmann::json j3 = j2["pv"];
                    for (nlohmann::json::iterator it2 = j3.begin(); it2 != j3.end(); ++it2) {
                        if (it2 > j3.begin()) {
                            pvstring += " ";
                        }
                        pvstring += *it2;
                    }
                    row.emplace_back(_("PV").utf8_str(), pvstring);
                    analysis_data.emplace_back(row);
                    move_data->emplace_back(j2["move"], (float)(j2["visits"].get<int>() / (double)res_1_json["rootInfo"]["visits"].get<int>()));
                }
                if (who == FastBoard::BLACK) {
                    //Utils::GUIprintf(cfg_lang, (_("Under analysis... ") + _("Win rate:%3.1f%% Score:%.1f")).utf8_str(), winrate * 100, scoreMean);
                    Utils::GUIprintf(cfg_lang, (_("Under analysis... ") + _("Win rate:%3.1f%% Score:%.1f")).utf8_str(), 100 - winrate * 100, -1 * scoreMean);
                } else {
                    //Utils::GUIprintf(cfg_lang, (_("Under analysis... ") + _("Win rate:%3.1f%% Score:%.1f")).utf8_str(), 100 - winrate * 100, -1 * scoreMean);
                    Utils::GUIprintf(cfg_lang, (_("Under analysis... ") + _("Win rate:%3.1f%% Score:%.1f")).utf8_str(), winrate * 100, scoreMean);
                }
                Utils::GUIAnalysis((void*)analysis_packet.release());
                Utils::GUIBestMoves((void*)move_data.release());
            }
            if (isDuringSearch) {
                nlohmann::json terminate_json = nlohmann::json::parse(R"({"action":"terminate"})");
                terminate_json["id"] = "terminate_" + query_id;
                terminate_json["terminateId"] = "analysis_" + query_id;
                string req_query_terminate = terminate_json.dump();
                bool terminate_res = false;
                for ( int i = 0; i < 100; i++ ) {
                    res_query = "";
                    GTPSend(req_query_terminate + "\n", res_query);
                    req_query_terminate = "";
                    if (!terminate_res && res_query.find("\"action\":\"terminate\"") != string::npos) {
                        terminate_res = true;
                    }
                    if (isDuringSearch && res_query.find("\"isDuringSearch\":false") != string::npos) {
                        isDuringSearch = false;
                    }
                    if (terminate_res && !isDuringSearch) {
                        break;
                    }
                }
            }
            if (m_update_score && !std::isnan(winrate)) {
                // Broadcast result from search
                auto event = new wxCommandEvent(wxEVT_EVALUATION_UPDATE);
                auto movenum = m_state->get_movenum();
                std::tuple<int, float, float, float> scoretuple = make_tuple(movenum, 1.0 - winrate, 1.0 - winrate, 1.0 - winrate);
                event->SetClientData((void*)new auto(scoretuple));

                wxQueueEvent(m_frame->GetEventHandler(), event);
            }
            return;
        }
        send_json["id"] = "play1_" + query_id;
        req_query = send_json.dump();
        nlohmann::json res_1_json;
        bool firstOK = false;
        while (isDuringSearch && m_runflag) {
            res_query = "";
            GTPSend(req_query + "\n", res_query);
            if (!res_query.length()) {
                break;
            }
            firstOK = true;
            if (isDuringSearch && res_query.find("\"isDuringSearch\":false") != string::npos) {
                isDuringSearch = false;
            }
            pos = res_query.rfind("\r\n");
            if (pos != string::npos) {
                last_query = res_query.substr(pos + 2);
                res_query = last_query;
            } else {
                pos = res_query.rfind("\n");
                if (pos != string::npos) {
                    last_query = res_query.substr(pos + 1);
                    res_query = last_query;
                }
            }
            req_query = "";
            res_1_json = nlohmann::json::parse(res_query);
        }
        if (isDuringSearch) {
            nlohmann::json terminate_json = nlohmann::json::parse(R"({"action":"terminate"})");
            terminate_json["id"] = "terminate_" + query_id;
            terminate_json["terminateId"] = "play1_" + query_id;
            string req_query_terminate = terminate_json.dump();
            bool terminate_res = false;
            for ( int i = 0; i < 100; i++ ) {
                res_query = "";
                GTPSend(req_query_terminate + "\n", res_query);
                req_query_terminate = "";
                if (!terminate_res && res_query.find("\"action\":\"terminate\"") != string::npos) {
                    terminate_res = true;
                }
                if (isDuringSearch && res_query.find("\"isDuringSearch\":false") != string::npos) {
                    isDuringSearch = false;
                }
                if (terminate_res && !isDuringSearch) {
                    break;
                }
            }
        }
        if (!firstOK) {
            return;
        }
        send_json["id"] = "play2_" + query_id;
        move_str = res_1_json["moveInfos"][0]["move"];
        send_json["includeOwnership"] = true;
        send_json["includePolicy"] = true;
        send_json["maxVisits"] = 1;
        send_json.erase("reportDuringSearchEvery");
        if (who == FastBoard::BLACK) {
            send_json["moves"][m_state->get_movenum()][0] = "B";
        } else {
            send_json["moves"][m_state->get_movenum()][0] = "W";
        }
        send_json["moves"][m_state->get_movenum()][1] = move_str;
        string req_query_2 = send_json.dump();
        res_query = "";
        GTPSend(req_query_2 + "\n", res_query);
        if (!res_query.length()) {
            return;
        }
        nlohmann::json res_2_json = nlohmann::json::parse(res_query);
        winrate = res_2_json["rootInfo"]["winrate"].get<float>();
        scoreMean = res_2_json["rootInfo"]["scoreLead"].get<float>();

        if (move_str.length() > 0) {
            float initialBlackAdvantageInPoints;
            if (m_handi.size() <= 1) {
                initialBlackAdvantageInPoints = 7.0 - m_state->get_komi();
            } else {
                initialBlackAdvantageInPoints = 14.0 * (m_handi.size() - 1) + (7.0 - m_state->get_komi() - m_handi.size());
            }
            int minTurnForResignation = 0;
            float noResignationWhenWhiteScoreAbove = board_size * board_size;
            if(initialBlackAdvantageInPoints > 0.9 && who == FastBoard::WHITE) {
                minTurnForResignation = 1 + noResignationWhenWhiteScoreAbove / 5;
                float numTurnsToCatchUp = 0.60 * noResignationWhenWhiteScoreAbove - minTurnForResignation;
                float numTurnsSpent = (float)(m_state->get_movenum() + 1) - minTurnForResignation;
                if (numTurnsToCatchUp <= 1.0) {
                    numTurnsToCatchUp = 1.0;
                }
                if (numTurnsSpent <= 0.0) {
                    numTurnsSpent = 0.0;
                }
                if (numTurnsSpent > numTurnsToCatchUp) {
                    numTurnsSpent = numTurnsToCatchUp;
                }
                float resignScore = -initialBlackAdvantageInPoints * ((numTurnsToCatchUp - numTurnsSpent) / numTurnsToCatchUp);
                resignScore -= 5.0;
                resignScore -= initialBlackAdvantageInPoints * 0.15;
                noResignationWhenWhiteScoreAbove = resignScore;
            }
            bool resign = true;
            if (m_state->get_movenum() + 1 < minTurnForResignation) {
                resign = false;
            } else if (who == FastBoard::WHITE && (-1.0 * scoreMean) > noResignationWhenWhiteScoreAbove) {
                resign = false;
            } else if ((who == FastBoard::WHITE && (-1.0 * scoreMean) > -1.0 * RESIGN_MINSCORE_DIFFERENCE) ||
                       (who == FastBoard::BLACK && scoreMean > (-1.0 * RESIGN_MINSCORE_DIFFERENCE))) {
                resign = false;
            }
            for (int i = 0; i < m_state->m_win_rate.size() - 1; i++) {
                m_state->m_win_rate[m_state->m_win_rate.size() - 1 - i]
                    = m_state->m_win_rate[m_state->m_win_rate.size() - 2 - i];
                if (m_state->m_win_rate[m_state->m_win_rate.size() - 1 - i] >= RESIGN_THRESHOLD) {
                    resign = false;
                }
            }
            if (who == FastBoard::BLACK) {
                m_state->m_win_rate[0] = winrate;
            } else {
                m_state->m_win_rate[0] = 1.0 - winrate;
            }
            if (resign && m_state->m_win_rate[0] < RESIGN_THRESHOLD) {
                move_str = "resign";
            }
            if (move_str == "pass") {
                m_state->play_move(who, FastBoard::PASS);
            } else if (move_str == "resign") {
                m_state->play_move(who, FastBoard::RESIGN);
            } else {
                m_state->play_move(who, m_state->board.text_to_move(move_str));
            }
        }
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
            std::tuple<int, float, float, float> scoretuple = make_tuple(movenum, winrate, winrate, winrate);
            event->SetClientData((void*)new auto(scoretuple));

            wxQueueEvent(m_frame->GetEventHandler(), event);
        }

        if (who == FastBoard::BLACK) {
            Utils::GUIprintf(cfg_lang, _("Win rate:%3.1f%% Score:%.1f").utf8_str(), 100 - winrate * 100, -1 * scoreMean);
        } else {
            Utils::GUIprintf(cfg_lang, _("Win rate:%3.1f%% Score:%.1f").utf8_str(), winrate * 100, scoreMean);
        }

        wxQueueEvent(m_frame->GetEventHandler(), new wxCommandEvent(wxEVT_NEW_MOVE));
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
    if (cfg_use_engine == GTP::USE_KATAGO_GTP) {
        std::lock_guard<std::mutex> guard(*m_GTPmutex);
    }
    res_msg = "";
    char buffer[8192];
    buffer[0] = 0;
    bool first = true;

    if (sendCmd.length() > 1) {
        while ( m_in->CanRead() ) {
            m_in->Read(buffer, WXSIZEOF(buffer) - 1);
        }
        m_out->Write(sendCmd.c_str(), sendCmd.length());
    }
    while ( true ) {
        sleep_for(chrono::milliseconds(sleep_ms));
        if ( m_in->CanRead() ) {
            first = false;
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
                    if (res_msg.length() > 0 && res_msg.back() == '\n') {
                        res_msg.erase(res_msg.length() - 1);
                        if (res_msg.length() > 0 && res_msg.back() == '\r') {
                            res_msg.erase(res_msg.length() - 1);
                        }
                        return;
                    }
                }
                sleep_for(chrono::milliseconds(sleep_ms / 10));
                if (m_in->CanRead()) {
                    m_in->Read(buffer, WXSIZEOF(buffer) - 1);
                    buffer[m_in->LastRead()] = '\0';
                    res_msg += buffer;
                    buffer[0] = 0;
                }
            }
        } else if (first && !m_runflag) {
            res_msg = "";
            return;
        }
    }
}
