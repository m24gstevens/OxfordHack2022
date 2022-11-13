#include "evaluate.h"
#include "tt.h"
#include "bitboard.h"

int piece_values[12] = {10000,100,300,320,500,900,-10000,-100,-300,-320,-500,-900};

// Bitboard routines for evaluation
static inline U64 northFill(U64 bb) {
    bb |= (bb << 8);
    bb |= (bb << 16);
    bb |= (bb << 32);
    return bb;
}
static inline U64 southFill(U64 bb) {
    bb |= (bb >> 8);
    bb |= (bb >> 16);
    bb |= (bb >> 32);
    return bb;
}
U64 fileFill(U64 bb) {
    return northFill(bb) | southFill(bb);
}

static inline int count_isolated_pawns(U64 pawn_mask) {
    U64 open = ~fileFill(pawn_mask);
    pawn_mask &= east_one(open);
    pawn_mask &= west_one(open);
    return Popcount(pawn_mask);
}

static inline int count_doubled_pawns(U64 pawn_mask) {
    U64 mask = north_one(northFill(pawn_mask)) & southFill(pawn_mask);
    return Popcount(mask);
}

static inline U64 white_passed_pawns(U64 wpawns, U64 bpawns) {
    U64 black_pawn_spans = south_one(southFill(bpawns));
    black_pawn_spans |= east_one(black_pawn_spans) | west_one(black_pawn_spans);
    return wpawns & ~black_pawn_spans;
}

static inline U64 black_passed_pawns(U64 wpawns, U64 bpawns) {
    U64 white_pawn_spans = north_one(northFill(wpawns));
    white_pawn_spans |= east_one(white_pawn_spans) | west_one(white_pawn_spans);
    return bpawns & ~white_pawn_spans;
}

static inline int count_white_backward_pawns(U64 wpawns) {
    U64 backfills = south_one(southFill(wpawns));
    U64 mask = west_one(backfills) | east_one(backfills);
    wpawns &= mask;
    return Popcount(wpawns); 
}

static inline int count_black_backward_pawns(U64 bpawns) {
    U64 backfills = north_one(northFill(bpawns));
    U64 mask = west_one(backfills) | east_one(backfills);
    bpawns &= mask;
    return Popcount(bpawns); 
}

static inline U64 wpawn_attack_mask(U64 bb) {
    U64 aside = west_one(bb) | east_one(bb);
    return north_one(aside);
}

static inline U64 bpawn_attack_mask(U64 bb) {
    U64 aside = west_one(bb) | east_one(bb);
    return south_one(aside);
}

// material_scores[phase][piece]
const int material_scores[2][6] = {
    {0,95,300,320,460,920,},
    {0,100,280,290,500,930,},
};

const int doubled_pawn_penalty[2] = {13,18};
const int isolated_pawn_penalty[2] = {15,12};
const int backward_pawn_penalty[2] = {15,12};
const int passed_pawn_bonus[8] = {0, 10, 20, 40, 60, 80, 100, 0};

const int semiopen_bonus[2] = {12,12};
const int open_bonus[2] = {20,20};

const int no_pawn_penalty = 150;
const int bad_bishop[2] = {25,25};
const int trapped_rook[2] = {40,40};

const U64 firstRank = C64(0xFF);
const U64 lastRank = C64(0xFF00000000000000);

// pstables[phase][piece][sq] from white perspective
const int pstables[2][6][64] = {
    {{-10, 10, 0, -20, 0, -10, 10, 0,
    -10,-10,-15,-25,-20,-20,-10,-5,
    -15,-15,-20,-35,-35,-20,-15,-15,
    -25,-25,-30,-40,-40,-30,-25,-25,
    -40,-40,-40,-40,-40,-40,-40,-40,
    -50,-50,-50,-50,-50,-50,-50,-50,
    -60,-60,-60,-60,-60,-60,-60,-60,
    -70,-70,-70,-70,-70,-70,-70,-70,},
    {0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -5, -20, -20, 0, 0, 0,
    3, 3, 3, -10, -10, -3, 3, 3,
    5, 0, 10, 10, 10, -5, 0, 0,
    10, 5, 15, 20, 20, 5, 5, 5,
    13, 13, 20, 25, 25, 20, 13, 13,
    20, 20, 30, 40, 40, 30, 20, 20,
    0, 0, 0, 0, 0, 0, 0, 0,},
    {-20, -15, -10, -10, -10, -10, -20, -25,
    -15, -10, -10, 0, 0, -10, -10, -15,
    -5, 0, 8, 5, 5, 8, 0, -5,
    0, 3, 12, 12, 12, 12, 3, 0,
    5, 8, 15, 25, 25, 15, 8, 5,
    0, 10, 25, 35, 35, 25, 10, 0,
    0, 0, 10, 10, 10, 10, 0, 0,
    -40, -30, -20, -15, -15, -20, -30, -40,},
    {-5, -10, -5, -10, -10, -5, -10, -5,
    3, 5, 3, 0, 0, 3, 5, 3,
    -2, 5, 5, 8, 8, 5, 5, -2,
    6, 10, 15, 15, 15, 15, 10, 6,
    6, 10, 15, 15, 15, 15, 10, 6,
    -2, 5, 5, 8, 8, 5, 5, -2,
    3, 5, 3, 0, 0, 3, 5, 3,
    -5, -10, -5, -10, -10, -5, -10, -5,},
    {-15, -10, 5, 5, 5, 0, -10, -15,
    -10, -5, 5, 10, 10, 0, -5, -10,
    -10, -5, 0, 5, 5, 0, -5, -10,
    -10, -5, 0, 5, 5, 0, -5, -10,
    -10, -5, 0, 5, 5, 0, -5, -10,
    0, 0, 10, 10, 10, 10, 0, 0,
    30, 35, 40, 50, 50, 40, 35, 30,
    30, 35, 40, 50, 50, 40, 35, 30,},
    {-15, -10, 0, 0, 0, 0, -10, -15,
    -20, -5, 5, 5, 5, 5, -5, -20,
    -10, 8, 5, 5, 5, 5, -5, -10,
    5, 0, 5, 8, 8, 5, 0, 5,
    5, 0, 5, 8, 8, 5, 0, 5,
    -10, 0, 5, 5, 5, 5, -5, -10,
    -20, -5, 5, 5, 5, 5, -5, -20,
    -15, -10, 0, 0, 0, 0, -10, -15,},
    },
    {{-30, -20, -10, 0, 0, -10, -20, -30,
    -20, -10, 0, 10, 10, 0, -10, -20,
    -10, 0, 10, 20, 20, 10, 0, -10,
    0, 10, 20, 30, 30, 20, 10, 0,
    0, 10, 20, 30, 30, 20, 10, 0,
    -10, 0, 10, 20, 20, 10, 0, -10,
    -20, -10, 0, 10, 10, 0, -10, -20,
    -30, -20, -10, 0, 0, -10, -20, -30,},
    {0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -10, -10, 0, 0, 0,
    0, 0, 0, 5, 5, 0, 0, 0,
    5, 5, 10, 20, 20, 10, 5, 5,
    10, 10, 10, 20, 20, 10, 10, 10,
    20, 20, 30, 30, 30, 30, 20, 20,
    30, 30, 40, 50, 50, 40, 30, 30,
    0, 0, 0, 0, 0, 0, 0, 0,},
    {-15, -10, -5, 0, 0, -5, -10, -15,
     -10, -5, 0, 5, 5, 0, -5, -10,
     -5, 0, 5, 10, 10, 5, 0, -5,
     0, 5, 10, 15, 15, 10, 5, 0,
     0, 5, 10, 15, 15, 10, 5, 0,
     -5, 0, 5, 10, 10, 5, 0, -5,
     -10, -5, 0, 5, 5, 0, -5, -10,
     -15, -10, -5, 0, 0, -5, -10, -15,},
    {-5, -10, -5, -10, -10, -5, -10, -5,
    3, 5, 3, 0, 0, 3, 5, 3,
    -2, 5, 5, 8, 8, 5, 5, -2,
    6, 10, 15, 15, 15, 15, 10, 6,
    6, 10, 15, 15, 15, 15, 10, 6,
    -2, 5, 5, 8, 8, 5, 5, -2,
    3, 5, 3, 0, 0, 3, 5, 3,
    -5, -10, -5, -10, -10, -5, -10, -5,},
    {0, 0, 0, 5, 5, 0, 0, 0,
    0, 0, 0, 5, 5, 0, 0, 0,
    0, 0, 0, 5, 5, 0, 0, 0,
    0, 0, 0, 5, 5, 0, 0, 0,
    0, 0, 0, 5, 5, 0, 0, 0,
    5, 5, 5, 10, 10, 5, 5, 5,
    25, 25, 25, 25, 25, 25, 25, 25,
    25, 25, 25, 25, 25, 25, 25, 25,},
    {-15, -10, 0, 0, 0, 0, -10, -15,
    -20, -5, 5, 5, 5, 5, -5, -20,
    -10, 8, 5, 5, 5, 5, -5, -10,
    5, 0, 5, 8, 8, 5, 0, 5,
    5, 0, 5, 8, 8, 5, 0, 5,
    -10, 0, 5, 5, 5, 5, -5, -10,
    -20, -5, 5, 5, 5, 5, -5, -20,
    -15, -10, 0, 0, 0, 0, -10, -15,},},
};

int evaluate(board_t* board) {
    int score, i, pc, side, sq, totalmat, mid, end, ct;
    int mgscore[2], egscore[2], matscore[2];
    U64 mask, bb;

    if ((score = probe_eval_tt(board)) != INVALID) {
        return score;
    }

    mgscore[0] = mgscore[1] = 0;
    egscore[0] = egscore[1] = 0;
    matscore[0] = matscore[1] = 0;

    for (i=0; i<64; i++) {
        pc = board->squares[i];
        if (pc == _) {continue;}

        side = pc / 6;
        sq = (side ? 56^i : i);
        
        matscore[side] += material_scores[EG][pc % 6];
        mgscore[side] += material_scores[MG][pc % 6];
        mgscore[side] += pstables[MG][pc % 6][sq];
        egscore[side] += pstables[EG][pc % 6][sq];
    }

    egscore[WHITE] += matscore[WHITE];
    egscore[BLACK] += matscore[BLACK];

    // Doubled, isolated, backward
    ct = count_doubled_pawns(board->bitboards[P]);
    mgscore[WHITE] -= ct*doubled_pawn_penalty[MG];
    egscore[WHITE] -= ct*doubled_pawn_penalty[EG];
    ct = count_doubled_pawns(board->bitboards[p]);
    mgscore[BLACK] -= ct*doubled_pawn_penalty[MG];
    egscore[BLACK] -= ct*doubled_pawn_penalty[EG];

    ct = count_isolated_pawns(board->bitboards[P]);
    mgscore[WHITE] -= ct*isolated_pawn_penalty[MG];
    egscore[WHITE] -= ct*isolated_pawn_penalty[EG];
    ct = count_isolated_pawns(board->bitboards[p]);
    mgscore[BLACK] -= ct*isolated_pawn_penalty[MG];
    egscore[BLACK] -= ct*isolated_pawn_penalty[EG];

    bb = white_passed_pawns(board->bitboards[P], board->bitboards[p]);
    while (bb) {
        sq = Bitscan(bb);
        POP_LS1B(bb);
        egscore[WHITE] += passed_pawn_bonus[sq / 8];
    }
    bb = black_passed_pawns(board->bitboards[P], board->bitboards[p]);
    while (bb) {
        sq = Bitscan(bb);
        POP_LS1B(bb);
        egscore[BLACK] += passed_pawn_bonus[7 - (sq / 8)];
    }

    if (!board->bitboards[P]) {egscore[WHITE] -= no_pawn_penalty;}
    if (!board->bitboards[p]) {egscore[BLACK] -= no_pawn_penalty;}

    mask = ~fileFill(board->bitboards[P] | board->bitboards[p]);

    bb = mask & board->bitboards[R];
    ct = Popcount(bb);
    mgscore[WHITE] += ct * open_bonus[MG];
    egscore[WHITE] += ct * open_bonus[EG];

    bb = mask & board->bitboards[r];
    ct = Popcount(bb);
    mgscore[BLACK] += ct * open_bonus[MG];
    egscore[BLACK] += ct * open_bonus[EG];

    bb = ~fileFill(board->bitboards[P]) & board->bitboards[R] & ~mask;
    ct = Popcount(bb);
    mgscore[WHITE] += ct * semiopen_bonus[MG];
    egscore[WHITE] += ct * semiopen_bonus[EG];

    bb = ~fileFill(board->bitboards[p]) & board->bitboards[r] & ~mask;
    ct = Popcount(bb);
    mgscore[BLACK] += ct * semiopen_bonus[MG];
    egscore[BLACK] += ct * semiopen_bonus[EG];


    mid = mgscore[WHITE] - mgscore[BLACK];
    end = egscore[WHITE] - egscore[BLACK];

    totalmat = matscore[WHITE] + matscore[BLACK];
    if (totalmat > EGTHRESH) {score = mid;} 
    else if (totalmat <= EGTHRESH) {score = end;}
    else {
        score = (mid*(totalmat - EGTHRESH) + end*(MGTHRESH - totalmat)) / (MGTHRESH - EGTHRESH);
    } 

    store_eval_tt(board,score*(1-2*board->side));

    return score*(1-2*board->side);
}
