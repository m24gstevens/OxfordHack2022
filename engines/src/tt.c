#include "tt.h"
#include "board.h"

xorshift64_state xs = {C64(0x23571113)};

U64 xorshift64() {
    U64 x = xs.state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;

    return (xs.state = x);
}

U64 piece_keys[13][64];
U64 ep_keys[2][8];
U64 castle_keys[16];
U64 side_key;

void initRandomKeys() {
    int i,j;

    for (i=0; i<12; i++) {
        for (j=0; j<64; j++) {
            piece_keys[i][j] = xorshift64();
        }
    }
    for (j=0; j<64; j++) {
        piece_keys[12][j] = C64(0);
    }

    for (i=0; i<2; i++) {
        for (j=0; j<8; j++) {
            ep_keys[i][j] = xorshift64();
        }
    }

    for (i=0; i<16; i++) {
        castle_keys[i] = xorshift64();
    }

    side_key = xorshift64();
}

U64 hash_position(board_t* board) {
    int i, pc;
    U64 bitboard, key;

    key = C64(0);
    for (i=0;i<64;i++) {
        pc = board->squares[i];
        key ^= piece_keys[pc][i];
    }
    if (board->ep_square != -1) {
        key ^= ep_keys[board->side][board->ep_square % 8];
    }
    key ^= castle_keys[board->castle_flags];
    if (board->side) {key ^= side_key;}

    return key;
}

const int hashsize = HASHSIZE;
const int hashmod = hashsize - 1;

const int evhashsize = EVHASHSIZE;
const int evhashmod = evhashsize - 1;

hash_t tt[HASHSIZE];
evhash_t evtt[EVHASHSIZE];

void initTT() {
    for (int i=0; i<hashsize; i++) {
        tt[i].score = INVALID;
    }
    for (int i=0; i<evhashsize; i++) {
        evtt[i].eval = INVALID;
    }
}

int probe_tt(board_t* board, int ply, int depth, int alpha, int beta, U16* best) {
    int score;
    hash_t* phashe = &tt[board->hash & hashmod];

    if (phashe->hash == board->hash) {
        *best = phashe->move;
        if (phashe->depth >= depth) {
            score = phashe->score;
            if (score > MATE_THRESHOLD) {score -= ply;}
            if (score < -MATE_THRESHOLD) {score += ply;}

            if (phashe->flag == HASH_FLAG_EXACT) {return score;}
            if ((phashe->flag == HASH_FLAG_ALPHA) && (score <= alpha)) {return alpha;}
            if ((phashe->flag == HASH_FLAG_BETA) && (score >= beta)) {return beta;}
        }
    }
    return INVALID;
}

void store_tt(board_t* board, int depth, int ply, int score, U8 flags, U16 best) {
    hash_t* phashe = &tt[board->hash & hashmod];

    if ((phashe->hash == board->hash) && (phashe->depth > depth)) {return;}

    if (score > MATE_THRESHOLD) {score += ply;}
    if (score < -MATE_THRESHOLD) {score -= ply;}

    phashe->hash = board->hash;
    phashe->depth = depth;
    phashe->flag = flags;
    phashe->move = best;
    phashe->score = score;
}

int probe_eval_tt(board_t* board) {
    evhash_t* phashe = &evtt[board->hash & evhashmod];

    if (phashe->hash == board->hash) {
        return phashe->eval;
    }
    return INVALID;
}

void store_eval_tt(board_t* board, int eval) {
    evhash_t* phashe = &evtt[board->hash & evhashmod];
    phashe->hash = board->hash;
    phashe->eval = eval;
}



