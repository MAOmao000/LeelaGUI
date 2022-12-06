#ifndef MAINFRAME_H
#define MAINFRAME_H

#include <memory>
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

wxDECLARE_EVENT(wxEVT_NEW_MOVE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_BOARD_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_STATUS_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_ANALYSIS_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_BESTMOVES_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_EVALUATION_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_SET_MOVENUM, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_PURGE_VIZ, wxCommandEvent);
#ifndef USE_THREAD
wxDECLARE_EVENT(wxEVT_RECIEVE_KATAGO, wxCommandEvent);
#endif

#ifndef USE_THREAD
class SubProcess;
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
		void OnIdleTimer(wxTimerEvent& event);
		void OnIdle(wxIdleEvent& event);
#endif

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
#ifndef USE_THREAD
	virtual void doRecieveKataGo(wxCommandEvent& event);
#endif
	void doEvalUpdate(wxCommandEvent& event);
	void doRealUndo(int count = 1);
	void doRealForward(int count = 1);
	void doPostMoveChange(bool wasAnalyzing);
	void gotoMoveNum(wxCommandEvent& event);
	void broadcastCurrentMove();

	void startEngine();
#ifndef USE_THREAD
	void startKataGo();
#endif
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
#ifdef USE_THREAD
	std::string GTPSend(const wxString& s, const int& sleep_ms = 50);
#else
	void postIdle();
#endif

	static constexpr int NO_WINDOW_AUTOSIZE = 1;

#ifndef USE_THREAD
	std::unique_ptr<GameState> m_StateEngine;
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
#ifndef USE_THREAD
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
#ifdef PERFORMANCE
	std::chrono::time_point<std::chrono::system_clock> m_query_start;
#endif
#endif
	std::vector<int> m_move_handi;
	std::unique_ptr<TEngineThread> m_engineThread;
	AnalysisWindow* m_analysisWindow{nullptr};
	ScoreHistogram* m_scoreHistogramWindow{nullptr};
	std::vector<std::string> m_overrideSettings;
	friend class TEngineThread;
	friend class TBoardPanel;

#ifdef USE_THREAD
	wxProcess* m_process{nullptr};
	std::mutex m_GTPmutex;
#else
	SubProcess* m_process{nullptr};
	wxTimer m_timerIdleWakeUp;
#endif
	wxOutputStream* m_out{nullptr};
	wxInputStream* m_in{nullptr};
	wxInputStream* m_err{nullptr};
	bool m_close_window;
	bool m_japanese_rule;
	std::vector<wxString> m_ini_line;
	std::string m_query_id;

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
