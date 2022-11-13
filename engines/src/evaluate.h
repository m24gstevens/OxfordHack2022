#ifndef _EVALUATE_H_
#define _EVALUATE_H_

#include "oxchess.h"

#define MG 0
#define EG 1

#define EGTHRESH 2580
#define MGTHRESH 5480

int evaluate(board_t*);

#endif