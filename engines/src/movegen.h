#ifndef _MOVEGEN_H_
#define _MOVEGEN_H_

#include "oxchess.h"


void print_moves(move_t*, int);

int generate_moves(board_t*, move_t*);

int generate_captures(board_t*, move_t*);

#endif