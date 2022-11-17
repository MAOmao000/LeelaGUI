#include "stdafx.h"
#include "GUI.h"
#include "MainFrame.h"
#include "TBoardPanel.h"
#include "SettingsDialog.h"
#include "Zobrist.h"
#include "Random.h"
#include "Utils.h"
#include "Network.h"
#include "Matcher.h"
#include "AttribScores.h"
#include "SGFTree.h"
#include "SGFParser.h"
#include "SMP.h"
#include "Network.h"
#include "EngineThread.h"
#include "AboutDialog.h"
#include "NewGameDialog.h"
#include "ClockAdjustDialog.h"
#include "RatedSizeDialog.h"
#include "CalculateDialog.h"
#include "AnalysisWindow.h"
#include "ScoreDialog.h"
#include "ScoreHistogram.h"
#include "MCOTable.h"
#include "TTable.h"
#ifdef USE_OPENCL
#include "OpenCL.h"
#endif
#ifndef WIN32
#include "img/leela_mock.xpm"
#include "snd/tock.h"
#include <wx/utils.h>
#endif
#include <chrono>
#include <thread>
#include <fstream>
#include <regex>

wxDEFINE_EVENT(wxEVT_NEW_MOVE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_BOARD_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_STATUS_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_ANALYSIS_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_BESTMOVES_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_EVALUATION_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_SET_MOVENUM, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_RECIEVE_KATAGO, wxCommandEvent);

static constexpr long MAX_RANK = 13L;
static constexpr long MIN_RANK = -30L;
// Resignation after 3 consecutive win rates of 5% or less
static constexpr float RESIGN_THRESHOLD = 0.05f;
static constexpr float RESIGN_MINSCORE_DIFFERENCE = -1e10f;
static constexpr int WAKE_UP_TIMER_MS = 100;

enum {
    INIT,
    KATAGO_STRATING,
    ANALYSIS_RESPONSE_WAIT,
    KATAGO_IDLE,
    KATAGO_STOPED,
    ANALYSIS_QUERY_WAIT,
    ANALYSIS_TERMINATE_WAIT,
    GAME_QUERY_WAIT,
};

enum {
    NON,
    POLICY,
    POLICYPASS,
    WHITEOWNERSHIP,
    LAST,
};


MainFrame::MainFrame(wxFrame *frame, const wxString& title)
          : TMainFrame(frame, wxID_ANY, title), m_timerIdleWakeUp(this) {

    m_process = nullptr;

#ifdef NDEBUG
    delete wxLog::SetActiveTarget(NULL);
#endif
    wxLog::SetTimestamp("");

    m_japaneseEnabled = wxConfig::Get()->ReadBool(wxT("japaneseEnabled"), false);
    int lang = 0;
    if (m_japaneseEnabled) {
        lang = 1;
    }

    int use_engine = GTP::ORIGINE_ENGINE;
    m_japanese_rule = false;
    m_close_window = false;
    bool board25 = true;

    GTP::setup_default_parameters(lang, use_engine, board25);

    Bind(wxEVT_NEW_MOVE, &MainFrame::doNewMove, this);
    Bind(wxEVT_BOARD_UPDATE, &MainFrame::doBoardUpdate, this);
    Bind(wxEVT_STATUS_UPDATE, &MainFrame::doStatusUpdate, this);
    Bind(wxEVT_DESTROY, &MainFrame::doCloseChild, this);
    // Receive movenum changes (from history panel)
    Bind(wxEVT_SET_MOVENUM, &MainFrame::gotoMoveNum, this);
    // Forward to histogram window, if it exists
    Bind(wxEVT_EVALUATION_UPDATE, &MainFrame::doEvalUpdate, this);
    // Forward mainline updates to the board panel
    Bind(wxEVT_DISPLAY_MAINLINE, [=](wxCommandEvent& event) {
        m_panelBoard->GetEventHandler()->AddPendingEvent(event);
    });
    // Forward top moves to board panel
    Bind(wxEVT_BESTMOVES_UPDATE, [=](wxCommandEvent& event) {
        m_panelBoard->GetEventHandler()->AddPendingEvent(event);
    });
    // Forward to analysis window, if it exists
    Bind(wxEVT_ANALYSIS_UPDATE, [=](wxCommandEvent& event) {
        if (m_analysisWindow) {
            m_analysisWindow->GetEventHandler()->AddPendingEvent(event);
        } else {
            void* rawdataptr = event.GetClientData();
            if (!rawdataptr) return;
            using TRowVector = std::vector<std::pair<std::string, std::string>>;
            using TDataVector = std::vector<TRowVector>;
            using TDataBundle = std::tuple<int, float, TDataVector>;

            // Take ownership of the data
            delete reinterpret_cast<TDataBundle*>(rawdataptr);
        }
    });

    Bind(wxEVT_RECIEVE_KATAGO, &MainFrame::doRecieveKataGo, this);
    Bind(wxEVT_TIMER, &MainFrame::OnIdleTimer, this);

    thread_pool.initialize(cfg_num_threads);

    auto rng = std::make_unique<Random>(5489UL);
    Zobrist::init_zobrist(*rng);
    AttribScores::get_attribscores();
    Matcher::get_Matcher();

#ifdef USE_OPENCL
    try {
        Network::get_Network();
    } catch (const cl::Error &e) {
        wxString errorString;
        errorString.Printf(_("Error initializing OpenCL: %s (error %d)"),
            e.what(), e.err());
        ::wxMessageBox(errorString, _("Leela"), wxOK | wxICON_EXCLAMATION, this);
        Close();
    } catch (const std::exception& e) {
        wxString errorString;
        errorString.Printf(_("Error initializing OpenCL: %s"), e.what());
        ::wxMessageBox(errorString, _("Leela"), wxOK | wxICON_EXCLAMATION, this);
        Close();
    }
#else
    Network::get_Network();
#endif

    // init game
    m_playerColor = FastBoard::BLACK;
    m_visitLimit = 5000;
    m_ratedGame = true;
    m_analyzing = false;
    m_pondering = false;
    m_disputing = false;
    m_ponderedOnce = true;

    m_netsEnabled = wxConfig::Get()->ReadBool(wxT("netsEnabled"), true);
    m_passEnabled = wxConfig::Get()->ReadBool(wxT("passEnabled"), true);
    m_soundEnabled = wxConfig::Get()->ReadBool(wxT("soundEnabled"), true);
    m_resignEnabled = wxConfig::Get()->ReadBool(wxT("resignEnabled"), true);
    m_ponderEnabled = wxConfig::Get()->ReadBool(wxT("ponderEnabled"), true);
    m_ratedSize     = wxConfig::Get()->ReadLong(wxT("ratedSize"), 9L);

    // This is a bug in 0.4.0, correct broken values.
    if (m_ratedSize != 9 && m_ratedSize != 19) {
        m_ratedSize = 9;
    }

#ifdef USE_OPENCL
    m_State.init_game(19, 7.5f);
    if (!GTP::perform_self_test(m_State)) {
        ::wxMessageBox(_("OpenCL self-test failed. Check your graphics drivers."),
                       _("Leela"), wxOK | wxICON_EXCLAMATION, this);
        Close();
    } else {
#ifdef __WXMAC__
        bool hasWhinedDrivers =
            wxConfig::Get()->ReadBool(wxT("hasWhinedDrivers"), false);
        if (!hasWhinedDrivers) {
            ::wxMessageBox(_("The GPU and OpenCL drivers on macOS are often outdated "
                "and of poor quality. Try switching to the regular version if "
                "you run into stability issues. If your GPU is slow, it may "
                "even be faster."), _("Leela"), wxOK, this);
            wxConfig::Get()->Write(wxT("hasWhinedDrivers"), true);
        }
#endif
    }
#endif

    m_State.init_game(m_ratedSize, 7.5f);
    m_State.set_timecontrol(2 * m_ratedSize * 60 * 100, 0, 0, 0);
    m_StateStack.clear();
    m_panelBoard->setState(&m_State);
    m_panelBoard->setPlayerColor(m_playerColor);
    m_menuAnalyze->FindItem(ID_ANALYSISWINDOWTOGGLE)->Check(false);

    // set us as the global message receiver
    Utils::setGUIQueue(this->GetEventHandler(), wxEVT_STATUS_UPDATE);

    // send analysis events here
    Utils::setAnalysisQueue(this->GetEventHandler(),
                            wxEVT_ANALYSIS_UPDATE,
                            wxEVT_BESTMOVES_UPDATE);

    SetIcon(wxICON(aaaa));

#ifdef __WXGTK__
    SetSize(530, 640);
    if (m_japaneseEnabled) {
        m_statusBar->SetMinHeight(30);
    }
#elif defined(__WXMAC__)
    SetSize(570, 640);
#else
    SetSize(530, 640);
#endif
    Center();
    setStartMenus(false);

    // Create the window already, so we start saving move evaluations
    m_scoreHistogramWindow = new ScoreHistogram(this);
    m_scoreHistogramWindow->Hide();

    // Restore open windows
    bool restoreAnalysisWindow =
        wxConfig::Get()->ReadBool(wxT("analysisWindowOpen"), false);
    bool restoreScoreHistogramWindow =
        wxConfig::Get()->ReadBool(wxT("scoreHistogramWindowOpen"), false);
    if (restoreAnalysisWindow) {
        auto event = new wxCommandEvent(wxEVT_MENU, ID_ANALYSISWINDOWTOGGLE);
        event->SetInt(NO_WINDOW_AUTOSIZE);
        GetEventHandler()->QueueEvent(event);
    }
    if (restoreScoreHistogramWindow) {
        auto event = new wxCommandEvent(wxEVT_MENU, ID_SCOREHISTOGRAMTOGGLE);
        GetEventHandler()->QueueEvent(event);
    }
    // Restore menu options
    bool restoreShowOwner =
        wxConfig::Get()->ReadBool("showOwnerEnabled", false);
    bool restoreShowMoyo =
        wxConfig::Get()->ReadBool("showMoyoEnabled", false);
    bool restoreShowBestMoves =
        wxConfig::Get()->ReadBool("showBestMovesEnabled", false);
    bool restoreShowProbabilities =
        wxConfig::Get()->ReadBool("showProbabilitiesEnabled", false);
    if (restoreShowOwner) {
        auto event = new wxCommandEvent(wxEVT_MENU, ID_SHOWTERRITORY);
        GetEventHandler()->QueueEvent(event);
    }
    if (restoreShowMoyo) {
        auto event = new wxCommandEvent(wxEVT_MENU, ID_SHOWMOYO);
        GetEventHandler()->QueueEvent(event);
    }
    if (restoreShowBestMoves) {
        auto event = new wxCommandEvent(wxEVT_MENU, ID_BEST_MOVES);
        GetEventHandler()->QueueEvent(event);
    }
    if (restoreShowProbabilities) {
        auto event = new wxCommandEvent(wxEVT_MENU, ID_MOVE_PROBABILITIES);
        GetEventHandler()->QueueEvent(event);
    }

    wxPersistentRegisterAndRestore(this, "MainFrame");
}

MainFrame::~MainFrame() {
    if (cfg_use_engine == GTP::KATAGO_ENGINE) {
        if (m_katagoStatus != KATAGO_IDLE) {
            m_runflag = false;
            m_post_destructor = true;
            return;
        }
    } else {
        stopEngine();
    }

    wxPersistentRegisterAndRestore(this, "MainFrame");
    wxConfig::Get()->Write(wxT("analysisWindowOpen"),
        m_analysisWindow != nullptr && m_analysisWindow->IsShown());
    wxConfig::Get()->Write(wxT("scoreHistogramWindowOpen"),
        m_scoreHistogramWindow != nullptr && m_scoreHistogramWindow->IsShown());
#ifdef NDEBUG
    delete wxLog::SetActiveTarget(new wxLogStderr(NULL));
#endif
    //m_panelBoard->setState(NULL);

    Unbind(wxEVT_EVALUATION_UPDATE, &MainFrame::doEvalUpdate, this);
    Unbind(wxEVT_NEW_MOVE, &MainFrame::doNewMove, this);
    Unbind(wxEVT_BOARD_UPDATE, &MainFrame::doBoardUpdate, this);
    Unbind(wxEVT_STATUS_UPDATE, &MainFrame::doStatusUpdate, this);
    Unbind(wxEVT_DESTROY, &MainFrame::doCloseChild, this);
    Unbind(wxEVT_SET_MOVENUM, &MainFrame::gotoMoveNum, this);
    Unbind(wxEVT_RECIEVE_KATAGO, &MainFrame::doRecieveKataGo, this);
    Unbind(wxEVT_TIMER, &MainFrame::OnIdleTimer, this);

    Hide();
}

void MainFrame::doInit() {
    m_katagoStatus = INIT;
    bool katagoEnabled = wxConfig::Get()->ReadBool(wxT("katagoEnabled"), true);
    if (!katagoEnabled) {
        m_timerIdleWakeUp.Stop();
        setStartMenus();
        setActiveMenus();
        wxCommandEvent evt;
        doNewRatedGame(evt);
        return;
    }
    std::string ini_file;
#ifdef USE_GPU
    ini_file = "LeelaGUI_OpenCL.ini";
#else
    ini_file = "LeelaGUI.ini";
#endif 
#ifdef WIN32
    char* ini_path = (char*)calloc(MAX_PATH + 1, sizeof(char));
    char buf[MAX_PATH+1];
    GetModuleFileNameA(NULL, buf, MAX_PATH);
    char drive[MAX_PATH+1], dir[MAX_PATH+1], fname[MAX_PATH+1], ext[MAX_PATH+1];
    _splitpath(buf, drive, dir, fname, ext);
    sprintf(ini_path, "%s%s%s", drive, dir, ini_file.c_str());
#else
    char* ini_path = (char*)calloc(4096 + 1, sizeof(char));
    char prg_path[4096];
    int len = readlink("/proc/self/exe", prg_path, sizeof(prg_path));
    prg_path[len] = 0;
    for (int i = len - 1; i > 0; i--) {
        if (prg_path[i] != '/') {
            prg_path[i] = 0;
        } else {
            break;
        }
    }
    sprintf(ini_path, "%s%s", prg_path, ini_file.c_str());
#endif
    std::ifstream fin(ini_path);
    free(ini_path);
    if (fin) {
        std::string line;
        while (std::getline(fin, line)) {
            std::string trim_line = std::regex_replace(line, std::regex("^\\s+"), std::string(""));
            if (trim_line.length() <= 0 || trim_line[0] == '#') {
                continue;
            }
            auto pos = trim_line.find('#');
            std::string erase_line = trim_line;
            if (pos != std::string::npos) {
                erase_line = trim_line.erase(pos);
            }
            std::string last_line = std::regex_replace(erase_line, std::regex("\\s+$"), std::string(""));
            std::regex reg(R"(\s+)");
            std::string s = std::regex_replace(last_line, reg, " ");
            m_ini_line.emplace_back(s);
        }
        if (m_ini_line.size() > 0) {
            auto pos = m_ini_line[0].find(" analysis ");
            if (pos != std::string::npos) {
                SubProcess *process = new SubProcess(this);
                long pid = wxExecute(m_ini_line[0], wxEXEC_ASYNC, process);
                m_process = process;
                if ( !pid ) {
                    wxLogDebug(_("Failed to launch the command."));
                } else if ( !(m_in = m_process->GetInputStream()) ) {
                    wxLogDebug(_("Failed to connect to child stdout"));
                    wxProcess::Kill(m_process->GetPid());
                } else if ( !(m_err = m_process->GetErrorStream()) ) {
                    wxLogDebug(_("Failed to connect to child stderr"));
                    wxProcess::Kill(m_process->GetPid());
                } else if ( !(m_out = m_process->GetOutputStream()) ) {
                    wxLogDebug(_("Failed to connect to child stdin"));
                    wxProcess::Kill(m_process->GetPid());
                } else {
                    cfg_use_engine = GTP::KATAGO_ENGINE;
                }
            }
            if (cfg_use_engine == GTP::ORIGINE_ENGINE) {
                 wxString errorString;
                 errorString.Printf(_("The first line of the ini file is incorrect: %s\n"
                                      "Start with the Leela engine?"), m_ini_line[0].mb_str());
                int answer = ::wxMessageBox(errorString, _("Leela"), wxYES_NO | wxICON_EXCLAMATION, this);
                if (answer != wxYES) {
                    m_close_window = true;
                }
            } else {
                m_timerIdleWakeUp.Start(WAKE_UP_TIMER_MS);
            }
        }
    }
    if (cfg_use_engine == GTP::ORIGINE_ENGINE) {
        m_timerIdleWakeUp.Stop();
        m_in = nullptr;
        m_err = nullptr;
        m_out = nullptr;
        if (m_close_window) {
            Close();
        } else {
            setStartMenus();
            setActiveMenus();
            wxCommandEvent evt;
            doNewRatedGame(evt);
        }
    } else {
        m_post_destructor = false;
        m_post_doExit = false;
        m_post_doNewMove = false;
        m_post_doSettingsDialog = false;
        m_post_doNewGame = false;
        m_post_doNewRatedGame = false;
        m_post_doScore = false;
        m_post_doPass = false;
        m_post_doRealUndo = 0;
        m_post_doRealForward = 0;
        m_post_doOpenSGF = false;
        m_post_doSaveSGF = false;
        m_post_doForceMove = false;
        m_post_doResign = false;
        m_post_doAnalyze = false;
        m_katagoStatus = KATAGO_STRATING;
    }
}

void MainFrame::doStatusUpdate(wxCommandEvent& event) {
    m_statusBar->SetStatusText(event.GetString(), 1);
}

void MainFrame::SetStatusBarText(wxString mess, int pos) {
    m_statusBar->SetStatusText(mess, pos);
}

void MainFrame::updateStatusBar(char *str) {
    //wxString wxstr(str);
    //SetStatusText(str);
}

// do whatever we need to do if the visible board gets updated
void MainFrame::doBoardUpdate(wxCommandEvent& event) {
    wxString mess;
    mess.Printf(_("Komi: %d.5; Prisoners white: %d/black: %d"),
                (int)m_State.get_komi(),
                m_State.board.get_prisoners(FastBoard::BLACK),
                m_State.board.get_prisoners(FastBoard::WHITE)
                );
    m_statusBar->SetStatusText(mess, 0);
    Refresh();
}

void MainFrame::doExit(wxCommandEvent & event) {
    if (cfg_use_engine == GTP::KATAGO_ENGINE) {
        if (m_katagoStatus != KATAGO_IDLE) {
            m_runflag = false;
            m_post_doExit = true;
            return;
        }
    } else {
        stopEngine();
    }
    Close();
}

void MainFrame::startEngine() {
    if (cfg_use_engine == GTP::KATAGO_ENGINE) {
        startKataGo();
        return;
    }
    if (!m_engineThread) {
        m_engineThread = std::make_unique<TEngineThread>(m_State, this);
        // lock the board
        if (!m_pondering && !m_analyzing) {
            m_panelBoard->lockState();
        }
        m_engineThread->limit_visits(m_visitLimit);
        m_engineThread->set_resigning(m_resignEnabled);
        m_engineThread->set_analyzing(m_analyzing | m_pondering);
        m_engineThread->set_pondering(m_pondering);
        m_engineThread->set_quiet(!m_analyzing);
        m_engineThread->set_nets(m_netsEnabled);
        if (m_passEnabled) {
            m_engineThread->set_nopass(m_disputing);
        }
        else {
            m_engineThread->set_nopass(true);
        }
        m_engineThread->Run();
        if (!m_analyzing && !m_pondering) {
            SetStatusBarText(_("Engine thinking...") + _(" (Leela"), 1);
        }
    } else {
        wxLogDebug(_("Engine already running"));
    }
}

void MainFrame::startKataGo() {
    if (m_katagoStatus == KATAGO_IDLE) {
        m_StateEngine = std::make_unique<GameState>(m_State);
        // lock the board
        if (!m_pondering && !m_analyzing) {
            m_panelBoard->lockState();
        }
        using namespace std::chrono;
        m_update_score = true;
        uint64_t ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        m_query_id = std::to_string(ms);
        int board_size = m_StateEngine->board.get_boardsize();
        std::string tmp_query = "";
        for (auto it = m_overrideSettings.begin(); it != m_overrideSettings.end(); ++it) {
            tmp_query += *it;
        }
        try {
            m_send_json = nlohmann::json::parse(tmp_query);
            m_send_json["id"] = "analysis_" + m_query_id;
            m_send_json["komi"] = m_StateEngine->get_komi();
            m_send_json["boardXSize"] = board_size;
            m_send_json["boardYSize"] = board_size;
            if (m_move_handi.size() > 0) {
                int i = 0;
                for (auto itr = m_move_handi.begin(); itr != m_move_handi.end(); ++itr) {
                    std::string handi_move = m_StateEngine->move_to_text(*itr);
                    m_send_json["initialStones"][i][0] = "B";
                    m_send_json["initialStones"][i][1] = handi_move;
                    i++;
                }
            }
            std::unique_ptr<GameState> state(new GameState);
            *state = *m_StateEngine;
            state->rewind();
            m_send_json["moves"] = nlohmann::json::array();
            if (m_StateEngine->get_movenum() > 0) {
                for (int i = 0; i < m_StateEngine->get_movenum(); i++) {
                    state->forward_move();
                    int move = state->get_last_move();
                    if (move == FastBoard::RESIGN) {
                        break;
                    }
                    std::string movestr = state->board.move_to_text(move);
                    if (state->get_to_move() == FastBoard::BLACK) {
                        m_send_json["moves"][i][0] = "W";
                    } else {
                        m_send_json["moves"][i][0] = "B";
                    }
                    m_send_json["moves"][i][1] = movestr;
                }
            } else {
                if (m_move_handi.size() > 0) {
                    m_send_json["initialPlayer"] = "W";
                } else {
                    m_send_json["initialPlayer"] = "B";
                }
            }
            if (m_send_json.contains("maxVisitsAnalysis")) {
                if (m_analyzing | m_pondering) {
                    m_send_json["maxVisits"] = m_send_json["maxVisitsAnalysis"].get<int>();
                }
                m_send_json.erase("maxVisitsAnalysis");
            } else if (m_analyzing | m_pondering) {
                m_send_json["maxVisits"] = 1000000;
            }
            if (m_send_json.contains("maxTimeAnalysis")) {
                if (m_analyzing | m_pondering) {
                    m_send_json["overrideSettings"]["maxTime"] = m_send_json["maxTimeAnalysis"].get<int>();
                }
                m_send_json.erase("maxTimeAnalysis");
            } else if (m_analyzing | m_pondering) {
                m_send_json["overrideSettings"]["maxTime"] = 3600;
            }
            if (m_analyzing | m_pondering) {
                std::string req_query = m_send_json.dump();
                wxString send_msg = req_query + "\n";
                m_out->Write(send_msg, send_msg.length());
                m_runflag = true;
                m_wasRunning = false;
                m_isDuringSearch = true;
                m_terminate_res= false;
                m_katagoStatus = ANALYSIS_QUERY_WAIT;
            } else {
                int color = m_StateEngine->get_to_move();
                m_StateEngine->start_clock(color);
                int time_for_move = m_StateEngine->get_timecontrol().max_time_for_move(color);
                if ( (time_for_move + 50) < 100 ) {
                    time_for_move = 1;
                } else {
                    time_for_move = (time_for_move + 50) / 100;
                }
                m_send_json["id"] = "play1_" + m_query_id;
                m_send_json["analysisPVLen"] = 1;
                if (m_visitLimit <= 0) {
                    m_send_json["maxVisits"] = INT_MAX;
                } else {
                    m_send_json["maxVisits"] = m_visitLimit;
                }
                m_send_json["includeOwnership"] = true;
                m_send_json["includePolicy"] = true;
                m_send_json["overrideSettings"]["maxTime"] = time_for_move;
                m_send_json.erase("reportDuringSearchEvery");
                std::string req_query = m_send_json.dump();
                wxString send_msg = req_query + "\n";
                m_out->Write(send_msg, send_msg.length());
                m_runflag = true;
                m_wasRunning = false;
                m_isDuringSearch = true;
                m_terminate_res = true;
                m_katagoStatus = GAME_QUERY_WAIT;
            }
        } catch(const std::exception& e) {
            wxLogError(_("Exception at startKataGo: %s %s\n"), typeid(e).name(), e.what());
        }
        if (!m_analyzing && !m_pondering) {
            SetStatusBarText(_("Engine thinking...") + _(" (KataGo)"), 1);
        }
        m_timerIdleWakeUp.Start(WAKE_UP_TIMER_MS);
    } else {
        wxLogDebug(_("Engine already running"));
    }
}

bool MainFrame::stopEngine(bool update_score) {
    if (!m_engineThread) {
        return false;
    }
    if (!update_score) {
        m_engineThread->kill_score_update();
    }
    m_engineThread->stop_engine();
    m_engineThread->Wait();
    // Copy back the state unless we were analyzing.
    // It's important we are stopped before these flags
    // are changed. Our state will be modified by panelBoard
    // so this should match whether that is locked.
    if (!m_analyzing && !m_pondering) {
        assert(m_panelBoard->isLockedState());
        m_State = m_engineThread->get_state();
    } else {
        assert(!m_panelBoard->isLockedState());
    }
    m_engineThread.reset();
    return true;
}

void MainFrame::doToggleTerritory(wxCommandEvent& event) {
    // This is really doToggleOwner but the option is named
    // territory in the menu.
    m_panelBoard->setShowOwner(!m_panelBoard->getShowOwner());

    if (m_panelBoard->getShowOwner()) {
        wxConfig::Get()->Write("showOwnerEnabled", true);
        // Enforce this so we aree checked if we were sent an event externally
        wxMenuItem * territory = m_menuTools->FindItem(ID_SHOWTERRITORY);
        territory->Check(true);

        m_panelBoard->setShowMoyo(false);
        wxMenuItem * moyo = m_menuTools->FindItem(ID_SHOWMOYO);
        moyo->Check(false);
        wxConfig::Get()->Write("showMoyoEnabled", false);

        gameNoLongerCounts();
    } else {
        wxConfig::Get()->Write("showOwnerEnabled", false);
    }

    m_panelBoard->setShowTerritory(false);

    m_panelBoard->Refresh();
}

void MainFrame::doToggleMoyo(wxCommandEvent& event) {
    m_panelBoard->setShowMoyo(!m_panelBoard->getShowMoyo());

    if (m_panelBoard->getShowMoyo()) {
        wxConfig::Get()->Write("showMoyoEnabled", true);
        wxMenuItem * moyo = m_menuTools->FindItem(ID_SHOWMOYO);
        moyo->Check(true);

        m_panelBoard->setShowOwner(false);
        wxMenuItem * territory = m_menuTools->FindItem(ID_SHOWTERRITORY);
        territory->Check(false);
        wxConfig::Get()->Write("showOwnerEnabled", false);

        gameNoLongerCounts();
    } else {
        wxConfig::Get()->Write("showMoyoEnabled", false);
    }

    m_panelBoard->setShowTerritory(false);

    m_panelBoard->Refresh();
}

void MainFrame::doToggleProbabilities(wxCommandEvent& event) {
    m_panelBoard->setShowProbabilities(!m_panelBoard->getShowProbabilities());

    if (m_panelBoard->getShowProbabilities()) {
        wxConfig::Get()->Write("showProbabilitiesEnabled", true);
        wxMenuItem * prob = m_menuTools->FindItem(ID_MOVE_PROBABILITIES);
        prob->Check(true);
        gameNoLongerCounts();
    } else {
        wxConfig::Get()->Write("showProbabilitiesEnabled", false);
    }

    if (m_panelBoard->getShowBestMoves()) {
        wxConfig::Get()->Write("showBestMovesEnabled", false);
        m_panelBoard->setShowBestMoves(false);
        wxMenuItem * bm = m_menuTools->FindItem(ID_BEST_MOVES);
        bm->Check(false);
    }

    m_panelBoard->Refresh();
}

void MainFrame::doToggleBestMoves(wxCommandEvent& event) {
    m_panelBoard->setShowBestMoves(!m_panelBoard->getShowBestMoves());

    if (m_panelBoard->getShowBestMoves()) {
        wxConfig::Get()->Write("showBestMovesEnabled", true);
        wxMenuItem * bm = m_menuTools->FindItem(ID_BEST_MOVES);
        bm->Check(true);
        gameNoLongerCounts();
    } else {
        wxConfig::Get()->Write("showBestMovesEnabled", false);
    }

    if (m_panelBoard->getShowProbabilities()) {
        wxConfig::Get()->Write("showProbabilitiesEnabled", false);
        m_panelBoard->setShowProbabilities(false);
        wxMenuItem * prob = m_menuTools->FindItem(ID_MOVE_PROBABILITIES);
        prob->Check(false);
    }

    m_panelBoard->Refresh();
}

void MainFrame::doNewMove(wxCommandEvent & event) {
    wxLogDebug(_("New move arrived"));

    if (cfg_use_engine == GTP::KATAGO_ENGINE) {
        if (m_katagoStatus != KATAGO_IDLE) {
            m_runflag = false;
            m_post_doNewMove = true;
            return;
        }
    } else {
        stopEngine();
    }

    m_panelBoard->unlockState();
    m_panelBoard->clearViz();

    if (m_pondering) {
        m_pondering = false;
        m_ponderedOnce = true;
    } else {
        m_ponderedOnce = false;
    }

    if (m_State.get_last_move() != FastBoard::PASS) {
        if (m_soundEnabled) {
#ifdef WIN32
            wxSound tock("IDW_TOCK", true);
#else
            wxSound tock(tock_data_length, tock_data);
#endif
            tock.Play(wxSOUND_ASYNC);

        }
    } else {
        if (m_State.get_to_move() == m_playerColor
            && m_State.get_last_move() == FastBoard::PASS) {

            ::wxMessageBox(_("Computer passes"), _("Pass"), wxOK, this);
        }
    }

    if (m_State.get_passes() >= 2 || m_State.get_last_move() == FastBoard::RESIGN) {
        float komi, score, prekomi, handicap;
        bool won = scoreGame(komi, handicap, score, prekomi);
        bool accepts = scoreDialog(komi, handicap, score, prekomi, m_State.get_last_move() != FastBoard::RESIGN);
        if (accepts || m_State.get_last_move() == FastBoard::RESIGN) {
            ratedGameEnd(won);
        } else {
            m_disputing = true;
            // undo passes
            m_State.undo_move();
            m_State.undo_move();

            if (m_State.get_to_move() != m_playerColor) {
                wxLogDebug(_("Computer to move"));
                startEngine();
            } else {
                m_pondering = true;
                startEngine();
            }
        }
    } else {

        if (!m_analyzing) {
            if (m_State.get_to_move() != m_playerColor) {
                wxLogDebug(_("Computer to move"));
                startEngine();
            } else {
                if (m_ponderEnabled && !m_ratedGame && !m_analyzing && !m_ponderedOnce && !m_visitLimit) {
                    m_pondering = true;
                    startEngine();
                }
            }
        } else {
            startEngine();
            m_panelBoard->unlockState();
            m_playerColor = m_State.get_to_move();
            m_panelBoard->setPlayerColor(m_playerColor);
        }
        m_panelBoard->setShowTerritory(false);
    }

    if (!m_ratedGame) {
        SetTitle(_("Leela") +
                 _(" - move ") + wxString::Format(wxT("%i"), m_State.get_movenum() + 1));
    }

    // signal update of visible board
    wxCommandEvent myevent(wxEVT_BOARD_UPDATE, GetId());
    myevent.SetEventObject(this);
    ::wxPostEvent(m_panelBoard->GetEventHandler(), myevent);
    broadcastCurrentMove();
}

void MainFrame::doPaint(wxPaintEvent& event) {
    event.Skip();
}

void MainFrame::doActivate(wxActivateEvent& event) {
    event.Skip();
}

void MainFrame::doResize(wxSizeEvent& event) {
    event.Skip();
}

void MainFrame::doBoardResize(wxSizeEvent& event) {
    event.Skip();
}

void MainFrame::doSettingsDialog(wxCommandEvent& event) {
    bool wasRunning;
    if (cfg_use_engine == GTP::KATAGO_ENGINE) {
        if (m_katagoStatus != KATAGO_IDLE) {
            m_runflag = false;
            m_wasRunning = true;
            m_update_score = false;
            m_post_doSettingsDialog = true;
            return;
        }
        wasRunning = m_wasRunning;
    } else {
        wasRunning = stopEngine();
    }
    bool wasPondering = m_pondering;
    bool wasAnalyzing = wasRunning && !wasPondering;

    SettingsDialog mydialog(this);

    if (mydialog.ShowModal() == wxID_OK) {
        wxLogDebug(_("OK clicked"));

        m_netsEnabled = wxConfig::Get()->Read(wxT("netsEnabled"), 1);
        m_passEnabled = wxConfig::Get()->Read(wxT("passEnabled"), 1);
        m_soundEnabled = wxConfig::Get()->Read(wxT("soundEnabled"), 1);
        m_resignEnabled = wxConfig::Get()->Read(wxT("resignEnabled"), 1);
        m_ponderEnabled = wxConfig::Get()->Read(wxT("ponderEnabled"), 1);
    }

    wxCommandEvent dummy;
    if (wasAnalyzing) doAnalyze(dummy);
}

void MainFrame::doNewGame(wxCommandEvent& event) {
    if (cfg_use_engine == GTP::KATAGO_ENGINE) {
        if (m_katagoStatus != KATAGO_IDLE) {
            m_runflag = false;
            m_update_score = false;
            m_post_doNewGame = true;
            return;
        }
    } else {
        stopEngine(false);
    }

    NewGameDialog mydialog(this);

    if (mydialog.ShowModal() == wxID_OK) {
        wxLogDebug(_("OK clicked"));

        m_State.init_game(mydialog.getBoardsize(), mydialog.getKomi());
        ::wxBeginBusyCursor();
        CalculateDialog calcdialog(this);
        calcdialog.Show();
        //::wxSafeYield();
        m_State.set_timecontrol(30 * 100, 0, 0, 0);

        m_move_handi = m_State.place_free_handicap(mydialog.getHandicap());
        calcdialog.Hide();
        ::wxEndBusyCursor();
        MCOwnerTable::get_MCO()->clear();
        m_panelBoard->clearViz();
        if (m_scoreHistogramWindow) {
            m_scoreHistogramWindow->ClearHistogram();
        }
        m_State.set_timecontrol(mydialog.getTimeControl() * 60 * 100, 0, 0, 0);
        m_StateStack.clear();
        m_visitLimit = mydialog.getSimulations();
        m_playerColor = mydialog.getPlayerColor();
        // XXX
        m_netsEnabled = mydialog.getNetsEnabled();
        wxConfig::Get()->Write(wxT("netsEnabled"), m_netsEnabled);
        setActiveMenus();
        // XXX
        m_panelBoard->setPlayerColor(m_playerColor);
        m_panelBoard->setShowTerritory(false);
        m_analyzing = false;
        m_pondering = false;
        m_disputing = false;
        gameNoLongerCounts();

        wxCommandEvent myevent(wxEVT_NEW_MOVE, GetId());
        myevent.SetEventObject(this);
        ::wxPostEvent(m_panelBoard->GetEventHandler(), myevent);
        broadcastCurrentMove();
    }
}

void MainFrame::setActiveMenus() {
    int boardsize = m_State.board.get_boardsize();
    if (cfg_use_engine == GTP::ORIGINE_ENGINE && boardsize != 19) {
        m_menuTools->FindItem(ID_MOVE_PROBABILITIES)->Enable(false);
    } else {
        m_menuTools->FindItem(ID_MOVE_PROBABILITIES)->Enable(true);
    }
    if (m_StateStack.empty()) {
        m_menuAnalyze->FindItem(ID_POPPOS)->Enable(false);
        m_menuAnalyze->FindItem(ID_MAINLINE)->Enable(false);
        GetToolBar()->EnableTool(ID_POPPOS, false);
        GetToolBar()->EnableTool(ID_MAINLINE, false);
    } else {
        m_menuAnalyze->FindItem(ID_POPPOS)->Enable(true);
        m_menuAnalyze->FindItem(ID_MAINLINE)->Enable(true);
        GetToolBar()->EnableTool(ID_POPPOS, true);
        GetToolBar()->EnableTool(ID_MAINLINE, true);
    }
}

void MainFrame::doSetRatedSize(wxCommandEvent& event) {
    RatedSizeDialog mydialog(this);
    mydialog.ShowModal();

    m_ratedSize = mydialog.getSizeSelected();

    wxConfig::Get()->Write(wxT("ratedSize"), m_ratedSize);

    doNewRatedGame(event);
}

void MainFrame::doNewRatedGame(wxCommandEvent& event) {
    if (cfg_use_engine == GTP::KATAGO_ENGINE) {
        if (m_katagoStatus != KATAGO_IDLE) {
            m_runflag = false;
            m_update_score = false;
            m_post_doNewRatedGame = true;
            return;
        }
    } else {
        stopEngine(false);
    }

    m_analyzing = false;
    m_disputing = false;
    m_pondering = false;

    int rank = 0;
    if (m_ratedSize == 9) {
        rank = wxConfig::Get()->ReadLong(wxT("userRank9"), (long)-30);
    } else if (m_ratedSize == 19) {
        rank = wxConfig::Get()->ReadLong(wxT("userRank19"), (long)-15);
    }

    wxLogDebug(_("Last rank was: %d"), rank);

    wxString mess = wxString(_("Your rank: "));

    mess += rankToString(rank);
    m_statusBar->SetStatusText(mess, 1);

    SetTitle(_("Leela - ") + mess);
    if (m_analysisWindow) {
        m_analysisWindow->Close();
    }
    if (m_scoreHistogramWindow) {
        m_scoreHistogramWindow->Hide();
        m_menuAnalyze->FindItem(ID_SCOREHISTOGRAMTOGGLE)->Check(false);
    }

    int used_rank = rank;
    int handicap;
    int simulations;

    // Correct for neural network enabled
    if (m_ratedSize == 19) {
        if (m_netsEnabled) {
            // Estimate it as being worth 7 stones
            used_rank = rank - 7;
        }
    }

    if (m_ratedSize == 9) {
        if (used_rank == -30) {
            simulations =  250;
            handicap = 5;
        } else if (used_rank == -29) {
            simulations =  500;
            handicap = 5;
        } else if (used_rank == -28) {
            simulations = 1000;
            handicap = 5;
        } else if (used_rank == -27) {
            simulations = 2500;
            handicap = 5;
        } else if (used_rank == -26) {
            simulations = 5000;
            handicap = 5;
        } else if (used_rank == -25) {
            simulations = 10000;
            handicap = 5;
        } else if (used_rank == -24) {
            simulations = 250;
            handicap = 4;
        } else if (used_rank == -23) {
            simulations = 500;
            handicap = 4;
        } else if (used_rank == -22) {
            simulations = 1000;
            handicap = 4;
        } else if (used_rank == -21) {
            simulations = 2500;
            handicap = 4;
        } else if (used_rank == -20) {
            simulations = 5000;
            handicap = 4;
        } else if (used_rank == -19) {
            simulations = 10000;
            handicap = 4;
        } else if (used_rank == -18) {
            simulations = 250;
            handicap = 3;
        } else if (used_rank == -17) {
            simulations = 500;
            handicap = 3;
        } else if (used_rank == -16) {
            simulations = 1000;
            handicap = 3;
        } else if (used_rank == -15) {
            simulations = 2500;
            handicap = 3;
        } else if (used_rank == -14) {
            simulations = 5000;
            handicap = 3;
        } else if (used_rank == -13) {
            simulations = 10000;
            handicap = 3;
        } else if (used_rank == -12) {
            simulations = 250;
            handicap = 2;
        } else if (used_rank == -11) {
            simulations = 500;
            handicap = 2;
        } else if (used_rank == -10) {
            simulations = 1000;
            handicap = 2;
        } else if (used_rank == -9) {
            simulations = 2500;
            handicap = 2;
        } else if (used_rank == -8) {
            simulations = 5000;
            handicap = 2;
        } else if (used_rank == -7) {
            simulations = 10000;
            handicap = 2;
        } else if (used_rank == -6) {
            simulations = 250;
            handicap = 0;
        } else if (used_rank == -5) {
            simulations = 500;
            handicap = 0;
        } else if (used_rank == -4) {
            simulations = 1000;
            handicap = 0;
        } else if (used_rank == -3) {
            simulations = 2500;
            handicap = 0;
        } else if (used_rank == -2) {
            simulations = 5000;
            handicap = 0;
        } else if (used_rank == -1) {
            simulations = 10000;
            handicap = 0;
        } else if (used_rank == 0) {
            simulations = 250;
            handicap = -2;
        } else if (used_rank == 1) {
            simulations = 500;
            handicap = -2;
        } else if (used_rank == 2) {
            simulations = 1000;
            handicap = -2;
        } else if (used_rank == 3) {
            simulations = 2500;
            handicap = -2;
        } else if (used_rank == 4) {
            simulations = 5000;
            handicap = -2;
        } else if (used_rank == 5) {
            simulations = 10000;
            handicap = -2;
        } else if (used_rank == 6) {
            simulations = 500;
            handicap = -3;
        } else if (used_rank == 7) {
            simulations = 1000;
            handicap = -3;
        } else if (used_rank == 8) {
            simulations = 2500;
            handicap = -3;
        } else if (used_rank == 9) {
            simulations = 5000;
            handicap = -3;
        } else if (used_rank == 10) {
            simulations = 10000;
            handicap = -3;
        } else if (used_rank == 11) {
            simulations = 2500;
            handicap = -4;
        } else if (used_rank == 12) {
            simulations = 5000;
            handicap = -4;
        } else if (used_rank == 13) {
            simulations = 10000;
            handicap = -4;
        }
    } else if (m_ratedSize == 19) {
        if (used_rank == -37) {
            simulations = 2500;
            handicap = 36;
        } else if (used_rank == -36) {
            simulations = 2500;
            handicap = 35;
        } else if (used_rank == -35) {
            simulations = 2500;
            handicap = 34;
        } else if (used_rank == -34) {
            simulations = 2500;
            handicap = 33;
        } else if (used_rank == -33) {
            simulations = 2500;
            handicap = 32;
        } else if (used_rank == -32) {
            simulations = 2500;
            handicap = 31;
        } else if (used_rank == -31) {
            simulations = 2500;
            handicap = 30;
        } else if (used_rank == -30) {
            simulations = 2500;
            handicap = 29;
        } else if (used_rank == -29) {
            simulations = 2500;
            handicap = 28;
        } else if (used_rank == -28) {
            simulations = 2500;
            handicap = 27;
        } else if (used_rank == -27) {
            simulations = 2500;
            handicap = 26;
        } else if (used_rank == -26) {
            simulations = 2500;
            handicap = 25;
        } else if (used_rank == -25) {
            simulations = 2500;
            handicap = 24;
        } else if (used_rank == -24) {
            simulations = 2500;
            handicap = 23;
        } else if (used_rank == -23) {
            simulations = 2500;
            handicap = 22;
        } else if (used_rank == -22) {
            simulations = 2500;
            handicap = 21;
        } else if (used_rank == -21) {
            simulations = 2500;
            handicap = 20;
        } else if (used_rank == -20) {
            simulations = 2500;
            handicap = 19;
        } else if (used_rank == -19) {
            simulations = 2500;
            handicap = 18;
        } else if (used_rank == -18) {
            simulations = 2500;
            handicap = 17;
        } else if (used_rank == -17) {
            simulations = 2500;
            handicap = 16;
        } else if (used_rank == -16) {
            simulations = 2500;
            handicap = 15;
        } else if (used_rank == -15) {
            simulations = 2500;
            handicap = 14;
        } else if (used_rank == -14) {
            simulations = 2500;
            handicap = 13;
        } else if (used_rank == -13) {
            simulations = 2500;
            handicap = 12;
        } else if (used_rank == -12) {
            simulations = 2500;
            handicap = 11;
        } else if (used_rank == -11) {
            simulations = 2500;
            handicap = 10;
        } else if (used_rank == -10) {
            simulations = 2500;
            handicap = 9;
        } else if (used_rank == -9) {
            simulations = 2500;
            handicap = 8;
        } else if (used_rank == -8) {                // 1 kyu
            simulations = 2500;
            handicap = 7;
        } else if (used_rank == -7) {                // 1 dan
            simulations = 2500;
            handicap = 6;
        } else if (used_rank == -6) {
            simulations = 2500;
            handicap = 5;
        } else if (used_rank == -5) {
            simulations = 2500;
            handicap = 4;
        } else if (used_rank == -4) {
            simulations = 2500;
            handicap = 3;
        } else if (used_rank == -3) {
            simulations = 2500;
            handicap = 2;
        } else if (used_rank == -2) {                // 6 dan
            simulations = 2500;
            handicap = 0;
        } else if (used_rank == -1) { // 1 kyu       // 7 dan
            simulations = 5000;
            handicap = 0;
        } else if (used_rank == 0) {  // 1 dan       // 1 pro
            simulations = 10000;
            handicap = 0;
        } else if (used_rank == 1) {
            simulations =  2500;
            handicap = -2;
        } else if (used_rank == 2) {
            simulations =  5000;
            handicap = -2;
        } else if (used_rank == 3) {
            simulations =  3750;
            handicap = -3;
        } else if (used_rank == 4) {
            simulations =  5000;
            handicap = -4;
        } else if (used_rank == 5) {
            simulations =  5000;
            handicap = -6;
        } else if (used_rank == 6) {
            simulations =  5000;
            handicap = -8;
        } else if (used_rank == 7) {
            simulations =  5000;
            handicap = -12;
        } else if (used_rank == 8) {
            simulations =  5000;
            handicap = -14;
        } else if (used_rank == 9) {
            simulations =  7500;
            handicap = -16;
        } else if (used_rank == 10) {
            simulations =  7500;
            handicap = -18;
        } else if (used_rank == 11) {
            simulations =  7500;
            handicap = -20;
        } else if (used_rank == 12) {
            simulations = 10000;
            handicap = -22;
        } else if (used_rank == 13) {
            simulations = 10000;
            handicap = -24;
        }
    }

    wxLogDebug(_("Handicap %d Simulations %d"), handicap, simulations);

    {
        float komi;
        if (cfg_use_engine == GTP::ORIGINE_ENGINE || !m_japanese_rule) {
            komi = handicap ? 0.5f : 7.5f;
        } else {
            komi = handicap ? 0.5f : 6.5f;
        }
        m_State.init_game(m_ratedSize, komi);
        ::wxBeginBusyCursor();
        CalculateDialog calcdialog(this);
        calcdialog.Show();
        //::wxSafeYield();
        m_State.set_timecontrol(30 * 100, 0, 0, 0);

        m_move_handi = m_State.place_free_handicap(abs(handicap));
        calcdialog.Hide();
        ::wxEndBusyCursor();
        // max 60 minutes per game
        if (m_scoreHistogramWindow) {
            m_scoreHistogramWindow->ClearHistogram();
        }
        m_StateStack.clear();
        MCOwnerTable::get_MCO()->clear();
        if (cfg_use_engine == GTP::ORIGINE_ENGINE) {
            m_State.set_timecontrol(2 * m_ratedSize * 60 * 100, 0, 0, 0);
            m_visitLimit = simulations;
        } else {
            m_State.set_timecontrol(m_ratedSize / 3 * 60 * 100, 0, 0, 0);
            m_visitLimit = simulations / 10;
        }
        m_playerColor = (handicap >= 0 ? FastBoard::BLACK : FastBoard::WHITE);
        m_panelBoard->setPlayerColor(m_playerColor);
        m_panelBoard->setShowTerritory(false);
        m_ratedGame = true;

        setActiveMenus();
    }

    wxCommandEvent myevent(wxEVT_NEW_MOVE, GetId());
    myevent.SetEventObject(this);
    ::wxPostEvent(m_panelBoard->GetEventHandler(), myevent);
    broadcastCurrentMove();
}

void MainFrame::ratedGameEnd(bool won) {
    if (!m_ratedGame) {
        return;
    }

    //wxString mess;
    wxString rankstr;
    long rank;
    long adjust;
    bool hadWon;

    if (m_ratedSize == 9) {
        adjust = wxConfig::Get()->ReadLong(wxT("userRankAdjust9"), 10);
    } else {
        adjust = wxConfig::Get()->ReadLong(wxT("userRankAdjust19"), 10);
    }

    if (m_ratedSize == 9) {
        rank = wxConfig::Get()->ReadLong(wxT("userRank9"), (long)-30);
        hadWon = wxConfig::Get()->ReadBool(wxT("userLastWon9"), true);
        if (won) {
            wxConfig::Get()->Write(wxT("userLastWon9"), true);
        } else {
            wxConfig::Get()->Write(wxT("userLastWon9"), false);
        }
    } else {
        assert(m_ratedSize == 19);
        rank = wxConfig::Get()->ReadLong(wxT("userRank19"), (long)-15);
        hadWon = wxConfig::Get()->ReadBool(wxT("userLastWon19"), true);
        if (won) {
            wxConfig::Get()->Write(wxT("userLastWon19"), true);
        } else {
            wxConfig::Get()->Write(wxT("userLastWon19"), false);
        }
    }

    if (won != hadWon) {
        adjust /= 2;
    }
    adjust = std::max(1L, adjust);

    if (won) {
        rank = rank + adjust;
        rank = std::min(MAX_RANK, rank);
    } else {
        rank = rank - adjust;
        rank = std::max(MIN_RANK, rank);
    }

    if (m_ratedSize == 9) {
        wxConfig::Get()->Write(wxT("userRank9"), rank);
        wxConfig::Get()->Write(wxT("userRankAdjust9"), adjust);
    } else {
        assert(m_ratedSize == 19);
        wxConfig::Get()->Write(wxT("userRank19"), rank);
        wxConfig::Get()->Write(wxT("userRankAdjust19"), adjust);
    }

    wxString mess = wxString(_("Your rank: "));
    mess += rankToString(rank);
    m_statusBar->SetStatusText(mess, 1);

    // don't adjust rank twice
    m_ratedGame = false;
}

bool MainFrame::scoreGame(float & komi, float & handicap,
                          float & score, float & prekomi) {
    bool won;
    if (cfg_use_engine == GTP::ORIGINE_ENGINE) {
        stopEngine();
    }

    if (m_State.get_last_move() == FastBoard::RESIGN) {
        komi = m_State.get_komi();
        handicap = m_State.get_handicap();

        int size = m_State.board.get_boardsize() * m_State.board.get_boardsize();
        if (m_State.get_to_move() == FastBoard::WHITE) {
            score = -size;
        } else {
            score = size;
        }
        prekomi = score + komi + handicap;
    } else {
        komi = m_State.get_komi();
        if (cfg_use_engine == GTP::ORIGINE_ENGINE) {
            score = m_State.final_score(nullptr, m_disputing ? false : true);
        } else {
            score = ceil(m_State.m_black_score) - 0.5;
        }
        handicap = m_State.get_handicap();
        prekomi = score + komi + handicap;
    }

    won = (score > 0.0f && m_playerColor == FastBoard::BLACK)
          || (score < 0.0f && m_playerColor == FastBoard::WHITE);

    m_panelBoard->doTerritory(m_disputing);
    m_panelBoard->setShowTerritory(true);

    return won;
}

bool MainFrame::scoreDialog(float komi, float handicap,
                            float score, float prekomi, bool dispute) {
    wxString mess;

    if (score > 0.0f) {
        if (m_State.get_last_move() == FastBoard::RESIGN) {
            mess.Printf(_("BLACK wins by resignation"));
        } else {
            if (handicap > 0.5f) {
                mess.Printf(_("BLACK wins by %.0f - %.1f (komi) - %0.f (handicap)\n= %.1f points"),        
                            prekomi, komi, handicap, score);
            } else {
                mess.Printf(_("BLACK wins by %.0f - %.1f (komi)\n= %.1f points"),        
                            prekomi, komi, score);
            }
        }
    } else {
        // avoid minus zero
        prekomi = prekomi - 0.001f;
        score = score - 0.001f;
        if (m_State.get_last_move() == FastBoard::RESIGN) {
            mess.Printf(_("WHITE wins by resignation"));
        } else {
            if (handicap > 0.5f) {
                mess.Printf(_("WHITE wins by %.0f + %.1f (komi) + %0.f (handicap)\n= %.1f points"),
                            -prekomi, komi, handicap, -score);
            } else {
                mess.Printf(_("WHITE wins by %.0f + %.1f (komi)\n= %.1f points"),
                            -prekomi, komi, -score);
            }
        }
    }

    wxString confidence("");
    if (dispute) {
        if (score > 0.0f) {
            confidence = _("BLACK wins.");
        } else {
            confidence = _("WHITE wins.");
        }
    } else if (cfg_use_engine == GTP::ORIGINE_ENGINE && m_State.board.get_boardsize() == 19) {
        float net_score = Network::get_Network()->get_value(&m_State,
                                                            Network::Ensemble::AVERAGE_ALL);
        net_score = (m_State.get_to_move() == FastBoard::BLACK) ?
                    net_score : (1.0f - net_score);
        if (m_State.get_last_move() != FastBoard::RESIGN) {
            if ((score > 0.0f && net_score < 0.5f)
                || (score < 0.0f && net_score > 0.5f)) {
                confidence = _("I am not sure I am scoring this correctly.");
            }
        }
    }

    ScoreDialog mydialog(this, mess, confidence, dispute);

    if (mydialog.ShowModal() == wxID_OK) {
        bool result = mydialog.Accepted();
        if (result) {
            return true;
        } else {
            return false;
        }
    }

    return true;
}

void MainFrame::doScore(wxCommandEvent& event) {
    if (cfg_use_engine == GTP::KATAGO_ENGINE) {
        if (m_katagoStatus != KATAGO_IDLE) {
            m_runflag = false;
            m_post_doScore = true;
            return;
        }
    }
    float komi, score, prekomi, handicap;

    scoreGame(komi, handicap, score, prekomi);
    scoreDialog(komi, handicap, score, prekomi);
}

wxString MainFrame::rankToString(int rank) {
    wxString res;

    if (rank < 0) {
        res.Printf(_("%d kyu"), -rank);
    } else {
        if (rank < 7) {
            res.Printf(_("%d dan"), rank + 1);
        } else {
            res.Printf(_("%d pro"), rank - 6);
        }
    }

    return res;
}

void MainFrame::doPass(wxCommandEvent& event) {
    if (cfg_use_engine == GTP::KATAGO_ENGINE) {
        if (m_katagoStatus != KATAGO_IDLE) {
            m_runflag = false;
            m_post_doPass = true;
            return;
        }
    } else {
        stopEngine();
    }
    m_State.play_pass();
    //::wxLogMessage("User passes");
    wxCommandEvent myevent(wxEVT_NEW_MOVE, GetId());
    myevent.SetEventObject(this);
    ::wxPostEvent(m_panelBoard->GetEventHandler(), myevent);
    broadcastCurrentMove();
}

void MainFrame::gameNoLongerCounts() {
    SetTitle(_("Leela") +
             _(" - move ") + wxString::Format(wxT("%i"), m_State.get_movenum() + 1));
    m_ratedGame = false;
}

void MainFrame::doRealUndo(int count) {
    bool wasRunning;
    if (cfg_use_engine == GTP::KATAGO_ENGINE) {
        if (m_katagoStatus != KATAGO_IDLE) {
            m_runflag = false;
            m_wasRunning = true;
            m_post_doRealUndo = count;
            return;
        }
        wasRunning = m_wasRunning;
    } else {
        wasRunning = stopEngine();
    }
    bool wasAnalyzing = m_analyzing && !m_pondering;
    for (int i = 0; i < count; i++) {
        if (m_State.undo_move()) {
            wxLogDebug(_("Undoing one move"));
        }
    }
    doPostMoveChange(wasAnalyzing && wasRunning);
}

void MainFrame::doRealForward(int count) {
    bool wasRunning;
    if (cfg_use_engine == GTP::KATAGO_ENGINE) {
        if (m_katagoStatus != KATAGO_IDLE) {
            m_runflag = false;
            m_wasRunning = true;
            m_post_doRealForward = count;
            return;
        }
        wasRunning = m_wasRunning;
    } else {
        wasRunning = stopEngine();
    }
    bool wasAnalyzing = m_analyzing && !m_pondering;
    for (int i = 0; i < count; i++) {
        if (m_State.forward_move()) {
            wxLogDebug(_("Forward one move"));
        }
    }
    doPostMoveChange(wasAnalyzing && wasRunning);
}

void MainFrame::doPostMoveChange(bool wasAnalyzing) {
    m_playerColor = m_State.get_to_move();
    MCOwnerTable::get_MCO()->clear();
    m_panelBoard->setPlayerColor(m_playerColor);
    m_panelBoard->setShowTerritory(false);
    m_panelBoard->clearViz();

    gameNoLongerCounts();

    wxCommandEvent myevent(wxEVT_BOARD_UPDATE, GetId());
    myevent.SetEventObject(this);
    ::wxPostEvent(m_panelBoard->GetEventHandler(), myevent);
    broadcastCurrentMove();

    if (wasAnalyzing) doAnalyze(myevent /* dummy */);
}

void MainFrame::doBack10(wxCommandEvent& event) {
    doRealUndo(10);
}

void MainFrame::doForward10(wxCommandEvent& event) {
    doRealForward(10);
}

void MainFrame::doGoRules(wxCommandEvent& event) {
    ::wxLaunchDefaultBrowser("http://senseis.xmp.net/?RulesOfGoIntroductory");
}

void MainFrame::doHomePage(wxCommandEvent& event) {
   ::wxLaunchDefaultBrowser("https://sjeng.org/leela");
}

void MainFrame::doHelpAbout(wxCommandEvent& event) {
    AboutDialog myabout(this);

    myabout.ShowModal();
}

void MainFrame::loadSGFString(const wxString & SGF, int movenum) {
    auto tree = std::make_unique<SGFTree>();
    try {
        const wxScopedCharBuffer sgfUtf8(SGF.ToUTF8());
        std::string sgfstring(sgfUtf8);
        std::istringstream strm(sgfstring);
        auto games = SGFParser::chop_stream(strm);
        if (games.size() < 1) {
            return;
        }
        tree->load_from_string(games[0]);
        m_State = tree->follow_mainline_state();
        int last_move = tree->count_mainline_moves();
        movenum = std::max(1, movenum);
        wxLogDebug(_("Read %d moves, going to move %d"), last_move, movenum);
        m_State.rewind();
        m_State.m_policy.clear();
        m_State.m_owner.clear();
        m_State.m_black_score = 0.0;
        m_japanese_rule = tree->get_rule();
        for (size_t i = 0; i < m_State.m_win_rate.size(); i++) {
            m_State.m_win_rate[i] = 100.0;
        }
        m_move_handi.clear();
        for (int i = 0; i < m_State.board.get_boardsize(); i++) {
            for (int j = 0; j < m_State.board.get_boardsize(); j++) {
                if (m_State.board.get_square(i, j) == FastBoard::BLACK) {
                    m_move_handi.emplace_back(m_State.board.get_vertex(i, j));
                }
            }
        }
        for (int i = 1; i < movenum; ++i) {
            m_State.forward_move();
        }
    } catch (...) {
    }

    // Reset to New Game dialog defaults
    int minutes = wxConfig::Get()->ReadLong(wxT("DefaultMinutes"), (long)20);
    m_State.set_timecontrol(minutes * 60 * 100, 0, 0, 0);
    bool nets = wxConfig::Get()->ReadBool(wxT("netsEnabled"), true);
    m_netsEnabled = (m_State.board.get_boardsize() == 19 ? nets : false);
    int simulations = wxConfig::Get()->ReadLong(wxT("DefaultSimulations"), (long)6);
    m_visitLimit = NewGameDialog::simulationsToVisitLimit(simulations);

    m_StateStack.clear();
    m_StateStack.push_back(m_State);
    m_playerColor = m_State.get_to_move();
    MCOwnerTable::get_MCO()->clear();
    m_panelBoard->setPlayerColor(m_playerColor);
    m_panelBoard->setShowTerritory(false);
    m_panelBoard->clearViz();
    if (m_scoreHistogramWindow) {
        m_scoreHistogramWindow->ClearHistogram();
    }
    gameNoLongerCounts();
    setActiveMenus();

    //signal board change
    wxCommandEvent myevent(wxEVT_BOARD_UPDATE, GetId());
    myevent.SetEventObject(this);
    ::wxPostEvent(m_panelBoard->GetEventHandler(), myevent);
    broadcastCurrentMove();
}

void MainFrame::doOpenSGF(wxCommandEvent& event) {
    if (cfg_use_engine == GTP::KATAGO_ENGINE) {
        if (m_katagoStatus != KATAGO_IDLE) {
            m_runflag = false;
            m_update_score = false;
            m_post_doOpenSGF = true;
            return;
        }
    } else {
        stopEngine(false);
    }

    wxString caption = _("Choose a file");
    wxString wildcard = _("Go games (*.sgf)|*.sgf");
    wxFileDialog dialog(this, caption, wxEmptyString, wxEmptyString, wildcard,
                        wxFD_OPEN | wxFD_CHANGE_DIR | wxFD_FILE_MUST_EXIST);

    if (dialog.ShowModal() == wxID_OK) {
        wxString path = dialog.GetPath();
        wxLogDebug(_("Opening: ") + path);

        // open the file
        wxFile sgfFile;
        if (sgfFile.Open(path)) {
            wxString SGF;

            if (sgfFile.ReadAll(&SGF)) {
                loadSGFString(SGF);
            }
        }
    }
}

void MainFrame::doSaveSGF(wxCommandEvent& event) {
    if (cfg_use_engine == GTP::KATAGO_ENGINE) {
        if (m_katagoStatus != KATAGO_IDLE) {
            m_runflag = false;
            m_post_doSaveSGF = true;
            return;
        }
    } else {
        stopEngine();
    }

    std::string sgfgame = SGFTree::state_to_string(&m_State, !m_playerColor, m_japanese_rule);

    wxString caption = _("Choose a file");
    wxString wildcard = _("Go games (*.sgf)|*.sgf");
    wxFileDialog dialog(this, caption, wxEmptyString, wxEmptyString, wildcard,
                        wxFD_SAVE | wxFD_CHANGE_DIR | wxFD_OVERWRITE_PROMPT);

    if (dialog.ShowModal() == wxID_OK) {
        wxString path = dialog.GetPath();

        wxLogDebug(_("Saving: " + path));

        wxFileOutputStream file(path);

        if (file.IsOk()) {
            file.Write(sgfgame.c_str(), sgfgame.size());
        }
    }
}

void MainFrame::doForceMove(wxCommandEvent& event) {
    bool wasRunning;
    if (cfg_use_engine == GTP::KATAGO_ENGINE) {
        if (m_katagoStatus != KATAGO_IDLE) {
            m_runflag = false;
            m_wasRunning = true;
            m_post_doForceMove = true;
            return;
        }
        wasRunning = m_wasRunning;
    } else {
        wasRunning = stopEngine();
    }
    gameNoLongerCounts();
    m_ponderedOnce = true;
    bool wasAnalyzing = m_analyzing && !m_pondering;
    bool wasPondering = m_pondering;
    if (!wasRunning || wasAnalyzing || wasPondering) {
        m_analyzing = false;
        m_pondering = false;
        m_playerColor = !m_State.get_to_move();
        m_panelBoard->setPlayerColor(m_playerColor);
        startEngine();
    }
}

void MainFrame::doResign(wxCommandEvent& event) {
    if (m_State.get_to_move() == m_playerColor) {
        if (cfg_use_engine == GTP::KATAGO_ENGINE) {
            if (m_katagoStatus != KATAGO_IDLE) {
                m_runflag = false;
                m_post_doResign = true;
                return;
            }
        } else {
            stopEngine();
        }
        m_State.play_move(FastBoard::RESIGN);
        wxCommandEvent myevent(wxEVT_NEW_MOVE, GetId());
        myevent.SetEventObject(this);
        ::wxPostEvent(GetEventHandler(), myevent);
    }
}

void MainFrame::doAnalyze(wxCommandEvent& event) {
    bool wasRunning;
    if (cfg_use_engine == GTP::KATAGO_ENGINE) {
        if (m_katagoStatus != KATAGO_IDLE) {
            m_runflag = false;
            m_wasRunning = true;
            m_post_doAnalyze = true;
            return;
        }
        wasRunning = m_wasRunning;
    } else {
        wasRunning = stopEngine();
    }
    gameNoLongerCounts();
    bool wasAnalyzing = m_analyzing && !m_pondering;
    m_ponderedOnce |= wasRunning;
    m_pondering = false;

    if (!wasAnalyzing || !wasRunning) {
        if (!wasAnalyzing) {
            m_StateStack.push_back(m_State);
            setActiveMenus();
        }
        m_analyzing = true;
        startEngine();
    } else if (wasAnalyzing) {
        m_analyzing = false;
        SetStatusBarText(_("Analysis stopped"), 1);
    }
    m_panelBoard->unlockState();
    m_playerColor = m_State.get_to_move();
    m_panelBoard->setPlayerColor(m_playerColor);
}

void MainFrame::doAdjustClocks(wxCommandEvent& event) {
    ClockAdjustDialog mydialog(this);

    mydialog.setTimeControl(m_State.get_timecontrol());

    if (mydialog.ShowModal() == wxID_OK) {
        wxLogDebug(_("Adjust clocks clicked"));

        m_State.set_timecontrol(mydialog.getTimeControl());
    }
}

void MainFrame::doKeyDown(wxKeyEvent& event) {
    auto keycode = event.GetKeyCode();
    if (keycode == WXK_LEFT) {
        doRealUndo();
    } else if (keycode == WXK_RIGHT) {
        doRealForward();
    } else if (keycode == WXK_TAB) {
        HandleAsNavigationKey(event);
    } else {
        event.Skip();
    }
}

void MainFrame::doShowHideAnalysisWindow(wxCommandEvent& event) {
    if (!m_analysisWindow) {
        m_analysisWindow = new AnalysisWindow(this);
        if (event.GetInt() == NO_WINDOW_AUTOSIZE) {
            m_analysisWindow->disableAutoSize();
        }
        m_analysisWindow->Show();
        m_menuAnalyze->FindItem(ID_ANALYSISWINDOWTOGGLE)->Check(true);
        gameNoLongerCounts();
    } else {
        if (!m_analysisWindow->IsShown()) {
            m_analysisWindow->Show();
        } else {
            m_analysisWindow->Hide();
        }
    }
}

void MainFrame::doShowHideScoreHistogram( wxCommandEvent& event ) {
    if (!m_scoreHistogramWindow) {
        m_scoreHistogramWindow = new ScoreHistogram(this);
        m_scoreHistogramWindow->Show();
        m_menuAnalyze->FindItem(ID_SCOREHISTOGRAMTOGGLE)->Check(true);
        gameNoLongerCounts();
    } else {
        if (!m_scoreHistogramWindow->IsShown()) {
            m_scoreHistogramWindow->Show();
            m_menuAnalyze->FindItem(ID_SCOREHISTOGRAMTOGGLE)->Check(true);
            gameNoLongerCounts();
        } else {
            m_menuAnalyze->FindItem(ID_SCOREHISTOGRAMTOGGLE)->Check(false);
            m_scoreHistogramWindow->Hide();
        }
    }
}

void MainFrame::doCloseChild( wxWindowDestroyEvent& event ) {
    if (event.GetWindow() == m_analysisWindow) {
        m_menuAnalyze->FindItem(ID_ANALYSISWINDOWTOGGLE)->Check(false);
        m_analysisWindow = nullptr;
    }
    if (event.GetWindow() == m_scoreHistogramWindow) {
        m_menuAnalyze->FindItem(ID_SCOREHISTOGRAMTOGGLE)->Check(false);
        m_scoreHistogramWindow = nullptr;
    }
}

void MainFrame::broadcastCurrentMove() {
    if (m_scoreHistogramWindow) {
        wxCommandEvent* cmd = new wxCommandEvent(wxEVT_SET_MOVENUM);
        cmd->SetInt(m_State.get_movenum());
        m_scoreHistogramWindow->GetEventHandler()->QueueEvent(cmd);
    }
}

void MainFrame::doMainLine(wxCommandEvent& event) {
    assert(!m_StateStack.empty());
    if (m_StateStack.empty()) return;
    m_State = m_StateStack.back();
    m_panelBoard->unlockState();
    m_playerColor = m_State.get_to_move();
    m_panelBoard->setPlayerColor(m_playerColor);
    gameNoLongerCounts();

    //signal board change
    wxCommandEvent myevent(wxEVT_BOARD_UPDATE, GetId());
    myevent.SetEventObject(this);
    ::wxPostEvent(m_panelBoard->GetEventHandler(), myevent);
    broadcastCurrentMove();
}

void MainFrame::doPushPosition( wxCommandEvent& event ) {
    m_StateStack.push_back(m_State);
    setActiveMenus();
}

void MainFrame::doPopPosition( wxCommandEvent& event ) {
    assert(!m_StateStack.empty());
    if (m_StateStack.empty()) return;
    doMainLine(event);
    m_StateStack.pop_back();
    gameNoLongerCounts();
    setActiveMenus();
}

void MainFrame::gotoMoveNum(wxCommandEvent& event) {
    int movenum = event.GetInt();

    int current_move = m_State.get_movenum();
    if (movenum > current_move) {
        doRealForward(movenum - current_move);
    } else if (movenum < current_move) {
        doRealUndo(current_move - movenum);
    }
}

void MainFrame::doEvalUpdate(wxCommandEvent& event) {
    if (m_scoreHistogramWindow) {
        m_scoreHistogramWindow->GetEventHandler()->AddPendingEvent(event);
    } else {
        // Need to free up the analysis data
        if (!event.GetClientData()) return;

        delete reinterpret_cast<std::tuple<int, float, float, float>*>(event.GetClientData());
    }
}
void MainFrame::doCopyClipboard(wxCommandEvent& event) {
    if (wxTheClipboard->Open()) {
        std::string sgfgame = SGFTree::state_to_string(&m_State, !m_playerColor, m_japanese_rule);
        auto data = std::make_unique<wxTextDataObject>(wxString(sgfgame));
        wxTheClipboard->SetData(data.release());
        wxTheClipboard->Flush();
        wxTheClipboard->Close();
    }
}

void MainFrame::doPasteClipboard(wxCommandEvent& event) {
    if (wxTheClipboard->Open()) {
        if (wxTheClipboard->IsSupported(wxDF_TEXT)) {
            wxTextDataObject data;
            wxTheClipboard->GetData(data);
            std::string sgfstring = data.GetText().ToStdString();
            std::istringstream strm(sgfstring);
            auto sgftree = std::make_unique<SGFTree>();
            auto games = SGFParser::chop_stream(strm);
            if (!games.empty()) {
                sgftree->load_from_string(games[0]);
                m_State = sgftree->follow_mainline_state();
                m_State.m_policy.clear();
                m_State.m_owner.clear();
                m_State.m_black_score = 0.0;
                m_japanese_rule = sgftree->get_rule();
                for (size_t i = 0; i < m_State.m_win_rate.size(); i++) {
                    m_State.m_win_rate[i] = 100.0;
                }
                m_move_handi.clear();
                std::unique_ptr<GameState> tmp_state = std::make_unique<GameState>(m_State);
                tmp_state->rewind();
                for (int i = 0; i < m_State.board.get_boardsize(); i++) {
                    for (int j = 0; j < m_State.board.get_boardsize(); j++) {
                        if (tmp_state->board.get_square(i, j) == FastBoard::BLACK) {
                            m_move_handi.emplace_back(m_State.board.get_vertex(i, j));
                        }
                    }
                }

                m_StateStack.clear();
                m_StateStack.push_back(m_State);
                m_playerColor = m_State.get_to_move();
                MCOwnerTable::get_MCO()->clear();
                m_panelBoard->setPlayerColor(m_playerColor);
                m_panelBoard->setShowTerritory(false);
                m_panelBoard->clearViz();
                if (m_scoreHistogramWindow) {
                    m_scoreHistogramWindow->ClearHistogram();
                }
                gameNoLongerCounts();
                setActiveMenus();

                //signal board change
                wxCommandEvent myevent(wxEVT_BOARD_UPDATE, GetId());
                myevent.SetEventObject(this);
                ::wxPostEvent(m_panelBoard->GetEventHandler(), myevent);
                broadcastCurrentMove();
            }
        }
        wxTheClipboard->Close();
    }
}

void MainFrame::loadSGF(const wxString & filename, int movenum) {
    // open the file
    wxFile sgfFile;
    if (sgfFile.Open(filename)) {
        wxString SGF;
        if (sgfFile.ReadAll(&SGF)) {
            loadSGFString(SGF, movenum);
        }
    }
}

void MainFrame::OnAsyncTermination(SubProcess *process) {
    delete process;
}

void MainFrame::OnProcessTerminated(SubProcess *process) {
    if ( m_process ) {
        m_timerIdleWakeUp.Stop();
    }
}

void MainFrame::OnIdleTimer(wxTimerEvent& WXUNUSED(event)) {
    if ( !m_runflag ) {
        try {
            if ( m_katagoStatus == GAME_QUERY_WAIT ) {
                nlohmann::json terminate_json = nlohmann::json::parse(R"({"action":"terminate"})");
                terminate_json["id"] = "terminate_" + m_query_id;
                terminate_json["terminateId"] = "play1_" + m_query_id;
                std::string req_query_terminate = terminate_json.dump();
                m_terminate_res = false;
                wxString send_msg = req_query_terminate + "\n";
                m_out->Write(send_msg, send_msg.length());
            } else if ( m_katagoStatus == ANALYSIS_QUERY_WAIT ) {
                nlohmann::json terminate_json = nlohmann::json::parse(R"({"action":"terminate"})");
                terminate_json["id"] = "terminate_" + m_query_id;
                terminate_json["terminateId"] = "analysis_" + m_query_id;
                std::string req_query_terminate = terminate_json.dump();
                m_terminate_res = false;
                wxString send_msg = req_query_terminate + "\n";
                m_out->Write(send_msg, send_msg.length());
                m_katagoStatus = ANALYSIS_TERMINATE_WAIT;
            }
        } catch(const std::exception& e) {
            wxLogError(_("Exception at startKataGo: %s %s\n"), typeid(e).name(), e.what());
        }
    }
    if ( m_katagoStatus != INIT &&
         m_katagoStatus != KATAGO_IDLE &&
         m_katagoStatus != KATAGO_STOPED) {
        Bind(wxEVT_IDLE, &MainFrame::OnIdle, this);
        wxWakeUpIdle();
    }
}

void MainFrame::OnIdle(wxIdleEvent& event) {
    if ( m_process && m_process->HasInput() ) {
        event.RequestMore();
    } else {
        Unbind(wxEVT_IDLE, &MainFrame::OnIdle, this);
    }
}

void MainFrame::doRecieveKataGo(wxCommandEvent & event) {
    wxLogDebug(_("KataGo response arrived"));

    void* rawdataptr = event.GetClientData();
    if (!rawdataptr) {
        return;
    }
    std::unique_ptr<wxString> bundle(reinterpret_cast<wxString*>(rawdataptr));
    auto kataRes = *bundle;

    if (m_katagoStatus == KATAGO_STRATING) {
        if (kataRes.find("Uncaught exception:") != std::string::npos ||
            kataRes.find("PARSE ERROR:") != std::string::npos ||
            kataRes.find("failed with error") != std::string::npos ||
            kataRes.find("what():") != std::string::npos) {
            wxString errStr;
            errStr.Printf(_("The first line of the ini file is incorrect: %s\n"
                            "Start with the Leela engine?"), kataRes.mb_str());
            int answer = ::wxMessageBox(errStr, _("Leela"), wxYES_NO | wxICON_EXCLAMATION, this);
            if (answer != wxYES) {
                m_close_window = true;
            }
            cfg_use_engine = GTP::ORIGINE_ENGINE;
            m_in = nullptr;
            m_err = nullptr;
            m_out = nullptr;
            wxProcess::Kill(m_process->GetPid());
        } else if (kataRes.length() > 35 && kataRes.substr(9, 1) == "2" && kataRes.substr(13, 1) == "-" && kataRes.substr(16, 1) == "-") {
            SetStatusBarText(_("KataGo starting... ") + kataRes.substr(35), 1); // "(stderr):YYYY-MM-DD HH:MM:SS+0900: "
        } else if (kataRes.length() > 9) {
            SetStatusBarText(_("KataGo starting... ") + kataRes.substr(9), 1);  // "(stderr):"
        }
        if (cfg_use_engine == GTP::KATAGO_ENGINE && kataRes.rfind("Started, ready to begin handling requests") != std::string::npos) {
            std::string tmp_query = "";
            if (m_ini_line.size() > 1) {
                for (auto it = m_ini_line.begin() + 1; it != m_ini_line.end(); ++it) {
                    m_overrideSettings.emplace_back(*it);
                    tmp_query += *it;
                }
            }
            try {
                if (tmp_query.length() <= 0) {
                    tmp_query = R"({"rules":"japanese","analysisPVLen":15,"reportDuringSearchEvery":2.0,"maxVisitsAnalysis":1000000,"maxTimeAnalysis":3600})";
                    m_overrideSettings.emplace_back(tmp_query);
                    m_japanese_rule = true;
                    m_State.init_game(m_ratedSize, 6.5f);
                } else {
                    nlohmann::json dummy = nlohmann::json::parse(tmp_query);
                    if (!dummy.contains("rules")) {
                        dummy["rules"] = "japanese";
                    }
                    if (dummy["rules"].get<std::string>() == "japanese") {
                        m_japanese_rule = true;
                        m_State.init_game(m_ratedSize, 6.5f);
                    }
                    if (!dummy.contains("analysisPVLen")) {
                        dummy["analysisPVLen"] = 15;
                    }
                    if (!dummy.contains("reportDuringSearchEvery")) {
                        dummy["reportDuringSearchEvery"] = 2.0;
                    }
                    if (!dummy.contains("maxVisitsAnalysis")) {
                        dummy["maxVisitsAnalysis"] = 1000000;
                    }
                    if (!dummy.contains("maxTimeAnalysis")) {
                        dummy["maxTimeAnalysis"] = 3600;
                    }
                    tmp_query = dummy.dump();
                    m_overrideSettings.clear();
                    m_overrideSettings.emplace_back(tmp_query);
                }
                wxString send_msg = R"({"boardXSize":25,"boardYSize":25,"id":"dummy","maxVisits":1,"moves":[],"rules":"chinese"})" + wxString("\n");
                m_out->Write(send_msg, strlen(send_msg));
                m_katagoStatus = ANALYSIS_RESPONSE_WAIT;
                return;
            } catch(const std::exception& e) {
                wxString errStr;
                errStr.Printf(_("The query definition is incorrect: %s\nStart with the Leela engine?"), wxString(e.what()).mb_str());
                int answer = ::wxMessageBox(errStr, _("Leela"), wxYES_NO | wxICON_EXCLAMATION, this);
                if (answer != wxYES) {
                    m_close_window = true;
                }
                cfg_use_engine = GTP::ORIGINE_ENGINE;
                m_in = nullptr;
                m_err = nullptr;
                m_out = nullptr;
                wxProcess::Kill(m_process->GetPid());
            }
        }
        if (m_close_window) {
            Close();
        } else if (cfg_use_engine == GTP::ORIGINE_ENGINE) {
            wxConfig::Get()->Write(wxT("katagoEnabled"), false);
            wxPersistentRegisterAndRestore(this, "MainFrame");
            m_timerIdleWakeUp.Stop();
            m_katagoStatus = KATAGO_STOPED;
            setStartMenus();
            setActiveMenus();
            wxCommandEvent evt;
            doNewRatedGame(evt);
        }
    } else if (m_katagoStatus == ANALYSIS_RESPONSE_WAIT) {
        if (kataRes.length() == 0 || (kataRes.length() >= 9 && kataRes.substr(0, 8) == "(stderr):")) {
           return;
        } else if (kataRes.find(R"("error":)") != std::string::npos) {
            cfg_board25 = false;
        }
        cfg_use_engine = GTP::KATAGO_ENGINE;
        m_katagoStatus = KATAGO_IDLE;
        wxPersistentRegisterAndRestore(this, "MainFrame");
        setStartMenus();
        setActiveMenus();
        wxCommandEvent evt;
        doNewRatedGame(evt);
    } else if (m_katagoStatus == ANALYSIS_QUERY_WAIT) {
        if (kataRes.find(R"("id":"dummy")") != std::string::npos) {
            return;
        } else if (kataRes.find(R"("error":)") != std::string::npos) {
            SetStatusBarText(kataRes, 1);
            postIdle();
            return;
        } else if (kataRes.find(R"("isDuringSearch":)") == std::string::npos) {
            return;
        } else if (m_isDuringSearch && kataRes.find(R"("isDuringSearch":false)") != std::string::npos) {
            m_isDuringSearch = false;
        }
        std::string req_query = "";
        try {
            nlohmann::json res_1_json = nlohmann::json::parse(kataRes);
            m_winrate = 1.0f - res_1_json["rootInfo"]["winrate"].get<float>();
            m_scoreMean = -1.0f * res_1_json["rootInfo"]["scoreLead"].get<float>();
            nlohmann::json j1 = res_1_json["moveInfos"];
            using TRowVector = std::vector<std::pair<std::string, std::string>>;
            using TDataVector = std::tuple<int, float, std::vector<TRowVector>>;
            using TMoveData = std::vector<std::pair<std::string, float>>;
            std::unique_ptr<TDataVector> analysis_packet(new TDataVector);
            std::unique_ptr<TMoveData> move_data(new TMoveData);
            int who = m_StateEngine->get_to_move();
            std::get<0>(*analysis_packet) = who;
            std::get<1>(*analysis_packet) = m_scoreMean;
            auto& analysis_data = std::get<2>(*analysis_packet);
            for (nlohmann::json::iterator it1 = j1.begin(); it1 != j1.end(); ++it1) {
                nlohmann::json j2 = it1.value();
                if (j2.contains("isSymmetryOf") || j2["move"].is_null()) {
                    continue;
                }
                TRowVector row;
                row.emplace_back(_("Move").utf8_str(), j2["move"].get<std::string>());
                row.emplace_back(_("Effort%").utf8_str(),
                    std::to_string(100.0f * j2["visits"].get<int>() / (float)res_1_json["rootInfo"]["visits"].get<int>()));
                row.emplace_back(_("Simulations").utf8_str(), std::to_string(j2["visits"].get<int>()));
                if (who == FastBoard::BLACK) {
                    row.emplace_back(_("Win%").utf8_str(), std::to_string(100.0f * j2["winrate"].get<float>()));
                    row.emplace_back(_("Score").utf8_str(), std::to_string(j2["scoreLead"].get<float>()));
                } else {
                    row.emplace_back(_("Win%").utf8_str(), std::to_string(100.0f - 100.0f * j2["winrate"].get<float>()));
                    row.emplace_back(_("Score").utf8_str(), std::to_string(-1.0f * j2["scoreLead"].get<float>()));
                }
                std::string pvstring;
                nlohmann::json j3 = j2["pv"];
                for (nlohmann::json::iterator it2 = j3.begin(); it2 != j3.end(); ++it2) {
                    if (it2 > j3.begin()) {
                        pvstring += " ";
                    }
                    pvstring += (*it2).get<std::string>();
                }
                row.emplace_back(_("PV").utf8_str(), pvstring);
                analysis_data.emplace_back(row);
                move_data->emplace_back(j2["move"].get<std::string>(), (float)(j2["visits"].get<int>() / (double)res_1_json["rootInfo"]["visits"].get<int>()));
            }
            wxString mess;
            if (who == FastBoard::BLACK) {
                mess.Printf((_("Under analysis... ") + _("Win rate:%3.1f%% Score:%.1f")), 100.0f - m_winrate * 100.0f, -1.0f * m_scoreMean);
            } else {
                mess.Printf((_("Under analysis... ") + _("Win rate:%3.1f%% Score:%.1f")), m_winrate * 100.0f, m_scoreMean);
            }
            SetStatusBarText(mess, 1);
            Utils::GUIAnalysis((void*)analysis_packet.release());
            Utils::GUIBestMoves((void*)move_data.release());
            m_StateEngine->m_black_score = -1.0f * m_scoreMean;
            if (m_update_score && !std::isnan(m_winrate)) {
                auto event = new wxCommandEvent(wxEVT_EVALUATION_UPDATE);
                auto movenum = m_StateEngine->get_movenum();
                float lead = 0.5f;
                if (m_scoreMean > 0.0f) {
                    lead = 0.5f - (std::min)(0.5f, std::sqrt(m_scoreMean) / 40.0f);
                } else if (m_scoreMean < 0.0) {
                    lead = 0.5f + (std::min)(0.5f, std::sqrt(-1.0f * m_scoreMean) / 40.0f);
                }
                std::tuple<int, float, float, float> scoretuple = std::make_tuple(movenum, 1.0f - m_winrate, lead, 1.0f - m_winrate);
                event->SetClientData((void*)new auto(scoretuple));
                wxQueueEvent(GetEventHandler(), event);
            }
            if (!m_isDuringSearch) {
                mess.Printf(_("Analysis stopped"));
                SetStatusBarText(mess, 1);
                postIdle();
            }
        } catch (const std::exception& e) {
            wxLogError(_("Exception at ANALYSIS_QUERY_WAIT: %s %s\n"), typeid(e).name(), e.what());
            postIdle();
        }
    } else if (m_katagoStatus == ANALYSIS_TERMINATE_WAIT) {
        if (kataRes.find(R"("id":"dummy")") != std::string::npos) {
            return;
        } else if (kataRes.find(R"("error":)") != std::string::npos) {
            SetStatusBarText(kataRes, 1);
            postIdle();
            return;
        } else if (!m_terminate_res && kataRes.find(R"("action":"terminate")") != std::string::npos) {
            m_terminate_res = true;
        } else if (m_isDuringSearch && kataRes.find(R"("isDuringSearch":false)") != std::string::npos) {
            m_isDuringSearch = false;
            std::string winrate_str = "";
            std::string scoreMean_str = "";
            std::string::size_type pos = kataRes.rfind("winrate ");
            if (pos == std::string::npos) {
                pos = 0;
            } else {
                for (auto i = pos + sizeof("winrate ") - 1; i < kataRes.length(); i++) {
                    if (kataRes[i] != ' ') {
                        winrate_str += kataRes[i];
                    } else {
                        break;
                    }
                }
            }
            pos = kataRes.rfind("scoreMean ", pos);
            if (pos == std::string::npos) {
                scoreMean_str = "";
            } else {
                for (auto i = pos + sizeof("scoreMean ") - 1; i < kataRes.length(); i++) {
                    if (kataRes[i] != ' ') {
                        scoreMean_str += kataRes[i];
                    } else {
                        break;
                    }
                }
            }
            if (winrate_str.length() > 0) {
                m_winrate = stof(winrate_str);
            }
            if (scoreMean_str.length() > 0) {
                m_scoreMean = stof(scoreMean_str);
            }
        }
        if (!m_terminate_res || m_isDuringSearch) {
            return;
        }
        m_StateEngine->m_black_score = -1.0f * m_scoreMean;
        if (m_update_score && !std::isnan(m_winrate)) {
            auto event = new wxCommandEvent(wxEVT_EVALUATION_UPDATE);
            auto movenum = m_StateEngine->get_movenum();
            float lead = 0.5f;
            if (m_scoreMean > 0.0f) {
                lead = 0.5f - (std::min)(0.5f, std::sqrt(m_scoreMean) / 40.0f);
            } else if (m_scoreMean < 0.0) {
                lead = 0.5f + (std::min)(0.5f, std::sqrt(-1.0f * m_scoreMean) / 40.0f);
            }
            std::tuple<int, float, float, float> scoretuple = std::make_tuple(movenum, 1.0f - m_winrate, lead, 1.0f - m_winrate);
            event->SetClientData((void*)new auto(scoretuple));
            wxQueueEvent(GetEventHandler(), event);
        }
        wxString mess;
        mess.Printf(_("Analysis stopped"));
        SetStatusBarText(mess, 1);
        postIdle();
    } else if (m_katagoStatus == GAME_QUERY_WAIT) {
        if (kataRes.find(R"("id":"dummy")") != std::string::npos) {
            return;
        } else if (kataRes.find(R"("error":)") != std::string::npos) {
            m_StateEngine->stop_clock(m_StateEngine->get_to_move());
            SetStatusBarText(kataRes, 1);
            postIdle();
            return;
        } else if (kataRes.find(R"("isDuringSearch":false)") == std::string::npos) {
            return;
        }
        int who = m_StateEngine->get_to_move();
        try {
            nlohmann::json res_1_json;
            res_1_json = nlohmann::json::parse(kataRes);
            m_move_str = "";
            if(res_1_json.contains("moveInfos") && res_1_json["moveInfos"].size() > 0 &&
                !res_1_json["moveInfos"][0]["move"].is_null()) {
                m_move_str = res_1_json["moveInfos"][0]["move"].get<std::string>();
            }
            m_winrate = res_1_json["rootInfo"]["winrate"].get<float>();
            m_scoreMean = res_1_json["rootInfo"]["scoreLead"].get<float>();
            int board_size = m_StateEngine->board.get_boardsize();
            if (m_move_str.length() > 0) {
                // KataGo's Resignation Decision
                float initialBlackAdvantageInPoints;
                if (m_move_handi.size() <= 1) {
                    initialBlackAdvantageInPoints = 7.0f - m_StateEngine->get_komi();
                } else {
                    initialBlackAdvantageInPoints = 14.0f * (m_move_handi.size() - 1) + (7.0f - m_StateEngine->get_komi() - m_move_handi.size());
                }
                int minTurnForResignation = 0;
                int noResignationWhenWhiteScoreAbove = board_size * board_size;
                if (initialBlackAdvantageInPoints > 0.9f && who == FastBoard::WHITE) {
                    minTurnForResignation = 1 + noResignationWhenWhiteScoreAbove / 5;
                    float numTurnsToCatchUp = 0.60f * noResignationWhenWhiteScoreAbove - (float)minTurnForResignation;
                    float numTurnsSpent = (float)(m_StateEngine->get_movenum() + 1) - (float)minTurnForResignation;
                    if (numTurnsToCatchUp <= 1.0f) {
                        numTurnsToCatchUp = 1.0f;
                    }
                    if (numTurnsSpent <= 0.0f) {
                        numTurnsSpent = 0.0f;
                    }
                    if (numTurnsSpent > numTurnsToCatchUp) {
                        numTurnsSpent = numTurnsToCatchUp;
                    }
                    float resignScore = -1.0f * initialBlackAdvantageInPoints * ((numTurnsToCatchUp - numTurnsSpent) / numTurnsToCatchUp);
                    resignScore -= 5.0f;
                    resignScore -= initialBlackAdvantageInPoints * 0.15f;
                    noResignationWhenWhiteScoreAbove = resignScore;
                }
                bool resign = true;
                if (m_StateEngine->get_movenum() + 1 < minTurnForResignation) {
                    resign = false;
                } else if (who == FastBoard::WHITE && (-1.0f * m_scoreMean) > noResignationWhenWhiteScoreAbove) {
                    resign = false;
                } else if ((who == FastBoard::WHITE && (-1.0f * m_scoreMean) > -1.0f * RESIGN_MINSCORE_DIFFERENCE) ||
                    (who == FastBoard::BLACK && m_scoreMean > (-1.0f * RESIGN_MINSCORE_DIFFERENCE))) {
                    resign = false;
                }
                for (size_t i = 0; i < m_StateEngine->m_win_rate.size() - 1; i++) {
                    m_StateEngine->m_win_rate[m_StateEngine->m_win_rate.size() - 1 - i]
                        = m_StateEngine->m_win_rate[m_StateEngine->m_win_rate.size() - 2 - i];
                    if (m_StateEngine->m_win_rate[m_StateEngine->m_win_rate.size() - 1 - i] >= RESIGN_THRESHOLD) {
                        resign = false;
                     }
                }
                if (who == FastBoard::BLACK) {
                    m_StateEngine->m_win_rate[0] = m_winrate;
                } else {
                    m_StateEngine->m_win_rate[0] = 1.0f - m_winrate;
                }
                if (resign && m_StateEngine->m_win_rate[0] < RESIGN_THRESHOLD) {
                    m_move_str = "resign";
                }
            } else {
                m_move_str = "pass";
            }
            if (m_move_str == "pass") {
                m_StateEngine->play_move(who, FastBoard::PASS);
            } else if (m_move_str == "resign") {
                m_StateEngine->play_move(who, FastBoard::RESIGN);
            } else {
                m_StateEngine->play_move(who, m_StateEngine->board.text_to_move(m_move_str));
            }
            // Edit Ownership and Policy Information
            std::vector<float> conv_owner((board_size + 2) * (board_size + 2), 0.0f);
            std::vector<float> conv_policy((board_size + 2) * (board_size + 2), 0.0f);
            float maxProbability = 0.0f;
            for (int vertex = 0; vertex < board_size * board_size; vertex++) {
                int x = vertex % board_size;
                int y = vertex / board_size;
                y = -1 * (y - board_size) - 1;
                int pos = m_StateEngine->board.get_vertex(x, y);
                float owner = res_1_json["ownership"][vertex].get<float>();
                conv_owner[pos] = (owner / 2.0f) + 0.5f;
                float policy = res_1_json["policy"][vertex].get<float>();
                conv_policy[pos] = policy;
                if (policy > maxProbability) {
                    maxProbability = policy;
                }
            }
            m_StateEngine->m_owner.clear();
            for (auto itr = conv_owner.begin(); itr != conv_owner.end(); ++itr) {
                m_StateEngine->m_owner.emplace_back(*itr);
            }
            float policy = res_1_json["policy"][board_size * board_size].get<float>();
            conv_policy[0] = maxProbability;
            conv_policy[1] = policy;
            m_StateEngine->m_policy.clear();
            for (auto itr = conv_policy.begin(); itr != conv_policy.end(); ++itr) {
                m_StateEngine->m_policy.emplace_back(*itr);
            }
            m_StateEngine->m_black_score = m_scoreMean;
            std::bitset<FastBoard::MAXSQ> blackowns;
            for (int i = 0; i < board_size; i++) {
                for (int j = 0; j < board_size; j++) {
                    int vtx = m_StateEngine->board.get_vertex(i, j);
                    if (m_StateEngine->m_owner[vtx] >= 0.5f) {
                        blackowns[vtx] = true;
                    }
                }
            }
            MCOwnerTable::get_MCO()->update_owns(blackowns, 1.0f - m_winrate, -1.0f * m_scoreMean);
            if (m_update_score) {
                // Broadcast result from search
                auto event = new wxCommandEvent(wxEVT_EVALUATION_UPDATE);
                auto movenum = m_StateEngine->get_movenum();
                float lead = 0.5f;
                if (m_scoreMean > 0.0f) {
                    lead = 0.5f + (std::min)(0.5f, std::sqrt(m_scoreMean) / 40.0f);
                } else if (m_scoreMean < 0.0f) {
                    lead = 0.5f - (std::min)(0.5f, std::sqrt(-1.0f * m_scoreMean) / 40.0f);
                }
                std::tuple<int, float, float, float> scoretuple = std::make_tuple(movenum, m_winrate, lead, m_winrate);
                event->SetClientData((void*)new auto(scoretuple));
                wxQueueEvent(GetEventHandler(), event);
            }
            wxString mess;
            if (who == FastBoard::BLACK) {
                mess.Printf(_("Win rate:%3.1f%% Score:%.1f"), 100.0f - m_winrate * 100.0f, -1.0f * m_scoreMean);
            } else {
                mess.Printf(_("Win rate:%3.1f%% Score:%.1f"), m_winrate * 100.0f, m_scoreMean);
            }
        } catch (const std::exception& e) {
            m_StateEngine->stop_clock(who);
            wxLogError(_("Exception at GAME_QUERY_WAIT: %s %s\n"), typeid(e).name(), e.what());
            postIdle();
            return;
        }
        wxString mess;
        if (who == FastBoard::BLACK) {
            mess.Printf(_("Win rate:%3.1f%% Score:%.1f"), 100.0f - m_winrate * 100.0f, -1.0f * m_scoreMean);
        } else {
            mess.Printf(_("Win rate:%3.1f%% Score:%.1f"), m_winrate * 100.0f, m_scoreMean);
        }
        SetStatusBarText(mess, 1);
        wxQueueEvent(GetEventHandler(), new wxCommandEvent(wxEVT_NEW_MOVE));
        m_StateEngine->stop_clock(who);
        postIdle();
    }
}

void MainFrame::postIdle() {
    wxCommandEvent evt;
    m_katagoStatus = KATAGO_IDLE;
    if (!m_analyzing && !m_pondering) {
        m_State = *m_StateEngine;
    }
    m_timerIdleWakeUp.Stop();
    if (m_post_destructor) {
        wxPersistentRegisterAndRestore(this, "MainFrame");
        wxConfig::Get()->Write(wxT("analysisWindowOpen"),
            m_analysisWindow != nullptr && m_analysisWindow->IsShown());
        wxConfig::Get()->Write(wxT("scoreHistogramWindowOpen"),
            m_scoreHistogramWindow != nullptr && m_scoreHistogramWindow->IsShown());
#ifdef NDEBUG
        delete wxLog::SetActiveTarget(new wxLogStderr(NULL));
#endif
        //m_panelBoard->setState(NULL);
        Unbind(wxEVT_EVALUATION_UPDATE, &MainFrame::doEvalUpdate, this);
        Unbind(wxEVT_NEW_MOVE, &MainFrame::doNewMove, this);
        Unbind(wxEVT_BOARD_UPDATE, &MainFrame::doBoardUpdate, this);
        Unbind(wxEVT_STATUS_UPDATE, &MainFrame::doStatusUpdate, this);
        Unbind(wxEVT_DESTROY, &MainFrame::doCloseChild, this);
        Unbind(wxEVT_SET_MOVENUM, &MainFrame::gotoMoveNum, this);
        Unbind(wxEVT_RECIEVE_KATAGO, &MainFrame::doRecieveKataGo, this);
        Unbind(wxEVT_TIMER, &MainFrame::OnIdleTimer, this);
        Hide();
        m_post_destructor = false;
    }
    if (m_post_doExit) {
        doExit(evt);
        m_post_doExit = false;
    }
    if (m_post_doNewMove) {
        doNewMove(evt);
        m_post_doNewMove = false;
    }
    if (m_post_doSettingsDialog) {
        doSettingsDialog(evt);
        m_post_doSettingsDialog = false;
    }
    if (m_post_doNewGame) {
        doNewGame(evt);
        m_post_doNewGame = false;
    }
    if (m_post_doNewRatedGame) {
        doNewRatedGame(evt);
        m_post_doNewRatedGame = false;
    }
    if (m_post_doScore) {
        doScore(evt);
        m_post_doScore = false;
    }
    if (m_post_doPass) {
        doPass(evt);
        m_post_doPass = false;
    }
    if (m_post_doRealUndo) {
        doRealUndo(m_post_doRealUndo);
        m_post_doRealUndo = 0;
    }
    if (m_post_doRealForward) {
        doRealForward(m_post_doRealForward);
        m_post_doRealForward = 0;
    }
    if (m_post_doOpenSGF) {
        doOpenSGF(evt);
        m_post_doOpenSGF = false;
    }
    if (m_post_doSaveSGF) {
        doSaveSGF(evt);
        m_post_doSaveSGF = false;
    }
    if (m_post_doForceMove) {
        doForceMove(evt);
        m_post_doForceMove = false;
    }
    if (m_post_doResign) {
        doResign(evt);
        m_post_doResign = false;
    }
    if (m_post_doAnalyze) {
        doAnalyze(evt);
        m_post_doAnalyze = false;
    }
    if (m_post_doAnalyze) {
        doAnalyze(evt);
        m_post_doAnalyze = false;
    }
    if (m_post_doAnalyze) {
        doAnalyze(evt);
        m_post_doAnalyze = false;
    }
}

void MainFrame::setStartMenus(bool enable) {
   m_menu1->FindItem(ID_NEWGAME)->Enable(enable);
   m_menu1->FindItem(ID_NEWRATED)->Enable(enable);
   m_menu1->FindItem(ID_RATEDSIZE)->Enable(enable);
   m_menu1->FindItem(ID_OPEN)->Enable(enable);
   m_menu1->FindItem(ID_SAVE)->Enable(enable);
   m_menu1->FindItem(ID_COPYCLIPBOARD)->Enable(enable);
   m_menu1->FindItem(ID_PASTECLIPBOARD)->Enable(enable);
   m_menu2->FindItem(ID_UNDO)->Enable(enable);
   m_menu2->FindItem(ID_REDO)->Enable(enable);
   m_menu2->FindItem(ID_BACK10)->Enable(enable);
   m_menu2->FindItem(ID_FWD10)->Enable(enable);
   m_menu2->FindItem(ID_FORCE)->Enable(enable);
   m_menu2->FindItem(ID_PASS)->Enable(enable);
   m_menu2->FindItem(ID_RESIGN)->Enable(enable);
   m_menu2->FindItem(ID_SCORE)->Enable(enable);
   m_menuAnalyze->FindItem(ID_ANALYZE)->Enable(enable);
   m_menuAnalyze->FindItem(ID_PUSHPOS)->Enable(enable);
   m_menuAnalyze->FindItem(ID_POPPOS)->Enable(enable);
   m_menuAnalyze->FindItem(ID_MAINLINE)->Enable(enable);
   m_menuAnalyze->FindItem(ID_ANALYSISWINDOWTOGGLE)->Enable(enable);
   m_menuAnalyze->FindItem(ID_SCOREHISTOGRAMTOGGLE)->Enable(enable);
   m_menuTools->FindItem(ID_MOVE_PROBABILITIES)->Enable(enable);
   m_menuTools->FindItem(ID_BEST_MOVES)->Enable(enable);
   m_menuTools->FindItem(ID_SHOWTERRITORY)->Enable(enable);
   m_menuTools->FindItem(ID_SHOWMOYO)->Enable(enable);
   m_menuTools->FindItem(wxID_PREFERENCES)->Enable(enable);
   m_menuTools->FindItem(ID_ADJUSTCLOCKS)->Enable(enable);
   GetToolBar()->EnableTool(ID_NEWGAME, enable);
   GetToolBar()->EnableTool(ID_NEWRATED, enable);
   GetToolBar()->EnableTool(ID_OPEN, enable);
   GetToolBar()->EnableTool(ID_SAVE, enable);
   GetToolBar()->EnableTool(ID_BACK10, enable);
   GetToolBar()->EnableTool(ID_UNDO, enable);
   GetToolBar()->EnableTool(ID_REDO, enable);
   GetToolBar()->EnableTool(ID_FWD10, enable);
   GetToolBar()->EnableTool(ID_FORCE, enable);
   GetToolBar()->EnableTool(ID_PUSHPOS, enable);
   GetToolBar()->EnableTool(ID_POPPOS, enable);
   GetToolBar()->EnableTool(ID_MAINLINE, enable);
   GetToolBar()->EnableTool(ID_ANALYZE, enable);
   GetToolBar()->EnableTool(ID_PASS, enable);
   GetToolBar()->EnableTool(ID_SCORE, enable);
   GetToolBar()->EnableTool(ID_RESIGN, enable);
}
