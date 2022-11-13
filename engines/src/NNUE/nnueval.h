#ifndef _NNUEVAL_H_
#define _NNUEVAL_H_

#include "oxchess.h"

void init_nnue(char*);
int eval_nnue(int, int*, int*);

int nevaluate(board_t*);

#endif