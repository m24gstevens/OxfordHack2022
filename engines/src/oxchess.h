#ifndef _OXCHESS_H_
#define _OXCHESS_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <windows.h>
#include <math.h>

// Magic numbers
#define MAXPLY 63
#define MAXMOVES 10000
#define MAXHIST 800

#define INF 60000
#define MATE 50000
#define MATE_THRESHOLD 49900

// Engine types
typedef uint64_t U64;
typedef uint32_t U32;
typedef uint16_t U16;
typedef uint8_t U8;

#define C64(x) x##ULL

#define MAX(x,y) (((x) > (y)) ? (x) : (y))

typedef struct {
    U64 bitboards[12];
    U64 occupancies[3];
    U8 squares[64];
    U64 hash;
    U8 side;
    int ep_square;
    int castle_flags;
    int rule50;
} board_t;

typedef struct {
    U64 hash;
    int ep_square;
    int rule50;
    U8 captured;
    U8 castle_flags;
} hist_t;

typedef struct {
    U16 move;
    int score;
} move_t;

typedef struct {
    int ply;
    U16 pv[MAXPLY][MAXPLY];
    move_t* msp[MAXPLY];
    U16 killers[2][MAXPLY];
    long nodes;
    int score;
    U16 bestmove;
    int sdepth;
    bool follow_pv;
} search_info_t;

typedef struct {
    long time[2];
    long inc[2];
    int movestogo;
    int depth;
    long nodes;
    int mate;
    int movetime;
    U8 flags;
    bool stop;
    long starttime;
    long stoptime;
} timeinfo_t;

typedef struct {
    U64 hash;
    U16 move;
    int score;
    int depth;
    U8 flag;
} hash_t;

typedef struct {
    U64 hash;
    int eval;
} evhash_t;



// enums

typedef enum {
    a1,b1,c1,d1,e1,f1,g1,h1,
    a2,b2,c2,d2,e2,f2,g2,h2,
    a3,b3,c3,d3,e3,f3,g3,h3,
    a4,b4,c4,d4,e4,f4,g4,h4,
    a5,b5,c5,d5,e5,f5,g5,h5,
    a6,b6,c6,d6,e6,f6,g6,h6,
    a7,b7,c7,d7,e7,f7,g7,h7,
    a8,b8,c8,d8,e8,f8,g8,h8,
} enumSquare;

typedef enum {WHITE, BLACK, BOTH} enumSide;

typedef enum {K,P,N,B,R,Q,k,p,n,b,r,q,_} enumPiece;

enum castleFlags {WCK=1, WCQ=2, BCK=4, BCQ=8};

enum {HASH_FLAG_ALPHA, HASH_FLAG_BETA, HASH_FLAG_EXACT};

#define PIECE_TYPE(pc) (pc % 6)
#define PIECE_COLOUR(pc) (pc / 6)

// Square defines
#define RANK(sq) ((sq)>>3)
#define FILE(sq) ((sq) & 7)

// Useful bitwise stuff

#define SET_BIT(bb,p) bb |= (C64(1) << p)
#define TEST_BIT(bb,p) ((bb) & (C64(1) << (p)))
#define CLEAR_BIT(bb,p) bb &= ~(C64(1) << p)
#define POP_LS1B(bb) bb &= (bb-1)

#define Popcount(x) __builtin_popcountll(x)
#define Bitscan(x) __builtin_ctzll(x)

// Timing function

long get_time_ms();


#endif
