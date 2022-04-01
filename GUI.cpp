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
#include "GTP.h"
#include "Msg.h"

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
	
	bool japaneseEnabled = wxConfig::Get()->ReadBool(wxT("japaneseEnabled"), true);
	cfg_lang = 0;
	if (japaneseEnabled) {
		cfg_lang = 1;
	}
	
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );
	
	m_panelBoard = new TBoardPanel( this, ID_BOARDPANEL, wxDefaultPosition, wxSize( -1,-1 ), wxFULL_REPAINT_ON_RESIZE|wxTAB_TRAVERSAL|wxWANTS_CHARS );
	bSizer2->Add( m_panelBoard, 1, wxEXPAND | wxALL, 1 );
	
	
	this->SetSizer( bSizer2 );
	this->Layout();
	m_statusBar = this->CreateStatusBar( 2, wxST_SIZEGRIP|wxWANTS_CHARS, ID_DEFAULT );
#ifndef WIN32
	if (cfg_lang == 1) {
		m_statusBar->SetMinHeight(30);
	}
#endif
    m_menubar1 = new wxMenuBar( 0 );
	m_menu1 = new wxMenu();
	wxMenuItem* menuItemNewGame;
	menuItemNewGame = new wxMenuItem( m_menu1, ID_NEWGAME, wxString(MENU_NEW_GAME_WXSTR[cfg_lang]) + wxT('\t') + wxT("Ctrl-N"), MENU_START_GAME_WXSTR[cfg_lang], wxITEM_NORMAL );
	m_menu1->Append( menuItemNewGame );
	
	wxMenuItem* menuItemNewRated;
	menuItemNewRated = new wxMenuItem( m_menu1, ID_NEWRATED, wxString(MENU_NEW_RATED_GAME_WXSTR[cfg_lang]) + wxT('\t') + wxT("Ctrl-R"), MENU_STARTS_RATED_GAME_WXSTR[cfg_lang], wxITEM_NORMAL );
	m_menu1->Append( menuItemNewRated );
	
	wxMenuItem* m_menuItemRatedSize;
	m_menuItemRatedSize = new wxMenuItem( m_menu1, ID_RATEDSIZE, wxString(MENU_SET_RATED_SIZE_WXSTR[cfg_lang]) + wxT('\t') + wxT("Ctrl-B"), MENU_CHANGE_SIZE_RATED_WXSTR[cfg_lang], wxITEM_NORMAL );
	m_menu1->Append( m_menuItemRatedSize );
	
	m_menu1->AppendSeparator();
	
	wxMenuItem* menuItem11;
	menuItem11 = new wxMenuItem( m_menu1, ID_OPEN, wxString(MENU_OPEN_GAME_WXSTR[cfg_lang]) + wxT('\t') + wxT("Ctrl-O"), MENU_OPENS_GAME_FROM_WXSTR[cfg_lang], wxITEM_NORMAL );
	m_menu1->Append( menuItem11 );
	
	wxMenuItem* menuItem12;
	menuItem12 = new wxMenuItem( m_menu1, ID_SAVE, wxString(MENU_SAVE_GAME_WXSTR[cfg_lang]) + wxT('\t') + wxT("Ctrl-S"), MENU_SAVE_GAME_TO_WXSTR[cfg_lang], wxITEM_NORMAL );
	m_menu1->Append( menuItem12 );
	
	m_menu1->AppendSeparator();
	
	wxMenuItem* m_menuItem30;
	m_menuItem30 = new wxMenuItem( m_menu1, ID_COPYCLIPBOARD, wxString(MENU_COPY_SGF_WXSTR[cfg_lang]) + wxT('\t') + wxT("Ctrl-C"), MENU_COPY_CLIPBOARD_WXSTR[cfg_lang], wxITEM_NORMAL );
	m_menu1->Append( m_menuItem30 );
	
	wxMenuItem* m_menuItem31;
	m_menuItem31 = new wxMenuItem( m_menu1, ID_PASTECLIPBOARD, wxString(MENU_PASTE_SGF_WXSTR[cfg_lang]) + wxT('\t') + wxT("Ctrl-V"), MENU_PASTE_CLIPBOARD_WXSTR[cfg_lang], wxITEM_NORMAL );
	m_menu1->Append( m_menuItem31 );
	
	m_menu1->AppendSeparator();
	
	wxMenuItem* menuItemExit;
	menuItemExit = new wxMenuItem( m_menu1, wxID_EXIT, wxString(MENU_EXIT_WXSTR[cfg_lang]) + wxT('\t') + wxT("Alt-F4"), MENU_EXIT_PROGRAM_WXSTR[cfg_lang], wxITEM_NORMAL );
	m_menu1->Append( menuItemExit );
	
	m_menubar1->Append( m_menu1, MENU_FILE_WXSTR[cfg_lang] );

	m_menu2 = new wxMenu();
	wxMenuItem* menuItem9;
	menuItem9 = new wxMenuItem( m_menu2, ID_UNDO, wxString(MENU_UNDO_WXSTR[cfg_lang]) + wxT('\t') + wxT("Ctrl-Z"), wxEmptyString, wxITEM_NORMAL );
	m_menu2->Append( menuItem9 );
	
	wxMenuItem* menuItem8;
	menuItem8 = new wxMenuItem( m_menu2, ID_REDO, wxString(MENU_FORWARD_WXSTR[cfg_lang]) + wxT('\t') + wxT("Ctrl-Y"), wxEmptyString, wxITEM_NORMAL );
	m_menu2->Append( menuItem8 );
	
	m_menu2->AppendSeparator();
	
	wxMenuItem* menuItem131;
	menuItem131 = new wxMenuItem( m_menu2, ID_BACK10, wxString(MENU_BACK10_WXSTR[cfg_lang]) + wxT('\t') + wxT("Alt-B"), MENU_BACK10_NO_KEY_WXSTR[cfg_lang], wxITEM_NORMAL );
	m_menu2->Append( menuItem131 );
	
	wxMenuItem* menuItem141;
	menuItem141 = new wxMenuItem( m_menu2, ID_FWD10, wxString(MENU_FORWARD10_WXSTR[cfg_lang]) + wxT('\t') + wxT("Alt-F"), MENU_FORWARD10_NO_KEY_WXSTR[cfg_lang], wxITEM_NORMAL );
	m_menu2->Append( menuItem141 );
	
	m_menu2->AppendSeparator();
	
	wxMenuItem* menuItem18;
	menuItem18 = new wxMenuItem( m_menu2, ID_FORCE, wxString(MENU_FORCE_COMPUTER_MOVE_WXSTR[cfg_lang]) + wxT('\t') + wxT("Ctrl-F"), MENU_FORCE_COMPUTER_NO_KEY_WXSTR[cfg_lang], wxITEM_NORMAL );
	m_menu2->Append( menuItem18 );
	
	m_menu2->AppendSeparator();
	
	wxMenuItem* m_menuItem24;
	m_menuItem24 = new wxMenuItem( m_menu2, ID_PASS, wxString(MENU_PASS_WXSTR[cfg_lang]) + wxT('\t') + wxT("Alt-P"), wxEmptyString, wxITEM_NORMAL );
	m_menu2->Append( m_menuItem24 );
	
	wxMenuItem* menuItem20;
	menuItem20 = new wxMenuItem( m_menu2, ID_RESIGN, wxString(MENU_RESIGN_WXSTR[cfg_lang]) + wxT('\t') + wxT("Alt-R"), MENU_RESIGN_NO_KEY_WXSTR[cfg_lang], wxITEM_NORMAL );
	m_menu2->Append( menuItem20 );
	
	m_menu2->AppendSeparator();
	
	wxMenuItem* menuItem5;
	menuItem5 = new wxMenuItem( m_menu2, ID_SCORE, wxString(MENU_SCORE_GAME_WXSTR[cfg_lang]) + wxT('\t') + wxT("Alt-S"), MENU_SCORE_GAME_NO_KEY_WXSTR[cfg_lang], wxITEM_NORMAL );
	m_menu2->Append( menuItem5 );
	
	m_menubar1->Append( m_menu2, MENU_GAME_WXSTR[cfg_lang]);
	
	m_menuAnalyze = new wxMenu();
	wxMenuItem* menuItemAnalyze;
	menuItemAnalyze = new wxMenuItem( m_menuAnalyze, ID_ANALYZE, wxString(MENU_ANALYZE_WXSTR[cfg_lang]) + wxT('\t') + wxT("F2"), MENU_ANALYZE_NO_KEY_WXSTR[cfg_lang], wxITEM_NORMAL );
	m_menuAnalyze->Append( menuItemAnalyze );
	
	m_menuAnalyze->AppendSeparator();
	
	wxMenuItem* m_menuItemStoreMainline;
	m_menuItemStoreMainline = new wxMenuItem( m_menuAnalyze, ID_PUSHPOS, wxString(MENU_PUSH_POS_WXSTR[cfg_lang]) + wxT('\t') + wxT("Shift-P"), MENU_PUSH_POS_NO_KEY_WXSTR[cfg_lang], wxITEM_NORMAL );
	m_menuAnalyze->Append( m_menuItemStoreMainline );
	
	wxMenuItem* m_menuPopVariation;
	m_menuPopVariation = new wxMenuItem( m_menuAnalyze, ID_POPPOS, wxString(MENU_POP_POS_WXSTR[cfg_lang]) + wxT('\t') + wxT("Ctrl-P"), MENU_POP_POS_NO_KEY_WXSTR[cfg_lang], wxITEM_NORMAL );
	m_menuAnalyze->Append( m_menuPopVariation );
	m_menuPopVariation->Enable( false );
	
	wxMenuItem* m_menuItemMainline;
	m_menuItemMainline = new wxMenuItem( m_menuAnalyze, ID_MAINLINE, wxString(MENU_REVERT_STORE_WXSTR[cfg_lang]) + wxT('\t') + wxT("Alt-M"), MENU_REVERT_STORE_NO_KEY_WXSTR[cfg_lang], wxITEM_NORMAL );
	m_menuAnalyze->Append( m_menuItemMainline );
	
	m_menuAnalyze->AppendSeparator();
	
	wxMenuItem* m_menuItemAnalysisWindow;
	m_menuItemAnalysisWindow = new wxMenuItem( m_menuAnalyze, ID_ANALYSISWINDOWTOGGLE, wxString(MENU_ANALYSIS_WINDOW_WXSTR[cfg_lang]) + wxT('\t') + wxT("F7"), wxEmptyString, wxITEM_CHECK );
	m_menuAnalyze->Append( m_menuItemAnalysisWindow );
	
	wxMenuItem* m_menuItemHistogram;
	m_menuItemHistogram = new wxMenuItem( m_menuAnalyze, ID_SCOREHISTOGRAMTOGGLE, wxString(MENU_SCORE_WINDOW_WXSTR[cfg_lang]) + wxT('\t') + wxT("F8"), wxEmptyString, wxITEM_CHECK );
	m_menuAnalyze->Append( m_menuItemHistogram );
	
	m_menubar1->Append( m_menuAnalyze, MENU_ANALYZE_TOP_WXSTR[cfg_lang]); 
	
	m_menuTools = new wxMenu();
	wxMenuItem* m_menuItemMoveProbabilities;
	m_menuItemMoveProbabilities = new wxMenuItem( m_menuTools, ID_MOVE_PROBABILITIES, wxString(MENU_MOVE_PROB_WXSTR[cfg_lang]) + wxT('\t') + wxT("F3"), MENU_MOVE_PROB_NO_KEY_WXSTR[cfg_lang], wxITEM_CHECK );
	m_menuTools->Append( m_menuItemMoveProbabilities );
	
	wxMenuItem* m_menuItemBestMoves;
	m_menuItemBestMoves = new wxMenuItem( m_menuTools, ID_BEST_MOVES, wxString(MENU_BEST_MOVES_WXSTR[cfg_lang]) + wxT('\t') + wxT("F4"), MENU_BEST_MOVES_NO_KEY_WXSTR[cfg_lang], wxITEM_CHECK );
	m_menuTools->Append( m_menuItemBestMoves );
	
	wxMenuItem* menuItem15;
	menuItem15 = new wxMenuItem( m_menuTools, ID_SHOWTERRITORY, wxString(MENU_SHOW_TERRITORY_WXSTR[cfg_lang]) + wxT('\t') + wxT("F5"), MENU_SHOW_TERRITORY_NO_KEY_WXSTR[cfg_lang], wxITEM_CHECK );
	m_menuTools->Append( menuItem15 );
	
	wxMenuItem* menuItem16;
	menuItem16 = new wxMenuItem( m_menuTools, ID_SHOWMOYO, wxString(MENU_SHOW_MOYO_WXSTR[cfg_lang]) + wxT('\t') + wxT("F6"), MENU_SHOW_MOYO_NO_KEY_WXSTR[cfg_lang], wxITEM_CHECK );
	m_menuTools->Append( menuItem16 );
	
	m_menuTools->AppendSeparator();
	
	wxMenuItem* m_menuItem28;
	m_menuItem28 = new wxMenuItem( m_menuTools, wxID_PREFERENCES, wxString( MENU_PREFERENCES_WXSTR[cfg_lang]) + wxT('\t') + wxT("Ctrl-E"), wxEmptyString, wxITEM_NORMAL );
	m_menuTools->Append( m_menuItem28 );
	
	m_menuTools->AppendSeparator();
	
	wxMenuItem* m_menuItemAdjustClocks;
	m_menuItemAdjustClocks = new wxMenuItem( m_menuTools, ID_ADJUSTCLOCKS, wxString(MENU_ADJUST_CLOCKS_WXSTR[cfg_lang]) , wxEmptyString, wxITEM_NORMAL );
	m_menuTools->Append( m_menuItemAdjustClocks );
	
	m_menubar1->Append( m_menuTools, MENU_TOOLS_WXSTR[cfg_lang]);
	
	m_menu3 = new wxMenu();
	wxMenuItem* menuItem13;
	menuItem13 = new wxMenuItem( m_menu3, ID_HELPRULES, wxString(MENU_HELP_RULES_WXSTR[cfg_lang]) + wxT('\t') + wxT("F1"), MENU_HELP_RULES_NO_KEY_WXSTR[cfg_lang], wxITEM_NORMAL );
	m_menu3->Append( menuItem13 );
	
	wxMenuItem* menuItem10;
	menuItem10 = new wxMenuItem( m_menu3, ID_HOMEPAGE, wxString(MENU_HOME_PAGE_WXSTR[cfg_lang]) , wxEmptyString, wxITEM_NORMAL );
	m_menu3->Append( menuItem10 );
	
	m_menu3->AppendSeparator();
	
	wxMenuItem* menuItem7;
	menuItem7 = new wxMenuItem( m_menu3, wxID_ABOUT, wxString(MENU_ABOUT_WXSTR[cfg_lang]) , MENU_ABOUT_NO_KEY_WXSTR[cfg_lang], wxITEM_NORMAL );
	m_menu3->Append( menuItem7 );
	
	m_menubar1->Append( m_menu3, MENU_HELP_WXSTR[cfg_lang]);
	
	this->SetMenuBar( m_menubar1 );
	
	m_toolBar = this->CreateToolBar( wxTB_FLAT|wxTB_HORIZONTAL, wxID_ANY ); 
	m_tool1 = m_toolBar->AddTool( ID_NEWGAME, TBAR_NEW_GAME_WXSTR[cfg_lang], new_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, TBAR_NEW_GAME_EXP_WXSTR[cfg_lang], TBAR_NEW_GAME_EXP_WXSTR[cfg_lang], NULL ); 
	
	m_tool12 = m_toolBar->AddTool( ID_NEWRATED, TBAR_NEW_RATED_WXSTR[cfg_lang], newrated_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, TBAR_NEW_RATED_EXP_WXSTR[cfg_lang], TBAR_NEW_RATED_EXP_WXSTR[cfg_lang], NULL ); 
	
	m_tool4 = m_toolBar->AddTool( ID_OPEN, TBAR_LOAD_GAME_WXSTR[cfg_lang], fileopen_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, TBAR_LOAD_GAME_EXP_WXSTR[cfg_lang], TBAR_LOAD_GAME_EXP_WXSTR[cfg_lang], NULL ); 
	
	m_tool5 = m_toolBar->AddTool( ID_SAVE, TBAR_SAVE_GAME_WXSTR[cfg_lang], filesave_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, TBAR_SAVE_GAME_EXP_WXSTR[cfg_lang], TBAR_SAVE_GAME_EXP_WXSTR[cfg_lang], NULL ); 
	
	m_toolBar->AddSeparator(); 
	
	m_tool10 = m_toolBar->AddTool( ID_BACK10, TBAR_BACK10_WXSTR[cfg_lang], back_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, TBAR_BACK10_EXP_WXSTR[cfg_lang], TBAR_BACK10_EXP_WXSTR[cfg_lang], NULL ); 
	
	m_tool13 = m_toolBar->AddTool( ID_UNDO, TBAR_UNDO_WXSTR[cfg_lang], undo_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, TBAR_UNDO_EXP_WXSTR[cfg_lang], TBAR_UNDO_EXP_WXSTR[cfg_lang], NULL ); 
	
	m_tool15 = m_toolBar->AddTool( ID_REDO, TBAR_REDO_WXSTR[cfg_lang], redo_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, TBAR_REDO_EXP_WXSTR[cfg_lang], TBAR_REDO_EXP_WXSTR[cfg_lang], NULL ); 
	
	m_tool9 = m_toolBar->AddTool( ID_FWD10, TBAR_FORWARD10_WXSTR[cfg_lang], forward_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, TBAR_FORWARD10_EXP_WXSTR[cfg_lang], TBAR_FORWARD10_EXP_WXSTR[cfg_lang], NULL ); 
	
	m_toolBar->AddSeparator(); 
	
	m_toolForce = m_toolBar->AddTool( ID_FORCE, TBAR_FORCE_COMPUTER_MOVE_WXSTR[cfg_lang], execute_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, TBAR_FORCE_COMPUTER_MOVE_EXP_WXSTR[cfg_lang], TBAR_FORCE_COMPUTER_MOVE_EXP_WXSTR[cfg_lang], NULL ); 
	
	m_toolPushPos = m_toolBar->AddTool( ID_PUSHPOS, TBAR_STORE_POSITION_WXSTR[cfg_lang], plus_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, TBAR_STORE_POSITION_EXP_WXSTR[cfg_lang], TBAR_STORE_POSITION_EXP_WXSTR[cfg_lang], NULL ); 
	
	m_toolPopPos = m_toolBar->AddTool( ID_POPPOS, TBAR_REMOVE_POSITION_WXSTR[cfg_lang], minus_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, TBAR_REMOVE_POSITION_EXP_WXSTR[cfg_lang], TBAR_REMOVE_POSITION_EXP_WXSTR[cfg_lang], NULL ); 
	
	m_toolMainline = m_toolBar->AddTool( ID_MAINLINE, TBAR_REVERT_WXSTR[cfg_lang], goparent3_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, TBAR_REVERT_EXP_WXSTR[cfg_lang], TBAR_REVERT_EXP_WXSTR[cfg_lang], NULL ); 
	
	m_toolAnalyze = m_toolBar->AddTool( ID_ANALYZE, TBAR_TOOL_WXSTR[cfg_lang], analyze_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, TBAR_TOOL_1_EXP_WXSTR[cfg_lang], TBAR_TOOL_1_EXP_WXSTR[cfg_lang], NULL ); 
	
	m_toolBar->AddSeparator(); 
	
	m_tool2 = m_toolBar->AddTool( ID_PASS, TBAR_TOOL_WXSTR[cfg_lang], delete_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, TBAR_TOOL_2_EXP_WXSTR[cfg_lang], TBAR_TOOL_2_EXP_WXSTR[cfg_lang], NULL ); 
	
	m_tool3 = m_toolBar->AddTool( ID_SCORE, TBAR_TOOL_WXSTR[cfg_lang], find_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, TBAR_TOOL_3_EXP_WXSTR[cfg_lang], TBAR_TOOL_3_EXP_WXSTR[cfg_lang], NULL ); 
	
	m_tool131 = m_toolBar->AddTool( ID_RESIGN, TBAR_TOOL_WXSTR[cfg_lang], resign_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, TBAR_TOOL_4_EXP_WXSTR[cfg_lang], TBAR_TOOL_4_EXP_WXSTR[cfg_lang], NULL ); 
	
	m_toolBar->AddSeparator(); 
	
	m_tool11 = m_toolBar->AddTool( wxID_EXIT, TBAR_EXIT_WXSTR[cfg_lang], quit_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, TBAR_EXIT_EXP_WXSTR[cfg_lang], TBAR_EXIT_EXP_WXSTR[cfg_lang], NULL ); 
	
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

TNewGameDialog::TNewGameDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, NEW_GAME_WXSTR[cfg_lang], pos, size, style )
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
	m_radioBoxBoardSize = new wxRadioBox( this, wxID_ANY, BOARD_SIZE_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, m_radioBoxBoardSizeNChoices, m_radioBoxBoardSizeChoices, 3, wxRA_SPECIFY_ROWS );
	m_radioBoxBoardSize->SetSelection( 4 );
	bSizer12->Add( m_radioBoxBoardSize, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	
	bSizer9->Add( bSizer12, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer2;

	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, KOMI_0_5_WXSTR[cfg_lang]), wxHORIZONTAL );
	m_spinCtrlKomi = new wxSpinCtrl( sbSizer2->GetStaticBox(), ID_KOMISPIN, wxT("7"), wxDefaultPosition, wxSize( 150,-1 ), wxSP_ARROW_KEYS, -100, 100, 7 );

	sbSizer2->Add( m_spinCtrlKomi, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_staticText2 = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, KOMI_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	m_staticText2->Wrap( -1 );
	sbSizer2->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizer9->Add( sbSizer2, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, HANDICAP_WXSTR[cfg_lang] ), wxHORIZONTAL );
	m_spinCtrlHandicap = new wxSpinCtrl( sbSizer3->GetStaticBox(), ID_HANDICAPSPIN, wxEmptyString, wxDefaultPosition, wxSize( 150,-1 ), wxSP_ARROW_KEYS, 0, 100, 0 );
	
	sbSizer3->Add( m_spinCtrlHandicap, 0, wxALIGN_CENTER|wxALL, 5 );
	
	m_staticText3 = new wxStaticText( sbSizer3->GetStaticBox(), wxID_ANY, HANDICAP_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	m_staticText3->Wrap( -1 );
	sbSizer3->Add( m_staticText3, 0, wxALIGN_CENTER|wxALL, 5 );
	
	
	bSizer9->Add( sbSizer3, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxStaticBoxSizer* sbSizer5;
	sbSizer5 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, TIME_FOR_GAME_WXSTR[cfg_lang] ), wxHORIZONTAL );
	m_spinCtrlTime = new wxSpinCtrl( sbSizer5->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 150,-1 ), wxSP_ARROW_KEYS, 1, 720, 90 );
	sbSizer5->Add( m_spinCtrlTime, 0, wxALIGN_CENTER|wxALL, 5 );
	
	m_staticText13 = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, MINUTES_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	m_staticText13->Wrap( -1 );
	sbSizer5->Add( m_staticText13, 0, wxALIGN_CENTER|wxALL, 5 );
	
	
	bSizer9->Add( sbSizer5, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	
	bSizer11->Add( bSizer9, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );
	
	wxString m_radioBoxColorChoices[] = { BLACK_UPPER_WXSTR[cfg_lang], WHITE_UPPER_WXSTR[cfg_lang] };
	int m_radioBoxColorNChoices = sizeof( m_radioBoxColorChoices ) / sizeof( wxString );
	m_radioBoxColor = new wxRadioBox( this, wxID_ANY, YOUR_COLOR_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, m_radioBoxColorNChoices, m_radioBoxColorChoices, 1, wxRA_SPECIFY_ROWS );
	m_radioBoxColor->SetSelection( 0 );
	bSizer10->Add( m_radioBoxColor, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxString m_radioBoxLevelChoices[] = { SIMULATE100_WXSTR[cfg_lang], SIMULATE500_WXSTR[cfg_lang], SIMULATE1000_WXSTR[cfg_lang], SIMULATE5000_WXSTR[cfg_lang], SIMULATE10000_WXSTR[cfg_lang], SIMULATE20000_WXSTR[cfg_lang], SIMULATE_UNLIMIT_WXSTR[cfg_lang] };
	int m_radioBoxLevelNChoices = sizeof( m_radioBoxLevelChoices ) / sizeof( wxString );
	m_radioBoxLevel = new wxRadioBox( this, wxID_ANY, ENGINE_MAX_LEVEL_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, m_radioBoxLevelNChoices, m_radioBoxLevelChoices, 1, wxRA_SPECIFY_COLS );
	m_radioBoxLevel->SetSelection( 6 );
	bSizer10->Add( m_radioBoxLevel, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxStaticBoxSizer* sbSizerEngineSettings;
	sbSizerEngineSettings = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, ENGINE_SETTINGS19_WXSTR[cfg_lang] ), wxVERTICAL );
	
	m_checkNeuralNet = new wxCheckBox( sbSizerEngineSettings->GetStaticBox(), wxID_ANY, USE_NET_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, 0 );
	m_checkNeuralNet->SetValue(true); 
	sbSizerEngineSettings->Add( m_checkNeuralNet, 0, wxALL, 5 );
	
	
	bSizer10->Add( sbSizerEngineSettings, 0, wxEXPAND, 5 );
	
	
	bSizer11->Add( bSizer10, 0, wxEXPAND, 5 );
	
	
	bSizer7->Add( bSizer11, 1, wxEXPAND, 5 );
	
	/*
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
		m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL, CANCEL_WXSTR[cfg_lang] );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bSizer7->Add( m_sdbSizer1, 0, wxALIGN_LEFT|wxALL, 2 );
	*/
	wxBoxSizer* bSizer19;
	bSizer19 = new wxBoxSizer( wxHORIZONTAL );
	bSizer19->Add( 0, 0, 1, wxEXPAND, 5 );
	m_sdbSizer1OK = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sdbSizer1OK->SetDefault();
	bSizer19->Add( m_sdbSizer1OK, 0, wxALL|wxEXPAND, 5 );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL, CANCEL_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, 0 );
	bSizer19->Add( m_sdbSizer1Cancel, 0, wxALL|wxEXPAND, 5 );
	bSizer7->Add( bSizer19, 0, wxALIGN_LEFT|wxALL, 2 );
	
	
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

TAboutDialog::TAboutDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, ABOUT_WXSTR[cfg_lang], pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextVersion = new wxStaticText( this, wxID_ANY, _("Leela ") + VERSION_WXSTR[cfg_lang] + wxString(_(" ")) + wxString(wxT(PROGRAM_VERSION)), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	m_staticTextVersion->Wrap( -1 );
	m_staticTextVersion->SetFont( wxFont( 11, 70, 90, 92, false, wxEmptyString ) );
	
	bSizer9->Add( m_staticTextVersion, 0, wxALL|wxEXPAND, 10 );
	
	m_staticTextEngine = new wxStaticText( this, wxID_ANY, ENGINE_NOTE_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
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

TClockAdjustDialog::TClockAdjustDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, ADJUST_CLOCKS_WXSTR[cfg_lang], pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer4;
	sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, BLACK_CLOCK_WXSTR[cfg_lang] ), wxHORIZONTAL );
	
	m_spinCtrlBlackMins = new wxSpinCtrl( sbSizer4->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), wxSP_ARROW_KEYS, 0, 999, 30 );
	sbSizer4->Add( m_spinCtrlBlackMins, 1, wxALL|wxEXPAND, 5 );
	
	m_spinCtrlBlackSecs = new wxSpinCtrl( sbSizer4->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 59, 0 );
	sbSizer4->Add( m_spinCtrlBlackSecs, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizer9->Add( sbSizer4, 1, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer6;
	sbSizer6 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, WHITE_CLOCK_WXSTR[cfg_lang] ), wxHORIZONTAL );
	
	m_spinCtrlWhiteMins = new wxSpinCtrl( sbSizer6->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), wxSP_ARROW_KEYS, 0, 999, 30 );
	sbSizer6->Add( m_spinCtrlWhiteMins, 1, wxALL|wxEXPAND, 5 );
	
	m_spinCtrlWhiteSecs = new wxSpinCtrl( sbSizer6->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 59, 0 );
	sbSizer6->Add( m_spinCtrlWhiteSecs, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizer9->Add( sbSizer6, 1, wxALL|wxEXPAND, 5 );
	
	/*
	m_sdbSizer3 = new wxStdDialogButtonSizer();
	m_sdbSizer3OK = new wxButton( this, wxID_OK );
	m_sdbSizer3->AddButton( m_sdbSizer3OK );
	m_sdbSizer3Cancel = new wxButton( this, wxID_CANCEL, CANCEL_WXSTR[cfg_lang] );
	m_sdbSizer3->AddButton( m_sdbSizer3Cancel );
	m_sdbSizer3->Realize();
	
	bSizer9->Add( m_sdbSizer3, 0, wxALIGN_CENTER|wxALL|wxEXPAND, 5 );
	*/
	wxBoxSizer* bSizer20;
	bSizer20 = new wxBoxSizer( wxHORIZONTAL );
	bSizer20->Add( 0, 0, 1, wxEXPAND, 5 );
	m_sdbSizer3OK = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sdbSizer3OK->SetDefault();
	bSizer20->Add( m_sdbSizer3OK, 0, wxALL|wxEXPAND, 5 );
	m_sdbSizer3Cancel = new wxButton( this, wxID_CANCEL, CANCEL_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, 0 );
	bSizer20->Add( m_sdbSizer3Cancel, 0, wxALL|wxEXPAND, 5 );
	bSizer9->Add( bSizer20, 0, wxALIGN_CENTER|wxALL|wxEXPAND, 5 );
	
	
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

TRatedSizeDialog::TRatedSizeDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, SELECT_BOARD_SIZE_WXSTR[cfg_lang], pos, size, style )
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

TCalculateDialog::TCalculateDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, CALCULATION_WXSTR[cfg_lang], pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextCalculating = new wxStaticText( this, wxID_ANY, CALCULATING_HOLD_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
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

TAnalysisWindow::TAnalysisWindow( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, ANALYSIS_WXSTR[cfg_lang], pos, size, style )
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
	m_moveGrid->SetColLabelValue( 0, MOVE_WXSTR[cfg_lang] );
	m_moveGrid->SetColLabelValue( 1, EFFORT_WXSTR[cfg_lang] + _("%") );
	m_moveGrid->SetColLabelValue( 2, SIMULATIONS_COL_LABEL_WXSTR[cfg_lang] );
	m_moveGrid->SetColLabelValue( 3, WIN_PER_WXSTR[cfg_lang] );
	m_moveGrid->SetColLabelValue( 4, _("Net") + PROB_WXSTR[cfg_lang] + _("%") );
	m_moveGrid->SetColLabelValue( 5, PV_WXSTR[cfg_lang] );
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

TSettingsDialog::TSettingsDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, SETTINGS_WXSTR[cfg_lang], pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxVERTICAL );
	
	m_panel4 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer15;
	bSizer15 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer7;
	sbSizer7 = new wxStaticBoxSizer( new wxStaticBox( m_panel4, wxID_ANY, ENGINE_SETTINGS_WXSTR[cfg_lang] ), wxVERTICAL );
	
	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer( 0, 2, 0, 0 );
	
	m_checkBoxPasses = new wxCheckBox( sbSizer7->GetStaticBox(), ID_PASSTOGGLE, ALLOW_PASSES_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxPasses->SetValue(true); 
	gSizer1->Add( m_checkBoxPasses, 0, wxALL, 5 );
	
	m_checkBoxPondering = new wxCheckBox( sbSizer7->GetStaticBox(), ID_PONDERTOGGLE, PONDERING_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxPondering->SetValue(true); 
	gSizer1->Add( m_checkBoxPondering, 0, wxALL, 5 );
	
	m_checkBoxResignations = new wxCheckBox( sbSizer7->GetStaticBox(), ID_RESIGNTOGGLE, ALLOW_RESIGN_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxResignations->SetValue(true); 
	gSizer1->Add( m_checkBoxResignations, 0, wxALL, 5 );
	
	m_checkBoxNeuralNet = new wxCheckBox( sbSizer7->GetStaticBox(), ID_NETWORKTOGGLE, NEURAL_NETWORKS_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxNeuralNet->SetValue(true); 
	gSizer1->Add( m_checkBoxNeuralNet, 0, wxALL, 5 );
	
	
	sbSizer7->Add( gSizer1, 1, wxEXPAND, 5 );
	
	
	bSizer15->Add( sbSizer7, 0, wxBOTTOM|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer9;
	sbSizer9 = new wxStaticBoxSizer( new wxStaticBox( m_panel4, wxID_ANY, INTERFACE_SETTINGS_WXSTR[cfg_lang] ), wxVERTICAL );
	
	m_checkBoxSound = new wxCheckBox( sbSizer9->GetStaticBox(), ID_SOUNDSWITCH, SOUND_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxSound->SetValue(true); 
	sbSizer9->Add( m_checkBoxSound, 0, wxALL, 5 );
	
	m_checkBoxDPIScaling = new wxCheckBox( sbSizer9->GetStaticBox(), ID_DPISCALING, ALLOW_DPI_SCALING_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer9->Add( m_checkBoxDPIScaling, 0, wxALL, 5 );
	
	m_checkBoxJapanese = new wxCheckBox( sbSizer9->GetStaticBox(), ID_JAPANESE, JAPANESE_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, 0 );
	if (cfg_lang == 0) {
		m_checkBoxJapanese->SetValue(false);
	} else {
		m_checkBoxJapanese->SetValue(true);
	}
	sbSizer9->Add( m_checkBoxJapanese, 0, wxALL, 5 );
	
	
	bSizer15->Add( sbSizer9, 0, wxBOTTOM|wxEXPAND, 5 );
	
	/*
	m_sdbSizer3 = new wxStdDialogButtonSizer();
	m_sdbSizer3OK = new wxButton( m_panel4, wxID_OK );
	m_sdbSizer3->AddButton( m_sdbSizer3OK );
	m_sdbSizer3Cancel = new wxButton( m_panel4, wxID_CANCEL, CANCEL_WXSTR[cfg_lang] );
	m_sdbSizer3->AddButton( m_sdbSizer3Cancel );
	m_sdbSizer3->Realize();
	
	bSizer15->Add( m_sdbSizer3, 0, wxEXPAND, 5 );
	*/
	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxHORIZONTAL );
	bSizer21->Add( 0, 0, 1, wxEXPAND, 5 );
	m_sdbSizer3OK = new wxButton( m_panel4, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sdbSizer3OK->SetDefault();
	bSizer21->Add( m_sdbSizer3OK, 0, wxALL|wxEXPAND, 5 );
	m_sdbSizer3Cancel = new wxButton( m_panel4, wxID_CANCEL, CANCEL_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, 0 );
	bSizer21->Add( m_sdbSizer3Cancel, 0, wxALL|wxEXPAND, 5 );
	bSizer15->Add( bSizer21, 0, wxEXPAND, 5 );
	
	
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

TScoreDialog::TScoreDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, GAME_SCORE_WXSTR[cfg_lang], pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer15;
	bSizer15 = new wxBoxSizer( wxVERTICAL );
	
	m_panel4 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer17;
	bSizer17 = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextScore = new wxStaticText( m_panel4, wxID_ANY, FINAL_SCORE_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextScore->Wrap( -1 );
	bSizer17->Add( m_staticTextScore, 1, wxALL|wxEXPAND, 5 );
	
	m_staticTextMessage = new wxStaticText( m_panel4, wxID_ANY, BLACK_WINS_BY00_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	m_staticTextMessage->Wrap( -1 );
	bSizer17->Add( m_staticTextMessage, 1, wxALL|wxEXPAND, 5 );
	
	m_staticTextConfidence = new wxStaticText( m_panel4, wxID_ANY, SCORED_CORRECTLY_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextConfidence->Wrap( -1 );
	bSizer17->Add( m_staticTextConfidence, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer16->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_buttonOK = new wxButton( m_panel4, ID_SCORE_ACCEPT, ACCEPT_BUTTON_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonOK->SetDefault(); 
	bSizer16->Add( m_buttonOK, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonDispute = new wxButton( m_panel4, ID_SCORE_DISPUTE, DISPUTE_BUTTON_WXSTR[cfg_lang], wxDefaultPosition, wxDefaultSize, 0 );
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

TScoreHistogram::TScoreHistogram( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, WIN_RATE_HISTOGRAM_WXSTR[cfg_lang], pos, size, style )
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
