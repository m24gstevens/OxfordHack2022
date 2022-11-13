#ifndef _TT_H_
#define _TT_H_

#include "oxchess.h"

#define INVALID 100000

#define HASHSIZE (1 << 21)
#define EVHASHSIZE (1 << 19)

typedef struct {
    U64 state;
} xorshift64_state;

extern U64 piece_keys[13][64];
extern U64 ep_keys[2][8];
extern U64 castle_keys[16];
extern U64 side_key;

void initRandomKeys();
void initTT();

U64 hash_position(board_t*);

int probe_tt(board_t*, int, int, int, int, U16*);
void store_tt(board_t*, int, int, int, U8, U16);

int probe_eval_tt(board_t*);
void store_eval_tt(board_t*, int);


#endif