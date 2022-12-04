#include <cassert>
#include "TimeControl.h"
#include "Utils.h"
#include "GTP.h"

using namespace Utils;

TimeControl::TimeControl(int boardsize, int maintime, int byotime,
                         int byostones, int byoperiods)
    : m_maintime(maintime),
      m_byotime(byotime),
      m_byostones(byostones),
      m_byoperiods(byoperiods),
      m_boardsize(boardsize) {

    reset_clocks();
    set_boardsize(boardsize);

    auto time_remaining = m_maintime;
    auto moves_remaining = get_moves_expected(m_boardsize, 1);
    m_mintime = std::max(time_remaining - cfg_lagbuffer_cs, 0) / std::max(moves_remaining, 1);
}

void TimeControl::reset_clocks() {
    m_remaining_time[0] = m_maintime;
    m_remaining_time[1] = m_maintime;
    m_stones_left[0] = m_byostones;
    m_stones_left[1] = m_byostones;
    m_periods_left[0] = m_byoperiods;
    m_periods_left[1] = m_byoperiods;
    m_inbyo[0] = m_maintime <= 0;
    m_inbyo[1] = m_maintime <= 0;
    // Now that byo-yomi status is set, add time
    // back to our clocks
    if (m_inbyo[0]) {
        m_remaining_time[0] = m_byotime;
    }
    if (m_inbyo[1]) {
        m_remaining_time[1] = m_byotime;
    }
}

void TimeControl::start(int color) {
    m_times[color] = Time();
}

void TimeControl::stop(int color) {
    Time stop;
    int elapsed = Time::timediff(m_times[color], stop);

    assert(elapsed >= 0);

    m_remaining_time[color] -= elapsed;

    if (m_inbyo[color]) {
        if (m_byostones) {
            m_stones_left[color]--;
        } else if (m_byoperiods) {
            if (elapsed > m_byotime) {
                m_periods_left[color]--;
            }
        }
    }

    /*
        time up, entering byo yomi
    */
    if (!m_inbyo[color] && m_remaining_time[color] <= 0) {
        m_remaining_time[color] = m_byotime;
        m_stones_left[color] = m_byostones;
        m_periods_left[color] = m_byoperiods;
        m_inbyo[color] = true;
    } else if (m_inbyo[color] && m_byostones && m_stones_left[color] <= 0) {
        // reset byoyomi time and stones
        m_remaining_time[color] = m_byotime;
        m_stones_left[color] = m_byostones;
    } else if (m_inbyo[color] && m_byoperiods) {
        m_remaining_time[color] = m_byotime;
    }
}

void TimeControl::display_times() {
    {
        int rem = m_remaining_time[0] / 100;  /* centiseconds to seconds */
        int hours = rem / (60 * 60);
        rem = rem % (60 * 60);
        int minutes = rem / 60;
        rem = rem % 60;
        int seconds = rem;
        myprintf("Black time: %02d:%02d:%02d", hours, minutes, seconds);
        if (m_inbyo[0]) {
            if (m_byostones) {
                myprintf(", %d stones left", m_stones_left[0]);
            } else if (m_byoperiods) {
                myprintf(", %d period(s) of %d seconds left",
                         m_periods_left[0], m_byotime / 100);
            }
        }
        myprintf("\n");
    }
    {
        int rem = m_remaining_time[1] / 100;  /* centiseconds to seconds */
        int hours = rem / (60 * 60);
        rem = rem % (60 * 60);
        int minutes = rem / 60;
        rem = rem % 60;
        int seconds = rem;
        myprintf("White time: %02d:%02d:%02d", hours, minutes, seconds);
        if (m_inbyo[1]) {
            if (m_byostones) {
                myprintf(", %d stones left", m_stones_left[1]);
            } else if (m_byoperiods) {
                myprintf(", %d period(s) of %d seconds left",
                         m_periods_left[1], m_byotime / 100);
            }
        }
        myprintf("\n");
    }
    myprintf("\n");
}

//int TimeControl::max_time_for_move(const int boardsize, const int color,
//                                   const size_t movenum) const {
int TimeControl::max_time_for_move(int color, const size_t movenum) {
    // default: no byo yomi (absolute)
    auto time_remaining = m_remaining_time[color];
    auto moves_remaining = get_moves_expected(m_boardsize, movenum);
    auto extra_time_per_move = 0;

    if (m_byotime != 0) {
        /*
          no periods or stones set means
          infinite time = 1 month
        */
        if (m_byostones == 0 && m_byoperiods == 0) {
            return 31 * 24 * 60 * 60 * 100;
        }

        // byo yomi and in byo yomi
        if (m_inbyo[color]) {
            if (m_byostones) {
                moves_remaining = m_stones_left[color];
            } else {
                assert(m_byoperiods);
                // Just use the byo yomi period
                time_remaining = 0;
                extra_time_per_move = m_byotime;
            }
        } else {
            /*
              byo yomi time but not in byo yomi yet
            */
            if (m_byostones) {
                int byo_extra = m_byotime / m_byostones;
                time_remaining = m_remaining_time[color] + byo_extra;
                // Add back the guaranteed extra seconds
                extra_time_per_move = byo_extra;
            } else {
                assert(m_byoperiods);
                int byo_extra = m_byotime * (m_periods_left[color] - 1);
                time_remaining = m_remaining_time[color] + byo_extra;
                // Add back the guaranteed extra seconds
                extra_time_per_move = m_byotime;
            }
        }
    }

    // always keep a cfg_lagbugger_cs centisecond margin
    // for network hiccups or GUI lag
    auto base_time = std::max(time_remaining - cfg_lagbuffer_cs, 0)
                     / std::max(moves_remaining, 1);
    auto inc_time = std::max(extra_time_per_move - cfg_lagbuffer_cs, 0);

    if (m_inbyo[color]) {
        return base_time + inc_time;
    }
    return std::max(base_time + inc_time, m_mintime);
}

size_t TimeControl::opening_moves(const int boardsize) const {
    auto num_intersections = boardsize * boardsize;
    //auto fast_moves = num_intersections / 6;
    auto fast_moves = num_intersections / 4;
    return fast_moves;
}

int TimeControl::get_moves_expected(const int boardsize,
                                    const size_t movenum) const {
    /*
    auto board_div = 5;
    if (cfg_timemanage != TimeManagement::OFF) {
        // We will take early exits with time management on, so
        // it's OK to make our base time bigger.
        board_div = 9;
    }
    */

    // Note this is constant as we play, so it's fair
    // to underestimate quite a bit.
    //auto base_remaining = (boardsize * boardsize) / board_div;

    // Don't think too long in the opening.
    auto fast_moves = opening_moves(boardsize);
    if (movenum < fast_moves) {
        auto base_remaining = (boardsize * boardsize) / 5;
        return (base_remaining + fast_moves) - movenum;
    } else {
        auto base_remaining = (boardsize * boardsize) / 7;
        return base_remaining;
    }
}

void TimeControl::adjust_time(int color, int time, int stones) {
    m_remaining_time[color] = time;
    // From pachi: some GTP things send 0 0 at the end of main time
    if (!time && !stones) {
        m_inbyo[color] = true;
        m_remaining_time[color] = m_byotime;
        m_stones_left[color] = m_byostones;
        m_periods_left[color] = m_byoperiods;
    }
    if (stones) {
        // stones are only given in byo-yomi
        m_inbyo[color] = true;
    }
    // we must be in byo-yomi before interpreting stones
    // the previous condition guarantees we do this if != 0
    if (m_inbyo[color]) {
        if (m_byostones) {
            m_stones_left[color] = stones;
        } else if (m_byoperiods) {
            // KGS extension
            m_periods_left[color] = stones;
        }
    }
}

void TimeControl::set_boardsize(int boardsize) {
    // Note this is constant as we play, so it's fair
    // to underestimate quite a bit.
    if (cfg_use_engine == GTP::KATAGO_ENGINE) {
        m_moves_expected = (boardsize * boardsize) / 2;
    } else {
        m_moves_expected = (boardsize * boardsize) / 5;
    }
}

int TimeControl::get_remaining_time(int color) {
    return m_remaining_time[color];
}
