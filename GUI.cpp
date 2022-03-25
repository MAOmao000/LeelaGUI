///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "TBoardPanel.h"
#include "TScorePanel.h"

#include "GUI.h"

#include "res/analyze.png.h"
#include "res/back.png.h"
#include "res/delete.png.h"
#include "res/execute.png.h"
#include "res/fileopen.png.h"
#include "res/filesave.png.h"
#include "res/find.png.h"
#include "res/forward.png.h"
#include "res/goparent3.png.h"
#include "res/minus.png.h"
#include "res/new.png.h"
#include "res/newrated.png.h"
#include "res/plus.png.h"
#include "res/quit.png.h"
#include "res/redo.png.h"
#include "res/resign.png.h"
#include "res/undo.png.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( TMainFrame, wxFrame )
	EVT_ACTIVATE( TMainFrame::_wxFB_doActivate )
	EVT_CLOSE( TMainFrame::_wxFB_doClose )
	EVT_KEY_DOWN( TMainFrame::_wxFB_doKeyDown )
	EVT_PAINT( TMainFrame::_wxFB_doPaint )
	EVT_SIZE( TMainFrame::_wxFB_doResize )
	EVT_MENU( ID_NEWGAME, TMainFrame::_wxFB_doNewGame )
	EVT_MENU( ID_NEWRATED, TMainFrame::_wxFB_doNewRatedGame )
	EVT_MENU( ID_RATEDSIZE, TMainFrame::_wxFB_doSetRatedSize )
	EVT_MENU( ID_OPEN, TMainFrame::_wxFB_doOpenSGF )
	EVT_MENU( ID_SAVE, TMainFrame::_wxFB_doSaveSGF )
	EVT_MENU( ID_COPYCLIPBOARD, TMainFrame::_wxFB_doCopyClipboard )
	EVT_MENU( ID_PASTECLIPBOARD, TMainFrame::_wxFB_doPasteClipboard )
	EVT_MENU( wxID_EXIT, TMainFrame::_wxFB_doExit )
	EVT_MENU( ID_UNDO, TMainFrame::_wxFB_doUndo )
	EVT_MENU( ID_REDO, TMainFrame::_wxFB_doForward )
	EVT_MENU( ID_BACK10, TMainFrame::_wxFB_doBack10 )
	EVT_MENU( ID_FWD10, TMainFrame::_wxFB_doForward10 )
	EVT_MENU( ID_FORCE, TMainFrame::_wxFB_doForceMove )
	EVT_MENU( ID_PASS, TMainFrame::_wxFB_doPass )
	EVT_MENU( ID_RESIGN, TMainFrame::_wxFB_doResign )
	EVT_MENU( ID_SCORE, TMainFrame::_wxFB_doScore )
	EVT_MENU( ID_ANALYZE, TMainFrame::_wxFB_doAnalyze )
	EVT_MENU( ID_PUSHPOS, TMainFrame::_wxFB_doPushPosition )
	EVT_MENU( ID_POPPOS, TMainFrame::_wxFB_doPopPosition )
	EVT_MENU( ID_MAINLINE, TMainFrame::_wxFB_doMainLine )
	EVT_MENU( ID_ANALYSISWINDOWTOGGLE, TMainFrame::_wxFB_doShowHideAnalysisWindow )
	EVT_MENU( ID_SCOREHISTOGRAMTOGGLE, TMainFrame::_wxFB_doShowHideScoreHistogram )
	EVT_MENU( ID_MOVE_PROBABILITIES, TMainFrame::_wxFB_doToggleProbabilities )
	EVT_MENU( ID_BEST_MOVES, TMainFrame::_wxFB_doToggleBestMoves )
	EVT_MENU( ID_SHOWTERRITORY, TMainFrame::_wxFB_doToggleTerritory )
	EVT_MENU( ID_SHOWMOYO, TMainFrame::_wxFB_doToggleMoyo )
	EVT_MENU( wxID_PREFERENCES, TMainFrame::_wxFB_doSettingsDialog )
	EVT_MENU( ID_ADJUSTCLOCKS, TMainFrame::_wxFB_doAdjustClocks )
	EVT_MENU( ID_HELPRULES, TMainFrame::_wxFB_doGoRules )
	EVT_MENU( ID_HOMEPAGE, TMainFrame::_wxFB_doHomePage )
	EVT_MENU( wxID_ABOUT, TMainFrame::_wxFB_doHelpAbout )
END_EVENT_TABLE()

TMainFrame::TMainFrame( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxFrame( parent, id, title, pos, size, style, name )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );
	
	m_panelBoard = new TBoardPanel( this, ID_BOARDPANEL, wxDefaultPosition, wxSize( -1,-1 ), wxFULL_REPAINT_ON_RESIZE|wxTAB_TRAVERSAL|wxWANTS_CHARS );
	bSizer2->Add( m_panelBoard, 1, wxEXPAND | wxALL, 1 );
	
	
	this->SetSizer( bSizer2 );
	this->Layout();
	m_statusBar = this->CreateStatusBar( 2, wxST_SIZEGRIP|wxWANTS_CHARS, ID_DEFAULT );
	m_menubar1 = new wxMenuBar( 0 );
	m_menu1 = new wxMenu();
	wxMenuItem* menuItemNewGame;
#ifdef JP
	menuItemNewGame = new wxMenuItem( m_menu1, ID_NEWGAME, wxString( wxT("新規対局(&N)...") ) + wxT('\t') + wxT("Ctrl-N"), wxT("レート対局開始"), wxITEM_NORMAL );
#else
	menuItemNewGame = new wxMenuItem( m_menu1, ID_NEWGAME, wxString( _("&New Game...") ) + wxT('\t') + wxT("Ctrl-N"), _("Start a new game"), wxITEM_NORMAL );
#endif
	m_menu1->Append( menuItemNewGame );
	
	wxMenuItem* menuItemNewRated;
#ifdef JP
	menuItemNewRated = new wxMenuItem( m_menu1, ID_NEWRATED, wxString( wxT("新規レート対局(&R)") ) + wxT('\t') + wxT("Ctrl-R"), wxT("新規レート対局開始"), wxITEM_NORMAL );
#else
	menuItemNewRated = new wxMenuItem( m_menu1, ID_NEWRATED, wxString( _("New &Rated Game") ) + wxT('\t') + wxT("Ctrl-R"), _("Starts a new rated game"), wxITEM_NORMAL );
#endif
	m_menu1->Append( menuItemNewRated );
	
	wxMenuItem* m_menuItemRatedSize;
#ifdef JP
	m_menuItemRatedSize = new wxMenuItem( m_menu1, ID_RATEDSIZE, wxString( wxT("レート対局碁盤サイズ設定(&B)...") ) + wxT('\t') + wxT("Ctrl-B"), wxT("レート対局碁盤サイズ変更"), wxITEM_NORMAL );
#else
	m_menuItemRatedSize = new wxMenuItem( m_menu1, ID_RATEDSIZE, wxString( _("Set Rated &Board Size...") ) + wxT('\t') + wxT("Ctrl-B"), _("Change the board size for rated games"), wxITEM_NORMAL );
#endif
	m_menu1->Append( m_menuItemRatedSize );
	
	m_menu1->AppendSeparator();
	
	wxMenuItem* menuItem11;
#ifdef JP
	menuItem11 = new wxMenuItem( m_menu1, ID_OPEN, wxString( wxT("開く(&O)...") ) + wxT('\t') + wxT("Ctrl-O"), wxT("ディスクからSGFを開く"), wxITEM_NORMAL );
#else
	menuItem11 = new wxMenuItem( m_menu1, ID_OPEN, wxString( _("&Open Game...") ) + wxT('\t') + wxT("Ctrl-O"), _("Opens a game from disk"), wxITEM_NORMAL );
#endif
	m_menu1->Append( menuItem11 );
	
	wxMenuItem* menuItem12;
#ifdef JP
	menuItem12 = new wxMenuItem( m_menu1, ID_SAVE, wxString( wxT("保存(&S)...") ) + wxT('\t') + wxT("Ctrl-S"), wxT("ディスクにSGFを保存"), wxITEM_NORMAL );
#else
	menuItem12 = new wxMenuItem( m_menu1, ID_SAVE, wxString( _("&Save Game...") ) + wxT('\t') + wxT("Ctrl-S"), _("Save a game to disk"), wxITEM_NORMAL );
#endif
	m_menu1->Append( menuItem12 );
	
	m_menu1->AppendSeparator();
	
	wxMenuItem* m_menuItem30;
#ifdef JP
	m_menuItem30 = new wxMenuItem( m_menu1, ID_COPYCLIPBOARD, wxString( wxT("SGFコピー") ) + wxT('\t') + wxT("Ctrl-C"), wxT("クリップボードにSGFコピー"), wxITEM_NORMAL );
#else
	m_menuItem30 = new wxMenuItem( m_menu1, ID_COPYCLIPBOARD, wxString( _("Copy SGF") ) + wxT('\t') + wxT("Ctrl-C"), _("Copy SGF to clipboard"), wxITEM_NORMAL );
#endif
	m_menu1->Append( m_menuItem30 );
	
	wxMenuItem* m_menuItem31;
#ifdef JP
	m_menuItem31 = new wxMenuItem( m_menu1, ID_PASTECLIPBOARD, wxString( wxT("SGF貼り付け") ) + wxT('\t') + wxT("Ctrl-V"), wxT("クリップボードからSGF貼り付け"), wxITEM_NORMAL );
#else
	m_menuItem31 = new wxMenuItem( m_menu1, ID_PASTECLIPBOARD, wxString( _("Paste SGF") ) + wxT('\t') + wxT("Ctrl-V"), _("Paste SGF from clipboard"), wxITEM_NORMAL );
#endif
	m_menu1->Append( m_menuItem31 );
	
	m_menu1->AppendSeparator();
	
	wxMenuItem* menuItemExit;
#ifdef JP
	menuItemExit = new wxMenuItem( m_menu1, wxID_EXIT, wxString( wxT("終了(&x)") ) + wxT('\t') + wxT("Alt-F4"), wxT("プログラムを終了"), wxITEM_NORMAL );
#else
	menuItemExit = new wxMenuItem( m_menu1, wxID_EXIT, wxString( _("E&xit") ) + wxT('\t') + wxT("Alt-F4"), _("Exit the program"), wxITEM_NORMAL );
#endif
	m_menu1->Append( menuItemExit );
	
#ifdef JP
	m_menubar1->Append( m_menu1, wxT("ファイル(&F)") );
#else
	m_menubar1->Append( m_menu1, _("&File") ); 
#endif
	
	m_menu2 = new wxMenu();
	wxMenuItem* menuItem9;
#ifdef JP
	menuItem9 = new wxMenuItem( m_menu2, ID_UNDO, wxString( wxT("1手戻る(&U)") ) + wxT('\t') + wxT("Ctrl-Z"), wxEmptyString, wxITEM_NORMAL );
#else
	menuItem9 = new wxMenuItem( m_menu2, ID_UNDO, wxString( _("&Undo") ) + wxT('\t') + wxT("Ctrl-Z"), wxEmptyString, wxITEM_NORMAL );
#endif
	m_menu2->Append( menuItem9 );
	
	wxMenuItem* menuItem8;
#ifdef JP
	menuItem8 = new wxMenuItem( m_menu2, ID_REDO, wxString( wxT("1手進む(&F)") ) + wxT('\t') + wxT("Ctrl-Y"), wxEmptyString, wxITEM_NORMAL );
#else
	menuItem8 = new wxMenuItem( m_menu2, ID_REDO, wxString( _("&Forward") ) + wxT('\t') + wxT("Ctrl-Y"), wxEmptyString, wxITEM_NORMAL );
#endif
	m_menu2->Append( menuItem8 );
	
	m_menu2->AppendSeparator();
	
	wxMenuItem* menuItem131;
#ifdef JP
	menuItem131 = new wxMenuItem( m_menu2, ID_BACK10, wxString( wxT("10手戻る(&B)") ) + wxT('\t') + wxT("Alt-B"), wxT("10手戻る"), wxITEM_NORMAL );
#else
	menuItem131 = new wxMenuItem( m_menu2, ID_BACK10, wxString( _("Go &Back 10 Moves") ) + wxT('\t') + wxT("Alt-B"), _("Undo 10 moves"), wxITEM_NORMAL );
#endif
	m_menu2->Append( menuItem131 );
	
	wxMenuItem* menuItem141;
#ifdef JP
	menuItem141 = new wxMenuItem( m_menu2, ID_FWD10, wxString( wxT("10手進む(&w)") ) + wxT('\t') + wxT("Alt-F"), wxT("10手進む"), wxITEM_NORMAL );
#else
	menuItem141 = new wxMenuItem( m_menu2, ID_FWD10, wxString( _("Go For&ward 10 Moves") ) + wxT('\t') + wxT("Alt-F"), _("Forward 10 moves"), wxITEM_NORMAL );
#endif
	m_menu2->Append( menuItem141 );
	
	m_menu2->AppendSeparator();
	
	wxMenuItem* menuItem18;
#ifdef JP
	menuItem18 = new wxMenuItem( m_menu2, ID_FORCE, wxString( wxT("コンピュータに着手催促(&C)") ) + wxT('\t') + wxT("Ctrl-F"), wxT("コンピュータに着手催促"), wxITEM_NORMAL );
#else
	menuItem18 = new wxMenuItem( m_menu2, ID_FORCE, wxString( _("Force &Computer Move") ) + wxT('\t') + wxT("Ctrl-F"), _("Force the computer to move now"), wxITEM_NORMAL );
#endif
	m_menu2->Append( menuItem18 );
	
	m_menu2->AppendSeparator();
	
	wxMenuItem* m_menuItem24;
#ifdef JP
	m_menuItem24 = new wxMenuItem( m_menu2, ID_PASS, wxString( wxT("パス(&P)") ) + wxT('\t') + wxT("Alt-P"), wxEmptyString, wxITEM_NORMAL );
#else
	m_menuItem24 = new wxMenuItem( m_menu2, ID_PASS, wxString( _("&Pass") ) + wxT('\t') + wxT("Alt-P"), wxEmptyString, wxITEM_NORMAL );
#endif
	m_menu2->Append( m_menuItem24 );
	
	wxMenuItem* menuItem20;
#ifdef JP
	menuItem20 = new wxMenuItem( m_menu2, ID_RESIGN, wxString( wxT("投了(&R)") ) + wxT('\t') + wxT("Alt-R"), wxT("投了する"), wxITEM_NORMAL );
#else
	menuItem20 = new wxMenuItem( m_menu2, ID_RESIGN, wxString( _("&Resign") ) + wxT('\t') + wxT("Alt-R"), _("Resign the game"), wxITEM_NORMAL );
#endif
	m_menu2->Append( menuItem20 );
	
	m_menu2->AppendSeparator();
	
	wxMenuItem* menuItem5;
#ifdef JP
	menuItem5 = new wxMenuItem( m_menu2, ID_SCORE, wxString( wxT("地合計算(&S)") ) + wxT('\t') + wxT("Alt-S"), wxT("地合計算"), wxITEM_NORMAL );
#else
	menuItem5 = new wxMenuItem( m_menu2, ID_SCORE, wxString( _("&Score Game") ) + wxT('\t') + wxT("Alt-S"), _("Score the position"), wxITEM_NORMAL );
#endif
	m_menu2->Append( menuItem5 );
	
#ifdef JP
	m_menubar1->Append( m_menu2, wxT("対局(&G)") );
#else
	m_menubar1->Append( m_menu2, _("&Game") ); 
#endif
	
	m_menuAnalyze = new wxMenu();
	wxMenuItem* menuItemAnalyze;
#ifdef JP
	menuItemAnalyze = new wxMenuItem( m_menuAnalyze, ID_ANALYZE, wxString( wxT("検討開始・停止(&A)") ) + wxT('\t') + wxT("F2"), wxT("検討開始"), wxITEM_NORMAL );
#else
	menuItemAnalyze = new wxMenuItem( m_menuAnalyze, ID_ANALYZE, wxString( _("Start/Stop &Analysis") ) + wxT('\t') + wxT("F2"), _("Start analyzing"), wxITEM_NORMAL );
#endif
	m_menuAnalyze->Append( menuItemAnalyze );
	
	m_menuAnalyze->AppendSeparator();
	
	wxMenuItem* m_menuItemStoreMainline;
#ifdef JP
	m_menuItemStoreMainline = new wxMenuItem( m_menuAnalyze, ID_PUSHPOS, wxString( wxT("局面保存(Push)(&S)") ) + wxT('\t') + wxT("Shift-P"), wxT("局面をスタック"), wxITEM_NORMAL );
#else
	m_menuItemStoreMainline = new wxMenuItem( m_menuAnalyze, ID_PUSHPOS, wxString( _("&Store Position (Push)") ) + wxT('\t') + wxT("Shift-P"), _("Store this position on the stack"), wxITEM_NORMAL );
#endif
	m_menuAnalyze->Append( m_menuItemStoreMainline );
	
	wxMenuItem* m_menuPopVariation;
#ifdef JP
	m_menuPopVariation = new wxMenuItem( m_menuAnalyze, ID_POPPOS, wxString( wxT("局面破棄(Pop)(&F)") ) + wxT('\t') + wxT("Ctrl-P"), wxT("スタック局面を破棄"), wxITEM_NORMAL );
#else
	m_menuPopVariation = new wxMenuItem( m_menuAnalyze, ID_POPPOS, wxString( _("&Forget Position (Pop)") ) + wxT('\t') + wxT("Ctrl-P"), _("Remove the top position from the stack"), wxITEM_NORMAL );
#endif
	m_menuAnalyze->Append( m_menuPopVariation );
	m_menuPopVariation->Enable( false );
	
	wxMenuItem* m_menuItemMainline;
#ifdef JP
	m_menuItemMainline = new wxMenuItem( m_menuAnalyze, ID_MAINLINE, wxString( wxT("局面復元(&e)") ) + wxT('\t') + wxT("Alt-M"), wxT("保存位置に戻す"), wxITEM_NORMAL );
#else
	m_menuItemMainline = new wxMenuItem( m_menuAnalyze, ID_MAINLINE, wxString( _("R&evert to Stored") ) + wxT('\t') + wxT("Alt-M"), _("Revert board position to the top of the stack"), wxITEM_NORMAL );
#endif
	m_menuAnalyze->Append( m_menuItemMainline );
	
	m_menuAnalyze->AppendSeparator();
	
	wxMenuItem* m_menuItemAnalysisWindow;
#ifdef JP
	m_menuItemAnalysisWindow = new wxMenuItem( m_menuAnalyze, ID_ANALYSISWINDOWTOGGLE, wxString( wxT("検討ウィンドウ(&W)") ) + wxT('\t') + wxT("F7"), wxEmptyString, wxITEM_CHECK );
#else
	m_menuItemAnalysisWindow = new wxMenuItem( m_menuAnalyze, ID_ANALYSISWINDOWTOGGLE, wxString( _("Analysis &Window") ) + wxT('\t') + wxT("F7"), wxEmptyString, wxITEM_CHECK );
#endif
	m_menuAnalyze->Append( m_menuItemAnalysisWindow );
	
	wxMenuItem* m_menuItemHistogram;
#ifdef JP
	m_menuItemHistogram = new wxMenuItem( m_menuAnalyze, ID_SCOREHISTOGRAMTOGGLE, wxString( wxT("グラフ表示(&H)") ) + wxT('\t') + wxT("F8"), wxEmptyString, wxITEM_CHECK );
#else
	m_menuItemHistogram = new wxMenuItem( m_menuAnalyze, ID_SCOREHISTOGRAMTOGGLE, wxString( _("Show &Histogram") ) + wxT('\t') + wxT("F8"), wxEmptyString, wxITEM_CHECK );
#endif
	m_menuAnalyze->Append( m_menuItemHistogram );
	
#ifdef JP
	m_menubar1->Append( m_menuAnalyze, wxT("&検討(&A)") );
#else
	m_menubar1->Append( m_menuAnalyze, _("&Analyze") ); 
#endif
	
	m_menuTools = new wxMenu();
	wxMenuItem* m_menuItemMoveProbabilities;
#ifdef JP
	m_menuItemMoveProbabilities = new wxMenuItem( m_menuTools, ID_MOVE_PROBABILITIES, wxString( wxT("候補手確率表示(&P)") ) + wxT('\t') + wxT("F3"), wxT("各着手がプロの棋士によって指される可能性を示す"), wxITEM_CHECK );
#else
	m_menuItemMoveProbabilities = new wxMenuItem( m_menuTools, ID_MOVE_PROBABILITIES, wxString( _("Show Network &Probabilities") ) + wxT('\t') + wxT("F3"), _("Show the likelihood of each move being played by a professional player"), wxITEM_CHECK );
#endif
	m_menuTools->Append( m_menuItemMoveProbabilities );
	
	wxMenuItem* m_menuItemBestMoves;
#ifdef JP
	m_menuItemBestMoves = new wxMenuItem( m_menuTools, ID_BEST_MOVES, wxString( wxT("最善手表示(&B)") ) + wxT('\t') + wxT("F4"), wxT("最善手"), wxITEM_CHECK );
#else
	m_menuItemBestMoves = new wxMenuItem( m_menuTools, ID_BEST_MOVES, wxString( _("Show &Best Moves") ) + wxT('\t') + wxT("F4"), _("Color the best moves on the board"), wxITEM_CHECK );
#endif
m_menuTools->Append( m_menuItemBestMoves );
	
	wxMenuItem* menuItem15;
#ifdef JP
	menuItem15 = new wxMenuItem( m_menuTools, ID_SHOWTERRITORY, wxString( wxT("地合表示(&T)") ) + wxT('\t') + wxT("F5"), wxT("地合表示"), wxITEM_CHECK );
#else
	menuItem15 = new wxMenuItem( m_menuTools, ID_SHOWTERRITORY, wxString( _("Show &Territory") ) + wxT('\t') + wxT("F5"), _("Show Territory"), wxITEM_CHECK );
#endif
	m_menuTools->Append( menuItem15 );
	
	wxMenuItem* menuItem16;
#ifdef JP
	menuItem16 = new wxMenuItem( m_menuTools, ID_SHOWMOYO, wxString( wxT("模様表示(&M)") ) + wxT('\t') + wxT("F6"), wxT("模様表示"), wxITEM_CHECK );
#else
	menuItem16 = new wxMenuItem( m_menuTools, ID_SHOWMOYO, wxString( _("Show &Moyo") ) + wxT('\t') + wxT("F6"), _("Show Moyo"), wxITEM_CHECK );
#endif
	m_menuTools->Append( menuItem16 );
	
	m_menuTools->AppendSeparator();
	
	wxMenuItem* m_menuItem28;
#ifdef JP
	m_menuItem28 = new wxMenuItem( m_menuTools, wxID_PREFERENCES, wxString( wxT("設定...") ) + wxT('\t') + wxT("Ctrl-E"), wxEmptyString, wxITEM_NORMAL );
#else
	m_menuItem28 = new wxMenuItem( m_menuTools, wxID_PREFERENCES, wxString( _("S&ettings...") ) + wxT('\t') + wxT("Ctrl-E"), wxEmptyString, wxITEM_NORMAL );
#endif
	m_menuTools->Append( m_menuItem28 );
	
	m_menuTools->AppendSeparator();
	
	wxMenuItem* m_menuItemAdjustClocks;
#ifdef JP
	m_menuItemAdjustClocks = new wxMenuItem( m_menuTools, ID_ADJUSTCLOCKS, wxString( wxT("持ち時間調整...(&A)") ) , wxEmptyString, wxITEM_NORMAL );
#else
	m_menuItemAdjustClocks = new wxMenuItem( m_menuTools, ID_ADJUSTCLOCKS, wxString( _("&Adjust Clocks...") ) , wxEmptyString, wxITEM_NORMAL );
#endif
	m_menuTools->Append( m_menuItemAdjustClocks );
	
#ifdef JP
	m_menubar1->Append( m_menuTools, wxT("ツール(&T)") );
#else
	m_menubar1->Append( m_menuTools, _("&Tools") );
#endif
	
	m_menu3 = new wxMenu();
	wxMenuItem* menuItem13;
#ifdef JP
	menuItem13 = new wxMenuItem( m_menu3, ID_HELPRULES, wxString( wxT("囲碁のルール(&G") ) + wxT('\t') + wxT("F1"), wxT("囲碁のルールの説明"), wxITEM_NORMAL );
#else
	menuItem13 = new wxMenuItem( m_menu3, ID_HELPRULES, wxString( _("&Go Rules") ) + wxT('\t') + wxT("F1"), _("Explanation of the rules of Go"), wxITEM_NORMAL );
#endif
	m_menu3->Append( menuItem13 );
	
	wxMenuItem* menuItem10;
#ifdef JP
	menuItem10 = new wxMenuItem( m_menu3, ID_HOMEPAGE, wxString( wxT("Leelaホームページ") ) , wxEmptyString, wxITEM_NORMAL );
#else
	menuItem10 = new wxMenuItem( m_menu3, ID_HOMEPAGE, wxString( _("Leela Homepage") ) , wxEmptyString, wxITEM_NORMAL );
#endif
	m_menu3->Append( menuItem10 );
	
	m_menu3->AppendSeparator();
	
	wxMenuItem* menuItem7;
#ifdef JP
	menuItem7 = new wxMenuItem( m_menu3, wxID_ABOUT, wxString( wxT("Leelaについて...(&A)") ) , wxT("Leelaについての情報"), wxITEM_NORMAL );
#else
	menuItem7 = new wxMenuItem( m_menu3, wxID_ABOUT, wxString( _("&About...") ) , _("Information about the program"), wxITEM_NORMAL );
#endif
	m_menu3->Append( menuItem7 );
	
#ifdef JP
	m_menubar1->Append( m_menu3, wxT("ヘルプ(&H)") );
#else
	m_menubar1->Append( m_menu3, _("&Help") );
#endif
	
	this->SetMenuBar( m_menubar1 );
	
	m_toolBar = this->CreateToolBar( wxTB_FLAT|wxTB_HORIZONTAL, wxID_ANY ); 
#ifdef JP
	m_tool1 = m_toolBar->AddTool( ID_NEWGAME, wxT("新規対局"), new_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxT("新規対局開始"), wxT("新規対局開始"), NULL );
#else
	m_tool1 = m_toolBar->AddTool( ID_NEWGAME, _("New Game"), new_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, _("Start a new game"), _("Start a new game"), NULL ); 
#endif
	
#ifdef JP
	m_tool12 = m_toolBar->AddTool( ID_NEWRATED, wxT("新規レート対局"), newrated_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxT("新規レート対局開始"), wxT("新規レート対局開始"), NULL );
#else
	m_tool12 = m_toolBar->AddTool( ID_NEWRATED, _("New Rated Game"), newrated_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, _("Start a new rated game"), _("Start a new rated game"), NULL ); 
#endif
	
#ifdef JP
	m_tool4 = m_toolBar->AddTool( ID_OPEN, wxT("対局結果を読み込む"), fileopen_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxT("対局結果を読み込む"), wxT("対局結果を読み込む"), NULL );
#else
	m_tool4 = m_toolBar->AddTool( ID_OPEN, _("Load Game"), fileopen_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, _("Load a game"), _("Load a game"), NULL ); 
#endif
	
#ifdef JP
	m_tool5 = m_toolBar->AddTool( ID_SAVE, wxT("保存"), filesave_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxT("対局結果を保存する"), wxT("対局結果を保存する"), NULL );
#else
	m_tool5 = m_toolBar->AddTool( ID_SAVE, _("Save"), filesave_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, _("Save the game"), _("Save the game"), NULL ); 
#endif
	
	m_toolBar->AddSeparator(); 
	
#ifdef JP
	m_tool10 = m_toolBar->AddTool( ID_BACK10, wxT("10手戻る"), back_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxT("10手戻るs"), wxT("10手戻る"), NULL );
#else
	m_tool10 = m_toolBar->AddTool( ID_BACK10, _("Go back 10 moves"), back_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, _("Go back 10 moves"), _("Go back 10 moves"), NULL ); 
#endif
	
#ifdef JP
	m_tool13 = m_toolBar->AddTool( ID_UNDO, wxT("1手戻る"), undo_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxT("1手戻る"), wxT("1手戻る"), NULL );
#else
	m_tool13 = m_toolBar->AddTool( ID_UNDO, _("Undo"), undo_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, _("Take back one move"), _("Take back one move"), NULL ); 
#endif
	
#ifdef JP
	m_tool15 = m_toolBar->AddTool( ID_REDO, wxT("1手進む"), redo_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxT("1手進む"), wxT("1手進む"), NULL );
#else
	m_tool15 = m_toolBar->AddTool( ID_REDO, _("Forward"), redo_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, _("Go forward one move"), _("Go forward one move"), NULL ); 
#endif
	
#ifdef JP
	m_tool9 = m_toolBar->AddTool( ID_FWD10, wxT("10手進む"), forward_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxT("10手進む"), wxT("10手進む"), NULL );
#else
	m_tool9 = m_toolBar->AddTool( ID_FWD10, _("Forward 10"), forward_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, _("Go forward 10 moves"), _("Go forward 10 moves"), NULL ); 
#endif
	
	m_toolBar->AddSeparator(); 
	
#ifdef JP
	m_toolForce = m_toolBar->AddTool( ID_FORCE, wxT("コンピュータに着手催促"), execute_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxT("コンピュータに着手催促"), wxT("コンピュータに着手催促"), NULL );
#else
	m_toolForce = m_toolBar->AddTool( ID_FORCE, _("Force computer move"), execute_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, _("Force computer move"), _("Force computer move"), NULL ); 
#endif
	
#ifdef JP
	m_toolPushPos = m_toolBar->AddTool( ID_PUSHPOS, wxT("局面保存"), plus_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxT("局面をスタック"), wxT("局面をスタック"), NULL );
#else
	m_toolPushPos = m_toolBar->AddTool( ID_PUSHPOS, _("Store Position"), plus_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, _("Remember this position on the stack"), _("Remember this position on the stack"), NULL ); 
#endif
	
#ifdef JP
	m_toolPopPos = m_toolBar->AddTool( ID_POPPOS, wxT("局面破棄"), minus_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxT("スタック局面を破棄"), wxT("スタック局面を破棄"), NULL );
#else
	m_toolPopPos = m_toolBar->AddTool( ID_POPPOS, _("Remove Position"), minus_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, _("Remove the top position from the stack"), _("Remove the top position from the stack"), NULL ); 
#endif
	
#ifdef JP
	m_toolMainline = m_toolBar->AddTool( ID_MAINLINE, wxT("局面復元"), goparent3_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxT("保存位置に戻す"), wxT("保存位置に戻す"), NULL );
#else
	m_toolMainline = m_toolBar->AddTool( ID_MAINLINE, _("Revert to mainline"), goparent3_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, _("Revert to stored position"), _("Revert to stored position"), NULL ); 
#endif
	
#ifdef JP
	m_toolAnalyze = m_toolBar->AddTool( ID_ANALYZE, wxT("ツール"), analyze_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxT("検討開始・停止"), wxT("検討開始・停止"), NULL );
#else
	m_toolAnalyze = m_toolBar->AddTool( ID_ANALYZE, _("tool"), analyze_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, _("Start/Stop analysis"), _("Start/Stop analysis"), NULL ); 
#endif
	
	m_toolBar->AddSeparator(); 
	
#ifdef JP
	m_tool2 = m_toolBar->AddTool( ID_PASS, wxT("ツール"), delete_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxT("パス"), wxT("パス"), NULL );
#else
	m_tool2 = m_toolBar->AddTool( ID_PASS, _("tool"), delete_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, _("Pass"), _("Pass"), NULL ); 
#endif
	
#ifdef JP
	m_tool3 = m_toolBar->AddTool( ID_SCORE, wxT("ツール"), find_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxT("地合計算"), wxT("地合計算"), NULL );
#else
	m_tool3 = m_toolBar->AddTool( ID_SCORE, _("tool"), find_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, _("Score the game"), _("Score the game"), NULL ); 
#endif
	
#ifdef JP
	m_tool131 = m_toolBar->AddTool( ID_RESIGN, wxT("ツール"), resign_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxT("投了"), wxT("投了"), NULL );
#else
	m_tool131 = m_toolBar->AddTool( ID_RESIGN, _("tool"), resign_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, _("Resign the game"), _("Resign the game"), NULL ); 
#endif
	
	m_toolBar->AddSeparator(); 
	
#ifdef JP
	m_tool11 = m_toolBar->AddTool( wxID_EXIT, wxT("終了"), quit_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxT("プログラムを終了"), wxT("プログラムを終了"), NULL );
#else
	m_tool11 = m_toolBar->AddTool( wxID_EXIT, _("Exit"), quit_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, _("Exit the program"), _("Exit the program"), NULL ); 
#endif
	
	m_toolBar->Realize(); 
	
}

TMainFrame::~TMainFrame()
{
}

BEGIN_EVENT_TABLE( TNewGameDialog, wxDialog )
	EVT_INIT_DIALOG( TNewGameDialog::_wxFB_doInit )
	EVT_RADIOBOX( wxID_ANY, TNewGameDialog::_wxFB_doRadioBox )
	EVT_SPINCTRL( ID_HANDICAPSPIN, TNewGameDialog::_wxFB_doHandicapUpdate )
	EVT_BUTTON( wxID_CANCEL, TNewGameDialog::_wxFB_doCancel )
	EVT_BUTTON( wxID_OK, TNewGameDialog::_wxFB_doOK )
END_EVENT_TABLE()

TNewGameDialog::TNewGameDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxVERTICAL );
	
	wxString m_radioBoxBoardSizeChoices[] = { _("7 x 7"), _("9 x 9"), _("13 x 13"), _("17 x 17"), _("19 x 19"), _("25 x 25") };
	int m_radioBoxBoardSizeNChoices = sizeof( m_radioBoxBoardSizeChoices ) / sizeof( wxString );
#ifdef JP
	m_radioBoxBoardSize = new wxRadioBox( this, wxID_ANY, wxT("碁盤サイズ"), wxDefaultPosition, wxDefaultSize, m_radioBoxBoardSizeNChoices, m_radioBoxBoardSizeChoices, 3, wxRA_SPECIFY_ROWS );
#else
	m_radioBoxBoardSize = new wxRadioBox( this, wxID_ANY, _("Board size"), wxDefaultPosition, wxDefaultSize, m_radioBoxBoardSizeNChoices, m_radioBoxBoardSizeChoices, 3, wxRA_SPECIFY_ROWS );
#endif
	m_radioBoxBoardSize->SetSelection( 4 );
	bSizer12->Add( m_radioBoxBoardSize, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	
	bSizer9->Add( bSizer12, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer2;
#ifdef JP
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("コミ (+0.5)") ), wxHORIZONTAL );
	m_spinCtrlKomi = new wxSpinCtrl( sbSizer2->GetStaticBox(), ID_KOMISPIN, wxT("7"), wxDefaultPosition, wxSize( 150,-1 ), wxSP_ARROW_KEYS, -100, 100, 7 );
#else
	m_spinCtrlKomi = new wxSpinCtrl( sbSizer2->GetStaticBox(), ID_KOMISPIN, wxT("7"), wxDefaultPosition, wxSize( 100,-1 ), wxSP_ARROW_KEYS, -100, 100, 7 );
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Komi (+0.5)") ), wxHORIZONTAL );
#endif
	
	//m_spinCtrlKomi = new wxSpinCtrl( sbSizer2->GetStaticBox(), ID_KOMISPIN, wxT("7"), wxDefaultPosition, wxSize( 100,-1 ), wxSP_ARROW_KEYS, -100, 100, 7 );
	sbSizer2->Add( m_spinCtrlKomi, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
#ifdef JP
	m_staticText2 = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, wxT("コミ"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
#else
	m_staticText2 = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Komi"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
#endif
	m_staticText2->Wrap( -1 );
	sbSizer2->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizer9->Add( sbSizer2, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxStaticBoxSizer* sbSizer3;
#ifdef JP
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("置石") ), wxHORIZONTAL );
	m_spinCtrlHandicap = new wxSpinCtrl( sbSizer3->GetStaticBox(), ID_HANDICAPSPIN, wxEmptyString, wxDefaultPosition, wxSize( 150,-1 ), wxSP_ARROW_KEYS, 0, 100, 0 );
#else
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Handicap") ), wxHORIZONTAL );
	m_spinCtrlHandicap = new wxSpinCtrl( sbSizer3->GetStaticBox(), ID_HANDICAPSPIN, wxEmptyString, wxDefaultPosition, wxSize( 100,-1 ), wxSP_ARROW_KEYS, 0, 100, 0 );
#endif
	
	//m_spinCtrlHandicap = new wxSpinCtrl( sbSizer3->GetStaticBox(), ID_HANDICAPSPIN, wxEmptyString, wxDefaultPosition, wxSize( 100,-1 ), wxSP_ARROW_KEYS, 0, 100, 0 );
	sbSizer3->Add( m_spinCtrlHandicap, 0, wxALIGN_CENTER|wxALL, 5 );
	
#ifdef JP
	m_staticText3 = new wxStaticText( sbSizer3->GetStaticBox(), wxID_ANY, wxT("置石"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
#else
	m_staticText3 = new wxStaticText( sbSizer3->GetStaticBox(), wxID_ANY, _("Handicap"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
#endif
	m_staticText3->Wrap( -1 );
	sbSizer3->Add( m_staticText3, 0, wxALIGN_CENTER|wxALL, 5 );
	
	
	bSizer9->Add( sbSizer3, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxStaticBoxSizer* sbSizer5;
#ifdef JP
	sbSizer5 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("持ち時間") ), wxHORIZONTAL );
	m_spinCtrlTime = new wxSpinCtrl( sbSizer5->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 150,-1 ), wxSP_ARROW_KEYS, 1, 720, 90 );
#else
	sbSizer5 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Time for game") ), wxHORIZONTAL );
	m_spinCtrlTime = new wxSpinCtrl( sbSizer5->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 100,-1 ), wxSP_ARROW_KEYS, 1, 720, 90 );
#endif
	
	//m_spinCtrlTime = new wxSpinCtrl( sbSizer5->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 100,-1 ), wxSP_ARROW_KEYS, 1, 720, 90 );
	sbSizer5->Add( m_spinCtrlTime, 0, wxALIGN_CENTER|wxALL, 5 );
	
#ifdef JP
	m_staticText13 = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, wxT("分"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
#else
	m_staticText13 = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("Minutes"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
#endif
	m_staticText13->Wrap( -1 );
	sbSizer5->Add( m_staticText13, 0, wxALIGN_CENTER|wxALL, 5 );
	
	
	bSizer9->Add( sbSizer5, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	
	bSizer11->Add( bSizer9, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );
	
#ifdef JP
	wxString m_radioBoxColorChoices[] = { wxT("黒"), wxT("白") };
#else
	wxString m_radioBoxColorChoices[] = { _("Black"), _("White") };
#endif
	int m_radioBoxColorNChoices = sizeof( m_radioBoxColorChoices ) / sizeof( wxString );
#ifdef JP
	m_radioBoxColor = new wxRadioBox( this, wxID_ANY, wxT("手番"), wxDefaultPosition, wxDefaultSize, m_radioBoxColorNChoices, m_radioBoxColorChoices, 1, wxRA_SPECIFY_ROWS );
#else
	m_radioBoxColor = new wxRadioBox( this, wxID_ANY, _("Your color"), wxDefaultPosition, wxDefaultSize, m_radioBoxColorNChoices, m_radioBoxColorChoices, 1, wxRA_SPECIFY_ROWS );
#endif
	m_radioBoxColor->SetSelection( 0 );
	bSizer10->Add( m_radioBoxColor, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
#ifdef JP
	wxString m_radioBoxLevelChoices[] = { wxT("100回試行"), wxT("500回試行"), wxT("1,000回試行"), wxT("5,000回試行"), wxT("10,000回試行"), wxT("20,000回試行"), wxT("無制限") };
#else
	wxString m_radioBoxLevelChoices[] = { _("100 simulations"), _("500 simulations"), _("1000 simulations"), _("5000 simulations"), _("10000 simulations"), _("20000 simulations"), _("Unlimited") };
#endif
	int m_radioBoxLevelNChoices = sizeof( m_radioBoxLevelChoices ) / sizeof( wxString );
#ifdef JP
	m_radioBoxLevel = new wxRadioBox( this, wxID_ANY, wxT("エンジン最大レベル"), wxDefaultPosition, wxDefaultSize, m_radioBoxLevelNChoices, m_radioBoxLevelChoices, 1, wxRA_SPECIFY_COLS );
#else
	m_radioBoxLevel = new wxRadioBox( this, wxID_ANY, _("Engine max level"), wxDefaultPosition, wxDefaultSize, m_radioBoxLevelNChoices, m_radioBoxLevelChoices, 1, wxRA_SPECIFY_COLS );
#endif
	m_radioBoxLevel->SetSelection( 6 );
	bSizer10->Add( m_radioBoxLevel, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxStaticBoxSizer* sbSizerEngineSettings;
#ifdef JP
	sbSizerEngineSettings = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("エンジン設定(19 x 19)") ), wxVERTICAL );
#else
	sbSizerEngineSettings = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Engine settings (19 x 19)") ), wxVERTICAL );
#endif
	
#ifdef JP
	m_checkNeuralNet = new wxCheckBox( sbSizerEngineSettings->GetStaticBox(), wxID_ANY, wxT("ニューラルネットワークを使う"), wxDefaultPosition, wxDefaultSize, 0 );
#else
	m_checkNeuralNet = new wxCheckBox( sbSizerEngineSettings->GetStaticBox(), wxID_ANY, _("Use Neural Network"), wxDefaultPosition, wxDefaultSize, 0 );
#endif
	m_checkNeuralNet->SetValue(true); 
	sbSizerEngineSettings->Add( m_checkNeuralNet, 0, wxALL, 5 );
	
	
	bSizer10->Add( sbSizerEngineSettings, 0, wxEXPAND, 5 );
	
	
	bSizer11->Add( bSizer10, 0, wxEXPAND, 5 );
	
	
	bSizer7->Add( bSizer11, 1, wxEXPAND, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
#ifdef JP
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL, wxT("取消") );
#else
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
#endif
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bSizer7->Add( m_sdbSizer1, 0, wxALIGN_LEFT|wxALL, 2 );
	
	
	this->SetSizer( bSizer7 );
	this->Layout();
	bSizer7->Fit( this );
}

TNewGameDialog::~TNewGameDialog()
{
}

BEGIN_EVENT_TABLE( TAboutDialog, wxDialog )
	EVT_INIT_DIALOG( TAboutDialog::_wxFB_doInit )
END_EVENT_TABLE()

TAboutDialog::TAboutDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );
	
#ifdef JP
	m_staticTextVersion = new wxStaticText( this, wxID_ANY, wxT("Leela バージョン 0.11.0"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
#else
	m_staticTextVersion = new wxStaticText( this, wxID_ANY, _("Leela version 0.11.0"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
#endif
	m_staticTextVersion->Wrap( -1 );
	m_staticTextVersion->SetFont( wxFont( 11, 70, 90, 92, false, wxEmptyString ) );
	
	bSizer9->Add( m_staticTextVersion, 0, wxALL|wxEXPAND, 10 );
	
#ifdef JP
	m_staticTextEngine = new wxStaticText( this, wxID_ANY, wxT("OpenCL/BLASエンジンは長時間かかります"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
#else
	m_staticTextEngine = new wxStaticText( this, wxID_ANY, _("OpenCL/BLAS Engine goes here and can be a very long string"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
#endif
	m_staticTextEngine->Wrap( 400 );
	bSizer9->Add( m_staticTextEngine, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText5 = new wxStaticText( this, wxID_ANY, _("Copyright (C) 2007-2017 Gian-Carlo Pascutto"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	m_staticText5->Wrap( -1 );
	bSizer9->Add( m_staticText5, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText9 = new wxStaticText( this, wxID_ANY, _("gcp@sjeng.org"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	m_staticText9->Wrap( -1 );
	bSizer9->Add( m_staticText9, 0, wxALL|wxEXPAND, 5 );
	
	m_hyperlink3 = new wxHyperlinkCtrl( this, wxID_ANY, _("https://sjeng.org/leela"), wxT("https://www.sjeng.org/leela"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	bSizer9->Add( m_hyperlink3, 0, wxALIGN_CENTER|wxALL, 5 );
	
	m_button1 = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer9->Add( m_button1, 0, wxALIGN_CENTER|wxALL, 5 );
	
	
	this->SetSizer( bSizer9 );
	this->Layout();
	bSizer9->Fit( this );
}

TAboutDialog::~TAboutDialog()
{
}

BEGIN_EVENT_TABLE( TClockAdjustDialog, wxDialog )
	EVT_INIT_DIALOG( TClockAdjustDialog::_wxFB_doInit )
	EVT_BUTTON( wxID_CANCEL, TClockAdjustDialog::_wxFB_doCancel )
	EVT_BUTTON( wxID_OK, TClockAdjustDialog::_wxFB_doOK )
END_EVENT_TABLE()

TClockAdjustDialog::TClockAdjustDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer4;
#ifdef JP
	sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("黒時間(分:秒)") ), wxHORIZONTAL );
#else
	sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Black clock (minutes:seconds)") ), wxHORIZONTAL );
#endif
	
	m_spinCtrlBlackMins = new wxSpinCtrl( sbSizer4->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), wxSP_ARROW_KEYS, 0, 999, 30 );
	sbSizer4->Add( m_spinCtrlBlackMins, 1, wxALL|wxEXPAND, 5 );
	
	m_spinCtrlBlackSecs = new wxSpinCtrl( sbSizer4->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 59, 0 );
	sbSizer4->Add( m_spinCtrlBlackSecs, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizer9->Add( sbSizer4, 1, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer6;
#ifdef JP
	sbSizer6 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("白時間(分:秒)") ), wxHORIZONTAL );
#else
	sbSizer6 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("White clock (minutes:seconds)") ), wxHORIZONTAL );
#endif
	
	m_spinCtrlWhiteMins = new wxSpinCtrl( sbSizer6->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), wxSP_ARROW_KEYS, 0, 999, 30 );
	sbSizer6->Add( m_spinCtrlWhiteMins, 1, wxALL|wxEXPAND, 5 );
	
	m_spinCtrlWhiteSecs = new wxSpinCtrl( sbSizer6->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 59, 0 );
	sbSizer6->Add( m_spinCtrlWhiteSecs, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizer9->Add( sbSizer6, 1, wxALL|wxEXPAND, 5 );
	
	m_sdbSizer3 = new wxStdDialogButtonSizer();
	m_sdbSizer3OK = new wxButton( this, wxID_OK );
	m_sdbSizer3->AddButton( m_sdbSizer3OK );
#ifdef JP
	m_sdbSizer3Cancel = new wxButton( this, wxID_CANCEL, wxT("取消") );
#else
	m_sdbSizer3Cancel = new wxButton( this, wxID_CANCEL );
#endif
	m_sdbSizer3->AddButton( m_sdbSizer3Cancel );
	m_sdbSizer3->Realize();
	
	bSizer9->Add( m_sdbSizer3, 0, wxALIGN_CENTER|wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bSizer9 );
	this->Layout();
	bSizer9->Fit( this );
}

TClockAdjustDialog::~TClockAdjustDialog()
{
}

BEGIN_EVENT_TABLE( TRatedSizeDialog, wxDialog )
	EVT_BUTTON( ID_SIZE9, TRatedSizeDialog::_wxFB_doSize9 )
	EVT_BUTTON( ID_SIZE19, TRatedSizeDialog::_wxFB_doSize19 )
END_EVENT_TABLE()

TRatedSizeDialog::TRatedSizeDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonSize9 = new wxButton( this, ID_SIZE9, _("9 x 9"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonSize9->SetDefault(); 
	bSizer10->Add( m_buttonSize9, 0, wxALL, 5 );
	
	m_buttonSize19 = new wxButton( this, ID_SIZE19, _("19 x 19"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer10->Add( m_buttonSize19, 0, wxALL, 5 );
	
	
	this->SetSizer( bSizer10 );
	this->Layout();
	bSizer10->Fit( this );
}

TRatedSizeDialog::~TRatedSizeDialog()
{
}

TCalculateDialog::TCalculateDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );
	
#ifdef JP
	m_staticTextCalculating = new wxStaticText( this, wxID_ANY, wxT("計算中、お待ちください..."), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
#else
	m_staticTextCalculating = new wxStaticText( this, wxID_ANY, _("Calculating, please hold..."), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
#endif
	m_staticTextCalculating->Wrap( -1 );
	bSizer11->Add( m_staticTextCalculating, 1, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bSizer11 );
	this->Layout();
}

TCalculateDialog::~TCalculateDialog()
{
}

BEGIN_EVENT_TABLE( TAnalysisWindow, wxFrame )
	EVT_CLOSE( TAnalysisWindow::_wxFB_doClose )
	EVT_SIZE( TAnalysisWindow::_wxFB_doResize )
	EVT_GRID_CELL_LEFT_CLICK( TAnalysisWindow::_wxFB_doLeftClick )
	EVT_GRID_CELL_LEFT_DCLICK( TAnalysisWindow::_wxFB_doDeselect )
	EVT_GRID_CELL_RIGHT_CLICK( TAnalysisWindow::_wxFB_doContextMenu )
END_EVENT_TABLE()

TAnalysisWindow::TAnalysisWindow( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxVERTICAL );
	
	m_panel3 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer121;
	bSizer121 = new wxBoxSizer( wxVERTICAL );
	
	m_moveGrid = new wxGrid( m_panel3, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_moveGrid->CreateGrid( 8, 6 );
	m_moveGrid->EnableEditing( false );
	m_moveGrid->EnableGridLines( true );
	m_moveGrid->EnableDragGridSize( false );
	m_moveGrid->SetMargins( 0, 0 );
	
	// Columns
	m_moveGrid->AutoSizeColumns();
	m_moveGrid->EnableDragColMove( false );
	m_moveGrid->EnableDragColSize( true );
	m_moveGrid->SetColLabelSize( 30 );
#ifdef JP
	m_moveGrid->SetColLabelValue( 0, wxT("着手") );
	m_moveGrid->SetColLabelValue( 1, wxT("探索比率%") );
	m_moveGrid->SetColLabelValue( 2, wxT("試行回数") );
	m_moveGrid->SetColLabelValue( 3, wxT("勝率%") );
	m_moveGrid->SetColLabelValue( 4, wxT("Net 確率%") );
	m_moveGrid->SetColLabelValue( 5, wxT("予想手順") );
#else
	m_moveGrid->SetColLabelValue( 0, _("Move") );
	m_moveGrid->SetColLabelValue( 1, _("Effort%") );
	m_moveGrid->SetColLabelValue( 2, _("Simulations") );
	m_moveGrid->SetColLabelValue( 3, _("Win%") );
	m_moveGrid->SetColLabelValue( 4, _("Net Prob%") );
	m_moveGrid->SetColLabelValue( 5, _("PV") );
#endif
	m_moveGrid->SetColLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTRE );
	
	// Rows
	m_moveGrid->AutoSizeRows();
	m_moveGrid->EnableDragRowSize( false );
	m_moveGrid->SetRowLabelSize( 0 );
	m_moveGrid->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_moveGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizer121->Add( m_moveGrid, 1, wxEXPAND, 5 );
	
	
	m_panel3->SetSizer( bSizer121 );
	m_panel3->Layout();
	bSizer121->Fit( m_panel3 );
	bSizer12->Add( m_panel3, 1, wxEXPAND | wxALL, 5 );
	
	
	this->SetSizer( bSizer12 );
	this->Layout();
	
	this->Centre( wxBOTH );
}

TAnalysisWindow::~TAnalysisWindow()
{
}

BEGIN_EVENT_TABLE( TSettingsDialog, wxDialog )
	EVT_INIT_DIALOG( TSettingsDialog::_wxFB_doInit )
	EVT_BUTTON( wxID_CANCEL, TSettingsDialog::_wxFB_doCancel )
	EVT_BUTTON( wxID_OK, TSettingsDialog::_wxFB_doOK )
END_EVENT_TABLE()

TSettingsDialog::TSettingsDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxVERTICAL );
	
	m_panel4 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer15;
	bSizer15 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer7;
#ifdef JP
	sbSizer7 = new wxStaticBoxSizer( new wxStaticBox( m_panel4, wxID_ANY, wxT("エンジン設定") ), wxVERTICAL );
#else
	sbSizer7 = new wxStaticBoxSizer( new wxStaticBox( m_panel4, wxID_ANY, _("Engine Settings") ), wxVERTICAL );
#endif
	
	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer( 0, 2, 0, 0 );
	
#ifdef JP
	m_checkBoxPasses = new wxCheckBox( sbSizer7->GetStaticBox(), ID_PASSTOGGLE, wxT("パスを許可"), wxDefaultPosition, wxDefaultSize, 0 );
#else
	m_checkBoxPasses = new wxCheckBox( sbSizer7->GetStaticBox(), ID_PASSTOGGLE, _("Allow Passes"), wxDefaultPosition, wxDefaultSize, 0 );
#endif
	m_checkBoxPasses->SetValue(true); 
	gSizer1->Add( m_checkBoxPasses, 0, wxALL, 5 );
	
#ifdef JP
	m_checkBoxPondering = new wxCheckBox( sbSizer7->GetStaticBox(), ID_PONDERTOGGLE, wxT("相手手番中も先読みする"), wxDefaultPosition, wxDefaultSize, 0 );
#else
	m_checkBoxPondering = new wxCheckBox( sbSizer7->GetStaticBox(), ID_PONDERTOGGLE, _("Pondering"), wxDefaultPosition, wxDefaultSize, 0 );
#endif
	m_checkBoxPondering->SetValue(true); 
	gSizer1->Add( m_checkBoxPondering, 0, wxALL, 5 );
	
#ifdef JP
	m_checkBoxResignations = new wxCheckBox( sbSizer7->GetStaticBox(), ID_RESIGNTOGGLE, wxT("投了を許可"), wxDefaultPosition, wxDefaultSize, 0 );
#else
	m_checkBoxResignations = new wxCheckBox( sbSizer7->GetStaticBox(), ID_RESIGNTOGGLE, _("Resignations"), wxDefaultPosition, wxDefaultSize, 0 );
#endif
	m_checkBoxResignations->SetValue(true); 
	gSizer1->Add( m_checkBoxResignations, 0, wxALL, 5 );
	
#ifdef JP
	m_checkBoxNeuralNet = new wxCheckBox( sbSizer7->GetStaticBox(), ID_NETWORKTOGGLE, wxT("ニューラルネットワーク"), wxDefaultPosition, wxDefaultSize, 0 );
#else
	m_checkBoxNeuralNet = new wxCheckBox( sbSizer7->GetStaticBox(), ID_NETWORKTOGGLE, _("Neural Networks"), wxDefaultPosition, wxDefaultSize, 0 );
#endif
	m_checkBoxNeuralNet->SetValue(true); 
	gSizer1->Add( m_checkBoxNeuralNet, 0, wxALL, 5 );
	
	
	sbSizer7->Add( gSizer1, 1, wxEXPAND, 5 );
	
	
	bSizer15->Add( sbSizer7, 0, wxBOTTOM|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer9;
#ifdef JP
	sbSizer9 = new wxStaticBoxSizer( new wxStaticBox( m_panel4, wxID_ANY, wxT("インタフェース設定") ), wxVERTICAL );
#else
	sbSizer9 = new wxStaticBoxSizer( new wxStaticBox( m_panel4, wxID_ANY, _("Interface Settings") ), wxVERTICAL );
#endif
	
#ifdef JP
	m_checkBoxSound = new wxCheckBox( sbSizer9->GetStaticBox(), ID_SOUNDSWITCH, wxT("音"), wxDefaultPosition, wxDefaultSize, 0 );
#else
	m_checkBoxSound = new wxCheckBox( sbSizer9->GetStaticBox(), ID_SOUNDSWITCH, _("Sound"), wxDefaultPosition, wxDefaultSize, 0 );
#endif
	m_checkBoxSound->SetValue(true); 
	sbSizer9->Add( m_checkBoxSound, 0, wxALL, 5 );
	
#ifdef JP
	m_checkBoxDPIScaling = new wxCheckBox( sbSizer9->GetStaticBox(), ID_DPISCALING, wxT("画面のDPIスケーリングを許可(要再起動)"), wxDefaultPosition, wxDefaultSize, 0 );
#else
	m_checkBoxDPIScaling = new wxCheckBox( sbSizer9->GetStaticBox(), ID_DPISCALING, _("Allow DPI Scaling (needs restart)"), wxDefaultPosition, wxDefaultSize, 0 );
#endif
	sbSizer9->Add( m_checkBoxDPIScaling, 0, wxALL, 5 );
	
	
	bSizer15->Add( sbSizer9, 0, wxBOTTOM|wxEXPAND, 5 );
	
	m_sdbSizer3 = new wxStdDialogButtonSizer();
	m_sdbSizer3OK = new wxButton( m_panel4, wxID_OK );
	m_sdbSizer3->AddButton( m_sdbSizer3OK );
#ifdef JP
	m_sdbSizer3Cancel = new wxButton( m_panel4, wxID_CANCEL, wxT("取消") );
#else
	m_sdbSizer3Cancel = new wxButton( m_panel4, wxID_CANCEL );
#endif
	m_sdbSizer3->AddButton( m_sdbSizer3Cancel );
	m_sdbSizer3->Realize();
	
	bSizer15->Add( m_sdbSizer3, 0, wxEXPAND, 5 );
	
	
	m_panel4->SetSizer( bSizer15 );
	m_panel4->Layout();
	bSizer15->Fit( m_panel4 );
	bSizer13->Add( m_panel4, 1, wxEXPAND | wxALL, 5 );
	
	
	this->SetSizer( bSizer13 );
	this->Layout();
	bSizer13->Fit( this );
	
	this->Centre( wxBOTH );
}

TSettingsDialog::~TSettingsDialog()
{
}

BEGIN_EVENT_TABLE( TScoreDialog, wxDialog )
	EVT_BUTTON( ID_SCORE_ACCEPT, TScoreDialog::_wxFB_doAccept )
	EVT_BUTTON( ID_SCORE_DISPUTE, TScoreDialog::_wxFB_doDispute )
END_EVENT_TABLE()

TScoreDialog::TScoreDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer15;
	bSizer15 = new wxBoxSizer( wxVERTICAL );
	
	m_panel4 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer17;
	bSizer17 = new wxBoxSizer( wxVERTICAL );
	
#ifdef JP
	m_staticTextScore = new wxStaticText( m_panel4, wxID_ANY, wxT("最終地合:"), wxDefaultPosition, wxDefaultSize, 0 );
#else
	m_staticTextScore = new wxStaticText( m_panel4, wxID_ANY, _("Final Score:"), wxDefaultPosition, wxDefaultSize, 0 );
#endif
	m_staticTextScore->Wrap( -1 );
	bSizer17->Add( m_staticTextScore, 1, wxALL|wxEXPAND, 5 );
	
#ifdef JP
	m_staticTextMessage = new wxStaticText( m_panel4, wxID_ANY, wxT("黒は0.0目で勝ちます"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
#else
	m_staticTextMessage = new wxStaticText( m_panel4, wxID_ANY, _("Black wins by 0.0"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
#endif
	m_staticTextMessage->Wrap( -1 );
	bSizer17->Add( m_staticTextMessage, 1, wxALL|wxEXPAND, 5 );
	
#ifdef JP
	m_staticTextConfidence = new wxStaticText( m_panel4, wxID_ANY, wxT("正しく採点したとかなり確信しています"), wxDefaultPosition, wxDefaultSize, 0 );
#else
	m_staticTextConfidence = new wxStaticText( m_panel4, wxID_ANY, _("I'm fairly sure I scored this correctly."), wxDefaultPosition, wxDefaultSize, 0 );
#endif
	m_staticTextConfidence->Wrap( -1 );
	bSizer17->Add( m_staticTextConfidence, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer16->Add( 0, 0, 1, wxEXPAND, 5 );
	
#ifdef JP
	m_buttonOK = new wxButton( m_panel4, ID_SCORE_ACCEPT, wxT("受け入れる(&A)"), wxDefaultPosition, wxDefaultSize, 0 );
#else
	m_buttonOK = new wxButton( m_panel4, ID_SCORE_ACCEPT, _("&Accept"), wxDefaultPosition, wxDefaultSize, 0 );
#endif
	m_buttonOK->SetDefault(); 
	bSizer16->Add( m_buttonOK, 0, wxALL|wxEXPAND, 5 );
	
#ifdef JP
	m_buttonDispute = new wxButton( m_panel4, ID_SCORE_DISPUTE, wxT("スコアを競う(&D)"), wxDefaultPosition, wxDefaultSize, 0 );
#else
	m_buttonDispute = new wxButton( m_panel4, ID_SCORE_DISPUTE, _("&Dispute Score"), wxDefaultPosition, wxDefaultSize, 0 );
#endif
	bSizer16->Add( m_buttonDispute, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer17->Add( bSizer16, 1, wxALL|wxEXPAND, 5 );
	
	
	m_panel4->SetSizer( bSizer17 );
	m_panel4->Layout();
	bSizer17->Fit( m_panel4 );
	bSizer15->Add( m_panel4, 1, wxEXPAND | wxALL, 5 );
	
	
	this->SetSizer( bSizer15 );
	this->Layout();
	
	this->Centre( wxBOTH );
}

TScoreDialog::~TScoreDialog()
{
}

TScoreHistogram::TScoreHistogram( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer18;
	bSizer18 = new wxBoxSizer( wxVERTICAL );
	
	m_DrawPanel = new TScorePanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE|wxTAB_TRAVERSAL );
	bSizer18->Add( m_DrawPanel, 1, wxALL|wxEXPAND, 1 );
	
	
	this->SetSizer( bSizer18 );
	this->Layout();
	
	this->Centre( wxBOTH );
}

TScoreHistogram::~TScoreHistogram()
{
}
