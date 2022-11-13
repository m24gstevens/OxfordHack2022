#ifndef _MOVE_H_ 
#define _MOVE_H_

#include "oxchess.h"


/* Encode a move in a U16 as follows 
-- Bits 0-5: "From" square
-- Bits 6-11: "To" square
-- Bits 12-15: Flags:
    0000 - Quiet
    0001 - Double pawn push
    0010 - O-O
    0011 - O-O-O
    0100 - Capture
    0101 - En Passent
    10XX - "quiet" promotion. 00 for knight, 01 for bishop, 02 for rook, 03 for queen
    11XX - Capture promotion.
*/

#define NOMOVE 0

#define ENCODE_MOVE(from,to,flags) ((from & 0x3F) | ((to & 0x3F) << 6) | ((flags & 0xF) << 12))
#define MOVE_FROM(move) ((move) & 0x3F)
#define MOVE_TO(move) (((move)>>6) & 0x3F)
#define MOVE_FLAGS(move) (((move)>>12) & 0xF)
#define IS_CAPTURE(move) ((move) & 0x4000)
#define IS_PROMOTION(move) ((move) & 0x8000)
#define PROMOTE_TO(move) ((move >> 12) & 0x3)
#define IS_TACTICAL(move) ((move) & 0xC000)

void print_move(U16);

bool is_square_attacked(board_t*, int, int);

bool make_move(board_t*, U16, hist_t*);
void unmake_move(board_t*, U16, hist_t*);

void make_nullmove(board_t*, hist_t*);
void unmake_nullmove(board_t*, hist_t*);

long perft(board_t*, int);
void perft_test(board_t*, int);
void divide(board_t*, int);

#endif