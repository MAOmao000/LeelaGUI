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

#include <iostream>
#include <filesystem>
#ifdef WIN32
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem::v1;
#else
namespace fs = std::filesystem;
#endif

using std::this_thread::sleep_for;
using namespace std::chrono;

wxDEFINE_EVENT(wxEVT_NEW_MOVE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_BOARD_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_STATUS_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_ANALYSIS_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_BESTMOVES_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_EVALUATION_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_SET_MOVENUM, wxCommandEvent);
#ifndef USE_THREAD
wxDEFINE_EVENT(wxEVT_RECIEVE_KATAGO, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_TERMINATED_KATAGO, wxCommandEvent);
#endif

// Case of non use thread engine, monitor query response by timer (WAKE_UP_TIMER_MS).
MainFrame::MainFrame(wxFrame *frame, const wxString& title)
    : TMainFrame(frame, wxID_ANY, title) {

    m_thinking_time = 0;
    m_thinking = false;
    m_think_num = 0;
    m_visits = 0;
    m_process = nullptr;
    m_katagoStatus = INIT;
    m_use_engine = -1;
    m_time_for_move = -1;

#ifdef NDEBUG
    delete wxLog::SetActiveTarget(NULL);
#endif

    wxLog::SetTimestamp("");

    int engine_type = wxConfig::Get()->ReadLong(wxT("EngineType"), (long)GTP::NONE);

#ifdef USE_THREAD
    if (engine_type == GTP::GTP_INTERFACE) {
        engine_type = GTP::ANALYSIS;
        wxConfig::Get()->Write(wxT("EngineType"), (long)GTP::ANALYSIS);
    }
#endif

    m_japanese_rule = false;
    if (wxConfig::Get()->ReadLong(wxT("KataGoRule"), (long)0) == 1) {
        m_japanese_rule_init = true;
    } else {
        m_japanese_rule_init = false;
    }
    bool board25 = true;

    GTP::setup_default_parameters(engine_type, board25);

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
            if (!rawdataptr) {
                return;
            }
            using TRowVector = std::vector<std::pair<std::string, std::string>>;
            using TDataVector = std::vector<TRowVector>;
            using TDataBundle = std::tuple<int, float, TDataVector>;

            // Take ownership of the data
            delete reinterpret_cast<TDataBundle*>(rawdataptr);
        }
    });
#ifndef USE_THREAD
    m_timerIdleWakeUp.SetOwner(this);
    Bind(wxEVT_IDLE, &MainFrame::OnIdle, this);
    Bind(wxEVT_RECIEVE_KATAGO, &MainFrame::doRecieveKataGo, this);
    Bind(wxEVT_TERMINATED_KATAGO, &MainFrame::doTerminatedKataGo, this);
    Bind(wxEVT_TIMER, &MainFrame::OnIdleTimer, this, m_timerIdleWakeUp.GetId());
#endif

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
    m_pass_send = true;
    m_resign_send = true;

    m_netsEnabled = wxConfig::Get()->ReadBool(wxT("netsEnabled"), true);
    m_passEnabled = wxConfig::Get()->ReadBool(wxT("passEnabled"), true);
    m_soundEnabled = wxConfig::Get()->ReadBool(wxT("soundEnabled"), true);
    m_resignEnabled = wxConfig::Get()->ReadBool(wxT("resignEnabled"), true);
    m_resignKataGoEnabled = wxConfig::Get()->ReadBool(wxT("resignKataGoEnabled"), true);
    m_ponderEnabled = wxConfig::Get()->ReadBool(wxT("ponderEnabled"), true);
    m_ponderKataGoEnabled = wxConfig::Get()->ReadBool(wxT("ponderKataGoEnabled"), true);
    int rule = wxConfig::Get()->ReadLong(wxT("KataGoRule"), (long)0);
    if (rule == 1) {
        m_japanese_rule_init = true;
    } else {
        m_japanese_rule_init = false;
    }
    m_ratedSize     = wxConfig::Get()->ReadLong(wxT("ratedSize"), 9L);

#ifdef WIN32
    m_tock.Create("IDW_TOCK", true);
#else
    m_tock.Create(tock_data_length, tock_data);
#endif

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
    if (wxConfig::Get()->ReadBool(wxT("japaneseEnabled"), false)) {
        m_statusBar->SetMinHeight(30);
    }
#elif defined(__WXMAC__)
    SetSize(570, 640);
#else
    SetSize(530, 640); // 527,630
#endif

    Center();
    setStartMenus(false);
}

MainFrame::~MainFrame() {
    stopEngine();
    if (cfg_use_engine == GTP::KATAGO_ENGINE &&
        m_katagoStatus != INIT &&
        m_katagoStatus != KATAGO_STOPING &&
        m_katagoStatus != KATAGO_STOPED) {
        if (!m_process && m_process->Exists(m_process->GetPid())) {
            wxProcess::Kill(m_process->GetPid(), (wxSignal)SIGNAL);
#ifndef USE_THREAD
            m_process->Detach();
#endif
        }

#ifndef USE_THREAD
        delete m_process;
#endif

    }
    wxConfig::Get()->Write(wxT("analysisWindowOpen"),
        m_analysisWindow != nullptr && m_analysisWindow->IsShown());
    wxConfig::Get()->Write(wxT("scoreHistogramWindowOpen"),
        m_scoreHistogramWindow != nullptr && m_scoreHistogramWindow->IsShown());

#ifdef NDEBUG
    delete wxLog::SetActiveTarget(new wxLogStderr(NULL));
#endif

    Unbind(wxEVT_EVALUATION_UPDATE, &MainFrame::doEvalUpdate, this);
    Unbind(wxEVT_NEW_MOVE, &MainFrame::doNewMove, this);
    Unbind(wxEVT_BOARD_UPDATE, &MainFrame::doBoardUpdate, this);
    Unbind(wxEVT_STATUS_UPDATE, &MainFrame::doStatusUpdate, this);
    Unbind(wxEVT_DESTROY, &MainFrame::doCloseChild, this);
    Unbind(wxEVT_SET_MOVENUM, &MainFrame::gotoMoveNum, this);

#ifndef USE_THREAD
    Unbind(wxEVT_IDLE, &MainFrame::OnIdle, this);
    Unbind(wxEVT_TIMER, &MainFrame::OnIdleTimer, this);
    Unbind(wxEVT_RECIEVE_KATAGO, &MainFrame::doRecieveKataGo, this);
    Unbind(wxEVT_TERMINATED_KATAGO, &MainFrame::doTerminatedKataGo, this);
#endif

    Hide();
}

void MainFrame::doInit() {
    bool close_window = false;
    wxString errorString;
    wxString engine_path, config_path, model_path;
    bool defined_ini = false;
    std::string ini_file;
    if (cfg_engine_type == GTP::NONE) {
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
                // Remove whitespace at the beginning of a line
                std::string trim_line
                    = std::regex_replace(line, std::regex("^\\s+"), std::string(""));
                if (trim_line.empty() || trim_line[0] == '#') {
                    continue;
                }
                // Remove the first character after `#` starting at the beginning of the line
                auto pos = trim_line.find('#');
                std::string erase_line = trim_line;
                if (pos != std::string::npos) {
                    erase_line = trim_line.erase(pos);
                }
                // Remove trailing whitespace characters from lines
                std::string last_line
                    = std::regex_replace(erase_line, std::regex("\\s+$"), std::string(""));
                // Replace 2 or more spaces with 1 space
                std::regex reg(R"(\s+)");
                std::string s = std::regex_replace(last_line, reg, " ");
                if (m_ini_line.empty() && s.find(" gtp ") != std::string::npos) {
                    pos = s.find(" -override-config");
                    if (pos == std::string::npos) {
                        if (m_ponderKataGoEnabled) {
                            s += R"( -override-config "ponderingEnabled=true,)";
                        } else {
                            s += R"( -override-config "ponderingEnabled=false,)";
                        }
                        if (m_resignKataGoEnabled) {
                            s += R"(allowResignation=true,)";
                        } else {
                            s += R"(allowResignation=false,)";
                        }
                        s += R"(reportAnalysisWinratesAs=SIDETOMOVE")";
                    } else {
                        auto pos_ponder = s.find("ponderingEnabled");
                        if (pos_ponder != std::string::npos) {
                            for (size_t i = pos_ponder + sizeof("ponderingEnabled") - 1;
                                 i < s.size(); i++) {
                                if (s.substr(i, 1) == "t" || s.substr(i, 1) == "T") {
                                    if (!m_ponderKataGoEnabled) {
                                        s.replace(i, 4, "false");
                                    }
                                    break;
                                } else if (s.substr(i, 1) == "f" || s.substr(i, 1) == "F") {
                                    if (m_ponderKataGoEnabled) {
                                        s.replace(i, 5, "true");
                                    }
                                    break;
                                }
                            }
                        } else {
                            if (m_ponderKataGoEnabled) {
                                s += R"( -override-config "ponderingEnabled=true")";
                            } else {
                                s += R"( -override-config "ponderingEnabled=false")";
                            }
                        }
                        auto pos_resign = s.find("allowResignation");
                        if (pos_resign != std::string::npos) {
                            for (size_t i = pos_resign + sizeof("allowResignation") - 1;
                                 i < s.size(); i++) {
                                if (s.substr(i, 1) == "t" || s.substr(i, 1) == "T") {
                                    if (!m_resignKataGoEnabled) {
                                        s.replace(i, 4, "false");
                                    }
                                    break;
                                } else if (s.substr(i, 1) == "f" || s.substr(i, 1) == "F") {
                                    if (m_resignKataGoEnabled) {
                                        s.replace(i, 5, "true");
                                    }
                                    break;
                                }
                            }
                        } else {
                            if (m_resignKataGoEnabled) {
                                s += R"( -override-config "allowResignation=true")";
                            } else {
                                s += R"( -override-config "allowResignation=false")";
                            }
                        }
                        auto pos_report = s.find("reportAnalysisWinratesAs");
                        if (pos_report != std::string::npos) {
                            for (size_t i = pos_report + sizeof("reportAnalysisWinratesAs") - 1;
                                 i < s.size(); i++) {
                                if (s.substr(i, 1) == "b" || s.substr(i, 1) == "B" ||
                                    s.substr(i, 1) == "w" || s.substr(i, 1) == "W") {

                                    s.replace(i, 5, "SIDETOMOVE");
                                    break;
                                } else if (s.substr(i, 1) == "s" || s.substr(i, 1) == "S") {
                                    break;
                                }
                            }
                        } else {
                            s += R"( -override-config "reportAnalysisWinratesAs=SIDETOMOVE")";
                        }
                    }
                } else if (m_ini_line.empty() && s.find(" analysis ") != std::string::npos) {
                    pos = s.find(" -override-config");
                    if (pos == std::string::npos) {
                        s += R"( -override-config "reportAnalysisWinratesAs=BLACK")";
                    } else {
                        auto pos_report = s.find("reportAnalysisWinratesAs");
                        if (pos_report != std::string::npos) {
                            for (size_t i = pos_report + sizeof("reportAnalysisWinratesAs") - 1;
                                 i < s.size(); i++) {
                                if (s.substr(i, 1) == "w" || s.substr(i, 1) == "W") {
                                    s.replace(i, 5, "BLACK");
                                    break;
                                } else if (s.substr(i, 1) == "s" || s.substr(i, 1) == "S") {
                                    s.replace(i, 10, "BLACK");
                                    break;
                                }
                            }
                        } else {
                            s += R"( -override-config "reportAnalysisWinratesAs=BLACK")";
                        }
                    }
                }
                m_ini_line.emplace_back(s);
            }
        }
        if (!m_ini_line.empty()) {
            defined_ini = true;
        } else {
            cfg_use_engine = GTP::ORIGINE_ENGINE;
        }
    } else {
        if (cfg_engine_type == GTP::ANALYSIS) {
            wxConfig::Get()->Read(wxT("AnalysisEnginePath"), &engine_path);
            wxConfig::Get()->Read(wxT("AnalysisConfigPath"), &config_path);
            wxConfig::Get()->Read(wxT("AnalysisModelPath"), &model_path);
        } else if (cfg_engine_type == GTP::GTP_INTERFACE) {
            wxConfig::Get()->Read(wxT("GTPEnginePath"), &engine_path);
            wxConfig::Get()->Read(wxT("GTPConfigPath"), &config_path);
            wxConfig::Get()->Read(wxT("GTPModelPath"), &model_path);
        }

        if (engine_path.IsEmpty()) {
            engine_path = wxString::FromUTF8(ENGINE_PATH);
            if (cfg_engine_type == GTP::ANALYSIS) {
                wxConfig::Get()->Write(wxT("AnalysisEnginePath"), engine_path);
            } else if (cfg_engine_type == GTP::GTP_INTERFACE) {
                wxConfig::Get()->Write(wxT("GTPEnginePath"), engine_path);
            }
        }
        if (model_path.IsEmpty()) {
            model_path = wxString::FromUTF8(MODEL_PATH);
            if (cfg_engine_type == GTP::ANALYSIS) {
                wxConfig::Get()->Write(wxT("AnalysisModelPath"), model_path);
            } else if (cfg_engine_type == GTP::GTP_INTERFACE) {
                wxConfig::Get()->Write(wxT("GTPModelPath"), model_path);
            }
        }
        if (config_path.IsEmpty()) {
            config_path = wxString::FromUTF8(CONFIG_PATH);
            if (cfg_engine_type == GTP::ANALYSIS) {
                wxConfig::Get()->Write(wxT("AnalysisConfigPath"), config_path);
            } else if (cfg_engine_type == GTP::GTP_INTERFACE) {
                wxConfig::Get()->Write(wxT("GTPConfigPath"), config_path);
            }
        }
        wxString exe_cmd;
        if (cfg_engine_type == GTP::ANALYSIS) {
            exe_cmd.Printf("\"%s\" analysis -config \"%s\" -model \"%s\"",
                           engine_path, config_path, model_path);
            exe_cmd += R"( -override-config "reportAnalysisWinratesAs=BLACK")";
            m_ini_line.emplace_back(exe_cmd);
        } else if (cfg_engine_type == GTP::GTP_INTERFACE) {
            exe_cmd.Printf("\"%s\" gtp -config \"%s\" -model \"%s\"",
                           engine_path, config_path, model_path);
            if (m_ponderKataGoEnabled) {
                exe_cmd += R"( -override-config "ponderingEnabled=true,)";
            } else {
                exe_cmd += R"( -override-config "ponderingEnabled=false,)";
            }
            if (m_resignKataGoEnabled) {
                exe_cmd += R"(allowResignation=true,)";
            } else {
                exe_cmd += R"(allowResignation=false,)";
            }
            exe_cmd += R"(reportAnalysisWinratesAs=SIDETOMOVE")";
            m_ini_line.emplace_back(exe_cmd);
        }
    }
#ifdef USE_THREAD
    if (defined_ini || cfg_engine_type != GTP::NONE) {
        cfg_use_engine = GTP::ORIGINE_ENGINE;
        cfg_engine_type = GTP::NONE;
        auto pos = m_ini_line[0].find(" analysis ");
        if (pos != std::string::npos) {
            // Start KataGo's analysis engine.
            if ( !(m_process = wxProcess::Open(m_ini_line[0])) ) {
                wxLogDebug(_("Failed to launch the command."));
            } else if ( !(m_in = m_process->GetInputStream()) ) {
                wxLogDebug(_("Failed to connect to child stdout"));
                wxProcess::Kill(m_process->GetPid(), (wxSignal)SIGNAL);
            } else if ( !(m_err = m_process->GetErrorStream()) ) {
                wxLogDebug(_("Failed to connect to child stderr"));
                wxProcess::Kill(m_process->GetPid(), (wxSignal)SIGNAL);
            } else if ( !(m_out = m_process->GetOutputStream()) ) {
                wxLogDebug(_("Failed to connect to child stdin"));
                wxProcess::Kill(m_process->GetPid(), (wxSignal)SIGNAL);
            } else {
                cfg_use_engine = GTP::KATAGO_ENGINE;
                cfg_engine_type = GTP::ANALYSIS;
            }
        }
        if (cfg_use_engine == GTP::ORIGINE_ENGINE) {
            m_katagoStatus = KATAGO_STOPED;
            if (defined_ini) {
                errorString.Printf(_("The first line of the ini file is incorrect: %s\n"
                                     "Start with the Leela engine?"), m_ini_line[0].mb_str());
            } else {
                errorString.Printf(_("There is an error in the configuration of the KataGo engine: %s\n"
                                     "Start with the Leela engine?"), engine_path);
            }
            int answer = ::wxMessageBox(errorString, _("Leela"), wxYES_NO | wxICON_EXCLAMATION, this);
            if (answer != wxYES) {
                close_window = true;
            }
        }
    } else {
        m_katagoStatus = KATAGO_STOPED;
        cfg_use_engine = GTP::ORIGINE_ENGINE;
        cfg_engine_type = GTP::NONE;
    }
#else
    if (defined_ini || cfg_engine_type != GTP::NONE) {
        cfg_use_engine = GTP::ORIGINE_ENGINE;
        cfg_engine_type = GTP::NONE;
        auto pos = m_ini_line[0].find(" analysis ");
        if (pos != std::string::npos) {
            cfg_use_engine = GTP::KATAGO_ENGINE;
            cfg_engine_type = GTP::ANALYSIS;
        } else {
            pos = m_ini_line[0].find(" gtp ");
            if (pos != std::string::npos) {
            cfg_use_engine = GTP::KATAGO_ENGINE;
                cfg_engine_type = GTP::GTP_INTERFACE;
            }
        }
        if (cfg_use_engine != GTP::ORIGINE_ENGINE) {
            // Start KataGo's analysis engine.
            // Case of non use thread engine, receive query response in SubProcess class.
            m_process = new SubProcess(this, cfg_engine_type, m_ini_line);
            long pid = wxExecute(m_ini_line[0], wxEXEC_ASYNC, m_process);
            if ( pid < 0 ) {
                wxLogDebug(_("Failed to launch the command."));
                cfg_use_engine = GTP::ORIGINE_ENGINE;
                delete m_process;
            } else if ( !(m_out = m_process->GetOutputStream()) ) {
                wxLogDebug(_("Failed to connect to child stdin"));
                wxProcess::Kill(m_process->GetPid(), (wxSignal)SIGNAL);
                cfg_use_engine = GTP::ORIGINE_ENGINE;
                delete m_process;
            }
        }
        if (cfg_use_engine == GTP::ORIGINE_ENGINE) {
            m_katagoStatus = KATAGO_STOPED;
            cfg_engine_type = GTP::NONE;
            if (defined_ini) {
                errorString.Printf(_("The first line of the ini file is incorrect: %s\n"
                                     "Start with the Leela engine?"), m_ini_line[0].mb_str());
            } else {
                errorString.Printf(_("There is an error in the configuration of the KataGo engine: %s\n"
                                     "Start with the Leela engine?"), engine_path);
            }
            int answer = ::wxMessageBox(errorString, _("Leela"), wxYES_NO | wxICON_EXCLAMATION, this);
            if (answer != wxYES) {
                close_window = true;
            }
        }
    } else {
        m_katagoStatus = KATAGO_STOPED;
        cfg_use_engine = GTP::ORIGINE_ENGINE;
        cfg_engine_type = GTP::NONE;
    }
#endif

#ifdef USE_THREAD
    // Case of use thread engine, check KataGo startup here.
    if (cfg_use_engine == GTP::KATAGO_ENGINE) {
        std::string res_msg;
        char buffer[4096];
        SetStatusBarText(_("KataGo starting... "), 1);
        while ( true ) {
            buffer[0] = '\0';
            if (m_process->IsErrorAvailable()) {
                buffer[m_err->Read(buffer, WXSIZEOF(buffer) - 1).LastRead()] = '\0';
                res_msg += buffer;
            }
            if (buffer[0] != '\0') {
                std::string last_msg;
                std::string::size_type pos = std::string::npos;
                int offset = res_msg.length() - 2;
                if (offset > 0) {
                    pos = res_msg.rfind("\n", offset);
                    if (pos == std::string::npos) {
                        pos = -1;
                    }
                    last_msg = res_msg.substr(pos + 1);
                    std::regex regex(R"(\d{4}-\d{2}-\d{2})");
                    if (std::regex_search(last_msg, regex)) {
                        last_msg.erase(0, sizeof("YYYY-MM-DD HH:MM:SS+0900: ") - 1); 
                    }
                    SetStatusBarText(_("KataGo starting... ") + last_msg, 1);
                    if (last_msg.find("Uncaught exception:") != std::string::npos ||
                        last_msg.find("PARSE ERROR:") != std::string::npos ||
                        last_msg.find("failed with error") != std::string::npos ||
                        last_msg.find(": error while loading shared libraries:") != std::string::npos ||
                        last_msg.find("what():") != std::string::npos) {
                        wxProcess::Kill(m_process->GetPid(), (wxSignal)SIGNAL);
                        m_katagoStatus = KATAGO_STOPED;
                        cfg_use_engine = GTP::ORIGINE_ENGINE;
                        cfg_engine_type = GTP::NONE;
                        errorString.Printf(_("A startup error was returned from KataGo engine: %s\n"
                                             "Start with the Leela engine?"), last_msg);
                        int answer = ::wxMessageBox(errorString, _("Leela"),
                                                    wxYES_NO | wxICON_EXCLAMATION, this);
                        if (answer != wxYES) {
                            close_window = true;
                        }
                        break;
                    }
                    if (last_msg.rfind("Started, ready to begin handling requests")
                            != std::string::npos) {
                        break;
                    }
                } else {
                    sleep_for(std::chrono::milliseconds(100));
                }
            } else {
                sleep_for(std::chrono::milliseconds(100));
            }
            sleep_for(std::chrono::milliseconds(100));
        }
    }
#endif

    if (cfg_engine_type == GTP::ANALYSIS) {
        std::string tmp_query;
        if (m_ini_line.size() <= 1) {
            std::string default_analysis_pv_len = std::to_string(DEFAULT_ANALYSIS_PV_LEN);
            std::string default_report_during_search = std::to_string(DEFAULT_REPORT_DURING_SEARCH);
            std::string default_max_visits_analysis = std::to_string(DEFAULT_MAX_VISITS_ANALYSIS);
            std::string default_max_time_analysis = std::to_string(DEFAULT_MAX_TIME_ANALYSIS);
            if (m_japanese_rule_init) {
                m_japanese_rule = true;
                tmp_query = R"({"rules":"japanese",)";
            } else {
                m_japanese_rule = false;
                tmp_query = R"({"rules":"chinese","whiteHandicapBonus":"N",)";
            }
            tmp_query += R"("analysisPVLen":)" +
                         default_analysis_pv_len +
                         R"(,"reportDuringSearchEvery":)" +
                         default_report_during_search +
                         R"(,"maxVisitsAnalysis":)" +
                         default_max_visits_analysis +
                         R"(,"maxTimeAnalysis":)" +
                         default_max_time_analysis +
                         R"(})";
            m_overrideSettings.emplace_back(tmp_query);
        } else {
            for (auto it = m_ini_line.begin() + 1; it != m_ini_line.end(); ++it) {
                tmp_query += *it;
            }
            try {
                nlohmann::json dummy = nlohmann::json::parse(tmp_query);
                if (!dummy.contains("rules")) {
                    if (m_japanese_rule_init) {
                        dummy["rules"] = "japanese";
                        dummy["whiteHandicapBonus"] = "0";
                    } else {
                        dummy["rules"] = "chinese";
                        dummy["whiteHandicapBonus"] = "N";
                    }
                }
                if (dummy["rules"].get<std::string>() == "japanese") {
                    m_japanese_rule = true;
                } else {
                    m_japanese_rule = false;
                }
                if (!dummy.contains("analysisPVLen")) {
                    dummy["analysisPVLen"] = DEFAULT_ANALYSIS_PV_LEN;
                }
                if (!dummy.contains("reportDuringSearchEvery")) {
                    dummy["reportDuringSearchEvery"] = DEFAULT_REPORT_DURING_SEARCH;
                }
                if (!dummy.contains("maxVisitsAnalysis")) {
                    dummy["maxVisitsAnalysis"] = DEFAULT_MAX_VISITS_ANALYSIS;
                }
                if (!dummy.contains("maxTimeAnalysis")) {
                    dummy["maxTimeAnalysis"] = DEFAULT_MAX_TIME_ANALYSIS;
                }
                tmp_query = dummy.dump();
                m_overrideSettings.emplace_back(tmp_query);
            } catch(const std::exception& e) {
                wxProcess::Kill(m_process->GetPid(), (wxSignal)SIGNAL);
                cfg_use_engine = GTP::ORIGINE_ENGINE;
                cfg_engine_type = GTP::NONE;
                errorString.Printf(_("The query definition is incorrect: %s\n"
                                     "Start with the Leela engine?"),
                                   wxString(e.what()).mb_str());
                int answer = ::wxMessageBox(errorString, _("Leela"),
                                            wxYES_NO | wxICON_EXCLAMATION, this);
                if (answer != wxYES) {
                    close_window = true;
                }
            }
        }
    }

    m_use_engine = cfg_use_engine;
    if (cfg_use_engine == GTP::ORIGINE_ENGINE) {
        m_katagoStatus = KATAGO_STOPED;
        if (close_window) {
            Close();
        } else {
            m_japanese_rule = false;
            m_japanese_rule_init = false;
            setStartMenus();
            wxCommandEvent evt;
            doNewRatedGame(evt);
        }
        return;
    }

#ifdef USE_THREAD
    // Case of use thread engine, check KataGo startup here.
    wxString send_msg = R"({"boardXSize":25,"boardYSize":25,"id":"dummy",)";
    send_msg += R"("maxVisits":1,"moves":[],"rules":"chinese"})";
    send_msg += "\n";
    std::string res_msg = GTPSend(send_msg);
    if (res_msg.find(R"("error":)") != std::string::npos) {
        cfg_board25 = false;
    }
    setStartMenus();
    wxCommandEvent evt;
    doNewRatedGame(evt);
#else
    // Case of non use thread engine, check KataGo startup with MainFrame::doRecieveKataGo.
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
    m_post_show_prob = false;
    m_gtp_pending_cmd.clear();
    SetStatusBarText(_("KataGo starting... "), 0);
    m_katagoStatus = KATAGO_STARTING;
    m_timerIdleWakeUp.Start(WAKE_UP_TIMER_MS);
#endif

}

void MainFrame::doStatusUpdate(wxCommandEvent& event) {
    m_statusBar->SetStatusText(event.GetString(), 1);
}

void MainFrame::SetStatusBarText(wxString mess, int pos) {
    m_statusBar->SetStatusText(mess, pos);
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
    wxLogDebug(_("MainFrame exit."));

#ifndef USE_THREAD
    if (cfg_use_engine == GTP::KATAGO_ENGINE) {
        // If KataGo is currently requesting analysis,
        // MainFrame::doRecieveKataGo will process it instead.
        if (m_katagoStatus != KATAGO_IDLE) {
            if (m_runflag) {
                m_runflag = false;
                m_post_doExit = true;
            }
            return;
        }
        if (m_post_doExit) {
            m_post_doExit = false;
        }
        if (m_process && m_process->Exists(m_process->GetPid())) {
            wxProcess::Kill(m_process->GetPid(), (wxSignal)SIGNAL);
            m_katagoStatus = KATAGO_STOPING;
            return;
        }
    }
#endif

    stopEngine();

    Close();
}

void MainFrame::startEngine() {
#ifndef USE_THREAD
    if (m_use_engine == GTP::KATAGO_ENGINE) {
        startKataGo();
        return;
    }
#endif

    if (!m_engineThread) {
        m_query_start = std::chrono::system_clock::now();
        m_thinking = true;

        if (m_use_engine == GTP::KATAGO_ENGINE) {
            uint64_t query_ms
                = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
            m_query_id = std::to_string(query_ms);
            m_engineThread = std::make_unique<TEngineThread>(m_State,
                                                             this,
                                                             m_process,
                                                             m_in, m_err, m_out,
                                                             m_overrideSettings,
                                                             m_query_id,
                                                             m_query_start,
                                                             &m_GTPmutex);
            m_engineThread->set_handi(m_move_handi);
            m_engineThread->set_thinking(true);
            m_engineThread->set_resigning(m_resignKataGoEnabled);
        } else {
            m_engineThread = std::make_unique<TEngineThread>(m_State, this);
            m_engineThread->set_resigning(m_resignEnabled);
        }

        // lock the board
        if (!m_pondering && !m_analyzing) {
            m_panelBoard->lockState();
        }
        m_engineThread->limit_visits(m_visitLimit);
        m_engineThread->set_analyzing(m_analyzing | m_pondering);
        m_engineThread->set_pondering(m_pondering);
        m_engineThread->set_quiet(!m_analyzing);
        m_engineThread->set_nets(m_netsEnabled);


        if (m_passEnabled) {
            m_engineThread->set_nopass(m_disputing);
        } else {
            m_engineThread->set_nopass(true);
        }
        m_engineThread->Run();
        if (!m_analyzing && !m_pondering) {
            if (m_use_engine == GTP::KATAGO_ENGINE) { 
                SetStatusBarText(_("Engine thinking...") + _(" (KataGo)"), 1);
            } else {
                SetStatusBarText(_("Engine thinking...") + _(" (Leela)"), 1);
            }
        }
    } else {
        wxLogDebug(_("Engine already running"));
    }
}

#ifndef USE_THREAD
void MainFrame::startKataGo() {
    if (m_katagoStatus == KATAGO_IDLE) {
        m_query_start = std::chrono::system_clock::now();
        m_thinking = true;

        int time_for_move;
        m_update_score = true;
        m_StateEngine = std::make_unique<GameState>(m_State);
        int color = m_StateEngine->get_to_move();
        // lock the board
        if (!m_pondering && !m_analyzing) {
            m_panelBoard->lockState();
            TimeControl tm = m_State.get_timecontrol();
            m_StateEngine->set_timecontrol(tm);
            time_for_move
                = m_StateEngine->get_timecontrol().max_time_for_move(color,
                                                                     m_StateEngine->get_movenum());
            if ( (time_for_move + 50) < 100 ) {
                time_for_move = 1 + 1;
            } else {
                time_for_move = (time_for_move + 50) / 100 + 1;
            }
            if (!m_StateEngine->get_timecontrol().byo_yomi(color) &&
                time_for_move > 2 &&
                m_StateEngine->m_win_rate[0] > 0.6f &&
                m_StateEngine->m_win_rate[1] > 0.6f &&
                m_StateEngine->m_win_rate[2] > 0.6f) {

                time_for_move--;
            }
        }
        if (cfg_engine_type == GTP::ANALYSIS) {
            uint64_t query_ms
                = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
            m_query_id = std::to_string(query_ms);
            int board_size = m_StateEngine->board.get_boardsize();
            std::string tmp_query;
            for (auto it = m_overrideSettings.begin(); it != m_overrideSettings.end(); ++it) {
                tmp_query += *it;
            }
            try {
                m_send_json = nlohmann::json::parse(tmp_query);
                m_send_json["id"] = "analysis_" + m_query_id;
                if (m_japanese_rule) {
                    m_send_json["rules"] = "japanese";
                    m_send_json["whiteHandicapBonus"] = "0";
                } else {
                    m_send_json["rules"] = "japanese";
                    m_send_json["whiteHandicapBonus"] = "N";
                }
                m_send_json["komi"] = m_StateEngine->get_komi();
                m_send_json["boardXSize"] = board_size;
                m_send_json["boardYSize"] = board_size;
                if (!m_move_handi.empty()) {
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
                    if (!m_move_handi.empty()) {
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
                    m_send_json["maxVisits"] = DEFAULT_MAX_VISITS_ANALYSIS;
                }
                if (m_send_json.contains("maxTimeAnalysis")) {
                    if (m_analyzing | m_pondering) {
                        m_send_json["overrideSettings"]["maxTime"]
                            = m_send_json["maxTimeAnalysis"].get<int>();
                    }
                    m_send_json.erase("maxTimeAnalysis");
                } else if (m_analyzing | m_pondering) {
                    m_send_json["overrideSettings"]["maxTime"] = DEFAULT_MAX_TIME_ANALYSIS;
                }
                if (m_analyzing | m_pondering) {
                    m_send_json["includeOwnership"] = true;
                    m_send_json["includePolicy"] = true;
                    std::string req_query = m_send_json.dump();
                    wxString send_msg = req_query + "\n";
                    m_out->Write(send_msg, send_msg.length());
                    m_runflag = true;
                    m_isDuringSearch = true;
                    m_katagoStatus = ANALYSIS_QUERY_WAIT;
                } else {
                    m_StateEngine->start_clock(color);
                    m_send_json["id"] = "play1_" + m_query_id;
                    m_send_json["includeOwnership"] = true;
                    if (m_visitLimit <= 0) {
                        m_send_json["maxVisits"] = INT_MAX;
                    } else {
                        m_send_json["maxVisits"] = m_visitLimit;
                    }
                    if (time_for_move > 1) {
                        m_send_json["overrideSettings"]["maxTime"] = time_for_move - 1;
                    }
                    m_send_json.erase("reportDuringSearchEvery");

                    std::string req_query = m_send_json.dump();
                    wxString send_msg = req_query + "\n";
                    m_out->Write(send_msg, send_msg.length());
                    m_runflag = true;
                    m_isDuringSearch = true;
                    m_katagoStatus = GAME_FIRST_QUERY_WAIT;
                }
            } catch(const std::exception& e) {
                wxLogError(_("Exception at startKataGo: %s %s"), typeid(e).name(), e.what());
                return;
            }
            if (!m_analyzing && !m_pondering) {
                wxString mess;
                mess.Printf(_("Thinking at most %d seconds..."), time_for_move);
                SetStatusBarText(mess, 1);
            }
        } else {
            if (m_analyzing | m_pondering) {
                if (m_StateEngine->get_to_move() == FastBoard::BLACK) {
                    m_gtp_send_cmd = wxString::Format("kata-analyze b interval %i maxmoves %i\n",
                                                      GTP_ANALYZE_INTERVAL, GTP_ANALYZE_MAX_MOVES);
                } else {
                    m_gtp_send_cmd = wxString::Format("kata-analyze w interval %i maxmoves %i\n",
                                                      GTP_ANALYZE_INTERVAL, GTP_ANALYZE_MAX_MOVES);
                }
                m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
                m_runflag = true;
                m_katagoStatus = KATAGO_GTP_ANALYSIS;
            } else {
                m_StateEngine->start_clock(color);
                if (time_for_move == m_time_for_move) {
                    if (m_StateEngine->get_to_move() == FastBoard::BLACK) {
                        m_gtp_send_cmd = "kata-genmove_analyze b ownership true\n";
                    } else {
                        m_gtp_send_cmd = "kata-genmove_analyze w ownership true\n";
                    }
                } else {
                    m_time_for_move = time_for_move;
                    m_gtp_send_cmd = wxString::Format("time_settings 0 %i 1\n", time_for_move + 1);
                }
                m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
                m_runflag = true;
                m_katagoStatus = KATAGO_GTP_WAIT;
                wxString mess;
                mess.Printf(_("Thinking at most %d seconds..."), time_for_move);
                SetStatusBarText(mess, 1);
            }
        }
        m_timerIdleWakeUp.Start(WAKE_UP_TIMER_MS);
    } else {
        wxLogDebug(_("Engine already running"));
    }
}
#endif

bool MainFrame::stopEngine(bool update_score) {
    if (!m_engineThread) {
        return false;
    }
    if (!update_score) {
        m_engineThread->kill_score_update();
    }
#ifdef USE_THREAD
    if (m_use_engine == GTP::KATAGO_ENGINE && m_engineThread->get_thinking()) {
        nlohmann::json terminate_json = nlohmann::json::parse(R"({"action":"terminate"})");
        terminate_json["id"] = "terminate_" + m_query_id;
        if (m_analyzing | m_pondering) {
            terminate_json["terminateId"] = "analysis_" + m_query_id;
        } else {
            // Terminate request for play1 even during play2 processing
            terminate_json["terminateId"] = "play1_" + m_query_id;
        }
        wxString req_query_terminate = terminate_json.dump() + "\n";
        m_GTPmutex.lock();
        m_out->Write(req_query_terminate, req_query_terminate.length());
        m_GTPmutex.unlock();
    }
#endif

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
}

void MainFrame::doToggleProbabilities(wxCommandEvent& event) {
#ifndef USE_THREAD
    if (m_use_engine == GTP::KATAGO_ENGINE) {
        if (!m_panelBoard->getShowProbabilities()) {
            wxMenuItem * prob = m_menuTools->FindItem(ID_MOVE_PROBABILITIES);
            prob->Enable(false);
            m_menuTools->FindItem(ID_BEST_MOVES)->Enable(false);
            if (m_katagoStatus == KATAGO_IDLE) {
                if (cfg_engine_type == GTP::ANALYSIS) {
                    if (getAnalysisPolicyAndOwner(true, false)) {
                        m_katagoStatus = GET_POLICY_WAIT;
                    }
                } else {
                    m_StateEngine = std::make_unique<GameState>(m_State);
                    m_policy.clear();
                    m_gtp_send_cmd = "kata-raw-nn 0\n";
                    m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
                    m_katagoStatus = KATA_RAW_WAIT;
                }
            } else {
                m_runflag = true;
                m_post_show_prob = true;
            }
            return;
        }
    }
#endif

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

#ifdef USE_THREAD
    stopEngine();

    if (m_thinking) {
        auto query_end = std::chrono::system_clock::now();
        m_thinking_time += (int)std::chrono::duration_cast<std::chrono::milliseconds>
                                (query_end - m_query_start).count();
        m_thinking = false;
    }
    int *visits = (int *)event.GetClientData();
    if (visits) {
        m_think_num += 1;
        m_visits += *visits;
        delete visits;
    }
#else
    if (m_use_engine == GTP::KATAGO_ENGINE) {
        if (cfg_engine_type == GTP::GTP_INTERFACE) {
            wxString GTPCmd = event.GetString();
            if (!GTPCmd.IsEmpty()) {
                m_gtp_pending_cmd = GTPCmd;
            }
        }
        // If KataGo is currently requesting analysis,
        // MainFrame::doRecieveKataGo will process it instead.
        if (m_katagoStatus != KATAGO_IDLE) {
            if (m_runflag) {
                m_runflag = false;
                m_post_doNewMove = true;
            }
            return;
        }
        if (m_post_doNewMove) {
            m_post_doNewMove = false;
        }
    } else {
        stopEngine();
    }
    if (m_use_engine == GTP::KATAGO_ENGINE && cfg_engine_type == GTP::GTP_INTERFACE) {
        if (!m_gtp_pending_cmd.IsEmpty()) {
            // lock the board
            m_panelBoard->lockState();
            m_StateEngine = std::make_unique<GameState>(m_State);
            m_gtp_send_cmd = m_gtp_pending_cmd + "\n";
            m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
            m_timerIdleWakeUp.Start(WAKE_UP_TIMER_MS);
            m_katagoStatus = KATAGO_GTP_WAIT;
            m_gtp_pending_cmd.clear();
            return;
        }
    }
#endif

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
            m_tock.Play(wxSOUND_ASYNC);
        }
    } else if (m_State.get_to_move() == m_playerColor) {
        ::wxMessageBox(_("Computer passes"), _("Pass"), wxOK, this);
    }

    if (m_State.get_passes() >= 2 || m_State.get_last_move() == FastBoard::RESIGN) {
        float komi, score, prekomi, handicap;
        bool won = scoreGame(komi, handicap, score, prekomi);
        bool accepts = scoreDialog(komi, handicap, score, prekomi,
                                   m_State.get_last_move() != FastBoard::RESIGN);
        if (accepts || m_State.get_last_move() == FastBoard::RESIGN) {
            // If it is a new game, nothing is processed.
            ratedGameEnd(won);
        } else {
            m_disputing = true;
            // undo passes
            m_State.undo_move();
            m_State.undo_move();

            if (m_State.get_to_move() != m_playerColor) {  // Next turn is computer
                wxLogDebug(_("Computer to move"));
            } else {                                       // Next turn is human
                m_pondering = true;
            }
            startEngine();
        }
    } else {
        if (!m_analyzing) {
            if (m_State.get_to_move() != m_playerColor) {  // Next turn is computer
                wxLogDebug(_("Computer to move"));
                startEngine();
            } else {                                       // Next turn is human
                // Pondering only works in new games and with unlimited engine max level.
                if (((m_use_engine == GTP::ORIGINE_ENGINE && m_ponderEnabled) ||
                     (m_use_engine == GTP::KATAGO_ENGINE && m_ponderKataGoEnabled)) &&
                    !m_ratedGame &&
                    !m_ponderedOnce &&
                    !m_visitLimit) {

                    m_pondering = true;
                    startEngine();
                }
                // If pondering is off, wait for the human side to move.
            }
        } else {
            // Human side moved during analysis.
            startEngine();
            m_panelBoard->unlockState();
            m_playerColor = m_State.get_to_move();
            m_panelBoard->setPlayerColor(m_playerColor);
        }
        m_panelBoard->setShowTerritory(false);
    }

    if (!m_ratedGame) {
        gameNoLongerCounts();
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
#ifdef USE_THREAD
    bool wasAnalyzing = m_analyzing && !m_pondering;
    bool wasRunning = stopEngine();
#else
    bool wasRunning = false;
    if (m_use_engine == GTP::KATAGO_ENGINE) {
        // If KataGo is currently requesting analysis,
        // MainFrame::doRecieveKataGo will process it instead.
        if (m_katagoStatus != KATAGO_IDLE) {
            if (m_runflag) {
                m_runflag = false;
                m_post_doSettingsDialog = true;
            }
            return;
        }
        if (m_post_doSettingsDialog) {
            m_post_doSettingsDialog = false;
            wasRunning = true;
        }
    } else {
        wasRunning = stopEngine();
    }
    bool wasAnalyzing = m_analyzing && !m_pondering;
#endif

    SettingsDialog mydialog(this);

    if (mydialog.ShowModal() == wxID_OK) {
        wxLogDebug(_("OK clicked"));

        m_netsEnabled = wxConfig::Get()->Read(wxT("netsEnabled"), 1);
        m_passEnabled = wxConfig::Get()->Read(wxT("passEnabled"), 1);
        m_soundEnabled = wxConfig::Get()->Read(wxT("soundEnabled"), 1);
        m_resignEnabled = wxConfig::Get()->Read(wxT("resignEnabled"), 1);
        m_resignKataGoEnabled = wxConfig::Get()->Read(wxT("resignKataGoEnabled"), 1);
        m_ponderEnabled = wxConfig::Get()->Read(wxT("ponderEnabled"), 1);
        m_ponderKataGoEnabled = wxConfig::Get()->Read(wxT("ponderKataGoEnabled"), 1);
    }

    wxCommandEvent dummy;
    if (wasAnalyzing && wasRunning) {
        doAnalyze(dummy);
    }
}

void MainFrame::doNewGame(wxCommandEvent& event) {
#ifdef USE_THREAD
    stopEngine(false);
#else
    if (m_use_engine == GTP::KATAGO_ENGINE) {
        // If KataGo is currently requesting analysis,
        // MainFrame::doRecieveKataGo will process it instead.
        if (m_katagoStatus != KATAGO_IDLE) {
            if (m_runflag) {
                m_runflag = false;
                m_update_score = false;
                m_post_doNewGame = true;
            }
            return;
        }
        if (m_post_doNewGame) {
            m_post_doNewGame = false;
        }
    } else {
        stopEngine(false);
    }
#endif

    NewGameDialog mydialog(this);

    if (mydialog.ShowModal() == wxID_OK) {
        wxLogDebug(_("OK clicked"));

        m_use_engine = wxConfig::Get()->ReadBool(wxT("useEngine"), (long)0);
        if (cfg_use_engine == GTP::ORIGINE_ENGINE) {
            m_use_engine = GTP::ORIGINE_ENGINE;
        }

        if (m_use_engine == GTP::KATAGO_ENGINE && m_think_num > 0) {
            m_thinking_time = 0;
            m_thinking = false;
            m_think_num = 0;
            m_visits = 0;
        }

        if (m_use_engine == GTP::KATAGO_ENGINE && cfg_engine_type == GTP::GTP_INTERFACE) {
            m_time_for_move = -1;
        }

        if (m_use_engine == GTP::KATAGO_ENGINE) {
            if (wxConfig::Get()->ReadLong(wxT("NewGameRule"), (long)0) == 1) {
                m_japanese_rule = true;
            } else {
                m_japanese_rule = false;
            }
        } else {
            m_japanese_rule = false;
        }
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
            m_scoreHistogramWindow->setUseKataGo(m_use_engine == GTP::KATAGO_ENGINE);
        }
        if (m_use_engine == GTP::KATAGO_ENGINE) {
            m_State.set_timecontrol(mydialog.getTimeControl() * 60 * 100,
                                    mydialog.getByoControl() * 100, 1, 0);
        } else {
            m_State.set_timecontrol(mydialog.getTimeControl() * 60 * 100,
                                    (mydialog.getByoControl() + 1) * 100, 1, 0);
        }
        m_StateStack.clear();
        m_visitLimit = mydialog.getSimulations();
        m_playerColor = mydialog.getPlayerColor();
        // XXX
        m_netsEnabled = mydialog.getNetsEnabled();
        setActiveMenus();
        // XXX
        m_panelBoard->setPlayerColor(m_playerColor);
        m_panelBoard->setShowTerritory(false);
        m_panelBoard->setUseKataGo(m_use_engine == GTP::KATAGO_ENGINE);
        m_panelBoard->setKataGoEngineType(cfg_engine_type);
        m_analyzing = false;
        m_pondering = false;
        m_disputing = false;
        gameNoLongerCounts();

#ifndef USE_THREAD
        if (m_use_engine == GTP::KATAGO_ENGINE &&
            cfg_engine_type == GTP::GTP_INTERFACE) {
            m_StateEngine = std::make_unique<GameState>(m_State);
            if (m_visitLimit <= 0) {
                m_gtp_send_cmd = wxString::Format("kata-set-param maxVisits %d\n", INT_MAX);
            } else {
                m_gtp_send_cmd = wxString::Format("kata-set-param maxVisits %d\n", m_visitLimit);
            }
            m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
            // lock the board
            m_panelBoard->lockState();
            m_timerIdleWakeUp.Start(WAKE_UP_TIMER_MS);
            m_katagoStatus = KATAGO_GAME_START;
            return;
        } else if (m_use_engine == GTP::KATAGO_ENGINE && 
                   cfg_engine_type == GTP::ANALYSIS &&
                   m_playerColor == m_State.get_to_move()) {
            if (getAnalysisPolicyAndOwner()) {
                // lock the board
                m_panelBoard->lockState();
                m_timerIdleWakeUp.Start(WAKE_UP_TIMER_MS);
                m_katagoStatus = ANALYSIS_STARTING_WAIT;
                return;
            }
        }
#endif
        wxCommandEvent myevent(wxEVT_NEW_MOVE, GetId());
        myevent.SetEventObject(this);
        ::wxPostEvent(m_panelBoard->GetEventHandler(), myevent);
        broadcastCurrentMove();
    }
}

#ifndef USE_THREAD
bool MainFrame::getAnalysisPolicyAndOwner(bool get_policy, bool get_ownership) {
    m_StateEngine = std::make_unique<GameState>(m_State);
    uint64_t query_ms
        = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    m_query_id = std::to_string(query_ms);
    int board_size = m_StateEngine->board.get_boardsize();
    m_send_json = {};
    try {
        m_send_json["id"] = "analysis_" + m_query_id;
        if (m_japanese_rule) {
            m_send_json["rules"] = "japanese";
            m_send_json["whiteHandicapBonus"] = "0";
        } else {
            m_send_json["rules"] = "chinese";
            m_send_json["whiteHandicapBonus"] = "N";
        }
        m_send_json["komi"] = m_StateEngine->get_komi();
        m_send_json["boardXSize"] = board_size;
        m_send_json["boardYSize"] = board_size;
        if (!m_move_handi.empty()) {
            int i = 0;
            for (auto itr = m_move_handi.begin(); itr != m_move_handi.end(); ++itr) {
                std::string handi_move = m_StateEngine->move_to_text(*itr);
                m_send_json["initialStones"][i][0] = "B";
                m_send_json["initialStones"][i][1] = handi_move;
                i++;
            }
            m_send_json["initialPlayer"] = "W";
        } else {
            m_send_json["initialPlayer"] = "B";
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
        }
        m_send_json["includeOwnership"] = get_ownership;
        m_send_json["includePolicy"] = get_policy;
        m_send_json["maxVisits"] = 1;
        std::string req_query = m_send_json.dump();
        wxString send_msg = req_query + "\n";
        m_out->Write(send_msg, send_msg.length());
    } catch(const std::exception& e) {
        wxLogError(_("Exception at getAnalysisPolicyAndOwner: %s %s"), typeid(e).name(), e.what());
        return false;
    }
    return true;
}
#endif

void MainFrame::setActiveMenus() {
    int boardsize = m_State.board.get_boardsize();
    if (m_use_engine == GTP::ORIGINE_ENGINE && boardsize != 19) {
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
#ifdef USE_THREAD
    stopEngine(false);
#else
    if (m_use_engine == GTP::KATAGO_ENGINE) {
        // If KataGo is currently requesting analysis,
        // MainFrame::doRecieveKataGo will process it instead.
        if (m_katagoStatus != KATAGO_IDLE) {
            if (m_runflag) {
                m_runflag = false;
                m_update_score = false;
                m_post_doNewRatedGame = true;
            }
            return;
        }
        if (m_post_doNewRatedGame) {
            m_post_doNewRatedGame = false;
        }
    } else {
        stopEngine(false);
    }
#endif

    m_use_engine = wxConfig::Get()->ReadBool(wxT("ratedGameEngine"), (long)0);
    if (cfg_use_engine == GTP::ORIGINE_ENGINE) {
        m_use_engine = GTP::ORIGINE_ENGINE;
    }

    if (m_use_engine == GTP::KATAGO_ENGINE && m_think_num > 0) {
        m_thinking_time = 0;
        m_thinking = false;
        m_think_num = 0;
        m_visits = 0;
    }

    if (m_use_engine == GTP::KATAGO_ENGINE && cfg_engine_type == GTP::GTP_INTERFACE) {
        m_time_for_move = -1;
    }

    m_analyzing = false;
    m_disputing = false;
    m_pondering = false;

    int rank = 0;
    if (m_ratedSize == 9) {
        if (m_use_engine == GTP::ORIGINE_ENGINE) {
            rank = wxConfig::Get()->ReadLong(wxT("userRank9"), (long)-30);
        } else {
            rank = wxConfig::Get()->ReadLong(wxT("userRank9KataGo"), (long)-15);
            if (rank < -15) {
                rank = -15;
            }
        }
    } else if (m_ratedSize == 19) {
        if (m_use_engine == GTP::ORIGINE_ENGINE) {
            rank = wxConfig::Get()->ReadLong(wxT("userRank19"), (long)-15);
        } else {
            rank = wxConfig::Get()->ReadLong(wxT("userRank19KataGo"), (long)-15);
            if (rank < -15) {
                rank = -15;
            }
        }
    }

    wxLogDebug(_("Last rank was: %d"), rank);

    wxString mess = wxString(_("Your rank: "));

    mess += rankToString(rank);
    m_statusBar->SetStatusText(mess, 1);

    if (m_use_engine == GTP::ORIGINE_ENGINE) {
        SetTitle(_("Leela - ") + mess);
    } else if (cfg_engine_type == GTP::ANALYSIS) {
        SetTitle(_("KataGo") + _("(Analysis)") + wxT(" - ") + mess);
    } else if (cfg_engine_type == GTP::GTP_INTERFACE) {
        SetTitle(_("KataGo") + _("(GTP)") + wxT(" - ") + mess);
    }
    if (m_analysisWindow) {
        m_analysisWindow->Close();
    }
    if (m_scoreHistogramWindow) {
        m_scoreHistogramWindow->Hide();
        m_menuAnalyze->FindItem(ID_SCOREHISTOGRAMTOGGLE)->Check(false);
    }

    int used_rank = rank;
    int handicap = 5;
    int simulations = 250;

    // Correct for neural network enabled
    m_netsEnabled = wxConfig::Get()->Read(wxT("netsEnabled"), 1);
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

    float komi;
    {
        if (wxConfig::Get()->ReadLong(wxT("KataGoRule"), (long)0) == 1) {
            if (m_use_engine == GTP::KATAGO_ENGINE) {
                m_japanese_rule = true;
                komi = handicap ? 0.5f : 6.5f;
            } else {
                m_japanese_rule = false;
                komi = handicap ? 0.5f : 7.5f;
            }
        } else {
            m_japanese_rule = false;
            komi = handicap ? 0.5f : 7.5f;
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
        if (m_scoreHistogramWindow) {
            m_scoreHistogramWindow->ClearHistogram();
            m_scoreHistogramWindow->setUseKataGo(m_use_engine == GTP::KATAGO_ENGINE);
        }
        m_StateStack.clear();
        MCOwnerTable::get_MCO()->clear();
        m_panelBoard->clearViz();
        // max 60 minutes per game
        m_visitLimit = simulations;
        if (m_use_engine == GTP::ORIGINE_ENGINE) {
            m_State.set_timecontrol(2 * m_ratedSize * 60 * 100, 0, 0, 0);
        } else {
            if (m_ratedSize == 19) {
                m_State.set_timecontrol((m_ratedSize / 2) * 60 * 100, 200, 1, 0);
            } else {
                m_State.set_timecontrol((m_ratedSize / 4) * 60 * 100, 200, 1, 0);
            }
        }
        m_playerColor = (handicap >= 0 ? FastBoard::BLACK : FastBoard::WHITE);
        m_panelBoard->setPlayerColor(m_playerColor);
        m_panelBoard->setShowTerritory(false);
        m_panelBoard->setUseKataGo(m_use_engine == GTP::KATAGO_ENGINE);
        m_panelBoard->setKataGoEngineType(cfg_engine_type);
        m_ratedGame = true;

        setActiveMenus();
    }

#ifndef USE_THREAD
    if (m_use_engine == GTP::KATAGO_ENGINE && cfg_engine_type == GTP::GTP_INTERFACE) {
        m_StateEngine = std::make_unique<GameState>(m_State);
        if (m_visitLimit <= 0) {
            m_gtp_send_cmd = wxString::Format("kata-set-param maxVisits %d\n", INT_MAX);
        } else {
            m_gtp_send_cmd = wxString::Format("kata-set-param maxVisits %d\n", m_visitLimit);
        }
        m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
        // lock the board
        m_panelBoard->lockState();
        m_timerIdleWakeUp.Start(WAKE_UP_TIMER_MS);
        m_katagoStatus = KATAGO_GAME_START;
        return;
    } else if (m_use_engine == GTP::KATAGO_ENGINE && 
               cfg_engine_type == GTP::ANALYSIS && m_playerColor == m_State.get_to_move()) {
        if (getAnalysisPolicyAndOwner()) {
            // lock the board
            m_panelBoard->lockState();
            m_timerIdleWakeUp.Start(WAKE_UP_TIMER_MS);
            m_katagoStatus = ANALYSIS_STARTING_WAIT;
            return;
        }
    }
#endif
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
        if (m_use_engine == GTP::ORIGINE_ENGINE) {
            adjust = wxConfig::Get()->ReadLong(wxT("userRankAdjust9"), 10);
        } else {
            adjust = wxConfig::Get()->ReadLong(wxT("userRankAdjust9KataGo"), 10);
        }
    } else {
        if (m_use_engine == GTP::ORIGINE_ENGINE) {
            adjust = wxConfig::Get()->ReadLong(wxT("userRankAdjust19"), 10);
        } else {
            adjust = wxConfig::Get()->ReadLong(wxT("userRankAdjust19KataGo"), 10);
        }
    }

    if (m_ratedSize == 9) {
        if (m_use_engine == GTP::ORIGINE_ENGINE) {
            rank = wxConfig::Get()->ReadLong(wxT("userRank9"), (long)-30);
            hadWon = wxConfig::Get()->ReadBool(wxT("userLastWon9"), true);
            if (won) {
                wxConfig::Get()->Write(wxT("userLastWon9"), true);
            } else {
                wxConfig::Get()->Write(wxT("userLastWon9"), false);
            }
        } else {
            rank = wxConfig::Get()->ReadLong(wxT("userRank9KataGo"), (long)-30);
            hadWon = wxConfig::Get()->ReadBool(wxT("userLastWon9KataGo"), true);
            if (won) {
                wxConfig::Get()->Write(wxT("userLastWon9KataGo"), true);
            } else {
                wxConfig::Get()->Write(wxT("userLastWon9KataGo"), false);
            }
        }
    } else {
        assert(m_ratedSize == 19);
        if (m_use_engine == GTP::ORIGINE_ENGINE) {
            rank = wxConfig::Get()->ReadLong(wxT("userRank19"), (long)-15);
            hadWon = wxConfig::Get()->ReadBool(wxT("userLastWon19"), true);
            if (won) {
                wxConfig::Get()->Write(wxT("userLastWon19"), true);
            } else {
                wxConfig::Get()->Write(wxT("userLastWon19"), false);
            }
        } else {
            rank = wxConfig::Get()->ReadLong(wxT("userRank19KataGo"), (long)-15);
            hadWon = wxConfig::Get()->ReadBool(wxT("userLastWon19KataGo"), true);
            if (won) {
                wxConfig::Get()->Write(wxT("userLastWon19KataGo"), true);
            } else {
                wxConfig::Get()->Write(wxT("userLastWon19KataGo"), false);
            }
        }
    }

    if (won != hadWon) {
        adjust /= 2;
    }
    adjust = std::max(1L, adjust);

    if (won) {
        rank = rank + adjust;
        long max_rank = MAX_RANK;
        rank = std::min(max_rank, rank);
    } else {
        rank = rank - adjust;
        long min_rank = MIN_RANK;
        rank = std::max(min_rank, rank);
    }

    if (m_ratedSize == 9) {
        if (m_use_engine == GTP::ORIGINE_ENGINE) {
            wxConfig::Get()->Write(wxT("userRank9"), rank);
            wxConfig::Get()->Write(wxT("userRankAdjust9"), adjust);
        } else {
            wxConfig::Get()->Write(wxT("userRank9KataGo"), rank);
            wxConfig::Get()->Write(wxT("userRankAdjust9KataGo"), adjust);
        }
    } else {
        assert(m_ratedSize == 19);
        if (m_use_engine == GTP::ORIGINE_ENGINE) {
            wxConfig::Get()->Write(wxT("userRank19"), rank);
            wxConfig::Get()->Write(wxT("userRankAdjust19"), adjust);
        } else {
            wxConfig::Get()->Write(wxT("userRank19KataGo"), rank);
            wxConfig::Get()->Write(wxT("userRankAdjust19KataGo"), adjust);
        }
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
#ifdef USE_THREAD
    stopEngine();
#else
    if (m_use_engine == GTP::ORIGINE_ENGINE) {
        stopEngine();
    }
#endif

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
        if (m_use_engine == GTP::ORIGINE_ENGINE) {
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

    wxString confidence;
    if (dispute) {
        if (score > 0.0f) {
            confidence = _("BLACK wins.");
        } else {
            confidence = _("WHITE wins.");
        }
    } else if (m_use_engine == GTP::ORIGINE_ENGINE && m_State.board.get_boardsize() == 19) {
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
    if (m_use_engine == GTP::KATAGO_ENGINE && m_think_num) {
        confidence.Printf(_("Average thinking time per one move    : %d[ms]\n") +
                          _("Average number of visits per one move : %d[times]"),
                          m_thinking_time / m_think_num, m_visits / m_think_num);
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
#ifndef USE_THREAD
    if (m_use_engine == GTP::KATAGO_ENGINE) {
        // If KataGo is currently requesting analysis,
        // MainFrame::doRecieveKataGo will process it instead.
        if (m_katagoStatus != KATAGO_IDLE) {
            if (m_runflag) {
                m_runflag = false;
                m_post_doScore = true;
            }
            return;
        }
        if (m_post_doScore) {
            m_post_doScore = false;
        }
    }
#endif

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
#ifdef USE_THREAD
    stopEngine();
#else
    if (m_use_engine == GTP::KATAGO_ENGINE) {
        // If KataGo is currently requesting analysis,
        // MainFrame::doRecieveKataGo will process it instead.
        if (m_katagoStatus != KATAGO_IDLE) {
            if (m_runflag) {
                m_runflag = false;
                m_post_doPass = true;
            }
            return;
        }
        if (m_post_doPass) {
            m_post_doPass = false;
        }
        if (cfg_engine_type == GTP::GTP_INTERFACE) {
            if (m_pass_send) {
                m_StateEngine = nullptr;
                if (m_playerColor == FastBoard::BLACK) {
                    m_gtp_send_cmd = "play b pass\n";
                } else {
                    m_gtp_send_cmd = "play w pass\n";
                }
                m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
                // lock the board
                m_panelBoard->lockState();
                m_timerIdleWakeUp.Start(WAKE_UP_TIMER_MS);
                m_katagoStatus = KATAGO_GTP_ETC_WAIT;
                return;
            } else {
                m_pass_send = true;
            }
        }
    } else {
        stopEngine();
    }
#endif

    m_State.play_pass();
    //::wxLogMessage("User passes");
    wxCommandEvent myevent(wxEVT_NEW_MOVE, GetId());
    myevent.SetEventObject(this);
    ::wxPostEvent(m_panelBoard->GetEventHandler(), myevent);
    broadcastCurrentMove();
}

void MainFrame::gameNoLongerCounts() {
    if (m_use_engine == GTP::ORIGINE_ENGINE) {
        SetTitle(_("Leela") +
                 _(" - move ") + wxString::Format(wxT("%i"), m_State.get_movenum() + 1));
    } else if (cfg_engine_type == GTP::ANALYSIS) {
        SetTitle(_("KataGo") + _("(Analysis)") +
                 _(" - move ") + wxString::Format(wxT("%i"), m_State.get_movenum() + 1));
    } else if (cfg_engine_type == GTP::GTP_INTERFACE) {
        SetTitle(_("KataGo") + _("(GTP)") +
                 _(" - move ") + wxString::Format(wxT("%i"), m_State.get_movenum() + 1));
    }
    m_ratedGame = false;
}

void MainFrame::doRealUndo(int count, bool force) {
#ifdef USE_THREAD
    bool wasAnalyzing = m_analyzing && !m_pondering;
    bool wasRunning = stopEngine();
#else
    bool wasRunning = false;
    if (m_use_engine == GTP::KATAGO_ENGINE) {
        // If KataGo is currently requesting analysis,
        // MainFrame::doRecieveKataGo will process it instead.
        if (m_katagoStatus != KATAGO_IDLE) {
            if (m_runflag) {
                m_runflag = false;
                m_post_doRealUndo = count;
            }
            return;
        }
        if (m_post_doRealUndo) {
            m_post_doRealUndo = 0;
            wasRunning = true;
        }
        if (cfg_engine_type == GTP::GTP_INTERFACE && !force) {
            m_undo_num = count;
            m_undo_count = 0;
            m_StateEngine = std::make_unique<GameState>(m_State);
            m_gtp_send_cmd = "undo\n";
            m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
            // lock the board
            m_panelBoard->lockState();
            m_timerIdleWakeUp.Start(WAKE_UP_TIMER_MS);
            m_katagoStatus = KATAGO_GTP_ETC_WAIT;
            return;
        }
    } else {
        wasRunning = stopEngine();
    }
    bool wasAnalyzing = m_analyzing && !m_pondering;
#endif

    for (int i = 0; i < count; i++) {
        if (m_State.undo_move()) {
            wxLogDebug(_("Undoing one move"));
        }
    }
#ifndef USE_THREAD
    if (m_use_engine == GTP::KATAGO_ENGINE && cfg_engine_type == GTP::ANALYSIS) {
        m_post_move_change = wasAnalyzing && wasRunning;
        if (getAnalysisPolicyAndOwner()) {
            // lock the board
            m_panelBoard->lockState();
            m_timerIdleWakeUp.Start(WAKE_UP_TIMER_MS);
            m_katagoStatus = ANALYSIS_UNDO_WAIT;
            return;
        }
    }
#endif
    doPostMoveChange(wasAnalyzing && wasRunning);
}

void MainFrame::doRealForward(int count, bool force) {
#ifdef USE_THREAD
    bool wasAnalyzing = m_analyzing && !m_pondering;
    bool wasRunning = stopEngine();
#else
    bool wasRunning = false;
    if (m_use_engine == GTP::KATAGO_ENGINE) {
        // If KataGo is currently requesting analysis,
        // MainFrame::doRecieveKataGo will process it instead.
        if (m_katagoStatus != KATAGO_IDLE) {
            if (m_runflag) {
                m_runflag = false;
                m_post_doRealForward = count;
            }
            return;
        }
        if (m_post_doRealForward) {
            m_post_doRealForward = 0;
            wasRunning = true;
        }
        if (cfg_engine_type == GTP::GTP_INTERFACE && !force) {
            m_forward_num = count;
            m_forward_count = 0;
            m_StateEngine = std::make_unique<GameState>(m_State);
            if (m_StateEngine->forward_move()) {
                std::string move_str = m_StateEngine->move_to_text(m_StateEngine->get_last_move());
                if (move_str == "resign") {
                    move_str = "pass";
                }
                if (m_StateEngine->get_to_move() == FastBoard::BLACK) {
                    m_gtp_send_cmd = "play w " + move_str + "\n";
                } else {
                    m_gtp_send_cmd = "play b " + move_str + "\n";
                }
                m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
                // lock the board
                m_panelBoard->lockState();
                m_timerIdleWakeUp.Start(WAKE_UP_TIMER_MS);
                m_katagoStatus = KATAGO_GTP_ETC_WAIT;
                return;
            }
        }
    } else {
        wasRunning = stopEngine();
    }
    bool wasAnalyzing = m_analyzing && !m_pondering;
#endif

    if (m_use_engine != GTP::KATAGO_ENGINE || cfg_engine_type != GTP::GTP_INTERFACE) {
        for (int i = 0; i < count; i++) {
            if (m_State.forward_move()) {
                wxLogDebug(_("Forward one move"));
            }
        }
    }
#ifndef USE_THREAD
    if (m_use_engine == GTP::KATAGO_ENGINE && cfg_engine_type == GTP::ANALYSIS) {
        m_post_move_change = wasAnalyzing && wasRunning;
        if (getAnalysisPolicyAndOwner()) {
            // lock the board
            m_panelBoard->lockState();
            m_timerIdleWakeUp.Start(WAKE_UP_TIMER_MS);
            m_katagoStatus = ANALYSIS_UNDO_WAIT;
            return;
        }
    }
#endif
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

    if (wasAnalyzing) {
        doAnalyze(myevent /* dummy */);
    }
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
        int rule = tree->get_rule();
        if (rule == -1) {
            if (wxConfig::Get()->ReadLong(wxT("KataGoRule"), (long)0) == 1) {
                m_japanese_rule = true;
            } else {
                m_japanese_rule = false;
            }
        } else if (rule == 0) {
            m_japanese_rule = false;
        } else {
            m_japanese_rule = true;
        }
        if (m_use_engine == GTP::ORIGINE_ENGINE) {
            m_japanese_rule = false;
        }
        for (size_t i = 0; i < m_State.m_win_rate.size(); i++) {
            m_State.m_win_rate[i] = 0.5f;
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
    if (m_use_engine == GTP::KATAGO_ENGINE) {
        int minutes = wxConfig::Get()->ReadLong(wxT("DefaultMinutesKataGo"), (long)20);
        int byo = wxConfig::Get()->ReadLong(wxT("DefaultByoKataGo"), (long)2);
        m_State.set_timecontrol(minutes * 60 * 100, byo * 100, 1, 0);
        int simulations = wxConfig::Get()->ReadLong(wxT("DefaultSimulationsKataGo"), (long)6);
        m_visitLimit = NewGameDialog::simulationsToVisitLimit(simulations);
    } else {
        int minutes = wxConfig::Get()->ReadLong(wxT("DefaultMinutes"), (long)20);
        m_State.set_timecontrol(minutes * 60 * 100, 0, 0, 0);
        int simulations = wxConfig::Get()->ReadLong(wxT("DefaultSimulations"), (long)6);
        m_visitLimit = NewGameDialog::simulationsToVisitLimit(simulations);
    }
    bool nets = wxConfig::Get()->ReadBool(wxT("netsEnabled"), true);
    m_netsEnabled = (m_State.board.get_boardsize() == 19 ? nets : false);

    m_StateStack.clear();
    m_StateStack.push_back(m_State);
    m_playerColor = m_State.get_to_move();
    MCOwnerTable::get_MCO()->clear();
    m_panelBoard->setPlayerColor(m_playerColor);
    m_panelBoard->setShowTerritory(false);
    m_panelBoard->setUseKataGo(m_use_engine == GTP::KATAGO_ENGINE);
    m_panelBoard->setKataGoEngineType(cfg_engine_type);
    m_panelBoard->clearViz();
    if (m_scoreHistogramWindow) {
        m_scoreHistogramWindow->ClearHistogram();
        m_scoreHistogramWindow->setUseKataGo(m_use_engine == GTP::KATAGO_ENGINE);
    }
    gameNoLongerCounts();
    setActiveMenus();

    if (m_use_engine == GTP::KATAGO_ENGINE && m_think_num > 0) {
        m_thinking_time = 0;
        m_thinking = false;
        m_think_num = 0;
        m_visits = 0;
    }
#ifndef USE_THREAD
    if (m_use_engine == GTP::KATAGO_ENGINE) {
        if (cfg_engine_type == GTP::ANALYSIS) {
            if (getAnalysisPolicyAndOwner()) {
                // lock the board
                m_panelBoard->lockState();
                m_timerIdleWakeUp.Start(WAKE_UP_TIMER_MS);
                m_katagoStatus = ANALYSIS_SGF_WAIT;
                return;
            }
        } else {
            return;
        }
    }
#endif
    //signal board change
    wxCommandEvent myevent(wxEVT_BOARD_UPDATE, GetId());
    myevent.SetEventObject(this);
    ::wxPostEvent(m_panelBoard->GetEventHandler(), myevent);
    broadcastCurrentMove();
}

void MainFrame::doOpenSGF(wxCommandEvent& event) {
#ifdef USE_THREAD
    stopEngine(false);
#else
    if (m_use_engine == GTP::KATAGO_ENGINE) {
        // If KataGo is currently requesting analysis,
        // MainFrame::doRecieveKataGo will process it instead.
        if (m_katagoStatus != KATAGO_IDLE) {
            if (m_runflag) {
                m_runflag = false;
                m_update_score = false;
                m_post_doOpenSGF = true;
            }
            return;
        }
        if (m_post_doOpenSGF) {
            m_post_doOpenSGF = false;
        }
    } else {
        stopEngine(false);
    }
#endif

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
#ifndef USE_THREAD
                if (m_use_engine == GTP::KATAGO_ENGINE && cfg_engine_type == GTP::GTP_INTERFACE) {
                    m_StateEngine = std::make_unique<GameState>(m_State);
                    if (m_State.board.get_stone_count() <= 0) {
                        m_gtp_send_cmd = "clear_board\n";
                    } else {
                        m_gtp_send_cmd = "loadsgf " + path + "\n";
                    }
                    m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
                    // lock the board
                    m_panelBoard->lockState();
                    m_timerIdleWakeUp.Start(WAKE_UP_TIMER_MS);
                    m_katagoStatus = KATAGO_GTP_ETC_WAIT;
                }
#endif
            }
        }
    }
}

void MainFrame::doSaveSGF(wxCommandEvent& event) {
#ifdef USE_THREAD
    stopEngine();
#else
    if (m_use_engine == GTP::KATAGO_ENGINE) {
        // If KataGo is currently requesting analysis,
        // MainFrame::doRecieveKataGo will process it instead.
        if (m_katagoStatus != KATAGO_IDLE) {
            if (m_runflag) {
                m_runflag = false;
                m_post_doSaveSGF = true;
            }
            return;
        }
        if (m_post_doSaveSGF) {
            m_post_doSaveSGF = false;
        }
    } else {
        stopEngine();
    }
#endif

    std::string sgfgame = SGFTree::state_to_string(&m_State,
                                                   !m_playerColor,
                                                   m_japanese_rule,
                                                   m_use_engine != GTP::ORIGINE_ENGINE);

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
#ifdef USE_THREAD
    gameNoLongerCounts();
    m_ponderedOnce = true;
    bool wasAnalyzing = m_analyzing && !m_pondering;
    bool wasPondering = m_pondering;
    bool wasRunning = stopEngine();
#else
    bool wasRunning = false;
    if (m_use_engine == GTP::KATAGO_ENGINE) {
        // If KataGo is currently requesting analysis,
        // MainFrame::doRecieveKataGo will process it instead.
        if (m_katagoStatus != KATAGO_IDLE) {
            if (m_runflag) {
                m_runflag = false;
                m_post_doForceMove = true;
            }
            return;
        }
        if (m_post_doForceMove) {
            m_post_doForceMove = false;
            wasRunning = true;
        }
    } else {
        wasRunning = stopEngine();
    }
    gameNoLongerCounts();
    m_ponderedOnce = true;
    bool wasAnalyzing = m_analyzing && !m_pondering;
    bool wasPondering = m_pondering;
#endif

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
#ifdef USE_THREAD
        stopEngine();
#else
        if (m_use_engine == GTP::KATAGO_ENGINE) {
            // If KataGo is currently requesting analysis,
            // MainFrame::doRecieveKataGo will process it instead.
            if (m_katagoStatus != KATAGO_IDLE) {
                if (m_runflag) {
                    m_runflag = false;
                    m_post_doResign = true;
                }
                return;
            }
            if (m_post_doResign) {
                m_post_doResign = false;
            }
            if (cfg_engine_type == GTP::GTP_INTERFACE) {
                if (m_resign_send) {
                    m_StateEngine = nullptr;
                    if (m_playerColor == FastBoard::BLACK) {
                        m_gtp_send_cmd = "play b pass\n";
                        m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
                        m_gtp_send_cmd = "play b resign\n";
                    } else {
                        m_gtp_send_cmd = "play w pass\n";
                        m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
                        m_gtp_send_cmd = "play w resign\n";
                    }
                    // lock the board
                    m_panelBoard->lockState();
                    m_timerIdleWakeUp.Start(WAKE_UP_TIMER_MS);
                    m_katagoStatus = KATAGO_GTP_ETC_WAIT;
                    return;
                } else {
                    m_resign_send = true;
                }
            }
        } else {
            stopEngine();
        }
#endif

        m_State.play_move(FastBoard::RESIGN);

        wxCommandEvent myevent(wxEVT_NEW_MOVE, GetId());
        myevent.SetEventObject(this);
        ::wxPostEvent(GetEventHandler(), myevent);
    }
}

void MainFrame::doAnalyze(wxCommandEvent& event) {
#ifdef USE_THREAD
    gameNoLongerCounts();
    bool wasAnalyzing = m_analyzing && !m_pondering;
    bool wasRunning = stopEngine();
#else
    bool wasRunning = false;
    if (m_use_engine == GTP::KATAGO_ENGINE) {
        // If KataGo is currently requesting analysis,
        // MainFrame::doRecieveKataGo will process it instead.
        if (m_katagoStatus != KATAGO_IDLE) {
            if (m_runflag) {
                m_runflag = false;
                m_post_doAnalyze = true;
            }
            return;
        }
        if (m_post_doAnalyze) {
            m_post_doAnalyze = false;
            wasRunning = true;
        }
    } else {
        wasRunning = stopEngine();
    }
    gameNoLongerCounts();
    bool wasAnalyzing = m_analyzing && !m_pondering;
#endif

    m_ponderedOnce |= wasRunning;
    m_pondering = false;

    if (!wasAnalyzing || !wasRunning) {
        if (!wasAnalyzing) {
            m_StateStack.push_back(m_State);
            setActiveMenus();
        }
        m_analyzing = true;
        SetStatusBarText(_("Thinking..."), 1);
        startEngine();
    } else if (wasAnalyzing) {
        m_analyzing = false;
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
        std::string sgfgame = SGFTree::state_to_string(&m_State,
                                                       !m_playerColor,
                                                       m_japanese_rule,
                                                       m_use_engine != GTP::ORIGINE_ENGINE);
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
                int rule = sgftree->get_rule();
                if (rule == -1) {
                    if (wxConfig::Get()->ReadLong(wxT("KataGoRule"), (long)0) == 1) {
                        m_japanese_rule = true;
                    } else {
                        m_japanese_rule = false;
                    }
                } else if (rule == 0) {
                    m_japanese_rule = false;
                } else {
                    m_japanese_rule = true;
                }
                if (m_use_engine == GTP::ORIGINE_ENGINE) {
                    m_japanese_rule = false;
                }
                for (size_t i = 0; i < m_State.m_win_rate.size(); i++) {
                    m_State.m_win_rate[i] = 0.5f;
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
                m_panelBoard->setUseKataGo(m_use_engine == GTP::KATAGO_ENGINE);
                m_panelBoard->setKataGoEngineType(cfg_engine_type);
                m_panelBoard->clearViz();
                if (m_scoreHistogramWindow) {
                    m_scoreHistogramWindow->ClearHistogram();
                    m_scoreHistogramWindow->setUseKataGo(m_use_engine == GTP::KATAGO_ENGINE);
                }
                gameNoLongerCounts();
                setActiveMenus();

                if (m_use_engine == GTP::KATAGO_ENGINE && m_think_num > 0) {
                    m_thinking_time = 0;
                    m_thinking = false;
                    m_think_num = 0;
                    m_visits = 0;
                }

#ifndef USE_THREAD
                if (m_use_engine == GTP::KATAGO_ENGINE &&
                    cfg_engine_type == GTP::ANALYSIS) {
                    if (getAnalysisPolicyAndOwner()) {
                        // lock the board
                        m_panelBoard->lockState();
                        m_timerIdleWakeUp.Start(WAKE_UP_TIMER_MS);
                        m_katagoStatus = ANALYSIS_SGF_WAIT;
                        return;
                    }
                } else if (m_use_engine == GTP::KATAGO_ENGINE &&
                           cfg_engine_type == GTP::GTP_INTERFACE) {
                    if (m_State.board.get_stone_count() <= 0) {
                        m_StateEngine = std::make_unique<GameState>(m_State);
                        m_gtp_send_cmd = "clear_board\n";
                        m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
                        // lock the board
                        m_panelBoard->lockState();
                        m_timerIdleWakeUp.Start(WAKE_UP_TIMER_MS);
                        m_katagoStatus = KATAGO_GTP_ETC_WAIT;
                        return;
                    }
                    m_StateEngine = std::make_unique<GameState>(m_State);
                    uint64_t now_ms
                        = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                    fs::path p = fs::temp_directory_path();
                    fs::path::string_type str = p;
#ifdef WIN32
                    char stringw[500];
                    sprintf(stringw, "%ls", str.c_str());
                    std::string filename = stringw;
                    filename += "LeelaGUI" + std::to_string(now_ms) + ".sgf";
#else
                    std::string filename = str + std::to_string(now_ms) + ".sgf";
#endif
                    wxFileOutputStream file(filename);
                    file.Write(sgfstring.c_str(), sgfstring.length());
                    m_gtp_send_cmd = "loadsgf " + filename + "\n";
                    m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
                    m_gtp_send_cmd = "loadsgf remove " +  filename + "\n";
                    // lock the board
                    m_panelBoard->lockState();
                    m_timerIdleWakeUp.Start(WAKE_UP_TIMER_MS);
                    m_katagoStatus = KATAGO_GTP_ETC_WAIT;
                    return;
                }
#endif
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

#ifdef USE_THREAD
std::string MainFrame::GTPSend(const wxString& s, const int& sleep_ms) {
    std::string res_msg;
    char buffer[2048];
    m_GTPmutex.lock();
    if (!m_process->GetOutputStream()) {
        m_GTPmutex.unlock();
        return res_msg;
    }
    while (m_process->IsInputAvailable()) {
        m_in->Read(buffer, WXSIZEOF(buffer) - 1);
    }
    while (m_process->IsErrorAvailable()) {
        m_err->Read(buffer, WXSIZEOF(buffer) - 1);
    }
    m_out->Write(s.c_str(), s.length());
    m_GTPmutex.unlock();
    sleep_for(std::chrono::milliseconds(sleep_ms));

    while ( true ) {
        m_GTPmutex.lock();
        if (m_process->IsInputAvailable()) {
            buffer[m_in->Read(buffer, WXSIZEOF(buffer) - 1).LastRead()] = '\0';
            m_GTPmutex.unlock();
            res_msg += buffer;
            buffer[0] = 0;
            if (!res_msg.empty() && res_msg.back() == '\n') {
                res_msg.erase(res_msg.length() - 1);
                if (!res_msg.empty() && res_msg.back() == '\r') {
                    res_msg.erase(res_msg.length() - 1);
                }
                return res_msg;
            }
        } else if (m_process->IsErrorAvailable()) {
            m_err->Read(buffer, WXSIZEOF(buffer) - 1);
            m_GTPmutex.unlock();
        } else {
            m_GTPmutex.unlock();
            sleep_for(std::chrono::milliseconds(sleep_ms));
        }
    }
    m_GTPmutex.unlock();
    return res_msg;
}

#else

void MainFrame::OnIdleTimer(wxTimerEvent& WXUNUSED(event)) {
    if ( !m_process || !m_process->Exists(m_process->GetPid()) ) {
        return;
    }
    if ( !m_runflag ) {
        try {
            if ( m_katagoStatus == GAME_FIRST_QUERY_WAIT ) {
                nlohmann::json terminate_json = nlohmann::json::parse(R"({"action":"terminate"})");
                terminate_json["id"] = "terminate_" + m_query_id;
                terminate_json["terminateId"] = "play1_" + m_query_id;
                std::string req_query_terminate = terminate_json.dump();
                wxString send_msg = req_query_terminate + "\n";
                m_out->Write(send_msg, send_msg.length());
            } else if ( m_katagoStatus == ANALYSIS_QUERY_WAIT ) {
                nlohmann::json terminate_json = nlohmann::json::parse(R"({"action":"terminate"})");
                terminate_json["id"] = "terminate_" + m_query_id;
                terminate_json["terminateId"] = "analysis_" + m_query_id;
                std::string req_query_terminate = terminate_json.dump();
                wxString send_msg = req_query_terminate + "\n";
                m_out->Write(send_msg, send_msg.length());
            } else if ( m_katagoStatus == KATAGO_GTP_WAIT ) {
                wxString send_cmd = "name\n";
                m_out->Write(send_cmd, strlen(send_cmd));
            } else if ( m_katagoStatus == KATAGO_GTP_ANALYSIS ) {
                m_gtp_send_cmd = "name\n";
                m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
            }
        } catch(const std::exception& e) {
            wxLogError(_("Exception at OnIdleTimer: %s %s"), typeid(e).name(), e.what());
        }
    }
    if ( m_katagoStatus != INIT &&
         m_katagoStatus != KATAGO_IDLE &&
         m_katagoStatus != KATAGO_STOPING &&
         m_katagoStatus != KATAGO_STOPED) {
        wxWakeUpIdle();
    }
}

void MainFrame::OnIdle(wxIdleEvent& event) {
    if ( m_process && m_process->Exists(m_process->GetPid()) && m_process->HasInput() ) {
        event.RequestMore();
    }
}

void MainFrame::doTerminatedKataGo(wxCommandEvent & event) {
    wxLogDebug(_("KataGo terminated"));
    m_timerIdleWakeUp.Stop();
    if (m_katagoStatus == KATAGO_STARTING) {
        delete m_process;
        m_katagoStatus = KATAGO_IDLE;
        cfg_use_engine = GTP::ORIGINE_ENGINE;
        cfg_engine_type = GTP::NONE;
        wxString errStr;
        errStr.Printf(_("A startup error was returned from KataGo engine: %s\n"
                        "Start with the Leela engine?"), "");
        int answer = ::wxMessageBox(errStr, _("Leela"), wxYES_NO | wxICON_EXCLAMATION);
        if (answer != wxYES) {
            Close();
        } else {
            wxConfig::Get()->Write(wxT("ratedGameEngine"), (long)0);
            m_japanese_rule = false;
            m_japanese_rule_init = false;
            m_ponderEnabled = wxConfig::Get()->ReadBool(wxT("ponderEnabled"), true);
            setStartMenus();
            wxCommandEvent evt;
            doNewRatedGame(evt);
        }
    } else if (m_katagoStatus == KATAGO_STOPING) {
        m_katagoStatus = KATAGO_IDLE;
        Close();
    } else {
        m_katagoStatus = KATAGO_IDLE;
        cfg_use_engine = GTP::ORIGINE_ENGINE;
        cfg_engine_type = GTP::NONE;
        wxConfig::Get()->Write(wxT("ratedGameEngine"), (long)0);
        wxLogError(_("KataGo terminated abnormally."));
    }
}

void MainFrame::doAnalysisStartingWait(const wxString& kataRes) {
    // Receive analysis response from KataGo (Get First board status).
    int who = 1 - m_StateEngine->get_to_move();
    nlohmann::json res_2_json;
    try {
        res_2_json = nlohmann::json::parse(kataRes);
        if (res_2_json.contains("id") &&
            res_2_json["id"].get<std::string>() != m_send_json["id"].get<std::string>()) {
            return;
        } else if (kataRes.find(R"("error":)") != std::string::npos) {
            wxLogError(_("Error was returned from KataGo engine: %s"), kataRes.mb_str());
            postIdle();
            return;
        } else if (res_2_json.contains("noResults") && res_2_json["noResults"].get<bool>()) {
            wxLogError(_("Error was returned from KataGo engine: %s"), "noResults");
            postIdle();
            return;
        }
        m_winrate = 1.0f - res_2_json["rootInfo"]["winrate"].get<float>();
        m_scoreMean = -1.0f * res_2_json["rootInfo"]["scoreLead"].get<float>();
        make_owner_policy(m_StateEngine, res_2_json, m_winrate, m_scoreMean);
    } catch (const std::exception& e) {
        wxLogError(_("Exception at ANALYSIS_STARTING_WAIT: %s %s"), typeid(e).name(), e.what());
        postIdle();
        return;
    }
    wxString mess;
    if (who == FastBoard::BLACK) {
        mess.Printf(_("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d"),
                    m_winrate * 100.0f, m_scoreMean, 0, 0);
    } else {
        mess.Printf(_("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d"),
                    100.0f - m_winrate * 100.0f, -1.0f * m_scoreMean, 0, 0);
    }
    SetStatusBarText(mess, 1);
    // signal update of visible board
    m_panelBoard->unlockState();
    wxCommandEvent myevent(wxEVT_NEW_MOVE, GetId());
    myevent.SetEventObject(this);
    ::wxPostEvent(m_panelBoard->GetEventHandler(), myevent);
    broadcastCurrentMove();
    postIdle();
}

void MainFrame::doAnalysisQuery(const wxString& kataRes) {
    std::string req_query;
    nlohmann::json res_1_json;
    try {
        res_1_json = nlohmann::json::parse(kataRes);
        if (res_1_json.contains("id") &&
            res_1_json["id"].get<std::string>() != m_send_json["id"].get<std::string>()) {
            return;
        } else if (kataRes.find(R"("error":)") != std::string::npos) {
            wxLogError(_("Error was returned from KataGo engine: %s"), kataRes.mb_str());
            SetStatusBarText(kataRes, 1);
            postIdle();
            return;
        }
        std::string::size_type pos = kataRes.find(R"("isDuringSearch":)");
        if (pos == std::string::npos) {
            return;
        } else if (m_isDuringSearch && kataRes.substr(pos + 17, 5) == "false") {
            m_isDuringSearch = false;
        }
        wxString mess;
        if (!res_1_json.contains("noResults") || !res_1_json["noResults"].get<bool>()) {
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
                if (j2["move"].is_null()) {
                    continue;
                }
                TRowVector row;
                row.emplace_back(_("Move").utf8_str(), j2["move"].get<std::string>());
                row.emplace_back(_("Effort%").utf8_str(),
                    std::to_string(100.0f * j2["visits"].get<int>()
                                   / (float)res_1_json["rootInfo"]["visits"].get<int>()));
                row.emplace_back(_("Simulations").utf8_str(), std::to_string(j2["visits"].get<int>()));
                if (who == FastBoard::BLACK) {
                    row.emplace_back(_("Win%").utf8_str(),
                                     std::to_string(100.0f * j2["winrate"].get<float>()));
                    row.emplace_back(_("Score").utf8_str(),
                                     std::to_string(j2["scoreLead"].get<float>()));
                } else {
                    row.emplace_back(_("Win%").utf8_str(),
                                     std::to_string(100.0f - 100.0f * j2["winrate"].get<float>()));
                    row.emplace_back(_("Score").utf8_str(),
                                     std::to_string(-1.0f * j2["scoreLead"].get<float>()));
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
                move_data->emplace_back(j2["move"].get<std::string>(),
                    (float)(j2["visits"].get<int>()
                        / (double)res_1_json["rootInfo"]["visits"].get<int>()));
            }
            if (who == FastBoard::BLACK) {
                mess.Printf((_("Under analysis... ") + _("Win rate:%3.1f%% Score:%.1f")),
                            100.0f - m_winrate * 100.0f, -1.0f * m_scoreMean);
            } else {
                mess.Printf((_("Under analysis... ") + _("Win rate:%3.1f%% Score:%.1f")),
                            m_winrate * 100.0f, m_scoreMean);
            }
            make_owner_policy(m_StateEngine, res_1_json, m_winrate, m_scoreMean);
            SetStatusBarText(mess, 1);
            Utils::GUIAnalysis((void*)analysis_packet.release());
            Utils::GUIBestMoves((void*)move_data.release());
            m_StateEngine->m_black_score = -1.0f * m_scoreMean;
            if (m_update_score && !std::isnan(m_winrate)) {
                auto event_w = new wxCommandEvent(wxEVT_EVALUATION_UPDATE);
                auto movenum = m_StateEngine->get_movenum();
                float lead = 0.5f;
                if (m_scoreMean > 0.0f) {
                    lead = 0.5f - (std::min)(0.5f, std::sqrt(m_scoreMean) / 40.0f);
                } else if (m_scoreMean < 0.0) {
                    lead = 0.5f + (std::min)(0.5f, std::sqrt(-1.0f * m_scoreMean) / 40.0f);
                }
                std::tuple<int, float, float, float> scoretuple
                    = std::make_tuple(movenum, 1.0f - m_winrate, lead, 1.0f - m_winrate);
                event_w->SetClientData((void*)new auto(scoretuple));
                wxQueueEvent(GetEventHandler(), event_w);
            }
            //signal board change
            m_State = *m_StateEngine;
            wxCommandEvent myevent(wxEVT_BOARD_UPDATE, GetId());
            myevent.SetEventObject(this);
            ::wxPostEvent(m_panelBoard->GetEventHandler(), myevent);
            broadcastCurrentMove();
        }
        if (!m_isDuringSearch) {
            m_visit = res_1_json["rootInfo"]["visits"].get<int>();
            m_think_num += 1;
            m_visits += m_visit;

            auto query_end = std::chrono::system_clock::now();
            int think_time = (int)std::chrono::duration_cast<std::chrono::milliseconds>
                                  (query_end - m_query_start).count();
            m_thinking_time += think_time;
            m_thinking = false;
            mess.Printf(_("Analysis stopped. Time:%d[ms] Visits:%d"), think_time, m_visit);
            SetStatusBarText(mess, 1);
            postIdle();
        }
    } catch (const std::exception& e) {
        wxLogError(_("Exception at ANALYSIS_QUERY_WAIT: %s %s"), typeid(e).name(), e.what());
        postIdle();
    }
}

void MainFrame::doGameFirstQuery(const wxString& kataRes) {
    int who = m_StateEngine->get_to_move();
    nlohmann::json res_1_json;
    try {
        res_1_json = nlohmann::json::parse(kataRes);
        if (res_1_json.contains("id") &&
            res_1_json["id"].get<std::string>() != m_send_json["id"].get<std::string>()) {
            return;
        } else if (kataRes.find(R"("error":)") != std::string::npos) {
            wxLogError(_("Error was returned from KataGo engine: %s"), kataRes.mb_str());
            SetStatusBarText(kataRes, 1);
            postIdle();
            return;
        } else if (res_1_json.contains("noResults") && res_1_json["noResults"].get<bool>()) {
            wxLogError(_("Error was returned from KataGo engine: %s"), "noResults");
            SetStatusBarText(kataRes, 1);
            postIdle();
            return;
        }
        m_move_str = "pass";
        if (res_1_json.contains("moveInfos") && !res_1_json["moveInfos"].empty() &&
            !res_1_json["moveInfos"][0]["move"].is_null()) {
            m_move_str = res_1_json["moveInfos"][0]["move"].get<std::string>();
        }
        m_visit = res_1_json["rootInfo"]["visits"].get<int>();

        m_think_num += 1;
        m_visits += m_visit;

        m_winrate = res_1_json["rootInfo"]["winrate"].get<float>();
        m_scoreMean = res_1_json["rootInfo"]["scoreLead"].get<float>();
        m_StateEngine->m_black_score = m_scoreMean;
        m_resignKataGoEnabled = wxConfig::Get()->ReadBool(wxT("resignKataGoEnabled"), true);
        if (m_resignKataGoEnabled) {
            playMove(who);
        }
        if (m_move_str == "pass") {
            m_StateEngine->play_move(who, FastBoard::PASS);
        } else if (m_move_str == "resign") {
            m_StateEngine->play_move(who, FastBoard::RESIGN);
            wxString mess;
            auto query_end = std::chrono::system_clock::now();
            int think_time = (int)std::chrono::duration_cast<std::chrono::milliseconds>
                                  (query_end - m_query_start).count();
            m_thinking_time += think_time;
            m_thinking = false;
            if (who == FastBoard::BLACK) {
                mess.Printf(_("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d"),
                            100.0f - m_winrate * 100.0f, -1.0f * m_scoreMean, think_time, m_visit);
            } else {
                mess.Printf(_("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d"),
                            m_winrate * 100.0f, m_scoreMean, think_time, m_visit);
            }
            SetStatusBarText(mess, 1);
            wxQueueEvent(GetEventHandler(), new wxCommandEvent(wxEVT_NEW_MOVE));
            m_StateEngine->stop_clock(who);
            postIdle();
            return;
        } else {
            m_StateEngine->play_move(who, m_StateEngine->board.text_to_move(m_move_str));
        }
        make_owner_policy(m_StateEngine, res_1_json, m_winrate, m_scoreMean, false);
        if (wxConfig::Get()->ReadBool("showProbabilitiesEnabled", false)) {
            m_send_json["id"] = "play2_" + m_query_id;
            m_send_json["includeOwnership"] = false;
            m_send_json["includePolicy"] = true;
            m_send_json["maxVisits"] = 1;
            m_send_json["analysisPVLen"] = 1;
            int movenum = m_StateEngine->get_movenum() - 1;
            if (who == FastBoard::BLACK) {
                m_send_json["moves"][movenum][0] = "B";
            } else {
                m_send_json["moves"][movenum][0] = "W";
            }
            if (m_move_str == "resign") {
                m_send_json["moves"][movenum][1] = "pass";
            } else {
                m_send_json["moves"][movenum][1] = m_move_str;
            }
            std::string req_query_2 = m_send_json.dump();
            wxString send_msg = req_query_2 + "\n";
            m_out->Write(send_msg, send_msg.length());
            m_katagoStatus = GAME_SECOND_QUERY_WAIT;
        } else {
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
                std::tuple<int, float, float, float> scoretuple
                    = std::make_tuple(movenum, m_winrate, lead, m_winrate);
                event->SetClientData((void*)new auto(scoretuple));
                wxQueueEvent(GetEventHandler(), event);
            }
            wxString mess;
            auto query_end = std::chrono::system_clock::now();
            int think_time = (int)std::chrono::duration_cast<std::chrono::milliseconds>
                                  (query_end - m_query_start).count();
            m_thinking_time += think_time;
            m_thinking = false;
            if (who == FastBoard::BLACK) {
                mess.Printf(_("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d"),
                            100.0f - m_winrate * 100.0f, -1.0f * m_scoreMean, think_time, m_visit);
            } else {
                mess.Printf(_("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d"),
                            m_winrate * 100.0f, m_scoreMean, think_time, m_visit);
            }
            SetStatusBarText(mess, 1);
            wxQueueEvent(GetEventHandler(), new wxCommandEvent(wxEVT_NEW_MOVE));
            m_StateEngine->stop_clock(who);
            postIdle();
        }
    } catch (const std::exception& e) {
        m_StateEngine->stop_clock(who);
        wxLogError(_("Exception at GAME_FIRST_QUERY_WAIT: %s %s"), typeid(e).name(), e.what());
        postIdle();
    }
}

void MainFrame::doGameSecondQuery(const wxString& kataRes) {
    int who = 1 - m_StateEngine->get_to_move();
    nlohmann::json res_2_json;
    try {
        res_2_json = nlohmann::json::parse(kataRes);
        if (res_2_json.contains("id") &&
            res_2_json["id"].get<std::string>() != m_send_json["id"].get<std::string>()) {
            return;
        } else if (kataRes.find(R"("error":)") != std::string::npos) {
            wxLogError(_("Error was returned from KataGo engine: %s"), kataRes.mb_str());
            postIdle();
            return;
        } else if (res_2_json.contains("noResults") && res_2_json["noResults"].get<bool>()) {
            wxLogError(_("Error was returned from KataGo engine: %s"), "noResults");
            postIdle();
            return;
        }
        make_owner_policy(m_StateEngine, res_2_json, m_winrate, m_scoreMean, true, false);
    } catch (const std::exception& e) {
        m_StateEngine->stop_clock(who);
        wxLogError(_("Exception at GAME_SECOND_QUERY_WAIT: %s %s"), typeid(e).name(), e.what());
        postIdle();
        return;
    }
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
        std::tuple<int, float, float, float> scoretuple
            = std::make_tuple(movenum, m_winrate, lead, m_winrate);
        event->SetClientData((void*)new auto(scoretuple));
        wxQueueEvent(GetEventHandler(), event);
    }
    wxString mess;
    auto query_end = std::chrono::system_clock::now();
    int think_time = (int)std::chrono::duration_cast<std::chrono::milliseconds>
                          (query_end - m_query_start).count();
    m_thinking_time += think_time;
    m_thinking = false;
    if (who == FastBoard::BLACK) {
        mess.Printf(_("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d"),
                    100.0f - m_winrate * 100.0f, -1.0f * m_scoreMean, think_time, m_visit);
    } else {
        mess.Printf(_("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d"),
                    m_winrate * 100.0f, m_scoreMean, think_time, m_visit);
    }
    SetStatusBarText(mess, 1);
    wxQueueEvent(GetEventHandler(), new wxCommandEvent(wxEVT_NEW_MOVE));
    m_StateEngine->stop_clock(who);
    postIdle();
}

void MainFrame::doKataGameStart(const wxString& kataRes) {
    if (kataRes == "= KataGo") {
        return;
    }
    if (kataRes.StartsWith("?")) {
        wxLogError(_("Error was returned from KataGo engine: %s"), kataRes.mb_str());
        SetStatusBarText(kataRes, 1);
        postIdle();
        return;
    }
    if (m_gtp_send_cmd.StartsWith("kata-set-param maxVisits ")) {
        m_gtp_send_cmd = wxString::Format("komi %.1f\n", m_StateEngine->get_komi());
        m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
    } else if (m_gtp_send_cmd.StartsWith("komi ")) {
        m_gtp_send_cmd = wxString::Format("boardsize %d\n", m_StateEngine->board.get_boardsize());
        m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
    } else if (m_gtp_send_cmd.StartsWith("boardsize ")) {
        if (m_japanese_rule) {
            m_gtp_send_cmd = R"(kata-set-rules {"friendlyPassOk":false,"hasButton":false,)";
            m_gtp_send_cmd += R"("ko":"SIMPLE","scoring":"TERRITORY","suicide":false,)";
            m_gtp_send_cmd += R"("tax":"SEKI","whiteHandicapBonus":"0"})";
        } else {
            m_gtp_send_cmd = R"(kata-set-rules {"friendlyPassOk":true,"hasButton":false,)";
            m_gtp_send_cmd += R"("ko":"SIMPLE","scoring":"AREA","suicide":false,)";
            m_gtp_send_cmd += R"("tax":"NONE","whiteHandicapBonus":"N"})";
        }
        m_gtp_send_cmd += "\n";
        m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
    } else if (m_gtp_send_cmd.StartsWith("kata-set-rules ")) {
        m_gtp_send_cmd = "clear_board\n";
        m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
    } else if (m_gtp_send_cmd.StartsWith("clear_board\n")) {
        if (!m_move_handi.empty()) {
            m_gtp_send_cmd = "set_free_handicap";
            for(auto itr = m_move_handi.begin(); itr != m_move_handi.end(); ++itr) {
                int handi_move = *itr;
                m_gtp_send_cmd += " ";
                m_gtp_send_cmd += wxString(m_StateEngine->move_to_text(handi_move));
            }
            m_gtp_send_cmd += "\n";
            m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
            return;
        }
        if (m_StateEngine->get_movenum() == 0 &&
            m_playerColor == m_StateEngine->get_to_move()) {
            m_policy.clear();
            m_ownership.clear();
            m_gtp_send_cmd = "kata-raw-nn 0\n";
            m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
            return;
        }
        m_katagoStatus = KATAGO_IDLE;
        wxCommandEvent myevent(wxEVT_NEW_MOVE, GetId());
        myevent.SetEventObject(this);
        ::wxPostEvent(m_panelBoard->GetEventHandler(), myevent);
        broadcastCurrentMove();
        postIdle();
    } else if (m_gtp_send_cmd.StartsWith("set_free_handicap")) {
        if (m_StateEngine->get_movenum() == 0 &&
            m_playerColor == m_StateEngine->get_to_move()) {
            m_policy.clear();
            m_ownership.clear();
            m_gtp_send_cmd = "kata-raw-nn 0\n";
            m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
            return;
        }
        m_katagoStatus = KATAGO_IDLE;
        wxCommandEvent myevent(wxEVT_NEW_MOVE, GetId());
        myevent.SetEventObject(this);
        ::wxPostEvent(m_panelBoard->GetEventHandler(), myevent);
        broadcastCurrentMove();
        postIdle();
    } else if (m_gtp_send_cmd == "kata-raw-nn 0\n") {
        try {
            kataRawParse(kataRes, true);
            m_StateEngine->m_black_score = m_black_score;
            make_owner_policy(m_StateEngine,
                              m_ownership,
                              m_policy,
                              m_policy_pass,
                              m_black_winrate,
                              m_black_score);
            wxString mess;
            if (m_playerColor == FastBoard::BLACK) {
                mess.Printf(_("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d"),
                            m_black_winrate * 100.0f,
                            m_black_score, 0, 0);
            } else {
                mess.Printf(_("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d"),
                            100.0f - m_black_winrate * 100.0f,
                            -1.0f * m_black_score, 0, 0);
            }
            SetStatusBarText(mess, 1);
            // signal update of visible board
            m_panelBoard->unlockState();
            wxCommandEvent myevent(wxEVT_NEW_MOVE, GetId());
            myevent.SetEventObject(this);
            ::wxPostEvent(m_panelBoard->GetEventHandler(), myevent);
            broadcastCurrentMove();
            postIdle();
        } catch (const std::exception& e) {
            wxLogError(_("Exception at KATAGO_GAME_START(kata-raw-nn 0): %s %s"),
                       typeid(e).name(), e.what());
            postIdle();
        }
    }
}

void MainFrame::doKataGTPAnalysis(const wxString& kataRes) {
    if (kataRes == "= KataGo") {
        auto query_end = std::chrono::system_clock::now();
        int think_time = (int)std::chrono::duration_cast<std::chrono::milliseconds>
                            (query_end - m_query_start).count();
        m_thinking_time += think_time;
        m_thinking = false;
        wxString mess;
        mess.Printf(_("Analysis stopped. Time:%d[ms] Visits:%d"), think_time, m_visit_count);
        SetStatusBarText(mess, 1);
        postIdle();
    } else if (kataRes.StartsWith("?")) {
        wxLogError(_("Error was returned from KataGo engine: %s"), kataRes.mb_str());
        SetStatusBarText(kataRes, 1);
        postIdle();
    } else if (m_gtp_send_cmd.StartsWith("kata-analyze ") &&
               kataRes.StartsWith("info move ")) {
        m_kataRes = kataRes;
        m_policy.clear();
        m_ownership.clear();
        m_gtp_send_cmd = "kata-raw-nn 0\n";
        m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
    } else if (m_gtp_send_cmd.StartsWith("kata-raw-nn 0\n")) {
        kataRawParse(kataRes, true);
        int who = m_StateEngine->get_to_move();
        m_visit_count = 0;
        float best_winrate = std::nan("");
        float best_score = std::nan("");
        std::string visits_str;
        std::string winrate_str;
        std::string score_str;
        std::string::size_type pos = m_kataRes.find(" visits ");
        std::string::size_type pos_w;
        while (true) {
            if (pos == std::string::npos) {
                break;
            }
            visits_str.clear();
            for (auto i = pos + sizeof(" visits ") - 1; i < m_kataRes.length(); i++) {
                if (m_kataRes[i] != ' ') {
                    visits_str += m_kataRes[i];
                } else {
                    break;
                }
            }
            m_visit_count += stoi(visits_str);
            if (std::isnan(best_winrate)) {
                winrate_str.clear();
                pos_w = m_kataRes.find("winrate ", pos);
                pos_w = pos_w + sizeof("winrate ") - 1;
                for ( ; pos_w < m_kataRes.length(); pos_w++) {
                    if (m_kataRes[pos_w] != ' ') {
                        winrate_str += m_kataRes[pos_w];
                    } else {
                        break;
                    }
                }
                best_winrate = stof(winrate_str);
            }
            if (std::isnan(best_score)) {
                score_str.clear();
                pos = m_kataRes.find("scoreLead ", pos);
                pos = pos + sizeof("scoreLead ") - 1;
                for ( ; pos < m_kataRes.length(); pos++) {
                    if (m_kataRes[pos] != ' ') {
                        score_str += m_kataRes[pos];
                    } else {
                        break;
                    }
                }
                best_score = stof(score_str);
            }
            pos = m_kataRes.find(" visits ", pos + sizeof(" visits "));
        }
        m_think_num += 1;
        m_visits += m_visit_count;

        using TRowVector = std::vector<std::pair<std::string, std::string>>;
        using TDataVector = std::tuple<int, float, std::vector<TRowVector>>;
        using TMoveData = std::vector<std::pair<std::string, float>>;
        std::unique_ptr<TDataVector> analysis_packet(new TDataVector);
        std::unique_ptr<TMoveData> move_data(new TMoveData);
        std::get<0>(*analysis_packet) = who;
        std::get<1>(*analysis_packet) = best_score;
        auto& analysis_data = std::get<2>(*analysis_packet);
        std::string move_str;
        std::string pv_str;
        int visits;
        float winrate = 0.0f;
        float score = 0.0f;
        pos = 0;
        while ((pos = m_kataRes.find("info move ", pos)) != std::string::npos) {
            move_str.clear();
            pos = pos + sizeof("info move ") - 1;
            for ( ; pos < m_kataRes.length(); pos++) {
                if (m_kataRes[pos] != ' ') {
                    move_str += m_kataRes[pos];
                } else {
                    break;
                }
            }
            if (move_str.empty()) {
                continue;
            }
            visits_str.clear();
            pos = m_kataRes.find("visits ", pos);
            pos = pos + sizeof("visits ") - 1;
            for ( ; pos < m_kataRes.length(); pos++) {
                if (m_kataRes[pos] != ' ') {
                    visits_str += m_kataRes[pos];
                } else {
                    break;
                }
            }
            visits = stoi(visits_str);
            winrate_str.clear();
            pos = m_kataRes.find("winrate ", pos);
            pos = pos + sizeof("winrate ") - 1;
            for ( ; pos < m_kataRes.length(); pos++) {
                if (m_kataRes[pos] != ' ') {
                    winrate_str += m_kataRes[pos];
                } else {
                    break;
                }
            }
            winrate = stof(winrate_str);
            score_str.clear();
            pos = m_kataRes.find("scoreLead ", pos);
            pos = pos + sizeof("scoreLead ") - 1;
            for ( ; pos < m_kataRes.length(); pos++) {
                if (m_kataRes[pos] != ' ') {
                    score_str += m_kataRes[pos];
                } else {
                    break;
                }
            }
            score = stof(score_str);
            pv_str.clear();
            pos = m_kataRes.find("pv ", pos);
            pos = pos + sizeof("pv ") - 1;
            for ( ; pos < m_kataRes.length(); pos++) {
                if (m_kataRes[pos] != 'i') {
                    pv_str += m_kataRes[pos];
                } else {
                    break;  // Loop end when the next "info muve" is found
                }
            }
            TRowVector row;
            row.emplace_back(_("Move").utf8_str(), move_str);
            row.emplace_back(_("Effort%").utf8_str(),
                             std::to_string(100.0f * visits / (float)m_visit_count));
            row.emplace_back(_("Simulations").utf8_str(), std::to_string(visits));
            row.emplace_back(_("Win%").utf8_str(), std::to_string(100.0f * winrate));
            row.emplace_back(_("Score").utf8_str(), std::to_string(score));
            row.emplace_back(_("PV").utf8_str(), pv_str);
            analysis_data.emplace_back(row);
            move_data->emplace_back(move_str, (float)(visits / (double)m_visit_count));
        }
        Utils::GUIAnalysis((void*)analysis_packet.release());
        Utils::GUIBestMoves((void*)move_data.release());
        if (who == FastBoard::BLACK) {
            m_StateEngine->m_black_score = best_score;
        } else {
            m_StateEngine->m_black_score = -1.0f * best_score;
        }
        if (m_update_score && !std::isnan(best_winrate)) {
            auto event_w = new wxCommandEvent(wxEVT_EVALUATION_UPDATE);
            auto movenum = m_StateEngine->get_movenum();
            float lead = 0.5f;
            if (m_StateEngine->m_black_score > 0.0f) {
                lead = 0.5f +
                       (std::min)(0.5f, std::sqrt(-1.0f * m_StateEngine->m_black_score) / 40.0f);
            } else if (m_StateEngine->m_black_score < 0.0f) {
                lead = 0.5f -
                       (std::min)(0.5f, std::sqrt(m_StateEngine->m_black_score) / 40.0f);
            }
            if (who == FastBoard::BLACK) {
                std::tuple<int, float, float, float> scoretuple
                    = std::make_tuple(movenum, best_winrate, lead, best_winrate);
                event_w->SetClientData((void*)new auto(scoretuple));
            } else {
                std::tuple<int, float, float, float> scoretuple
                    = std::make_tuple(movenum, 1.0f - best_winrate, lead, 1.0f - best_winrate);
                event_w->SetClientData((void*)new auto(scoretuple));
            }
            wxQueueEvent(GetEventHandler(), event_w);
        }
        if (!std::isnan(best_winrate)) {
            m_StateEngine->m_black_score = m_black_score;
            make_owner_policy(m_StateEngine,
                              m_ownership,
                              m_policy,
                              m_policy_pass,
                              m_black_winrate,
                              m_black_score);
            wxString mess;
            mess.Printf((_("Under analysis... ") + _("Win rate:%3.1f%% Score:%.1f")),
                        best_winrate * 100.0f, best_score);
            SetStatusBarText(mess, 1);
            //signal board change
            m_State = *m_StateEngine;
            wxCommandEvent myevent(wxEVT_BOARD_UPDATE, GetId());
            myevent.SetEventObject(this);
            ::wxPostEvent(m_panelBoard->GetEventHandler(), myevent);
            broadcastCurrentMove();
        }
        if (m_StateEngine->get_to_move() == FastBoard::BLACK) {
            m_gtp_send_cmd = wxString::Format("kata-analyze b interval %i maxmoves %i\n",
                                              GTP_ANALYZE_INTERVAL, GTP_ANALYZE_MAX_MOVES);
        } else {
            m_gtp_send_cmd = wxString::Format("kata-analyze w interval %i maxmoves %i\n",
                                              GTP_ANALYZE_INTERVAL, GTP_ANALYZE_MAX_MOVES);
        }
        m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
    }
}

void MainFrame::doKataGTPWait(const wxString& kataRes) {
    if (kataRes == "= KataGo") {
        return;
    }
    if (m_gtp_send_cmd.StartsWith("time_settings ")) {
        if (kataRes.StartsWith("?")) {
            wxLogError(_("Error was returned from KataGo engine: %s"), kataRes.mb_str());
            SetStatusBarText(kataRes, 1);
            postIdle();
            return;
        }
        if (m_StateEngine->get_to_move() == FastBoard::BLACK) {
            m_gtp_send_cmd = "kata-genmove_analyze b ownership true\n";
        } else {
            m_gtp_send_cmd = "kata-genmove_analyze w ownership true\n";
        }
        m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
    } else if (kataRes.StartsWith("?")) {
        wxLogError(_("Error was returned from KataGo engine: %s"), kataRes.mb_str());
        SetStatusBarText(kataRes, 1);
        postIdle();
    } else if (m_gtp_send_cmd.StartsWith("kata-genmove_analyze ")) {
        try {
            int who = m_StateEngine->get_to_move();
            if (kataRes.StartsWith("play ")) {
                std::string move_str;
                std::string winrate_str;
                std::string scoreMean_str;
                std::string::size_type pos;
                move_str = kataRes.substr(sizeof("play ") - 1,
                                          kataRes.length() - (sizeof("play ") - 1));
                if (!move_str.empty()) {
                    std::string find_str = "move " + move_str;
                    if (move_str != "pass" && move_str != "resign") {
                        pos = m_info_move.find(find_str);
                        if (pos == std::string::npos) {
                            pos = 0;
                        }
                    } else {
                        pos = 0;
                    }
                } else {  // GTP error response
                    m_StateEngine->stop_clock(who);
                    SetStatusBarText(kataRes, 1);
                    postIdle();
                    return;
                }
                pos = m_info_move.find("winrate ", pos);
                if (pos == std::string::npos) {
                    pos = 0;
                    winrate_str = "0.0";
                } else {
                    for (auto i = pos + sizeof("winrate ") - 1; i < m_info_move.length(); i++) {
                        if (m_info_move[i] != ' ') {
                            winrate_str += m_info_move[i];
                        } else {
                            break;
                        }
                    }
                }
                pos = m_info_move.find("scoreMean ", pos);
                if (pos == std::string::npos) {
                    scoreMean_str = "0.0";
                } else {
                    for (auto i = pos + sizeof("scoreMean ") - 1; i < m_info_move.length(); i++) {
                        if (m_info_move[i] != ' ') {
                            scoreMean_str += m_info_move[i];
                        } else {
                            break;
                        }
                    }
                }
                m_visit_count = 0;
                std::string visit_str;
                pos = m_info_move.find(" visits ");
                while (true) {
                    if (pos == std::string::npos) {
                        break;
                    }
                    visit_str.clear();
                    for (auto i = pos + sizeof(" visits ") - 1; i < m_info_move.length(); i++) {
                        if (m_info_move[i] != ' ') {
                            visit_str += m_info_move[i];
                        } else {
                            break;
                        }
                    }
                    m_visit_count += stoi(visit_str);
                    pos = m_info_move.find(" visits ", pos + sizeof(" visits "));
                }
                m_think_num += 1;
                m_visits += m_visit_count;
                pos = m_info_move.find("ownership ") + sizeof("ownership ") - 1;
                bool prev_sp = true;
                wxString s;
                m_ownership.clear();
                int board_size = m_StateEngine->board.get_boardsize();
                for (auto i = 0; i < board_size * board_size && pos < m_info_move.length(); pos++) {
                    if (m_info_move[pos] == (const char)' ') {
                        if (!prev_sp) {
                            prev_sp = true;
                            if (who == FastBoard::BLACK) {
                                m_ownership.emplace_back(wxAtof(s) * -1.0f);
                            } else {
                                m_ownership.emplace_back(wxAtof(s));
                            }
                            s.Empty();
                            i++;
                        }
                    } else {
                        if (prev_sp) {
                            prev_sp = false;
                            s.Empty();
                        }
                        s += m_info_move[pos];
                    }
                }
                if (!s.IsEmpty()) {
                    m_ownership.emplace_back(wxAtof(s));
                }

                if (move_str == "pass") {
                    m_StateEngine->play_move(who, FastBoard::PASS);
                } else if (move_str == "resign") {
                    m_StateEngine->play_move(who, FastBoard::RESIGN);
                } else {
                    m_StateEngine->play_move(who, m_StateEngine->board.text_to_move(move_str));
                }
                m_winrate = stof(winrate_str);
                m_scoreMean = stof(scoreMean_str);
                if (who == FastBoard::BLACK) {
                    m_black_winrate = m_winrate;
                    m_black_score = m_scoreMean;
                } else {
                    m_black_winrate = -1.0f * (m_winrate - 1.0f);
                    m_black_score = -1.0f * m_scoreMean;
                }
                m_black_win = m_black_winrate >= 0.5f;
                make_owner_policy(m_StateEngine,
                                  m_ownership,
                                  m_policy,
                                  m_policy_pass,
                                  m_black_winrate,
                                  m_black_score,
                                  false);
                if (wxConfig::Get()->ReadBool("showProbabilitiesEnabled", false)) {
                    m_StateEngine->m_black_score = m_black_score;
                    m_policy.clear();
                    m_gtp_send_cmd = "kata-raw-nn 0\n";
                    m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
                } else {
                    m_StateEngine->m_black_score = m_black_score;
                    if (m_update_score) {
                        // Broadcast result from search
                        auto event = new wxCommandEvent(wxEVT_EVALUATION_UPDATE);
                        auto movenum = m_StateEngine->get_movenum();
                        float lead = 0.5;
                        if (m_black_score > 0.0) {
                            lead = 0.5 + (std::min)(0.5f, std::sqrt(m_black_score) / 40);
                        } else if (m_black_score < 0.0) {
                            lead = 0.5 - (std::min)(0.5f, std::sqrt(-1.0f * m_black_score) / 40);
                        }
                        std::tuple<int, float, float, float> scoretuple
                            = std::make_tuple(movenum, m_black_winrate, lead, m_black_winrate);
                        event->SetClientData((void*)new auto(scoretuple));
                        wxQueueEvent(GetEventHandler(), event);
                    }
                    wxString mess;
                    auto query_end = std::chrono::system_clock::now();
                    int think_time = (int)std::chrono::duration_cast<std::chrono::milliseconds>
                                          (query_end - m_query_start).count();
                    m_thinking_time += think_time;
                    m_thinking = false;
                    if (m_playerColor == FastBoard::BLACK) {
                        mess.Printf(_("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d"),
                                    m_black_winrate * 100.0f,
                                    m_black_score,
                                    think_time,
                                    m_visit_count);
                    } else {
                        mess.Printf(_("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d"),
                                    100.0f - m_black_winrate * 100.0f,
                                    -1.0f * m_black_score,
                                    think_time,
                                    m_visit_count);
                    }
                    SetStatusBarText(mess, 1);
                    wxQueueEvent(GetEventHandler(), new wxCommandEvent(wxEVT_NEW_MOVE));
                    m_StateEngine->stop_clock(1 - m_StateEngine->get_to_move());
                    postIdle();
                }
            } else if (kataRes.StartsWith("info move ")) {
                m_info_move = kataRes;
            }
        } catch (const std::exception& e) {
            m_StateEngine->stop_clock(m_StateEngine->get_to_move());
            wxLogError(_("Exception at KATAGO_GTP_WAIT(kata-genmove_analyze): %s %s"),
                       typeid(e).name(), e.what());
            postIdle();
        }
    } else if (m_gtp_send_cmd.StartsWith("kata-raw-nn 0\n")) {
        try {
            kataRawParse(kataRes);
            m_StateEngine->m_black_score = m_black_score;
            make_owner_policy(m_StateEngine,
                              m_ownership,
                              m_policy,
                              m_policy_pass,
                              m_black_winrate,
                              m_black_score,
                              true,
                              false);

            if (m_update_score) {
                // Broadcast result from search
                auto event = new wxCommandEvent(wxEVT_EVALUATION_UPDATE);
                auto movenum = m_StateEngine->get_movenum();
                float lead = 0.5;
                if (m_black_score > 0.0) {
                    lead = 0.5 + (std::min)(0.5f, std::sqrt(m_black_score) / 40);
                } else if (m_black_score < 0.0) {
                    lead = 0.5 - (std::min)(0.5f, std::sqrt(-1.0f * m_black_score) / 40);
                }
                std::tuple<int, float, float, float> scoretuple
                    = std::make_tuple(movenum, m_black_winrate, lead, m_black_winrate);
                event->SetClientData((void*)new auto(scoretuple));
                wxQueueEvent(GetEventHandler(), event);
            }
            wxString mess;
            auto query_end = std::chrono::system_clock::now();
            int think_time = (int)std::chrono::duration_cast<std::chrono::milliseconds>
                                  (query_end - m_query_start).count();
            m_thinking_time += think_time;
            m_thinking = false;
            if (m_playerColor == FastBoard::BLACK) {
                mess.Printf(_("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d"),
                            m_black_winrate * 100.0f,
                            m_black_score,
                            think_time,
                            m_visit_count);
            } else {
                mess.Printf(_("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d"),
                            100.0f - m_black_winrate * 100.0f,
                            -1.0f * m_black_score,
                            think_time,
                            m_visit_count);
            }
            SetStatusBarText(mess, 1);
            wxQueueEvent(GetEventHandler(), new wxCommandEvent(wxEVT_NEW_MOVE));
            m_StateEngine->stop_clock(1 - m_StateEngine->get_to_move());
            postIdle();
        } catch (const std::exception& e) {
            m_StateEngine->stop_clock(1 - m_StateEngine->get_to_move());
            wxLogError(_("Exception at KATAGO_GTP_WAIT(kata-raw-nn 0): %s %s"),
                       typeid(e).name(), e.what());
            postIdle();
        }
    } else if (m_gtp_send_cmd.StartsWith("play ")) {
        wxQueueEvent(GetEventHandler(), new wxCommandEvent(wxEVT_NEW_MOVE));
        postIdle();
    }
}

void MainFrame::doKataGTPEtcWait(const wxString& kataRes) {
    if (kataRes == "= KataGo") {
        return;
    }
    if (m_gtp_send_cmd == "undo\n") {
        m_undo_count++;
        if (m_undo_count >= m_undo_num || kataRes.StartsWith("?")) {
            m_policy.clear();
            m_ownership.clear();
            m_gtp_send_cmd = "kata-raw-nn 0\n";
            m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
            m_gtp_send_cmd += "doRealUndo";
        } else if (kataRes.StartsWith("=")) {
            m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
        }
    } else if (kataRes.StartsWith("?")) {
        wxLogError(_("Error was returned from KataGo engine: %s"), kataRes.mb_str());
        SetStatusBarText(kataRes, 1);
        postIdle();
    } else if (m_gtp_send_cmd.find(" pass\n") != std::string::npos) {
        m_pass_send = false;
        postIdle();
        m_panelBoard->unlockState();
        wxCommandEvent evt;
        doPass(evt);
    } else if (m_gtp_send_cmd.find(" resign\n") != std::string::npos) {
        m_resign_send = false;
        postIdle();
        m_panelBoard->unlockState();
        wxCommandEvent evt;
        doResign(evt);
    } else if (m_gtp_send_cmd.StartsWith("play ")) {
        m_forward_count++;
        if (m_forward_count >= m_forward_num) {
            m_policy.clear();
            m_ownership.clear();
            m_gtp_send_cmd = "kata-raw-nn 0\n";
            m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
            m_gtp_send_cmd += "doRealForward";
        } else {
            if (m_StateEngine->forward_move()) {
                std::string move_str = m_StateEngine->move_to_text(m_StateEngine->get_last_move());
                if (move_str == "resign") {
                    move_str = "pass";
                }
                if (m_StateEngine->get_to_move() == FastBoard::BLACK) {
                    m_gtp_send_cmd = "play w " + move_str + "\n";
                } else {
                    m_gtp_send_cmd = "play b " + move_str + "\n";
                }
                m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
            } else {
                m_policy.clear();
                m_ownership.clear();
                m_gtp_send_cmd = "kata-raw-nn 0\n";
                m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
                m_gtp_send_cmd += "doRealForward";
            }
        }
    } else if (m_gtp_send_cmd.StartsWith("loadsgf ")) {
        if (m_gtp_send_cmd.StartsWith("loadsgf remove ")) {
            std::remove((const char*)m_gtp_send_cmd.substr(sizeof("loadsgf remove ") - 1,
                m_gtp_send_cmd.length() - sizeof("loadsgf remove ")).c_str());
        }
        if (m_visitLimit <= 0) {
            m_gtp_send_cmd = wxString::Format("kata-set-param maxVisits %d\n", INT_MAX);
        } else {
            m_gtp_send_cmd = wxString::Format("kata-set-param maxVisits %d\n", m_visitLimit);
        }
        m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
    } else if (m_gtp_send_cmd == "clear_board\n") {
        m_gtp_send_cmd = wxString::Format("komi %.1f\n", m_StateEngine->get_komi());
        m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
    } else if (m_gtp_send_cmd.StartsWith("komi ")) {
        m_gtp_send_cmd = wxString::Format("boardsize %d\n",
                                          m_StateEngine->board.get_boardsize());
        m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
    } else if (m_gtp_send_cmd.StartsWith("boardsize ")) {
        if (m_japanese_rule) {
            m_gtp_send_cmd = R"(kata-set-rules {"friendlyPassOk":false,)";
            m_gtp_send_cmd += R"("hasButton":false,"ko":"SIMPLE","scoring":"TERRITORY",)";
            m_gtp_send_cmd += R"("suicide":false,"tax":"SEKI","whiteHandicapBonus":"0"})";
        } else {
            m_gtp_send_cmd = R"(kata-set-rules {"friendlyPassOk":true,)";
            m_gtp_send_cmd += R"("hasButton":false,"ko":"SIMPLE","scoring":"AREA",)";
            m_gtp_send_cmd += R"("suicide":false,"tax":"NONE","whiteHandicapBonus":"N"})";
        }
        m_gtp_send_cmd += "\n";
        m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
    } else if (m_gtp_send_cmd.StartsWith("kata-set-rules ")) {
        if (m_visitLimit <= 0) {
            m_gtp_send_cmd = wxString::Format("kata-set-param maxVisits %d\n", INT_MAX);
        } else {
            m_gtp_send_cmd = wxString::Format("kata-set-param maxVisits %d\n", m_visitLimit);
        }
        m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
    } else if (m_gtp_send_cmd.StartsWith("kata-set-param maxVisits ")) {
        m_policy.clear();
        m_ownership.clear();
        m_gtp_send_cmd = "kata-raw-nn 0\n";
        m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
    } else if (m_gtp_send_cmd.StartsWith("kata-raw-nn 0\n")) {
        kataRawParse(kataRes, true);
        m_StateEngine->m_black_score = m_black_score;
        make_owner_policy(m_StateEngine,
                          m_ownership,
                          m_policy,
                          m_policy_pass,
                          m_black_winrate,
                          m_black_score);
        wxString mess;
        if (m_playerColor == FastBoard::BLACK) {
            mess.Printf(_("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d"),
                        m_black_winrate * 100.0f,
                        m_black_score, 0, 0);
        } else {
            mess.Printf(_("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d"),
                        100.0f - m_black_winrate * 100.0f,
                        -1.0f * m_black_score, 0, 0);
        }
        SetStatusBarText(mess, 1);
        m_panelBoard->unlockState();
        //signal board change
        wxCommandEvent myevent(wxEVT_BOARD_UPDATE, GetId());
        myevent.SetEventObject(this);
        ::wxPostEvent(m_panelBoard->GetEventHandler(), myevent);
        broadcastCurrentMove();
        postIdle();
        if (m_gtp_send_cmd.find("doRealUndo") != std::string::npos) {
            doRealUndo(m_undo_num, true);
        } else if (m_gtp_send_cmd.find("doRealForward") != std::string::npos) {
            doRealForward(m_forward_num, true);
        }
    }
}

void MainFrame::doAnalysisSGFWait(const wxString& kataRes) {
    // Receive analysis response from KataGo (Get current board status).
    int who = 1 - m_StateEngine->get_to_move();
    nlohmann::json res_json;
    try {
        res_json = nlohmann::json::parse(kataRes);
        if (res_json.contains("id") &&
            res_json["id"].get<std::string>() != m_send_json["id"].get<std::string>()) {
            return;
        } else if (kataRes.find(R"("error":)") != std::string::npos) {
            wxLogError(_("Error was returned from KataGo engine: %s"), kataRes.mb_str());
            postIdle();
            return;
        } else if (res_json.contains("noResults") && res_json["noResults"].get<bool>()) {
            wxLogError(_("Error was returned from KataGo engine: %s"), "noResults");
            postIdle();
            return;
        }
        m_winrate = 1.0f - res_json["rootInfo"]["winrate"].get<float>();
        m_scoreMean = -1.0f * res_json["rootInfo"]["scoreLead"].get<float>();
        make_owner_policy(m_StateEngine, res_json, m_winrate, m_scoreMean);
    } catch (const std::exception& e) {
        wxLogError(_("Exception at ANALYSIS_SGF_WAIT: %s %s"), typeid(e).name(), e.what());
        postIdle();
        return;
    }
    wxString mess;
    if (who == FastBoard::BLACK) {
        mess.Printf(_("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d"),
                    m_winrate * 100.0f, m_scoreMean, 0, 0);
    } else {
        mess.Printf(_("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d"),
                    100.0f - m_winrate * 100.0f, -1.0f * m_scoreMean, 0, 0);
    }
    SetStatusBarText(mess, 1);
    // signal update of visible board
    m_panelBoard->unlockState();
    wxCommandEvent myevent(wxEVT_BOARD_UPDATE, GetId());
    myevent.SetEventObject(this);
    ::wxPostEvent(m_panelBoard->GetEventHandler(), myevent);
    broadcastCurrentMove();
    postIdle();
}

void MainFrame::doAnalysisUndoWait(const wxString& kataRes) {
    // Receive analysis response from KataGo (Get current board status).
    int who = 1 - m_StateEngine->get_to_move();
    nlohmann::json res_json;
    try {
        res_json = nlohmann::json::parse(kataRes);
        if (res_json.contains("id") &&
            res_json["id"].get<std::string>() != m_send_json["id"].get<std::string>()) {
            return;
        } else if (kataRes.find(R"("error":)") != std::string::npos) {
            wxLogError(_("Error was returned from KataGo engine: %s"), kataRes.mb_str());
            postIdle();
            return;
        } else if (res_json.contains("noResults") && res_json["noResults"].get<bool>()) {
            wxLogError(_("Error was returned from KataGo engine: %s"), "noResults");
            postIdle();
            return;
        }
        m_winrate = 1.0f - res_json["rootInfo"]["winrate"].get<float>();
        m_scoreMean = -1.0f * res_json["rootInfo"]["scoreLead"].get<float>();
        make_owner_policy(m_StateEngine, res_json, m_winrate, m_scoreMean);
    } catch (const std::exception& e) {
        wxLogError(_("Exception at ANALYSIS_UNDO_WAIT: %s %s"), typeid(e).name(), e.what());
        postIdle();
        return;
    }
    wxString mess;
    if (who == FastBoard::BLACK) {
        mess.Printf(_("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d"),
                    m_winrate * 100.0f, m_scoreMean, 0, 0);
    } else {
        mess.Printf(_("Win rate:%3.1f%% Score:%.1f Time:%d[ms] Visits:%d"),
                    100.0f - m_winrate * 100.0f, -1.0f * m_scoreMean, 0, 0);
    }
    SetStatusBarText(mess, 1);
    // signal update of visible board
    m_panelBoard->unlockState();
    doPostMoveChange(m_post_move_change);
    postIdle();
}

void MainFrame::doKataRawWait(const wxString& kataRes) {
    if (kataRes == "= KataGo") {
        return;
    }
    if (kataRes.StartsWith("?")) {
        wxLogError(_("Error was returned from KataGo engine: %s"), kataRes.mb_str());
        SetStatusBarText(kataRes, 1);
        postIdle();
    } else if (m_gtp_send_cmd.StartsWith("kata-raw-nn 0\n")) {
        kataRawParse(kataRes, true);
        m_StateEngine->m_black_score = m_black_score;
        make_owner_policy(m_StateEngine,
                          m_ownership,
                          m_policy,
                          m_policy_pass,
                          m_black_winrate,
                          m_black_score,
                          true,
                          false);
        wxMenuItem * prob = m_menuTools->FindItem(ID_MOVE_PROBABILITIES);
        wxMenuItem * bm = m_menuTools->FindItem(ID_BEST_MOVES);
        prob->Enable(true);
        bm->Enable(true);
        m_panelBoard->setShowProbabilities(!m_panelBoard->getShowProbabilities());
        wxConfig::Get()->Write("showProbabilitiesEnabled", true);
        prob->Check(true);
        gameNoLongerCounts();
        if (m_panelBoard->getShowBestMoves()) {
            wxConfig::Get()->Write("showBestMovesEnabled", false);
            m_panelBoard->setShowBestMoves(false);
            bm->Check(false);
        }
        m_panelBoard->Refresh();
        m_post_show_prob = false;
        postIdle();
    }
}

void MainFrame::doGetPolicyWait(const wxString& kataRes) {
    nlohmann::json res_json;
    try {
        res_json = nlohmann::json::parse(kataRes);
        if (res_json.contains("id") &&
            res_json["id"].get<std::string>() != m_send_json["id"].get<std::string>()) {
            return;
        } else if (kataRes.find(R"("error":)") != std::string::npos) {
            wxLogError(_("Error was returned from KataGo engine: %s"), kataRes.mb_str());
            postIdle();
            return;
        } else if (res_json.contains("noResults") && res_json["noResults"].get<bool>()) {
            wxLogError(_("Error was returned from KataGo engine: %s"), "noResults");
            postIdle();
            return;
        }
        make_owner_policy(m_StateEngine, res_json, 0.0f, 0.0f, true, false);
    } catch (const std::exception& e) {
        wxLogError(_("Exception at ANALYSIS_SGF_WAIT: %s %s"), typeid(e).name(), e.what());
        postIdle();
        return;
    }
    wxMenuItem * prob = m_menuTools->FindItem(ID_MOVE_PROBABILITIES);
    wxMenuItem * bm = m_menuTools->FindItem(ID_BEST_MOVES);
    prob->Enable(true);
    bm->Enable(true);
    m_panelBoard->setShowProbabilities(!m_panelBoard->getShowProbabilities());
    wxConfig::Get()->Write("showProbabilitiesEnabled", true);
    prob->Check(true);
    gameNoLongerCounts();
    if (m_panelBoard->getShowBestMoves()) {
        wxConfig::Get()->Write("showBestMovesEnabled", false);
        m_panelBoard->setShowBestMoves(false);
        bm->Check(false);
    }
    m_panelBoard->Refresh();
    m_post_show_prob = false;
    postIdle();
}

void MainFrame::doRecieveKataGo(wxCommandEvent & event) {
    wxLogDebug(_("KataGo response arrived"));

    void* rawdataptr = event.GetClientData();
    if (!rawdataptr) {
        return;
    }
    std::unique_ptr<wxString> bundle(reinterpret_cast<wxString*>(rawdataptr));
    auto kataRes = *bundle;

    if (m_katagoStatus == KATAGO_STARTING) {
        if (kataRes.StartsWith("START OK")) {
            if (kataRes.find("board25:0") != std::string::npos) {
                cfg_board25 = false;
            }
            if (kataRes.find("m_japanese_rule:1") != std::string::npos) {
                m_japanese_rule_init = true;
                m_japanese_rule = true;
            }
            m_katagoStatus = KATAGO_IDLE;
            setStartMenus();
            wxCommandEvent evt;
            doNewRatedGame(evt);
        }
    } else if (m_katagoStatus == ANALYSIS_STARTING_WAIT) {
        doAnalysisStartingWait(kataRes);
    } else if (m_katagoStatus == ANALYSIS_QUERY_WAIT) {
        doAnalysisQuery(kataRes);
    } else if (m_katagoStatus == GAME_FIRST_QUERY_WAIT) {
        doGameFirstQuery(kataRes);
    } else if (m_katagoStatus == GAME_SECOND_QUERY_WAIT) {
        doGameSecondQuery(kataRes);
    } else if (m_katagoStatus == KATAGO_GAME_START) {
        doKataGameStart(kataRes);
    } else if (m_katagoStatus == KATAGO_GTP_ANALYSIS) {
        doKataGTPAnalysis(kataRes);
    } else if (m_katagoStatus == KATAGO_GTP_WAIT) {
        doKataGTPWait(kataRes);
    } else if (m_katagoStatus == KATAGO_GTP_ETC_WAIT) {
        doKataGTPEtcWait(kataRes);
    } else if (m_katagoStatus == ANALYSIS_SGF_WAIT) {
        doAnalysisSGFWait(kataRes);
    } else if (m_katagoStatus == ANALYSIS_UNDO_WAIT) {
        doAnalysisUndoWait(kataRes);
    } else if (m_katagoStatus == KATA_RAW_WAIT) {
        doKataRawWait(kataRes);
    } else if (m_katagoStatus == GET_POLICY_WAIT) {
        doGetPolicyWait(kataRes);
    }
}

void MainFrame::kataRawParse(const wxString& msg, const bool& win_score_read) {
    std::string::size_type pos1 = msg.find("whiteWin ");
    std::string::size_type pos2 = msg.find(" ", pos1 + sizeof("whiteWin ") - 1);
    wxString s = msg.substr(pos1 + sizeof("whiteWin ") - 1,
                            pos2 - pos1 - sizeof("whiteWin ") + 1);
    if (win_score_read) {
        m_black_winrate = 1.0f - wxAtof(s);
    } else {
        m_winrate = 1.0f - wxAtof(s);
    }
    pos1 = msg.find("whiteLead ");
    pos2 = msg.find(" ", pos1 + sizeof("whiteLead ") - 1);
    s = msg.substr(pos1 + sizeof("whiteLead ") - 1,
                   pos2 - pos1 - sizeof("whiteLead ") + 1);
    if (win_score_read) {
        m_black_score = -1.0f * wxAtof(s);
    } else {
        m_scoreMean = 1.0f - wxAtof(s);
    }
    pos1 = msg.find("policy ") + sizeof("policy ") - 1;
    int board_size = m_StateEngine->board.get_boardsize();
    int vtx_size = board_size * board_size;
    bool prev_sp = true;
    for (auto i = 0; i < vtx_size && pos1 < msg.length(); pos1++) {
        if (msg[pos1] == (const char)' ') {
            if (!prev_sp) {
                prev_sp = true;
                m_policy.emplace_back(wxAtof(s));
                s.Empty();
                i++;
            }
        } else {
            if (prev_sp) {
                prev_sp = false;
                s.Empty();
            }
            s += msg[pos1];
        }
    }
    if (!s.IsEmpty()) {
        m_policy.emplace_back(wxAtof(s));
    }
    pos1 = msg.find("policyPass ");
    pos2 = msg.find(" ", pos1 + 11);
    s = msg.substr(pos1 + sizeof("policyPass ") - 1,
                   pos2 - pos1 - sizeof("policyPass ") + 1);
    m_policy_pass = wxAtof(s);
    pos1 = msg.find("whiteOwnership ") + sizeof("whiteOwnership ") - 1;
    prev_sp = true;
    for (auto i = 0; i < vtx_size && pos1 < msg.length(); pos1++) {
        if (msg[pos1] == (const char)' ') {
            if (!prev_sp) {
                prev_sp = true;
                m_ownership.emplace_back(wxAtof(s));
                s.Empty();
                i++;
            }
        } else {
            if (prev_sp) {
                prev_sp = false;
                s.Empty();
            }
            s += msg[pos1];
        }
    }
    if (!s.IsEmpty()) {
        m_ownership.emplace_back(wxAtof(s));
    }
}

void MainFrame::make_owner_policy(std::unique_ptr<GameState>& state,
                                  nlohmann::json& res_json,
                                  float winrate,
                                  float score,
                                  bool make_policy,
                                  bool make_ownership) {
    int board_size = state->board.get_boardsize();
    // Edit Ownership Information
    if (make_ownership) {
        std::vector<float> conv_owner((board_size + 2) * (board_size + 2), 0.0f);
        for (int vertex = 0; vertex < board_size * board_size; vertex++) {
            int x = vertex % board_size;
            int y = vertex / board_size;
            y = -1 * (y - board_size) - 1;
            int pos_owner = state->board.get_vertex(x, y);
            float owner = res_json["ownership"][vertex].get<float>();
            conv_owner[pos_owner] = (owner / 2.0f) + 0.5f;
        }
        state->m_owner.clear();
        for (auto itr = conv_owner.begin(); itr != conv_owner.end(); ++itr) {
            state->m_owner.emplace_back(*itr);
        }
        std::bitset<FastBoard::MAXSQ> blackowns;
        for (int i = 0; i < board_size; i++) {
            for (int j = 0; j < board_size; j++) {
                int vtx = state->board.get_vertex(i, j);
                if (state->m_owner[vtx] >= 0.5f) {
                    blackowns[vtx] = true;
                }
            }
        }
        MCOwnerTable::get_MCO()->update_owns(blackowns,
                                             (1.0f - winrate >= 0.5f),
                                             -1.0f * score);
    }
    // Edit Policy Information
    if (make_policy) {
        std::vector<float> conv_policy((board_size + 2) * (board_size + 2), 0.0f);
        float maxProbability = 0.0f;
        for (int vertex = 0; vertex < board_size * board_size; vertex++) {
            int x = vertex % board_size;
            int y = vertex / board_size;
            y = -1 * (y - board_size) - 1;
            int pos_policy = state->board.get_vertex(x, y);
            float policy = res_json["policy"][vertex].get<float>();
            conv_policy[pos_policy] = policy;
            if (policy > maxProbability) {
                maxProbability = policy;
            }
        }
        float policy = res_json["policy"][board_size * board_size].get<float>();
        if (policy > maxProbability) {
            maxProbability = policy;
        }
        conv_policy[0] = maxProbability;
        conv_policy[1] = policy;
        state->m_policy.clear();
        for (auto itr = conv_policy.begin(); itr != conv_policy.end(); ++itr) {
            state->m_policy.emplace_back(*itr);
        }
    }
}

void MainFrame::make_owner_policy(std::unique_ptr<GameState>& state,
                                  std::vector<float> ownership,
                                  std::vector<float> policy,
                                  float policy_pass,
                                  float winrate,
                                  float score,
                                  bool make_policy,
                                  bool make_ownership) {
    int board_size = state->board.get_boardsize();
    // Edit Ownership Information
    if (make_ownership) {
        std::vector<float> conv_owner((board_size + 2) * (board_size + 2), 0.0f);
        for (int vertex = 0; vertex < board_size * board_size; vertex++) {
            int x = vertex % board_size;
            int y = vertex / board_size;
            y = -1 * (y - board_size) - 1;
            int pos_owner = state->board.get_vertex(x, y);
            conv_owner[pos_owner] = -1 * (ownership[vertex] / 2) + 0.5f;
        }
        state->m_owner.clear();
        for (auto itr = conv_owner.begin(); itr != conv_owner.end(); ++itr) {
            state->m_owner.emplace_back(*itr);
        }
        std::bitset<FastBoard::MAXSQ> blackowns;
        for (int i = 0; i < board_size; i++) {
            for (int j = 0; j < board_size; j++) {
                int vtx = state->board.get_vertex(i, j);
                if (state->m_owner[vtx] >= 0.5f) {
                    blackowns[vtx] = true;
                }
            }
        }
        MCOwnerTable::get_MCO()->update_owns(blackowns, winrate >= 0.5f, score);
    }
    // Edit Policy Information
    if (make_policy) {
        std::vector<float> conv_policy((board_size + 2) * (board_size + 2), 0.0f);
        float maxProbability = 0.0f;
        for (int vertex = 0; vertex < board_size * board_size; vertex++) {
            int x = vertex % board_size;
            int y = vertex / board_size;
            y = -1 * (y - board_size) - 1;
            int pos_policy = state->board.get_vertex(x, y);
            float policy_ent = policy[vertex];
            conv_policy[pos_policy] = policy_ent;
            if (policy_ent > maxProbability) {
                maxProbability = policy_ent;
            }
        }
        if (policy_pass > maxProbability) {
            maxProbability = policy_pass;
        }
        conv_policy[0] = maxProbability;
        conv_policy[1] = policy_pass;
        state->m_policy.clear();
        for (auto itr = conv_policy.begin(); itr != conv_policy.end(); ++itr) {
            state->m_policy.emplace_back(*itr);
        }
    }
}

void MainFrame::playMove(int who) {
    int board_size = m_StateEngine->board.get_boardsize();
    // KataGo's Resignation Decision
    float initialBlackAdvantageInPoints;
    if (m_move_handi.size() <= 1) {
        initialBlackAdvantageInPoints = 7.0f - m_StateEngine->get_komi();
    } else {
        initialBlackAdvantageInPoints = 14.0f * (m_move_handi.size() - 1) +
                                        (7.0f - m_StateEngine->get_komi()
                                              - m_move_handi.size());
    }
    int minTurnForResignation = 0;
    int noResignationWhenWhiteScoreAbove = board_size * board_size;
    if (initialBlackAdvantageInPoints > 0.9f && who == FastBoard::WHITE) {
        minTurnForResignation = 1 + noResignationWhenWhiteScoreAbove / 5;
        float numTurnsToCatchUp = 0.60f * noResignationWhenWhiteScoreAbove
                                - (float)minTurnForResignation;
        float numTurnsSpent = (float)(m_StateEngine->get_movenum() + 1)
                            - (float)minTurnForResignation;
        if (numTurnsToCatchUp <= 1.0f) {
            numTurnsToCatchUp = 1.0f;
        }
        if (numTurnsSpent <= 0.0f) {
            numTurnsSpent = 0.0f;
        }
        if (numTurnsSpent > numTurnsToCatchUp) {
            numTurnsSpent = numTurnsToCatchUp;
        }
        float resignScore = -1.0f * initialBlackAdvantageInPoints *
                                    ((numTurnsToCatchUp - numTurnsSpent) / numTurnsToCatchUp);
        resignScore -= 5.0f;
        resignScore -= initialBlackAdvantageInPoints * 0.15f;
        noResignationWhenWhiteScoreAbove = resignScore;
    }
    bool resign = true;
    if (m_StateEngine->get_movenum() + 1 < minTurnForResignation) {
        resign = false;
    } else if (who == FastBoard::WHITE &&
               (-1.0f * m_scoreMean) > noResignationWhenWhiteScoreAbove) {
        resign = false;
    } else if ((who == FastBoard::WHITE &&
                (-1.0f * m_scoreMean) > -1.0f * RESIGN_MINSCORE_DIFFERENCE) ||
               (who == FastBoard::BLACK &&
                m_scoreMean > (-1.0f * RESIGN_MINSCORE_DIFFERENCE))) {
        resign = false;
    }
    for (size_t i = 0; i < m_StateEngine->m_win_rate.size() - 1; i++) {
        m_StateEngine->m_win_rate[m_StateEngine->m_win_rate.size() - 1 - i]
            = m_StateEngine->m_win_rate[m_StateEngine->m_win_rate.size() - 2 - i];
        if (m_StateEngine->m_win_rate[m_StateEngine->m_win_rate.size() - 1 - i]
                >= RESIGN_THRESHOLD) {
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
}

void MainFrame::postIdle() {
    wxCommandEvent evt;
    if (m_StateEngine) {
        m_State = *m_StateEngine;
        m_StateEngine.reset();
    }
    m_katagoStatus = KATAGO_IDLE;
    m_timerIdleWakeUp.Stop();
    if (m_post_doExit) {
        doExit(evt);
        return;
    }
    if (m_post_doNewMove) {
        doNewMove(evt);
        if (m_katagoStatus != KATAGO_IDLE) {
            return;
        }
    }
    if (m_post_doSettingsDialog) {
        doSettingsDialog(evt);
        if (m_katagoStatus != KATAGO_IDLE) {
            return;
        }
    }
    if (m_post_doNewGame) {
        doNewGame(evt);
        if (m_katagoStatus != KATAGO_IDLE) {
            return;
        }
    }
    if (m_post_doNewRatedGame) {
        doNewRatedGame(evt);
        if (m_katagoStatus != KATAGO_IDLE) {
            return;
        }
    }
    if (m_post_doScore) {
        doScore(evt);
        if (m_katagoStatus != KATAGO_IDLE) {
            return;
        }
    }
    if (m_post_doPass) {
        doPass(evt);
        if (m_katagoStatus != KATAGO_IDLE) {
            return;
        }
    }
    if (m_post_doRealUndo) {
        doRealUndo(m_post_doRealUndo);
        if (m_katagoStatus != KATAGO_IDLE) {
            return;
        }
    }
    if (m_post_doRealForward) {
        doRealForward(m_post_doRealForward);
        if (m_katagoStatus != KATAGO_IDLE) {
            return;
        }
    }
    if (m_post_doOpenSGF) {
        doOpenSGF(evt);
        if (m_katagoStatus != KATAGO_IDLE) {
            return;
        }
    }
    if (m_post_doSaveSGF) {
        doSaveSGF(evt);
        if (m_katagoStatus != KATAGO_IDLE) {
            return;
        }
    }
    if (m_post_doForceMove) {
        doForceMove(evt);
        if (m_katagoStatus != KATAGO_IDLE) {
            return;
        }
    }
    if (m_post_doResign) {
        doResign(evt);
        if (m_katagoStatus != KATAGO_IDLE) {
            return;
        }
    }
    if (m_post_doAnalyze) {
        doAnalyze(evt);
        if (m_katagoStatus != KATAGO_IDLE) {
            return;
        }
    }
    if (m_post_show_prob) {
        if (cfg_engine_type == GTP::ANALYSIS) {
            getAnalysisPolicyAndOwner(true, false);
            m_katagoStatus = GET_POLICY_WAIT;
        } else {
            m_StateEngine = std::make_unique<GameState>(m_State);
            m_policy.clear();
            m_gtp_send_cmd = "kata-raw-nn 0\n";
            m_out->Write(m_gtp_send_cmd, strlen(m_gtp_send_cmd));
            m_katagoStatus = KATA_RAW_WAIT;
        }
    }
}
#endif

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
    if (!enable) {
        return;
    }
    setActiveMenus();
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
    if (cfg_use_engine == GTP::KATAGO_ENGINE) {
        cfg_num_threads = std::max(1, cfg_num_threads / 2);
    }
    thread_pool.initialize(cfg_num_threads);
}
