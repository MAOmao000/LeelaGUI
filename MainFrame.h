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

#define Exec_TimerIdle 2000
#define RESIGN_THRESHOLD 0.05  // Resignation after 3 consecutive win rates of 5% or less
#define RESIGN_MINSCORE_DIFFERENCE -1e10

enum {
    INIT,
    KATAGO_STRATING,
    ANALYSIS_RESPONSE_WAIT,
    KATAGO_IDLE,
    KATAGO_STOPED,
    ANALYSIS_QUERY_WAIT,
    ANALYSIS_TERMINATE_WAIT,
    GAME_FIRST_QUERY_WAIT,
    GAME_TERMINATE_QUERY_WAIT,
    GAME_SECOND_QUERY_WAIT,
};

enum {
    NON,
    POLICY,
    POLICYPASS,
    WHITEOWNERSHIP,
    LAST,
};

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
wxDECLARE_EVENT(wxEVT_RECIEVE_KATAGO, wxCommandEvent);

static wxLocale m_locale;

class SubProcess;

class MainFrame : public TMainFrame {
	public:
		MainFrame(wxFrame *frame, const wxString& title);
		~MainFrame();
		void SetStatusBarText(wxString mess, int pos);
		void loadSGF(const wxString & filename, int movenum = 999);
		virtual void doNewRatedGame(wxCommandEvent& event);
		void OnAsyncTermination(SubProcess *process);
		void OnProcessTerminated(SubProcess *process);
		void OnIdleTimer(wxTimerEvent& event);
		void OnIdle(wxIdleEvent& event);
		void doInit();

	private:
	virtual void doActivate(wxActivateEvent& event);
	virtual void doPaint(wxPaintEvent& event);
	virtual void doNewMove(wxCommandEvent& event);
	virtual void doBoardUpdate(wxCommandEvent& event);
	virtual void doExit(wxCommandEvent& event);
	virtual void doResize(wxSizeEvent& event);
	virtual void doBoardResize(wxSizeEvent& event);
	virtual void doNewGame(wxCommandEvent& event);
	virtual void doScore(wxCommandEvent& event);
	virtual void doPass(wxCommandEvent& event);
	virtual void doSetRatedSize(wxCommandEvent& event);
	virtual void doGoRules(wxCommandEvent& event);
	virtual void doHelpAbout(wxCommandEvent& event);
	virtual void doHomePage(wxCommandEvent& event);
	virtual void doOpenSGF(wxCommandEvent& event);
	virtual void doSaveSGF(wxCommandEvent& event);
	virtual void doBack10(wxCommandEvent& event);
	virtual void doForward10(wxCommandEvent& event);
	virtual void doForceMove(wxCommandEvent& event);
	virtual void doToggleTerritory(wxCommandEvent& event);
	virtual void doToggleMoyo(wxCommandEvent& event);
	virtual void doStatusUpdate(wxCommandEvent& event);
	virtual void doResign(wxCommandEvent& event);
	virtual void doAnalyze(wxCommandEvent& event) override;
	virtual void doAdjustClocks(wxCommandEvent& event);
	virtual void doKeyDown( wxKeyEvent& event ) override;
	virtual void doUndo(wxCommandEvent& event) { doRealUndo(); };
	virtual void doForward(wxCommandEvent& event) { doRealForward(); };
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
	virtual void doRecieveKataGo(wxCommandEvent& event);
	void doEvalUpdate(wxCommandEvent& event);
	void doRealUndo(int count = 1);
	void doRealForward(int count = 1);
	void doPostMoveChange(bool wasAnalyzing);
	void gotoMoveNum(wxCommandEvent& event);
	void broadcastCurrentMove();

	void startEngine();
	void startKataGo();
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

	void postIdle();

	static constexpr int NO_WINDOW_AUTOSIZE = 1;

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
	nlohmann::json m_send_json;
	int  m_katagoStatus;
	bool m_runflag;
	bool m_isDuringSearch;
	bool m_terminate_res;
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

	std::vector<int> m_move_handi;
	std::unique_ptr<TEngineThread> m_engineThread;
	AnalysisWindow* m_analysisWindow{nullptr};
	ScoreHistogram* m_scoreHistogramWindow{nullptr};
	std::vector<std::string> m_overrideSettings;
	std::string m_move_str;
	float m_winrate;
	float m_scoreMean;
	std::string m_query_id;

	friend class TEngineThread;
	friend class TBoardPanel;

	wxOutputStream* m_out{nullptr};
	wxInputStream* m_in{nullptr};
	wxInputStream* m_err{nullptr};
	bool m_close_window;
	bool m_japanese_rule;
	SubProcess *m_process;
	wxTimer m_timerIdleWakeUp;
	std::vector<wxString> m_ini_line;

public:
	static void setLocale(bool japanese) {
		if (japanese) {
			if (!wxLocale::IsAvailable(wxLANGUAGE_JAPANESE)) {
				wxConfig::Get()->Write(wxT("japaneseEnabled"), false);
				return;
			}
			m_locale.Init(wxLANGUAGE_JAPANESE, wxLOCALE_DONT_LOAD_DEFAULT);
			m_locale.AddCatalogLookupPathPrefix(_T("catalogs"));
			m_locale.AddCatalog(_T("messages"));
			m_locale.AddCatalog(_T("wxstd"));
		}
	}
};

class SubProcess : public wxProcess {
public:
	SubProcess(MainFrame *parent);
	virtual void OnTerminate(int pid, int status) wxOVERRIDE;
	bool HasInput();

protected:
	MainFrame *m_parent;
};
#endif
