#include <vector>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cctype>
#include <string>
#include <sstream>
#include <cmath>
#include <climits>
#include <algorithm>

#include "config.h"
#include "Utils.h"
#include "GameState.h"
#include "GTP.h"
#include "Playout.h"
#include "UCTSearch.h"
#include "UCTNode.h"
#include "SGFTree.h"
#include "AttribScores.h"
#include "PNSearch.h"
#include "Network.h"
#include "Book.h"
#include "TTable.h"
#include "MCPolicy.h"

using namespace Utils;

// Configuration flags
bool cfg_allow_pondering;
bool cfg_allow_book;
int cfg_num_threads;
int cfg_max_playouts;
bool cfg_enable_nets;
bool cfg_komi_adjust;
int cfg_mature_threshold;
int cfg_expand_threshold;
int cfg_lagbuffer_cs;
#ifdef USE_OPENCL
std::vector<int> cfg_gpus;
int cfg_rowtiles;
#endif
float cfg_bound;
float cfg_fpu;
float cfg_cutoff_offset;
float cfg_cutoff_ratio;
float cfg_puct;
float cfg_uct;
float cfg_psa;
float cfg_softmax_temp;
float cfg_mix_opening;
float cfg_mix_ending;
float cfg_mc_softmax;
int cfg_eval_thresh;
float cfg_beta;
float cfg_patternbonus;
int cfg_rave_moves;
int cfg_extra_symmetry;
int cfg_random_loops;
std::string cfg_logfile;
FILE* cfg_logfile_handle;
bool cfg_quiet;

void GTP::setup_default_parameters() {
    cfg_allow_pondering = true;
    cfg_allow_book = true;
    cfg_num_threads = std::max(1, std::min(SMP::get_num_cpus(), MAX_CPUS));
    cfg_enable_nets = true;
    cfg_komi_adjust = false;
#ifdef USE_OPENCL
    cfg_mature_threshold = 25;
    cfg_extra_symmetry =  350;
    cfg_eval_thresh = 2;
#else
    cfg_mature_threshold = 85;
    cfg_extra_symmetry = 3000;
    cfg_eval_thresh = 9;
#endif
    cfg_max_playouts = INT_MAX;
    cfg_lagbuffer_cs = 100;
#ifdef USE_OPENCL
    cfg_gpus = { };
    cfg_rowtiles = 5;
#endif
    cfg_bound = 32.0f;
    cfg_fpu = 1.1f;
    cfg_puct = 1.1f;
    cfg_psa = 0.0018f;
#ifdef USE_SEARCH
    cfg_softmax_temp = 0.75f;
#else
    cfg_softmax_temp = 0.094f;
#endif
    cfg_cutoff_offset = 25.44f;
    cfg_cutoff_ratio = 4.72f;
    cfg_mix_opening = 0.66f;
    cfg_mix_ending = 0.49f;
    cfg_rave_moves = 10;
    cfg_mc_softmax = 1.0f;
    cfg_random_loops = 4;
    cfg_logfile_handle = nullptr;
    cfg_quiet = false;
}

bool GTP::perform_self_test(GameState & state) {
    bool testPassed = true;
#ifdef USE_OPENCL
#ifndef USE_TUNER
    myprintf("OpenCL self-test: ");
    // Perform self-test
    auto vec = Network::get_Network()->get_scored_moves(
        &state, Network::Ensemble::DIRECT, 0);
    testPassed &= vec[60].first > 0.176 && vec[60].first < 0.177;
    testPassed &= vec[60].second == 88;
    testPassed &= vec[72].first > 0.175 && vec[72].first < 0.176;
    testPassed &= vec[72].second == 100;
    if (testPassed) {
        myprintf("passed.\n");
    } else {
        myprintf("failed. Check your OpenCL drivers.\n");
    }
#endif
#endif
    return testPassed;
}

const std::string GTP::s_commands[] = {
    "protocol_version",
    "name",
    "version",
    "quit",
    "known_command",
    "list_commands",
    "quit",
    "boardsize",
    "clear_board",
    "komi",
    "play",
    "genmove",
    "showboard",
    "undo",
    "final_score",
    "final_status_list",
    "time_settings",
    "time_left",
    "fixed_handicap",
    "place_free_handicap",
    "set_free_handicap",
    "loadsgf",
    "printsgf",
    "kgs-genmove_cleanup",
    "kgs-time_settings",
    "kgs-game_over",
    "influence",
    "mc_score",
    "mc_winrate",
    "vn_winrate",
    "winrate",
    "heatmap",
    ""
};

std::string GTP::get_life_list(GameState & game, bool live) {
    std::vector<std::string> stringlist;
    std::string result;
    FastBoard & board = game.board;

    std::vector<bool> dead = game.mark_dead();

    for (int i = 0; i < board.get_boardsize(); i++) {
        for (int j = 0; j < board.get_boardsize(); j++) {
            int vertex = board.get_vertex(i, j);

            if (board.get_square(vertex) != FastBoard::EMPTY) {
                if (live ^ dead[vertex]) {
                    stringlist.push_back(board.get_string(vertex));
                }
            }
        }
    }

    // remove multiple mentions of the same string
    // unique reorders and returns new iterator, erase actually deletes
    std::sort(stringlist.begin(), stringlist.end());
    stringlist.erase(std::unique(stringlist.begin(), stringlist.end()),
                     stringlist.end());

    for (size_t i = 0; i < stringlist.size(); i++) {
        result += (i == 0 ? "" : "\n") + stringlist[i];
    }

    return result;
}

bool GTP::execute(GameState & game, std::string xinput) {
    std::string input;

    bool transform_lowercase = true;

    // Required on Unixy systems
    if (xinput.find("loadsgf") != std::string::npos) {
        transform_lowercase = false;
    }

    /* eat empty lines, simple preprocessing, lower case */
    for (unsigned int tmp = 0; tmp < xinput.size(); tmp++) {
        if (xinput[tmp] == 9) {
            input += " ";
        } else if ((xinput[tmp] > 0 && xinput[tmp] <= 9)
	        || (xinput[tmp] >= 11 && xinput[tmp] <= 31)
	        || xinput[tmp] == 127) {
	       continue;
        } else {
            if (transform_lowercase) {
                input += std::tolower(xinput[tmp]);
            } else {
                input += xinput[tmp];
            }
        }

        // eat multi whitespace
        if (input.size() > 1) {
            if (std::isspace(input[input.size() - 2]) &&
                std::isspace(input[input.size() - 1])) {
                input.resize(input.size() - 1);
            }
        }
    }

    std::string command;
    int id = -1;

    if (input == "") {
        return true;
    } else if (input == "exit") {
        exit(EXIT_SUCCESS);
        return true;
    } else if (input == "#") {
        return true;
    } else if (std::isdigit(input[0])) {
        std::istringstream strm(input);
        char spacer;
        strm >> id;
        strm >> std::noskipws >> spacer;
        std::getline(strm, command);
    } else {
        command = input;
    }

    /* process commands */
    if (command == "protocol_version") {
        gtp_printf(id, "%d", GTP_VERSION);
        return true;
    } else if (command == "name") {
        gtp_printf(id, PROGRAM_NAME);
        return true;
    } else if (command == "version") {
        gtp_printf(id, PROGRAM_VERSION);
        return true;
    } else if (command == "quit") {
        gtp_printf(id, "");
        exit(EXIT_SUCCESS);
    } else if (command.find("known_command") == 0) {
        std::istringstream cmdstream(command);
        std::string tmp;

        cmdstream >> tmp;     /* remove known_command */
        cmdstream >> tmp;

        for (int i = 0; s_commands[i].size() > 0; i++) {
            if (tmp == s_commands[i]) {
                gtp_printf(id, "true");
                return 1;
            }
        }

        gtp_printf(id, "false");
        return true;
    } else if (command.find("list_commands") == 0) {
        std::string outtmp(s_commands[0]);
        for (int i = 1; s_commands[i].size() > 0; i++) {
            outtmp = outtmp + "\n" + s_commands[i];
        }
        gtp_printf(id, outtmp.c_str());
        return true;
    } else if (command.find("boardsize") == 0) {
        std::istringstream cmdstream(command);
        std::string stmp;
        int tmp;

        cmdstream >> stmp;  // eat boardsize
        cmdstream >> tmp;

        if (!cmdstream.fail()) {
            if (tmp < 2 || tmp > FastBoard::MAXBOARDSIZE) {
                gtp_fail_printf(id, "unacceptable size");
            } else {
                float old_komi = game.get_komi();
                game.init_game(tmp, old_komi);
                gtp_printf(id, "");
            }
        } else {
            gtp_fail_printf(id, "syntax not understood");
        }

        return true;
    } else if (command.find("clear_board") == 0) {
        game.reset_game();
        gtp_printf(id, "");
        return true;
    } else if (command.find("komi") == 0) {
        std::istringstream cmdstream(command);
        std::string tmp;
        float komi = 7.5f;
        float old_komi = game.get_komi();

        cmdstream >> tmp;  // eat komi
        cmdstream >> komi;

        if (!cmdstream.fail()) {
            if (komi != old_komi) {
                game.set_komi(komi);
            }
            gtp_printf(id, "");
        } else {
            gtp_fail_printf(id, "syntax not understood");
        }

        return true;
    } else if (command.find("play") == 0) {
        if (command.find("pass") != std::string::npos
            || command.find("resign") != std::string::npos) {
            game.play_pass();
            gtp_printf(id, "");
        } else {
            std::istringstream cmdstream(command);
            std::string tmp;
            std::string color, vertex;

            cmdstream >> tmp;   //eat play
            cmdstream >> color;
            cmdstream >> vertex;

            if (!cmdstream.fail()) {
                if (!game.play_textmove(color, vertex)) {
                    gtp_fail_printf(id, "illegal move");
                } else {
                    gtp_printf(id, "");
                }
            } else {
                gtp_fail_printf(id, "syntax not understood");
            }
        }
        return true;
    } else if (command.find("genmove") == 0) {
        std::istringstream cmdstream(command);
        std::string tmp;

        cmdstream >> tmp;  // eat genmove
        cmdstream >> tmp;

        if (!cmdstream.fail()) {
            int who;
            if (tmp == "w" || tmp == "white") {
                who = FastBoard::WHITE;
            } else if (tmp == "b" || tmp == "black") {
                who = FastBoard::BLACK;
            } else {
                gtp_fail_printf(id, "syntax error");
                return 1;
            }
            float old_komi = game.get_komi();
            float new_komi = old_komi;
            if (cfg_komi_adjust) {
                if (who == FastBoard::BLACK) {
                    new_komi = old_komi + 1.0f;
                } else {
                    new_komi = old_komi - 1.0f;
                }
            }
            game.set_komi(new_komi);

            // start thinking
            {
                std::unique_ptr<UCTSearch> search(new UCTSearch(game));

                int move = search->think(who);
                game.play_move(who, move);

                std::string vertex = game.move_to_text(move);
                gtp_printf(id, "%s", vertex.c_str());
            }
            if (cfg_allow_pondering) {
                // now start pondering
                if (game.get_last_move() != FastBoard::RESIGN) {
                    std::unique_ptr<UCTSearch> search(new UCTSearch(game));
                    search->ponder();
                }
            }
            game.set_komi(old_komi);
        } else {
            gtp_fail_printf(id, "syntax not understood");
        }
        return true;
    } else if (command.find("kgs-genmove_cleanup") == 0) {
        std::istringstream cmdstream(command);
        std::string tmp;

        cmdstream >> tmp;  // eat kgs-genmove
        cmdstream >> tmp;

        if (!cmdstream.fail()) {
            int who;
            if (tmp == "w" || tmp == "white") {
                who = FastBoard::WHITE;
            } else if (tmp == "b" || tmp == "black") {
                who = FastBoard::BLACK;
            } else {
                gtp_fail_printf(id, "syntax error");
                return 1;
            }
            float old_komi = game.get_komi();
            float new_komi = old_komi;
            if (cfg_komi_adjust) {
                if (who == FastBoard::BLACK) {
                    new_komi = old_komi + 1.0f;
                } else {
                    new_komi = old_komi - 1.0f;
                }
            }
            game.set_komi(new_komi);
            game.set_passes(0);

            {
                std::unique_ptr<UCTSearch> search(new UCTSearch(game));

                int move = search->think(who, UCTSearch::NOPASS);
                game.play_move(who, move);

                std::string vertex = game.move_to_text(move);
                gtp_printf(id, "%s", vertex.c_str());
            }
            if (cfg_allow_pondering) {
                // now start pondering
                if (game.get_last_move() != FastBoard::RESIGN) {
                    std::unique_ptr<UCTSearch> search(new UCTSearch(game));
                    search->ponder();
                }
            }
            game.set_komi(old_komi);
        } else {
            gtp_fail_printf(id, "syntax not understood");
        }
        return true;
    } else if (command.find("undo") == 0) {
        if (game.undo_move()) {
            gtp_printf(id, "");
        } else {
            gtp_fail_printf(id, "cannot undo");
        }
        return true;
    } else if (command.find("showboard") == 0) {
        gtp_printf(id, "");
        game.display_state();
        return true;
    } else if (command.find("mc_score") == 0) {
        float ftmp = game.board.final_mc_score(game.get_komi());
        /* white wins */
        if (ftmp < -0.1) {
            gtp_printf(id, "W+%3.1f", (float)fabs(ftmp));
        } else if (ftmp > 0.1) {
            gtp_printf(id, "B+%3.1f", ftmp);
        } else {
            gtp_printf(id, "0");
        }
        return true;
    } else if (command.find("final_score") == 0) {
        float ftmp = game.final_score();
        /* white wins */
        if (ftmp < -0.1) {
            gtp_printf(id, "W+%3.1f", (float)fabs(ftmp));
        } else if (ftmp > 0.1) {
            gtp_printf(id, "B+%3.1f", ftmp);
        } else {
            gtp_printf(id, "0");
        }
        return true;
    } else if (command.find("vn_winrate") == 0) {
        float net_score = Network::get_Network()->get_value(&game,
                                                            Network::Ensemble::AVERAGE_ALL);
        gtp_printf(id, "%f", net_score);
        return true;
    } else if (command.find("mc_winrate") == 0) {
        float mc_winrate = Playout::mc_owner(game, 512);
        gtp_printf(id, "%f", mc_winrate);
        return true;
    } else if (command.find("winrate") == 0) {
        float mc_winrate = Playout::mc_owner(game, 512);
        float net_score = Network::get_Network()->get_value(&game,
                                                            Network::Ensemble::AVERAGE_ALL);
        float comb_winrate = UCTNode::score_mix_function(game.get_movenum(),
                                                         net_score, mc_winrate);
        gtp_printf(id, "%f", comb_winrate);
        return true;
    } else if (command.find("winrate") == 0) {
        float mc_winrate = Playout::mc_owner(game, 512);
        float net_score = Network::get_Network()->get_value(&game,
                                                            Network::Ensemble::AVERAGE_ALL);
        float comb_winrate = UCTNode::score_mix_function(game.get_movenum(),
                                                         net_score, mc_winrate);
        gtp_printf(id, "%f", comb_winrate);
        return true;
    } else if (command.find("final_status_list") == 0) {
        if (command.find("alive") != std::string::npos) {
            std::string livelist = get_life_list(game, true);
            gtp_printf(id, livelist.c_str());
        } else if (command.find("dead") != std::string::npos) {
            std::string deadlist = get_life_list(game, false);
            gtp_printf(id, deadlist.c_str());
        } else {
            gtp_printf(id, "");
        }
        return true;
    } else if (command.find("time_settings") == 0) {
        std::istringstream cmdstream(command);
        std::string tmp;
        int maintime, byotime, byostones;

        cmdstream >> tmp >> maintime >> byotime >> byostones;

        if (!cmdstream.fail()) {
            // convert to centiseconds and set
            game.set_timecontrol(maintime * 100, byotime * 100, byostones, 0);

            gtp_printf(id, "");
        } else {
            gtp_fail_printf(id, "syntax not understood");
        }
        return true;
    } else if (command.find("time_left") == 0) {
        std::istringstream cmdstream(command);
        std::string tmp, color;
        int time, stones;

        cmdstream >> tmp >> color >> time >> stones;

        if (!cmdstream.fail()) {
            int icolor;

            if (color == "w" || color == "white") {
                icolor = FastBoard::WHITE;
            } else if (color == "b" || color == "black") {
                icolor = FastBoard::BLACK;
            } else {
                gtp_fail_printf(id, "Color in time adjust not understood.\n");
                return 1;
            }

            game.adjust_time(icolor, time * 100, stones);

            gtp_printf(id, "");

            if (cfg_allow_pondering) {
                // KGS sends this after our move
                // now start pondering
                if (game.get_last_move() != FastBoard::RESIGN) {
                    float old_komi = game.get_komi();
                    float new_komi = old_komi;
                    if (cfg_komi_adjust) {
                        // We are the the color not to move
                        int who = !game.get_to_move();
                        if (who == FastBoard::BLACK) {
                            new_komi = old_komi + 1.0f;
                        } else {
                            new_komi = old_komi - 1.0f;
                        }
                    }
                    game.set_komi(new_komi);
                    std::unique_ptr<UCTSearch> search(new UCTSearch(game));
                    search->ponder();
                    game.set_komi(old_komi);
                }
            }
        } else {
            gtp_fail_printf(id, "syntax not understood");
        }
        return true;
    } else if (command.find("auto") == 0) {
        do {
            std::unique_ptr<UCTSearch> search(new UCTSearch(game));

            int move = search->think(game.get_to_move(), UCTSearch::NORMAL);
            game.play_move(move);
            game.display_state();

        } while (game.get_passes() < 2
                 && game.get_last_move() != FastBoard::RESIGN);

        return true;
    } else if (command.find("bench") == 0) {
        Playout::do_playout_benchmark(game);
        return true;
    } else if (command.find("go") == 0) {
        std::unique_ptr<UCTSearch> search(new UCTSearch(game));

        int move = search->think(game.get_to_move());
        game.play_move(move);

        std::string vertex = game.move_to_text(move);
        myprintf("%s\n", vertex.c_str());
        return true;
    } else if (command.find("influence") == 0) {
        gtp_printf(id, "");
        game.board.display_map(game.board.influence());
        return true;
    } else if (command.find("heatmap") == 0) {
        gtp_printf(id, "");
        auto vec = Network::get_Network()->get_scored_moves(
            &game, Network::Ensemble::AVERAGE_ALL);
        Network::show_heatmap(&game, vec, false);
        return true;
    } else if (command.find("fixed_handicap") == 0) {
        std::istringstream cmdstream(command);
        std::string tmp;
        int stones;

        cmdstream >> tmp;   // eat fixed_handicap
        cmdstream >> stones;

        if (game.set_fixed_handicap(stones)) {
            std::string stonestring = game.board.get_stone_list();
            gtp_printf(id, "%s", stonestring.c_str());
        } else {
            gtp_fail_printf(id, "Not a valid number of handicap stones");
        }
        return true;
    } else if (command.find("place_free_handicap") == 0) {
        std::istringstream cmdstream(command);
        std::string tmp;
        int stones;

        cmdstream >> tmp;   // eat place_free_handicap
        cmdstream >> stones;

        game.place_free_handicap(stones);

        std::string stonestring = game.board.get_stone_list();
        gtp_printf(id, "%s", stonestring.c_str());

        return true;
    } else if (command.find("set_free_handicap") == 0) {
        std::istringstream cmdstream(command);
        std::string tmp;

        cmdstream >> tmp;   // eat set_free_handicap

        do {
            std::string vertex;

            cmdstream >> vertex;

            if (!cmdstream.fail()) {
                if (!game.play_textmove("black", vertex)) {
                    gtp_fail_printf(id, "illegal move");
                } else {
                    game.set_handicap(game.get_handicap() + 1);
                }
            }
        } while (!cmdstream.fail());

        std::string stonestring = game.board.get_stone_list();
        gtp_printf(id, "%s", stonestring.c_str());

        return true;
    } else if (command.find("loadsgf") == 0) {
        std::istringstream cmdstream(command);
        std::string tmp, filename;
        int movenum;

        cmdstream >> tmp;   // eat loadsgf
        cmdstream >> filename;

        if (!cmdstream.fail()) {
            cmdstream >> movenum;

            if (cmdstream.fail()) {
                movenum = 999;
            }
        } else {
            gtp_fail_printf(id, "Missing filename.");
            return true;
        }

        std::unique_ptr<SGFTree> sgftree(new SGFTree);

        try {
            sgftree->load_from_file(filename);
            game = sgftree->follow_mainline_state(movenum - 1);
            gtp_printf(id, "");
        } catch (const std::exception&) {
            gtp_fail_printf(id, "cannot load file");
        }
        return true;
    } else if (command.find("kgs-chat") == 0) {
        // kgs-chat (game|private) Name Message
        std::istringstream cmdstream(command);
        std::string tmp;

        cmdstream >> tmp; // eat kgs-chat
        cmdstream >> tmp; // eat game|private
        cmdstream >> tmp; // eat player name
        do {
            cmdstream >> tmp; // eat message
        } while (!cmdstream.fail());

        gtp_fail_printf(id, "I'm a go bot, not a chat bot.");
        return true;
    } else if (command.find("kgs-game_over") == 0) {
        // Do nothing. Particularly, don't ponder.
        gtp_printf(id, "");
        return true;
    } else if (command.find("kgs-time_settings") == 0) {
        // none, absolute, byoyomi, or canadian
        std::istringstream cmdstream(command);
        std::string tmp;
        std::string tc_type;
        int maintime, byotime, byostones, byoperiods;

        cmdstream >> tmp >> tc_type;

        if (tc_type.find("none") != std::string::npos) {
            // 30 mins
            game.set_timecontrol(30 * 60 * 100, 0, 0, 0);
        } else if (tc_type.find("absolute") != std::string::npos) {
            cmdstream >> maintime;
            game.set_timecontrol(maintime * 100, 0, 0, 0);
        } else if (tc_type.find("canadian") != std::string::npos) {
            cmdstream >> maintime >> byotime >> byostones;
            // convert to centiseconds and set
            game.set_timecontrol(maintime * 100, byotime * 100, byostones, 0);
        } else if (tc_type.find("byoyomi") != std::string::npos) {
            // KGS style Fischer clock
            cmdstream >> maintime >> byotime >> byoperiods;
            game.set_timecontrol(maintime * 100, byotime * 100, 0, byoperiods);
        } else {
            gtp_fail_printf(id, "syntax not understood");
            return true;
        }

        if (!cmdstream.fail()) {
            gtp_printf(id, "");
        } else {
            gtp_fail_printf(id, "syntax not understood");
        }
        return true;
    } else if (command.find("tune") == 0) {
        std::istringstream cmdstream(command);
        std::string tmp, filename;

        cmdstream >> tmp;   // eat tune

        cmdstream >> filename;

        std::unique_ptr<AttribScores> scores(new AttribScores);

        scores->autotune_from_file(filename);

        gtp_printf(id, "");
        return true;
    } else if (command.find("pn") == 0) {
        std::unique_ptr<PNSearch> pnsearch(new PNSearch(game));

        pnsearch->classify_groups();

        gtp_printf(id, "");
        return true;
    }  else if (command.find("nettune") == 0) {
        std::istringstream cmdstream(command);
        std::string tmp, filename;

        cmdstream >> tmp;   // eat nettune
        cmdstream >> filename;

        std::unique_ptr<Network> network(new Network);
        network->autotune_from_file(filename);
        gtp_printf(id, "");
        return true;
    } else if (command.find("netbench") == 0) {
        Network::get_Network()->benchmark(&game);
        gtp_printf(id, "");
        return true;

    } else if (command.find("bookgen") == 0) {
        std::istringstream cmdstream(command);
        std::string tmp, filename;

        cmdstream >> tmp;   // eat bookgen
        cmdstream >> filename;

        Book::bookgen_from_file(filename);
        return true;
    } else if (command.find("printsgf") == 0) {
        std::istringstream cmdstream(command);
        std::string tmp, filename;

        cmdstream >> tmp;   // eat printsgf
        cmdstream >> filename;

        auto sgf_text = SGFTree::state_to_string(&game, 0);

        if (cmdstream.fail()) {
            gtp_printf(id, "%s\n", sgf_text.c_str());
        } else {
            std::ofstream out(filename);
            out << sgf_text;
            out.close();
        }

        return true;
    } else if (command.find("rltune") == 0) {
        std::istringstream cmdstream(command);
        std::string tmp, filename;

        cmdstream >> tmp;   // eat rltune
        cmdstream >> filename;

        MCPolicy::mse_from_file(filename);
        return true;
    } else if (command.find("sltune") == 0) {
        std::istringstream cmdstream(command);
        std::string tmp, filename;

        cmdstream >> tmp;   // eat rltune
        cmdstream >> filename;

        MCPolicy::mse_from_file2(filename);
        return true;
    }

    gtp_fail_printf(id, "unknown command");
    return true;
}
