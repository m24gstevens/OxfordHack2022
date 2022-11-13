#ifndef _ORDER_H_
#define _ORDER_H_

#include "oxchess.h"

#define SCORE_PV 900000000
#define SCORE_HASH 890000000
#define SCORE_CAPTURE 850000000
#define SCORE_PROMOTE 840000000
#define SCORE_KILLER1 830000000
#define SCORE_KILLER2 820000000
#define SCORE_LCAPTURE 800000000
#define SCORE_UNDERPROMOTE 800000000
#define HIST_MAX 800000000

void age_history();

void score_cutoff(board_t*, search_info_t*, U16, int, int);

void score_pv(search_info_t*, move_t*, int, U16);

void swap_moves(move_t*, move_t*);

void score_moves(board_t*, search_info_t*, move_t*, int, U16);

U16 pick_move(move_t*, int);

#endif