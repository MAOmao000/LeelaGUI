#include <wx/wx.h>
#include <wx/string.h>

wxString OPENCL_ERR_WXSTR[] = {"Error initializing OpenCL", wxT("エラー OpenCL初期化")};
wxString ERR_WXSTR[] = {"error", wxT("エラー")};
wxString OPENCL_TEST_ERR_WXSTR[] = {_("OpenCL self-test failed. Check your graphics drivers."),
                                               wxT("OpenCLのセルフテストに失敗しました。グラフィックドライバを確認してください。")};
wxString MAC_NOTE_WXSTR[] = {_("The GPU and OpenCL drivers on macOS are often outdated "
                                          "and of poor quality. Try switching to the regular version if "
                                          "you run into stability issues. If your GPU is slow, it may "
                                          "even be faster."),
                                        wxT("macOSのGPUとOpenCLのドライバは、古いものが多く、品質も良くありません。"
                                            "安定性に問題がある場合は、通常バージョンに切り替えてみてください。"
                                            "GPUが遅ければ、もっと速くなるかもしれません。")};
wxString KOMI_WXSTR[] = {_("Komi"), wxT("コミ")};
wxString PRISONERS_WXSTR[] = {_("Prisoners"), wxT("アゲハマ")};
wxString WHITE_WXSTR[] = {_("white"), wxT("白")};
wxString PRISONERS_WHITE_WXSTR[] = {_("white"), wxT("黒")};
wxString BLACK_WXSTR[] = {_("black"), wxT("黒")};
wxString PRISONERS_BLACK_WXSTR[] = {_("black"), wxT("白")};
wxString THINKING_WXSTR[] = {_("ENGINE THINKING..."), wxT("エンジン思考中...")};
wxString ENGINE_ALREADY_WXSTR[] = {_("Engine already running"), wxT("エンジンはすでに稼働しています")};
wxString NEW_MOVE_WXSTR[] = {_("New move arrived"), wxT("新しい着手")};
wxString COMPUTER_PASS_WXSTR[] = {_("Computer passes"), wxT("コンピュータ・パス")};
wxString PASS_WXSTR[] = {_("Pass"), wxT("パス")};
wxString COMPUTER_MOVE_WXSTR[] = {_("Computer to move"), wxT("コンピュータの着手")};
wxString HYPHEN_MOVE_WXSTR[] = {_(" - move "), wxT(" - 手数 ")};
wxString OK_CLICKED_WXSTR[] = {_("OK clicked"), wxT("OKがクリックされました")};
wxString LAST_RANK_WXSTR[] = {_("Last rank was"), wxT("最終ランク")};
wxString YOUR_RANK_WXSTR[] = {_("Your rank"), wxT("あなたのランク")};
wxString HANDICAP_WXSTR[] = {_("Handicap"), wxT("置き石")};
wxString SIMULATIONS_WXSTR[] = {_("Simulations"), wxT("シミュレーション")};
wxString BLACK_WINS_BY_RESIGN_WXSTR[] = {_("BLACK wins by resignation"), wxT("白が投了しました")};
wxString BLACK_WINS_BY_WXSTR[] = {_("BLACK wins by"), wxT("黒地")};
wxString POINTS_WXSTR[] = {_("points"), wxT("目")};
wxString WHITE_WINS_BY_RESIGN_WXSTR[] = {_("WHITE wins by resignation"), wxT("黒が投了しました")};
wxString WHITE_WINS_BY_WXSTR[] = {_("WHITE wins by"), wxT("白地")};
wxString NOT_SURE_SCORE_WXSTR[] = {_("I am not sure I am scoring this correctly."), wxT("この地合計算は曖昧です")};
wxString KYU_WXSTR[] = {_("kyu"), wxT("級")};
wxString DAN_WXSTR[] = {_("dan"), wxT("段")};
wxString PRO_WXSTR[] = {_("pro"), wxT("プロ")};
wxString UNDO_ONE_WXSTR[] = {_("Undoing one move"), wxT("一手戻す")};
wxString FORWARD_ONE_WXSTR[] = {_("Forward one move"), wxT("一手進める")};
wxString READ_WXSTR[] = {_("Read"), wxT("読み込み")};
wxString MOVES_COMMA_WXSTR[] = {_("moves,"), wxT("手,")};
wxString GOING_TO_MOVE_WXSTR[] = {_(" going to move"), wxT("")};
wxString TO_MOVE_WXSTR[] = {_(""), wxT(" 手へ")};
wxString CHOOSE_FILE_WXSTR[] = {_("Choose a file"), wxT("ファイルを選択")};
wxString OPENING_WXSTR[] = {_("Opening"), wxT("布石")};
wxString SAVING_WXSTR[] = {_("Saving"), wxT("保存中")};
wxString ADJUST_CLOCK_WXSTR[] = {_("Adjust clocks clicked"), wxT("時間調整がクリックされました")};

wxString VERSION_WXSTR[] = {_("version"), wxT("バージョン")};

wxString MENU_NEW_GAME_WXSTR[] = {_("&New Game..."), wxT("新規対局...(&N)")};
wxString MENU_START_GAME_WXSTR[] = {_("Start a new game"), wxT("新規対局開始")};
wxString MENU_NEW_RATED_GAME_WXSTR[] = {_("New &Rated Game"), wxT("新規レート対局(&R)")};
wxString MENU_STARTS_RATED_GAME_WXSTR[] = {_("Starts a new rated game"), wxT("新規レート対局開始")};
wxString MENU_SET_RATED_SIZE_WXSTR[] = {_("Set Rated &Board Size..."), wxT("レート対局碁盤サイズ設定...(&B)")};
wxString MENU_CHANGE_SIZE_RATED_WXSTR[] = {_("Change the board size for rated games"), wxT("レート対局碁盤サイズ変更")};
wxString MENU_OPEN_GAME_WXSTR[] = {_("&Open Game..."), wxT("開く...(&O)")};
wxString MENU_OPENS_GAME_FROM_WXSTR[] = {_("Opens a game from disk"), wxT("ディスクからSGFを開く")};
wxString MENU_SAVE_GAME_WXSTR[] = {_("&Save Game..."), wxT("保存...(&S)")};
wxString MENU_SAVE_GAME_TO_WXSTR[] = {_("Save a game to disk"), wxT("ディスクにSGFを保存")};
wxString MENU_COPY_SGF_WXSTR[] = {_("Copy SGF"), wxT("SGFコピー")};
wxString MENU_COPY_CLIPBOARD_WXSTR[] = {_("Copy SGF to clipboard"), wxT("クリップボードにSGFコピー")};
wxString MENU_PASTE_SGF_WXSTR[] = {_("Paste SGF"), wxT("SGF貼り付け")};
wxString MENU_PASTE_CLIPBOARD_WXSTR[] = {_("Paste SGF from clipboard"), wxT("クリップボードからSGF貼り付け")};
wxString MENU_EXIT_WXSTR[] = {_("E&xit"), wxT("終了(&x)")};
wxString MENU_EXIT_PROGRAM_WXSTR[] = {_("Exit the program"), wxT("プログラムを終了")};
wxString MENU_FILE_WXSTR[] = {_("&File"), wxT("ファイル(&F)")};

wxString MENU_UNDO_WXSTR[] = {_("&Undo"), wxT("1手戻る(&U)")};
wxString MENU_FORWARD_WXSTR[] = {_("&Forward"), wxT("1手進む(&F)")};
wxString MENU_BACK10_WXSTR[] = {_("Go &Back 10 Moves"), wxT("10手戻る(&B)")};
wxString MENU_BACK10_NO_KEY_WXSTR[] = {_("Undo 10 moves"), wxT("10手戻る")};
wxString MENU_FORWARD10_WXSTR[] = {_("Go For&ward 10 Moves"), wxT("10手進む(&w)")};
wxString MENU_FORWARD10_NO_KEY_WXSTR[] = {_("Forward 10 moves"), wxT("10手進む")};
wxString MENU_FORCE_COMPUTER_MOVE_WXSTR[] = {_("Force &Computer Move"), wxT("コンピュータに着手催促(&C)")};
wxString MENU_FORCE_COMPUTER_NO_KEY_WXSTR[] = {_("Force the computer to move now"), wxT("コンピュータに着手催促")};
wxString MENU_PASS_WXSTR[] = {_("&Pass"), wxT("パス(&P)")};
wxString MENU_RESIGN_WXSTR[] = {_("&Resign"), wxT("投了(&R)")};
wxString MENU_RESIGN_NO_KEY_WXSTR[] = {_("Resign the game"), wxT("投了する")};
wxString MENU_SCORE_GAME_WXSTR[] = {_("&Score Game"), wxT("地合計算(&S)")};
wxString MENU_SCORE_GAME_NO_KEY_WXSTR[] = {_("Score the position"), wxT("地合計算")};
wxString MENU_GAME_WXSTR[] = {_("&Game"), wxT("対局(&G)")};

wxString MENU_ANALYZE_WXSTR[] = {_("Start/Stop &Analysis"), wxT("検討開始・停止(&A)")};
wxString MENU_ANALYZE_NO_KEY_WXSTR[] = {_("Start analyzing"), wxT("検討開始")};
wxString MENU_PUSH_POS_WXSTR[] = {_("&Store Position (Push)"), wxT("局面保存(Push)(&S)")};
wxString MENU_PUSH_POS_NO_KEY_WXSTR[] = {_("Store this position on the stack"), wxT("局面をスタック")};
wxString MENU_POP_POS_WXSTR[] = {_("&Forget Position (Pop)"), wxT("局面破棄(Pop)(&F)")};
wxString MENU_POP_POS_NO_KEY_WXSTR[] = {_("Remove the top position from the stack"), wxT("スタック局面を破棄")};
wxString MENU_REVERT_STORE_WXSTR[] = {_("R&evert to Stored"), wxT("局面復元(&e)")};
wxString MENU_REVERT_STORE_NO_KEY_WXSTR[] = {_("Revert board position to the top of the stack"), wxT("保存位置に戻す")};
wxString MENU_ANALYSIS_WINDOW_WXSTR[] = {_("Analysis &Window"), wxT("検討ウィンドウ(&W)")};
wxString MENU_SCORE_WINDOW_WXSTR[] = {_("Show &Histogram"), wxT("グラフ表示(&H)")};
wxString MENU_ANALYZE_TOP_WXSTR[] = {_("&Analyze"), wxT("検討(&A)")};

wxString MENU_MOVE_PROB_WXSTR[] = {_("Show Network &Probabilities"), wxT("候補手確率表示(&P)")};
wxString MENU_MOVE_PROB_NO_KEY_WXSTR[] = {_("Show the likelihood of each move being played by a professional player"), wxT("各着手がプロの棋士によって指される可能性を示す")};
wxString MENU_BEST_MOVES_WXSTR[] = {_("Show &Best Moves"), wxT("最善手表示(&B)")};
wxString MENU_BEST_MOVES_NO_KEY_WXSTR[] = {_("Color the best moves on the board"), wxT("最善手")};
wxString MENU_SHOW_TERRITORY_WXSTR[] = {_("Show &Territory"), wxT("地合表示(&T)")};
wxString MENU_SHOW_TERRITORY_NO_KEY_WXSTR[] = {_("Show Territory"), wxT("地合表示")};
wxString MENU_SHOW_MOYO_WXSTR[] = {_("Show &Moyo"), wxT("模様表示(&M)")};
wxString MENU_SHOW_MOYO_NO_KEY_WXSTR[] = {_("Show Moyo"), wxT("模様表示")};
wxString MENU_PREFERENCES_WXSTR[] = {_("S&ettings..."), wxT("設定...(&e)")};
wxString MENU_ADJUST_CLOCKS_WXSTR[] = {_("&Adjust Clocks..."), wxT("持ち時間調整...(&A)")};
wxString MENU_TOOLS_WXSTR[] = {_("&Tools"), wxT("ツール(&T)")};

wxString MENU_HELP_RULES_WXSTR[] = {_("&Go Rules"), wxT("囲碁のルール(&G)")};
wxString MENU_HELP_RULES_NO_KEY_WXSTR[] = {_("Explanation of the rules of Go"), wxT("囲碁のルールの説明")};
wxString MENU_HOME_PAGE_WXSTR[] = {_("Leela Homepage"), wxT("Leelaホームページ")};
wxString MENU_ABOUT_WXSTR[] = {_("&About..."), wxT("Leelaについて...(&A)")};
wxString MENU_ABOUT_NO_KEY_WXSTR[] = {_("Information about the program"), wxT("Leelaについての情報")};
wxString MENU_HELP_WXSTR[] = {_("&Help"), wxT("ヘルプ(&H)")};

wxString TBAR_NEW_GAME_WXSTR[] = {_("New Game"), wxT("新規対局")};
wxString TBAR_NEW_GAME_EXP_WXSTR[] = {_("start a new game"), wxT("新規対局開始")};
wxString TBAR_NEW_RATED_WXSTR[] = {_("New Rated Game"), wxT("新規レート対局")};
wxString TBAR_NEW_RATED_EXP_WXSTR[] = {_("Start a new rated game"), wxT("新規レート対局開始")};
wxString TBAR_LOAD_GAME_WXSTR[] = {_("Load Game"), wxT("対局結果読み込み")};
wxString TBAR_LOAD_GAME_EXP_WXSTR[] = {_("Load a game"), wxT("対局結果を読み込む")};
wxString TBAR_SAVE_GAME_WXSTR[] = {_("Save"), wxT("対局結果保存")};
wxString TBAR_SAVE_GAME_EXP_WXSTR[] = {_("Save the game"), wxT("対局結果を保存する")};
wxString TBAR_BACK10_WXSTR[] = {_("Go back 10 moves"), wxT("10手戻る")};
wxString TBAR_BACK10_EXP_WXSTR[] = {_("Go back 10 moves"), wxT("10手戻る")};
wxString TBAR_UNDO_WXSTR[] = {_("Undo"), wxT("1手戻る")};
wxString TBAR_UNDO_EXP_WXSTR[] = {_("Take back one move"), wxT("1手戻る")};
wxString TBAR_REDO_WXSTR[] = {_("Forward"), wxT("1手進む")};
wxString TBAR_REDO_EXP_WXSTR[] = {_("Go forward one move"), wxT("1手進む")};
wxString TBAR_FORWARD10_WXSTR[] = {_("Forward 10"), wxT("10手進む")};
wxString TBAR_FORWARD10_EXP_WXSTR[] = {_("Go forward 10 moves"), wxT("10手進む")};
wxString TBAR_FORCE_COMPUTER_MOVE_WXSTR[] = {_("Force computer move"), wxT("コンピュータに着手催促")};
wxString TBAR_FORCE_COMPUTER_MOVE_EXP_WXSTR[] = {_("Force computer move"), wxT("コンピュータに着手催促")};
wxString TBAR_STORE_POSITION_WXSTR[] = {_("Store Position"), wxT("局面保存")};
wxString TBAR_STORE_POSITION_EXP_WXSTR[] = {_("Remember this position on the stack"), wxT("スタック局面を覚える")};
wxString TBAR_REMOVE_POSITION_WXSTR[] = {_("Remove Position"), wxT("局面破棄")};
wxString TBAR_REMOVE_POSITION_EXP_WXSTR[] = {_("Remove the top position from the stack"), wxT("スタック局面を破棄")};
wxString TBAR_REVERT_WXSTR[] = {_("Revert to mainline"), wxT("局面復元")};
wxString TBAR_REVERT_EXP_WXSTR[] = {_("Revert to stored position"), wxT("保存位置に戻す")};
wxString TBAR_TOOL_WXSTR[] = {_("tool"), wxT("ツール")};
wxString TBAR_TOOL_1_EXP_WXSTR[] = {_("Start/Stop analysis"), wxT("検討開始・停止")};
wxString TBAR_TOOL_2_EXP_WXSTR[] = {_("Pass"), wxT("パス")};
wxString TBAR_TOOL_3_EXP_WXSTR[] = {_("Score the game"), wxT("地合計算")};
wxString TBAR_TOOL_4_EXP_WXSTR[] = {_("Resign the game"), wxT("投了")};
wxString TBAR_EXIT_WXSTR[] = {_("Exit"), wxT("終了")};
wxString TBAR_EXIT_EXP_WXSTR[] = {_("Exit the program"), wxT("プログラムを終了")};

wxString BOARD_SIZE_WXSTR[] = {_("Board size"), wxT("碁盤サイズ")};
wxString KOMI_0_5_WXSTR[] = {_("Komi (+0.5)"), wxT("コミ (+0.5)")};
wxString TIME_FOR_GAME_WXSTR[] = {_("Time for game"), wxT("持ち時間")};
wxString MINUTES_WXSTR[] = {_("Minutes"), wxT("分")};
wxString BLACK_UPPER_WXSTR[] = {_("Black"), wxT("黒")};
wxString WHITE_UPPER_WXSTR[] = {_("White"), wxT("白")};
wxString YOUR_COLOR_WXSTR[] = {_("Your color"), wxT("手番")};
wxString SIMULATE100_WXSTR[] = {_("100 simulations"), wxT("100回試行")};
wxString SIMULATE500_WXSTR[] = {_("500 simulations"), wxT("500回試行")};
wxString SIMULATE1000_WXSTR[] = {_("1000 simulations"), wxT("1,000回試行")};
wxString SIMULATE5000_WXSTR[] = {_("5000 simulations"), wxT("5,000回試行")};
wxString SIMULATE10000_WXSTR[] = {_("10000 simulations"), wxT("10,000回試行")};
wxString SIMULATE20000_WXSTR[] = {_("20000 simulations"), wxT("20,000回試行")};
wxString SIMULATE_UNLIMIT_WXSTR[] = {_("Unlimited"), wxT("無制限")};
wxString ENGINE_MAX_LEVEL_WXSTR[] = {_("Engine max level"), wxT("エンジン最大レベル")};
wxString ENGINE_SETTINGS19_WXSTR[] = {_("Engine settings (19 x 19)"), wxT("エンジン設定(19 x 19)")};
wxString USE_NET_WXSTR[] = {_("Use Neural Network"), wxT("ニューラルネットワークを使う")};
wxString CANCEL_WXSTR[] = {_("Cancel"), wxT("取消")};
wxString ENGINE_NOTE_WXSTR[] = {_("OpenCL/BLAS Engine goes here and can be a very long string"), wxT("OpenCL/BLASエンジンは長時間かかります")};
wxString BLACK_CLOCK_WXSTR[] = {_("Black clock (minutes:seconds)"), wxT("黒時間(分:秒)")};
wxString WHITE_CLOCK_WXSTR[] = {_("White clock (minutes:seconds)"), wxT("白時間(分:秒)")};
wxString CALCULATING_HOLD_WXSTR[] = {_("Calculating, please hold..."), wxT("計算中、お待ちください...")};

wxString EFFORT_WXSTR[] = {_("Effort"), wxT("探索比率")};
wxString SIMULATIONS_COL_LABEL_WXSTR[] = {_("Simulations"), wxT("試行回数")};
wxString PROB_WXSTR[] = {_("Prob"), wxT("確率")};

wxString ENGINE_SETTINGS_WXSTR[] = {_("Engine Settings"), wxT("エンジン設定")};
wxString ALLOW_PASSES_WXSTR[] = {_("Allow Passes"), wxT("パスを許可")};
wxString PONDERING_WXSTR[] = {_("PONDERING"), wxT("相手手番中も先読みする")};
wxString ALLOW_RESIGN_WXSTR[] = {_("Resignations"), wxT("投了を許可")};
wxString NEURAL_NETWORKS_WXSTR[] = {_("Neural Networks"), wxT("ニューラルネットワーク")};
wxString INTERFACE_SETTINGS_WXSTR[] = {_("Interface Settings"), wxT("インタフェース設定")};
wxString SOUND_WXSTR[] = {_("Sound"), wxT("音")};
wxString ALLOW_DPI_SCALING_WXSTR[] = {_("Allow DPI Scaling (needs restart)"), wxT("画面のDPIスケーリングを許可(要再起動)")};
wxString JAPANESE_WXSTR[] = {_("Japanese"), wxT("日本語")};

wxString FINAL_SCORE_WXSTR[] = {_("Final Score:"), wxT("最終地合:")};
wxString BLACK_WINS_BY00_WXSTR[] = {_("Black wins by 0.0"), wxT("黒は0.0目で勝ちます")};
wxString SCORED_CORRECTLY_WXSTR[] = {_("I'm fairly sure I scored this correctly."), wxT("正しく採点したとかなり確信しています")};
wxString ACCEPT_BUTTON_WXSTR[] = {_("&Accept"), wxT("受け入れる(&A)")};
wxString DISPUTE_BUTTON_WXSTR[] = {_("&Dispute Score"), wxT("スコアを競う(&D)")};

wxString MOVE_WXSTR[] = {_T("Move"), wxT("着手")};
wxString PV_WXSTR[] = {_("PV"), wxT("予想手順")};
wxString PV_T_WXSTR[] = {_T("PV"), wxT("予想手順")};
wxString WIN_PER_WXSTR[] = {_T("Win%"), wxT("勝率%")};
wxString ANALYSIS_SCORE_WXSTR[] = {_("Analysis - Score Estimate "), wxT("検討 - 地合見積り ")};
wxString COPY_PV_WXSTR[] = {"Copy PV", wxT("予想手順コピー")};
wxString COPY_ENTIRE_WXSTR[] = {"Copy entire line", wxT("行全体をコピー")};
wxString DESELECT_LINE_WXSTR[] = {"Deselect line", wxT("行選択を解除")};

wxString NEW_GAME_WXSTR[] = {_("New Game"), wxT("新規対局")};
wxString ABOUT_WXSTR[] = {_("About"), wxT("Leelaについて")};
wxString ADJUST_CLOCKS_WXSTR[] = {_("Adjust clocks"), wxT("持ち時間調整")};
wxString SELECT_BOARD_SIZE_WXSTR[] = {_("Calculation in progress"), wxT("碁盤サイズ選択")};
wxString CALCULATION_WXSTR[] = {_("Calculation in progress"), wxT("計算")};
wxString ANALYSIS_WXSTR[] = {_("Analysis"), wxT("検討")};
wxString SETTINGS_WXSTR[] = {_("Settings"), wxT("設定")};
wxString GAME_SCORE_WXSTR[] = {_("Game Score"), wxT("地合計算")};

wxString WIN_RATE_HISTOGRAM_WXSTR[] = {_("Win Rate Histogram"), wxT("勝率グラフ")};
wxString WHITE_T_WXSTR[] = {_T("WHITE"), wxT("白")};
wxString BLACK_T_WXSTR[] = {_T("BLACK"), wxT("黒")};
wxString COMBINED_T_WXSTR[] = {_T("Combined"), wxT("複合")};
wxString MONTE_CARLO_T_WXSTR[] = {_T("Monte Carlo"), wxT("モンテカルロ")};
wxString NETWORK_T_WXSTR[] = {_T("Network"), wxT("ネットワーク")};

wxString WIDTH_WXSTR[] = {"width", wxT("幅")};
wxString HEIGHT_WXSTR[] = {"height", wxT("高さ")};
wxString CELL_SIZE_WXSTR[] = {"cell size", wxT("セルサイズ")};
wxString PAINT_ON_EMPTY_WXSTR[] = {"Paint on empty state", wxT("空の状態を描画")};
wxString LEFT_DOWN_WXSTR[] = {"Left down at", wxT("左クリック")};
wxString CLICK_ON_EMPTY_WXSTR[] = {"Click on empty board", wxT("空きボードクリック")};
wxString GAME_RESIGNED_WXSTR[] = {"Game has been resigned", wxT("対局は終了しました")};
wxString CLICK_ON_LOCKED_WXSTR[] = {"Click on locked state", wxT("ロック状態でのクリック")};
wxString NOT_YOUR_MOVE_WXSTR[] = {"It's not your move!", wxT("あなたの手番ではありません！")};

std::string MOVE_STR[] = {"Move", "着手"};
std::string EFFORT_STR[] = {"Effort", "探索比率"};
std::string SIMULATIONS_STR[] = {"Simulations", "試行回数"};
std::string WIN_STR[] = {"Win", "勝率"};
std::string PROB_STR[] = {"Prob", "確率"};
std::string EVAL_STR[] = {"Eval", "評価値"};
std::string PV_STR[] = {"PV", "予想手順"};
std::string NODES_STR[] = {"nodes", "ノード"};
std::string NODES_UPPER_STR[] = {"Nodes", "ノード"};
std::string SEARCHED_STR[] = {"searched", "探索"};
std::string THINKING_AT_STR[] = {"Thinking at most ", "最大"};
std::string THINKING_SEC_STR[] = {" seconds...", "秒で思考中..."};
std::string THINKING_STR[] = {"Thinking...", "思考中..."};
std::string VISITS_STR[] = {"visits", "訪問"};
std::string PLAYOUTS_STR[] = {"playouts", "プレイアウト"};
std::string PLAYOUTS_PER_STR[] = {"p/s", "プレイアウト/秒"};
std::string BEST_MOVE_STR[] = {"Best move", "最善手"};
