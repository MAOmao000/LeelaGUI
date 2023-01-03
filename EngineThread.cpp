#include "stdafx.h"

#include "EngineThread.h"
#include "UCTSearch.h"
#include "MainFrame.h"
#include "Utils.h"
#ifdef USE_THREAD
#include "MCOTable.h"
#endif

using namespace std;
#ifdef USE_THREAD
using std::this_thread::sleep_for;
#endif

#ifdef USE_THREAD
TEngineThread::TEngineThread(GameState& state,
                             MainFrame *frame,
                             wxProcess *process,
                             wxInputStream *std_in,
                             wxInputStream *std_err,
                             wxOutputStream *std_out,
                             std::vector<std::string>& overrideSettings,
                             const std::string& query_id,
                             const std::chrono::time_point<std::chrono::system_clock>& query_start,
                             std::mutex *GTPmutex)
    :m_tm(state.get_timecontrol()),
     m_process(process),
     m_in(std_in),
     m_err(std_err),
     m_out(std_out),
     m_query_id(query_id),
     m_query_start(query_start),
     m_overrideSettings(overrideSettings),
     m_GTPmutex(GTPmutex),
     m_state(std::make_unique<GameState>(state)),
     m_frame(frame),
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
    m_state->set_timecontrol(m_tm);
}
#else
TEngineThread::TEngineThread(const GameState& state, MainFrame *frame)
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
     m_runflag(true)
{
}
#endif

void TEngineThread::Run() {
    auto Func = [this] {
        int who = m_state->get_to_move();
        try {
            if (cfg_use_engine == GTP::ORIGINE_ENGINE) {
                // Leela original engine start
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
                // Leela original engine end
            }
#ifdef USE_THREAD
            // KataGo analysis engine start
            int board_size = m_state->board.get_boardsize();
            string tmp_query = "";
            for (auto it = m_overrideSettings.begin(); it != m_overrideSettings.end(); ++it) {
                tmp_query += *it;
            }
            nlohmann::json send_json = nlohmann::json::parse(tmp_query);
            send_json["id"] = "analysis_" + m_query_id;
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
            send_json["moves"] = nlohmann::json::array();
            if (m_state->get_movenum() > 0) {
                for (int i = 0; i < m_state->get_movenum(); i++) {
                    state->forward_move();
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
                }
            } else {
                if (m_handi.size() > 0) {
                    send_json["initialPlayer"] = "W";
                } else {
                    send_json["initialPlayer"] = "B";
                }
            }
            if (send_json.contains("maxVisitsAnalysis")) {
                if (m_analyseflag) {
                    send_json["maxVisits"] = send_json["maxVisitsAnalysis"].get<int>();
                }
                send_json.erase("maxVisitsAnalysis");
            } else if (m_analyseflag) {
                send_json["maxVisits"] = atoi(MainFrame::DEFAULT_MAX_VISITS_ANALYSIS);
            }
            if (send_json.contains("maxTimeAnalysis")) {
                if (m_analyseflag) {
                    send_json["overrideSettings"]["maxTime"] = send_json["maxTimeAnalysis"].get<int>();
                }
                send_json.erase("maxTimeAnalysis");
            } else if (m_analyseflag) {
                send_json["overrideSettings"]["maxTime"] = atoi(MainFrame::DEFAULT_MAX_TIME_ANALYSIS);
            }
            string req_query = send_json.dump();
            string move_str;
            float winrate = std::nanf("");
            float scoreMean;
            string res_query;
            string last_query = "";
            bool isDuringSearch = true;
            std::string::size_type pos, pos1, pos2;
            using TRowVector = std::vector<std::pair<std::string, std::string>>;
            using TDataVector = std::tuple<int, float, std::vector<TRowVector>>;
            using TMoveData = std::vector<std::pair<std::string, float>>;
            if (m_analyseflag) {
                // KataGo analysis engine (analysis) start
                nlohmann::json res_1_json;
                while (isDuringSearch) {
                    res_query = "";
                    GTPSend(req_query + "\n", res_query);
                    req_query = "";
                    if (!res_query.length()) {
                        continue;
                    }
                    if (res_query.find(R"("error":)") != string::npos) {
                        ::wxMessageBox(res_query, _("Leela"), wxOK | wxICON_ERROR);
                        Utils::GUIprintf(_(""));
                        m_thinking.store(false, std::memory_order_release);
                        return;
                    }
                    pos = res_query.find(R"("isDuringSearch":)");
                    if (pos == string::npos) {
                        continue;
                    }
                    if (res_query.substr(pos + 17, 5) == "false") {
                        isDuringSearch = false;
                    }
                    pos1 = res_query.substr(0, pos).rfind(R"({"id":"analysis_)");
                    pos2 = res_query.substr(pos).find("\r\n");
                    if (pos2 == string::npos) {
                        pos2 = res_query.substr(pos).find("\n");
                    }
                    if (pos2 == string::npos) {
                        last_query = res_query.substr(pos1);
                        res_query = last_query;
                    } else {
                        last_query = res_query.substr(pos1, pos + pos2 - pos1);
                        res_query = last_query;
                    }
                    res_1_json = nlohmann::json::parse(res_query);
                    if (res_1_json["id"].get<std::string>() != send_json["id"].get<std::string>()) {
                        continue;
                    } else if (res_1_json.contains("noResults") && res_1_json["noResults"].get<bool>()) {
                        Utils::GUIprintf(_(""));
                        m_thinking.store(false, std::memory_order_release);
                        return;
                    }
                    winrate = 1.0 - res_1_json["rootInfo"]["winrate"].get<float>();
                    scoreMean = -1.0 * res_1_json["rootInfo"]["scoreLead"].get<float>();
                    nlohmann::json j1 = res_1_json["moveInfos"];
                    std::unique_ptr<TDataVector> analysis_packet(new TDataVector);
                    std::unique_ptr<TMoveData> move_data(new TMoveData);
                    std::get<0>(*analysis_packet) = who;
                    std::get<1>(*analysis_packet) = scoreMean;
                    auto& analysis_data = std::get<2>(*analysis_packet);
                    for (nlohmann::json::iterator it1 = j1.begin(); it1 != j1.end(); ++it1) {
                        nlohmann::json j2 = it1.value();
                        if (j2["move"].is_null()) {
                            continue;
                        }
                        TRowVector row;
                        row.emplace_back(_("Move").utf8_str(), j2["move"].get<std::string>());
                        row.emplace_back(_("Effort%").utf8_str(),
                            std::to_string(100.0 * j2["visits"].get<int>() / (double)res_1_json["rootInfo"]["visits"].get<int>()));
                        row.emplace_back(_("Simulations").utf8_str(), std::to_string(j2["visits"].get<int>()));
                        if (who == FastBoard::BLACK) {
                            row.emplace_back(_("Win%").utf8_str(), std::to_string(100.0 * j2["winrate"].get<float>()));
                            row.emplace_back(_("Score").utf8_str(), std::to_string(j2["scoreLead"].get<float>()));
                        } else {
                            row.emplace_back(_("Win%").utf8_str(), std::to_string(100.0 - 100.0 * j2["winrate"].get<float>()));
                            row.emplace_back(_("Score").utf8_str(), std::to_string(-1.0 * j2["scoreLead"].get<float>()));
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
                        move_data->emplace_back(j2["move"].get<std::string>(), (float)(j2["visits"].get<int>() / (double)res_1_json["rootInfo"]["visits"].get<int>()));
                    }
                    auto query_end = std::chrono::system_clock::now();
                    int think_time = (int)std::chrono::duration_cast<std::chrono::milliseconds>(query_end - m_query_start).count();
                    if (who == FastBoard::BLACK) {
                        Utils::GUIprintf((_("Under analysis... ") + _("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d")).utf8_str(),
                            100 - winrate * 100, -1 * scoreMean, think_time, res_1_json["rootInfo"]["visits"].get<int>());
                    } else {
                        Utils::GUIprintf((_("Under analysis... ") + _("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d")).utf8_str(),
                            winrate * 100, scoreMean, think_time, res_1_json["rootInfo"]["visits"].get<int>());
                    }
                    Utils::GUIAnalysis((void*)analysis_packet.release());
                    Utils::GUIBestMoves((void*)move_data.release());
                }
                m_state->m_black_score = -1.0f * scoreMean;
                if (m_update_score && !std::isnan(winrate)) {
                    // Broadcast result from search
                    auto event = new wxCommandEvent(wxEVT_EVALUATION_UPDATE);
                    auto movenum = m_state->get_movenum();
                    float lead = 0.5;
                    if (scoreMean > 0.0) {
                        lead = 0.5 - (std::min)(0.5f, std::sqrt(scoreMean) / 40);
                    } else if (scoreMean < 0.0) {
                        lead = 0.5 + (std::min)(0.5f, std::sqrt(-1.0f * scoreMean) / 40);
                    }
                    std::tuple<int, float, float, float> scoretuple = make_tuple(movenum, 1.0 - winrate, lead, 1.0 - winrate);
                    event->SetClientData((void*)new auto(scoretuple));

                    wxQueueEvent(m_frame->GetEventHandler(), event);
                }
                auto query_end = std::chrono::system_clock::now();
                int think_time = (int)std::chrono::duration_cast<std::chrono::milliseconds>(query_end - m_query_start).count();
                Utils::GUIprintf(_("Analysis stopped. Time:%d[ms] Visits:%d").utf8_str(),
                                 think_time, res_1_json["rootInfo"]["visits"].get<int>());
                m_thinking.store(false, std::memory_order_release);
                return;
                // KataGo analysis engine (analysis) end
            }
            // KataGo analysis engine (games) start
            m_state->start_clock(who);
            int time_for_move = m_state->get_timecontrol().max_time_for_move(who, m_state->get_movenum());
            if ( (time_for_move + 50) < 100 ) {
                time_for_move = 1 + 1;
            } else {
                time_for_move = (time_for_move + 50) / 100 + 1;
            }
            if (!m_state->get_timecontrol().byo_yomi(who) &&
                time_for_move > 2 &&
                m_state->m_win_rate[0] > 0.6f &&
                m_state->m_win_rate[1] > 0.6f &&
                m_state->m_win_rate[2] > 0.6f) {
                time_for_move--;
            }
            Utils::GUIprintf(_("Thinking at most %d seconds...").utf8_str(), time_for_move);
            nlohmann::json res_1_json;
            send_json["id"] = "play1_" + m_query_id;
            if (m_maxvisits <= 0) {
                send_json["maxVisits"] = INT_MAX;
            } else {
                send_json["maxVisits"] = m_maxvisits;
            }
            send_json["overrideSettings"]["maxTime"] = time_for_move - 1;
            send_json.erase("reportDuringSearchEvery");
            send_json["includeOwnership"] = true;
            req_query = send_json.dump();
            while (true) {
                res_query = "";
                GTPSend(req_query + "\n", res_query);
                req_query = "";
                if (!res_query.length()) {
                    continue;
                }
                if (res_query.find(R"("error":)") != string::npos) {
                    ::wxMessageBox(res_query, _("Leela"), wxOK | wxICON_EXCLAMATION);
                    Utils::GUIprintf(_(""));
                    m_state->stop_clock(who);
                    m_thinking.store(false, std::memory_order_release);
                    return;
                }
                pos = res_query.find(R"("isDuringSearch":)");
                if (pos == string::npos) {
                    continue;
                }
                if (res_query.substr(pos + 17, 5) == "false") {
                    pos1 = res_query.substr(0, pos).rfind(R"({"id":"play1_)");
                    pos2 = res_query.substr(pos).find("\r\n");
                    if (pos2 == string::npos) {
                        pos2 = res_query.substr(pos).find("\n");
                    }
                    if (pos2 == string::npos) {
                        last_query = res_query.substr(pos1);
                        res_query = last_query;
                    } else {
                        last_query = res_query.substr(pos1, pos + pos2 - pos1);
                        res_query = last_query;
                    }
                    res_1_json = nlohmann::json::parse(res_query);
                    if (res_1_json["id"].get<std::string>() != send_json["id"].get<std::string>()) {
                        continue;
                    } else if (res_1_json.contains("noResults") && res_1_json["noResults"].get<bool>()) {
                        Utils::GUIprintf(_(""));
                        m_state->stop_clock(who);
                        m_thinking.store(false, std::memory_order_release);
                        return;
                    }
                    break;
                }
            }
            m_thinking.store(false, std::memory_order_release);
            move_str = "";
            if(res_1_json.contains("moveInfos") && res_1_json["moveInfos"].size() > 0 &&
                !res_1_json["moveInfos"][0]["move"].is_null()) {
                move_str = res_1_json["moveInfos"][0]["move"].get<std::string>();
            }
            winrate = res_1_json["rootInfo"]["winrate"].get<float>();
            scoreMean = res_1_json["rootInfo"]["scoreLead"].get<float>();
            int visits = res_1_json["rootInfo"]["visits"].get<int>();
            m_state->m_black_score = scoreMean;
            send_json.erase("includeOwnership");
            // Edit Ownership Information
            std::vector<float> conv_owner((board_size + 2) * (board_size + 2), 0.0f);
            for (int vertex = 0; vertex < board_size * board_size; vertex++) {
                int x = vertex % board_size;
                int y = vertex / board_size;
                y = -1 * (y - board_size) - 1;
                int pos = m_state->board.get_vertex(x, y);
                float owner = res_1_json["ownership"][vertex].get<float>();
                conv_owner[pos] = (owner / 2.0f) + 0.5f;
            }
            m_state->m_owner.clear();
            for (auto itr = conv_owner.begin(); itr != conv_owner.end(); ++itr) {
                m_state->m_owner.emplace_back(*itr);
            }
            std::bitset<FastBoard::MAXSQ> blackowns;
            for (int i = 0; i < board_size; i++) {
                for (int j = 0; j < board_size; j++) {
                    int vtx = m_state->board.get_vertex(i, j);
                    if (m_state->m_owner[vtx] >= 0.5) {
                        blackowns[vtx] = true;
                    }
                }
            }
            MCOwnerTable::get_MCO()->update_owns(blackowns, 1.0f - winrate, -1.0f * scoreMean);
            // Send query to get ownership and policy
            send_json["id"] = "play2_" + m_query_id;
            send_json["includePolicy"] = true;
            send_json["maxVisits"] = 1;
            send_json["analysisPVLen"] = 1;
            if (who == FastBoard::BLACK) {
                send_json["moves"][m_state->get_movenum()][0] = "B";
            } else {
                send_json["moves"][m_state->get_movenum()][0] = "W";
            }
            if(move_str.length() > 0) {
                send_json["moves"][m_state->get_movenum()][1] = move_str;
            } else {
                send_json["moves"][m_state->get_movenum()][1] = "pass";
            }
            string req_query_2 = send_json.dump();
            nlohmann::json res_2_json;
            while (true) {
                res_query = "";
                GTPSend(req_query_2 + "\n", res_query);
                req_query_2 = "";
                if (!res_query.length()) {
                    continue;
                }
                if (res_query.find(R"("error":)") != string::npos) {
                    ::wxMessageBox(res_query, _("Leela"), wxOK | wxICON_EXCLAMATION);
                    Utils::GUIprintf(_(""));
                    m_state->stop_clock(who);
                    return;
                }
                pos = res_query.find(R"("isDuringSearch":)");
                if (pos == string::npos) {
                    continue;
                }
                if (res_query.substr(pos + 17, 5) == "false") {
                    pos1 = res_query.substr(0, pos).rfind(R"({"id":"play2_)");
                    pos2 = res_query.substr(pos).find("\r\n");
                    if (pos2 == string::npos) {
                        pos2 = res_query.substr(pos).find("\n");
                    }
                    if (pos2 == string::npos) {
                        last_query = res_query.substr(pos1);
                        res_query = last_query;
                    } else {
                        last_query = res_query.substr(pos1, pos + pos2 - pos1);
                        res_query = last_query;
                    }
                    res_2_json = nlohmann::json::parse(res_query);
                    if (res_2_json["id"].get<std::string>() != send_json["id"].get<std::string>()) {
                        continue;
                    } else if (res_2_json.contains("noResults") && res_2_json["noResults"].get<bool>()) {
                        Utils::GUIprintf(_(""));
                        m_state->stop_clock(who);
                        return;
                    }
                    break;
                }
            }
            // Edit Policy Information
            std::vector<float> conv_policy((board_size + 2) * (board_size + 2), 0.0f);
            float maxProbability = 0.0f;
            for (int vertex = 0; vertex < board_size * board_size; vertex++) {
                int x = vertex % board_size;
                int y = vertex / board_size;
                y = -1 * (y - board_size) - 1;
                int pos = m_state->board.get_vertex(x, y);
                float policy = res_2_json["policy"][vertex].get<float>();
                conv_policy[pos] = policy;
                if (policy > maxProbability) {
                    maxProbability = policy;
                }
            }
            float policy = res_2_json["policy"][board_size * board_size].get<float>();
            if (policy > maxProbability) {
                maxProbability = policy;
            }
            conv_policy[0] = maxProbability;
            conv_policy[1] = policy;
            m_state->m_policy.clear();
            for (auto itr = conv_policy.begin(); itr != conv_policy.end(); ++itr) {
                m_state->m_policy.emplace_back(*itr);
            }
            if (move_str.length() > 0) {
                // KataGo's Resignation Decision
                float initialBlackAdvantageInPoints;
                if (m_handi.size() <= 1) {
                    initialBlackAdvantageInPoints = 7.0 - m_state->get_komi();
                } else {
                    initialBlackAdvantageInPoints = 14.0 * (m_handi.size() - 1) + (7.0 - m_state->get_komi() - m_handi.size());
                }
                int minTurnForResignation = 0;
                float noResignationWhenWhiteScoreAbove = board_size * board_size;
                if (initialBlackAdvantageInPoints > 0.9 && who == FastBoard::WHITE) {
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
                for (size_t i = 0; i < m_state->m_win_rate.size() - 1; i++) {
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
            } else {
                move_str = "pass";
            }
            if (move_str == "pass") {
                m_state->play_move(who, FastBoard::PASS);
            } else if (move_str == "resign") {
                m_state->play_move(who, FastBoard::RESIGN);
            } else {
                m_state->play_move(who, m_state->board.text_to_move(move_str));
            }
            if (m_update_score) {
                // Broadcast result from search
                auto event = new wxCommandEvent(wxEVT_EVALUATION_UPDATE);
                auto movenum = m_state->get_movenum();
                float lead = 0.5f;
                if (scoreMean > 0.0f) {
                    lead = 0.5f + (std::min)(0.5f, std::sqrt(scoreMean) / 40.0f);
                } else if (scoreMean < 0.0) {
                    lead = 0.5f - (std::min)(0.5f, std::sqrt(-1.0f * scoreMean) / 40.0f);
                }
                std::tuple<int, float, float, float> scoretuple = make_tuple(movenum, winrate, lead, winrate);
                event->SetClientData((void*)new auto(scoretuple));

                wxQueueEvent(m_frame->GetEventHandler(), event);
            }

            auto query_end = std::chrono::system_clock::now();
            int think_time = (int)std::chrono::duration_cast<std::chrono::milliseconds>(query_end - m_query_start).count();
            if (who == FastBoard::BLACK) {
                Utils::GUIprintf(_("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d").utf8_str(),
                    100.0f - winrate * 100.0f, -1.0f * scoreMean, think_time, visits);
            } else {
                Utils::GUIprintf(_("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d").utf8_str(),
                    winrate * 100.0f, scoreMean, think_time, visits);
            }

            auto event = new wxCommandEvent(wxEVT_NEW_MOVE);
            int *num = new int(visits);
            event->SetClientData((void*)num);
            wxQueueEvent(m_frame->GetEventHandler(), event);

            m_state->stop_clock(who);
            // KataGo analysis engine (games) end
#endif
        } catch (std::exception& e) {
            wxString errorString;
            errorString.Printf(_("Exception %s %s\n"), typeid(e).name(), e.what());
            ::wxMessageBox(errorString, _("Leela"), wxOK | wxICON_ERROR);
            Utils::GUIprintf( _(""));
            m_state->stop_clock(who);
            return;
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

#ifdef USE_THREAD
void TEngineThread::set_handi(std::vector<int> handi) {
    m_handi = handi;
}

void TEngineThread::set_thinking(bool flag) {
    m_thinking.store(flag, std::memory_order_release);
}

bool TEngineThread::get_thinking() {
    return m_thinking.load(std::memory_order_acquire);
}

void TEngineThread::GTPSend(const wxString& sendCmd, string &res_msg, const int &sleep_ms) {

    res_msg = "";
    char buffer[8192];
    buffer[0] = 0;

    m_GTPmutex->lock();
    if (!m_process->GetOutputStream()) {
        m_GTPmutex->unlock();
        return;
    }

    if (sendCmd.length() > 1) {
        while (m_process->IsInputAvailable()) {
            m_in->Read(buffer, WXSIZEOF(buffer) - 1);
        }
        while (m_process->IsErrorAvailable()) {
            m_err->Read(buffer, WXSIZEOF(buffer) - 1);
        }
        m_out->Write(sendCmd.c_str(), sendCmd.length());
    }
    m_GTPmutex->unlock();
    int sleep_current = sleep_ms;
    sleep_for(chrono::milliseconds(sleep_current));

    while ( true ) {
        m_GTPmutex->lock();
        if (m_process->IsInputAvailable()) {
            m_in->Read(buffer, WXSIZEOF(buffer) - 1);
            m_GTPmutex->unlock();
            sleep_current = sleep_ms / 10;
            buffer[m_in->LastRead()] = '\0';
            res_msg += buffer;
            buffer[0] = 0;
            if (res_msg.length() > 0 && res_msg.back() == '\n') {
                res_msg.erase(res_msg.length() - 1);
                if (res_msg.length() > 0 && res_msg.back() == '\r') {
                    res_msg.erase(res_msg.length() - 1);
                }
                return;
            }
        } else if (m_process->IsErrorAvailable()) {
            m_err->Read(buffer, WXSIZEOF(buffer) - 1);
            m_GTPmutex->unlock();
        } else {
            m_GTPmutex->unlock();
            sleep_for(chrono::milliseconds(sleep_current));
        }
    }
    m_GTPmutex->unlock();
}
#endif
