#ifndef TIMECONTROL_H_INCLUDED
#define TIMECONTROL_H_INCLUDED

#include <array>
#include "Timing.h"

class TimeControl {
public:
    /*
        Initialize time control. Timing info is per GTP and in centiseconds
    */
    TimeControl(int boardsize = 19,
                int maintime = 30 * 60 * 100,
                int byotime = 0, int byostones = 25,
                int byoperiods = 0);

    void start(int color);
    void stop(int color);
    int max_time_for_move(int color, const size_t movenum = 0);
    void adjust_time(int color, int time, int stones);
    void set_boardsize(int boardsize);
    void display_times();
    int get_remaining_time(int color);
    void reset_clocks();
    bool byo_yomi(int color);

private:
    size_t opening_moves(int boardsize) const;
    int get_moves_expected(int boardsize, size_t movenum) const;

    int m_maintime;
    int m_byotime;
    int m_byostones;
    int m_byoperiods;
    int m_moves_expected;
    int m_boardsize;
    int m_mintime;

    std::array<int,  2> m_remaining_time;    /* main time per player */
    std::array<int,  2> m_stones_left;       /* stones to play in byo period */
    std::array<int,  2> m_periods_left;      /* byo periods */
    std::array<bool, 2> m_inbyo;             /* player is in byo yomi */

    std::array<Time, 2> m_times;             /* storage for player times */
};

#endif
