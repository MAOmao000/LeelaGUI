#ifndef GAMESTATE_H_INCLUDED
#define GAMESTATE_H_INCLUDED

#include <vector>
#include <string>
#include <array>

#include "FastState.h"
#include "FullBoard.h"
#include "KoState.h"
#include "TimeControl.h"

class GameState : public KoState {
public:
    explicit GameState() = default;
    explicit GameState(const KoState* rhs) {
        // Copy in fields from base class.
        *(static_cast<KoState*>(this)) = *rhs;
        anchor_game_history();
    };
    void init_game(int size, float komi);
    void reset_game();
    bool set_fixed_handicap(int stones, std::vector<int> & move_handi);
    int set_fixed_handicap_2(int stones, std::vector<int> & move_handi);
    std::vector<int>/*void*/ place_free_handicap(int stones);
    void anchor_game_history(void);
    void trim_game_history(int lastmove);

    void rewind(void); /* undo infinite */
    bool undo_move(void);
    bool forward_move(void);

    void play_move(int color, int vertex);
    void play_move(int vertex);
    void play_pass();
    bool play_textmove(std::string color, std::string vertex);

    void start_clock(int color);
    void stop_clock(int color);
    TimeControl& get_timecontrol();
    void set_timecontrol(int maintime, int byotime, int byostones,
                         int byoperiods);
    void set_timecontrol(TimeControl tmc);
    void adjust_time(int color, int time, int stones);
    int get_maintime();
    int get_byotime();
    int get_byostones();

    void display_state();

    std::vector<float> m_policy;
    std::vector<float> m_owner;
    std::array<float, 3> m_win_rate;
    float m_black_score;
private:
    bool valid_handicap(int stones);

    std::vector<KoState> game_history;
    TimeControl m_timecontrol;
};

#endif
