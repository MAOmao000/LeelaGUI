#ifndef MAINFRAME_H
#define MAINFRAME_H

#include <memory>
#include <chrono>
#include <thread>
#include <fstream>
#include <regex>
#include <wx/process.h>

#include "stdafx.h"
#include "GUI.h"
#include "FastBoard.h"
#include "GameState.h"
#include "EngineThread.h"
#include "GTP.h"
#include "json.hpp"

class AnalysisWindow;
class ScoreHistogram;
#ifndef USE_THREAD
class SubProcess;
#endif

wxDECLARE_EVENT(wxEVT_NEW_MOVE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_BOARD_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_STATUS_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_ANALYSIS_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_BESTMOVES_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_EVALUATION_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_SET_MOVENUM, wxCommandEvent);
#ifndef USE_THREAD
wxDECLARE_EVENT(wxEVT_RECIEVE_KATAGO, wxCommandEvent);
#endif

static wxLocale m_locale;

class MainFrame : public TMainFrame {
	public:
		MainFrame(wxFrame *frame, const wxString& title);
		~MainFrame();
		void SetStatusBarText(wxString mess, int pos);
		void loadSGF(const wxString & filename, int movenum = 999);
		virtual void doNewRatedGame(wxCommandEvent& event) override;
		void doInit();
#ifndef USE_THREAD
		void OnAsyncTermination(SubProcess *process);
		void OnProcessTerminated(SubProcess *process);
#endif
	static constexpr char DEFAULT_MAX_VISITS_ANALYSIS[] = "1000000";
	static constexpr char DEFAULT_MAX_TIME_ANALYSIS[] = "3600";

	private:
	virtual void doActivate(wxActivateEvent& event) override;
	virtual void doPaint(wxPaintEvent& event) override;
	virtual void doNewMove(wxCommandEvent& event);
	virtual void doBoardUpdate(wxCommandEvent& event);
	virtual void doExit(wxCommandEvent& event) override;
	virtual void doResize(wxSizeEvent& event) override;
	virtual void doBoardResize(wxSizeEvent& event);
	virtual void doNewGame(wxCommandEvent& event) override;
	virtual void doScore(wxCommandEvent& event) override;
	virtual void doPass(wxCommandEvent& event) override;
	virtual void doSetRatedSize(wxCommandEvent& event) override;
	virtual void doGoRules(wxCommandEvent& event) override;
	virtual void doHelpAbout(wxCommandEvent& event) override;
	virtual void doHomePage(wxCommandEvent& event) override;
	virtual void doOpenSGF(wxCommandEvent& event) override;
	virtual void doSaveSGF(wxCommandEvent& event) override;
	virtual void doBack10(wxCommandEvent& event) override;
	virtual void doForward10(wxCommandEvent& event) override;
	virtual void doForceMove(wxCommandEvent& event) override;
	virtual void doToggleTerritory(wxCommandEvent& event) override;
	virtual void doToggleMoyo(wxCommandEvent& event) override;
	virtual void doStatusUpdate(wxCommandEvent& event);
	virtual void doResign(wxCommandEvent& event) override;
	virtual void doAnalyze(wxCommandEvent& event) override;
	virtual void doAdjustClocks(wxCommandEvent& event) override;
	virtual void doKeyDown( wxKeyEvent& event ) override;
	virtual void doUndo(wxCommandEvent& event) override { doRealUndo(); };
	virtual void doForward(wxCommandEvent& event) override { doRealForward(); };
	virtual void doShowHideAnalysisWindow( wxCommandEvent& event ) override;
	virtual void doCloseChild( wxWindowDestroyEvent& event );
	virtual void doToggleProbabilities( wxCommandEvent& event ) override;
	virtual void doToggleBestMoves( wxCommandEvent& event ) override;
	virtual void doSettingsDialog( wxCommandEvent& event ) override;
	virtual void doMainLine( wxCommandEvent& event ) override;
	virtual void doPushPosition( wxCommandEvent& event ) override;
	virtual void doPopPosition( wxCommandEvent& event ) override;
	virtual void doShowHideScoreHistogram( wxCommandEvent& event ) override;
	virtual void doCopyClipboard( wxCommandEvent& event ) override;
	virtual void doPasteClipboard( wxCommandEvent& event ) override;
	void doEvalUpdate(wxCommandEvent& event);
	void doRealUndo(int count = 1, bool force = false);
	void doRealForward(int count = 1, bool force = false);
	void doPostMoveChange(bool wasAnalyzing);
	void gotoMoveNum(wxCommandEvent& event);
	void broadcastCurrentMove();
	void startEngine();
	bool stopEngine(bool update_score = true);
	// true = user accepts score
	bool scoreDialog(float komi, float handicap, float score, float prekomi, bool dispute = false);
	bool scoreGame(float & komi, float & handicap, float & score, float & prescore);
	void ratedGameEnd(bool won);
	wxString rankToString(int rank);
	void updateStatusBar(char *str);
	void setActiveMenus();
	void gameNoLongerCounts();
	void loadSGFString(const wxString& SGF, int movenum = 999);
	void setStartMenus(bool enable = true);
	void MainFrameEnd();
#ifdef USE_THREAD
	std::string GTPSend(const wxString& s, const int& sleep_ms = 50);
#else
	virtual void doRecieveKataGo(wxCommandEvent& event);
	void OnIdleTimer(wxTimerEvent& event);
	void OnIdle(wxIdleEvent& event);
	void startKataGo();
	void playMove(int who);
	void postIdle();
	void doKataGoStart(const wxString& kataRes);
	void doAnalysisResponse(const wxString& kataRes);
	void doAnalysisQuery(const wxString& kataRes);
	void doGameFirstQuery(const wxString& kataRes);
	void doGameSecondQuery(const wxString& kataRes);
	void doKataGameStartingWait(const wxString& kataRes);
	void doKataGameStart(const wxString& kataRes);
	void doKataGTPAnalysis(const wxString& kataRes);
	void doKataGTPWait(const wxString& kataRes);
	void doKataGTPEtcWait(const wxString& kataRes);
#endif

	static constexpr int NO_WINDOW_AUTOSIZE = 1;
	static constexpr long MAX_RANK = 13L;
	static constexpr long MIN_RANK = -30L;
	static constexpr char DEFAULT_ANALYSIS_PV_LEN[] = "15";
	static constexpr char DEFAULT_REPORT_DURING_SEARCH[] = "1.0";
	static constexpr char GTP_ANALYZE_INTERVAL[] = "200";
	static constexpr char GTP_ANALYZE_MAX_MOVES[] = "50";

#ifndef USE_THREAD
	static constexpr int WAKE_UP_TIMER_MS = 100;
	enum {
		INIT,
		KATAGO_STARTING,
		 KATAGO_STARTING_WAIT,
		ANALYSIS_RESPONSE_WAIT,
		KATAGO_IDLE,
		KATAGO_STOPED,
		ANALYSIS_QUERY_WAIT,
		GAME_FIRST_QUERY_WAIT,
		GAME_SECOND_QUERY_WAIT,
		KATAGO_GAME_START,
		KATAGO_GTP_ANALYSIS,
		KATAGO_GTP_WAIT,
		KATAGO_GTP_ETC_WAIT,
	};
#endif
	GameState m_State;
	std::vector<GameState> m_StateStack;
	int m_playerColor;
	int m_visitLimit;
	int m_ratedSize;
	bool m_netsEnabled;
	bool m_soundEnabled;
	bool m_resignEnabled;
	bool m_ponderEnabled;
	bool m_passEnabled;
	bool m_japaneseEnabled;
	bool m_ratedGame;
	bool m_analyzing;
	bool m_pondering;
	bool m_disputing;
	bool m_ponderedOnce;
	std::unique_ptr<TEngineThread> m_engineThread;
	AnalysisWindow* m_analysisWindow{nullptr};
	ScoreHistogram* m_scoreHistogramWindow{nullptr};
	friend class TEngineThread;
	friend class TBoardPanel;
	std::vector<int> m_move_handi;
	std::vector<std::string> m_overrideSettings;
	std::chrono::time_point<std::chrono::system_clock> m_query_start;
	int m_visit;
	wxOutputStream* m_out{nullptr};
	wxInputStream* m_in{nullptr};
	wxInputStream* m_err{nullptr};
	bool m_japanese_rule;
	bool m_japanese_rule_init;
	std::vector<wxString> m_ini_line;
	std::string m_query_id;
	int m_ini_line_idx;
	wxString m_gtp_send_cmd;
	wxString m_info_move;
	int m_undo_num;
	int m_undo_count;
	int m_forward_num;
	int m_forward_count;
	bool m_pass_send;
	bool m_resign_send;
	bool m_black_win;
	float m_black_winrate;
	float m_black_score;
	int m_visit_count;
	bool m_pick_policy;
	bool m_picked_pass;
	bool m_pick_ownership;
	float m_policy_pass;
	int m_policy_row;
	int m_ownership_row;
	std::vector<float> m_policy;
	std::vector<float> m_ownership;
	int m_thinking_time;
	bool m_thinking;
	int m_think_num;
	int m_visits;
#ifdef USE_THREAD
	wxProcess* m_process{nullptr};
	std::mutex m_GTPmutex;
#else
	std::unique_ptr<GameState> m_StateEngine;
	nlohmann::json m_send_json;
	int  m_katagoStatus;
	bool m_runflag;
	bool m_wasRunning;
	bool m_isDuringSearch;
	bool m_update_score;
	bool m_post_destructor;
	bool m_post_doExit;
	bool m_post_doNewMove;
	bool m_post_doSettingsDialog;
	bool m_post_doNewGame;
	bool m_post_doNewRatedGame;
	bool m_post_doScore;
	bool m_post_doPass;
	int m_post_doRealUndo;
	int m_post_doRealForward;
	bool m_post_doOpenSGF;
	bool m_post_doSaveSGF;
	bool m_post_doForceMove;
	bool m_post_doResign;
	bool m_post_doAnalyze;
	std::string m_move_str;
	float m_winrate;
	float m_scoreMean;
	SubProcess* m_process{nullptr};
	wxTimer m_timerIdleWakeUp;
#endif

	public:
	static void setLocale(bool japanese) {
		if (japanese) {
			if (!wxLocale::IsAvailable(wxLANGUAGE_JAPANESE)) {
				wxConfig::Get()->Write(wxT("japaneseEnabled"), false);
				return;
			}
			m_locale.Init(wxLANGUAGE_JAPANESE, wxLOCALE_DONT_LOAD_DEFAULT);
			m_locale.AddCatalogLookupPathPrefix(wxT("catalogs"));
			m_locale.AddCatalog(wxT("messages"));
			m_locale.AddCatalog(wxT("wxstd"));
		}
	}
};

#ifndef USE_THREAD
class SubProcess : public wxProcess {
public:
	SubProcess(MainFrame *parent);
	virtual void OnTerminate(int pid, int status) wxOVERRIDE;
	bool HasInput();
protected:
	MainFrame *m_parent;
};
#endif
#endif
