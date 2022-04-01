#ifndef MAINFRAME_H
#define MAINFRAME_H

#include <memory>
#include "stdafx.h"
#include "GUI.h"
#include "FastBoard.h"
#include "GameState.h"
#include "EngineThread.h"

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

class MainFrame : public TMainFrame {
	public:
		MainFrame(wxFrame *frame, const wxString& title);
		~MainFrame();
		void SetStatusBarText(wxString mess, int pos);
		void loadSGF(const wxString & filename, int movenum = 999);
		virtual void doNewRatedGame(wxCommandEvent& event);

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
	void doEvalUpdate(wxCommandEvent& event);
	void doRealUndo(int count = 1);
	void doRealForward(int count = 1);
	void doPostMoveChange(bool wasAnalyzing);
	void gotoMoveNum(wxCommandEvent& event);
	void broadcastCurrentMove();

	void startEngine();
	bool stopEngine(bool update_score = true);
	// true = user accepts score
	bool scoreDialog(float komi, float handicap, float score, float prekomi);
	bool scoreGame(float & komi, float & handicap, float & score, float & prescore);
	void ratedGameEnd(bool won);
	wxString rankToString(int rank);
	void updateStatusBar(char *str);
	void setActiveMenus();
	void gameNoLongerCounts();
	void loadSGFString(const wxString & SGF, int movenum = 999);

	static constexpr int NO_WINDOW_AUTOSIZE = 1;

	GameState m_State;
	std::vector<GameState> m_StateStack;
	int m_playerColor;
	int m_visitLimit;
	int m_ratedSize;
	int m_lang;
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
};

#endif