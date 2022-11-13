#ifndef _BOARD_H_
#define _BOARD_H_

#include "oxchess.h"

extern U64 game_history[MAXHIST];
extern int hply;

extern char *starting_position;
extern char *kiwipete;
extern char *capture_position;
extern char *unsafe_king;

extern char* piece_characters;
extern enumPiece char_to_piece_code[];
extern char* square_strings[64];
extern char* promoted_pieces;

void print_board(board_t*);

void parse_fen(board_t*, char*);

bool is_draw(board_t*, search_info_t* si);

#endif