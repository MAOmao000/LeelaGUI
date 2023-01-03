#ifndef GTP_H_INCLUDED
#define GTP_H_INCLUDED

#include <string>
#include <vector>
#include "GameState.h"

extern bool cfg_allow_pondering;
extern bool cfg_allow_book;
extern int cfg_num_threads;
extern int cfg_max_playouts;
extern bool cfg_enable_nets;
extern bool cfg_komi_adjust;
extern int cfg_mature_threshold;
extern int cfg_expand_threshold;
extern int cfg_lagbuffer_cs;
#ifdef USE_OPENCL
extern std::vector<int> cfg_gpus;
extern int cfg_rowtiles;
#endif
extern float cfg_bound;
extern float cfg_fpu;
extern float cfg_cutoff_offset;
extern float cfg_cutoff_ratio;
extern float cfg_puct;
extern float cfg_uct;
extern float cfg_psa;
extern float cfg_softmax_temp;
extern float cfg_beta;
extern float cfg_patternbonus;
extern float cfg_mix_opening;
extern float cfg_mix_ending;
extern float cfg_mc_softmax;
extern int cfg_eval_thresh;
extern int cfg_rave_moves;
extern int cfg_extra_symmetry;
extern int cfg_random_loops;
extern std::string cfg_logfile;
extern FILE* cfg_logfile_handle;
extern bool cfg_quiet;
extern int cfg_lang;
extern int cfg_use_engine;
extern int cfg_engine_type;
extern bool cfg_board25;
class GTP {
public:
    static constexpr int ORIGINE_ENGINE = 0;
    static constexpr int KATAGO_ENGINE = 1;
    static constexpr int NONE = 0;
    static constexpr int ANALYSIS = 1;
    static constexpr int GTP_INTERFACE = 2;
    static bool execute(GameState & game, std::string xinput);
    static void setup_default_parameters(int lang=0, int use_engine=ORIGINE_ENGINE, bool board25=true);
    static bool perform_self_test(GameState & state);
private:
    static const int GTP_VERSION = 2;

    static std::string get_life_list(GameState & game, bool live);
    static const std::string s_commands[];
};


#endif
