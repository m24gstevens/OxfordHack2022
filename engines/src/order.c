#include "order.h"
#include "move.h"

int history_heuristic[2][64][64];

void age_history() {
    for (int i=0; i<2; i++) {
        for (int j=0; j<64; j++) {
            for (int k=0; k<64; k++) {
                history_heuristic[i][j][k] >>= 3;
            }
        }
    }
}

void calibrate_history() {
    for (int i=0; i<2; i++) {
        for (int j=0; j<64; j++) {
            for (int k=0; k<64; k++) {
                history_heuristic[i][j][k] >>= 1;
            }
        }
    }
}

// MVV_LVA[victim][aggressor]
const int MVV_LVA[13][12] = {
    61, 66, 65, 64, 63, 62,     61, 66, 65, 64, 63, 62,
    11, 11, 15, 14, 13, 12,     11, 11, 15, 14, 13, 12,
    21, 22, 25, 24, 23, 22,     21, 22, 25, 24, 23, 22,
    31, 36, 35, 34, 33, 32,     31, 36, 35, 34, 33, 32,
    41, 46, 45, 44, 43, 42,     41, 46, 45, 44, 43, 42,
    51, 56, 55, 54, 53, 52,     51, 56, 55, 54, 53, 52,

    61, 66, 65, 64, 63, 62,     61, 66, 65, 64, 63, 62,
    11, 11, 15, 14, 13, 12,     11, 11, 15, 14, 13, 12,
    21, 22, 25, 24, 23, 22,     21, 22, 25, 24, 23, 22,
    31, 36, 35, 34, 33, 32,     31, 36, 35, 34, 33, 32,
    41, 46, 45, 44, 43, 42,     41, 46, 45, 44, 43, 42,
    51, 56, 55, 54, 53, 52,     51, 56, 55, 54, 53, 52,

    11, 11, 15, 14, 13, 12,     11, 11, 15, 14, 13, 12,
};

const int simple_piece_values[12] = {10000,100,300,300,500,900,10000,100,300,300,500,900};

bool good_capture(board_t* board, U16 move) {
    int from = MOVE_FROM(move);
    int to = MOVE_TO(move);
    int moved = board->squares[from];
    int captured = board->squares[to];
    int value = simple_piece_values[captured];

    if (PIECE_TYPE(moved) == P) {return true;}
    if (simple_piece_values[captured] >= simple_piece_values[to]) {return true;}
    board->occupancies[BOTH] ^= (C64(1) << from);
    if (!is_square_attacked(board,to,1^board->side)) {
        board->occupancies[BOTH] ^= (C64(1) << from);
        return true;
    }
    board->occupancies[BOTH] ^= (C64(1) << from);
    return false;
}

void score_cutoff(board_t* board, search_info_t* si, U16 move, int ply, int depth) {
    si->killers[1][ply] = si->killers[0][ply];
    si->killers[0][ply] = move;
    history_heuristic[board->side][MOVE_FROM(move)][MOVE_TO(move)] += depth;
    if (history_heuristic[board->side][MOVE_FROM(move)][MOVE_TO(move)] >= HIST_MAX) {
        calibrate_history();
    }
}

void swap_moves(move_t* m1, move_t* m2) {
    m1->move ^= m2->move;
    m2->move ^= m1->move;
    m1->move ^= m2->move;

    m1->score ^= m2->score;
    m2->score ^= m1->score;
    m1->score ^= m2->score;
}

void score_pv(search_info_t* si, move_t* mp, int nmoves, U16 pvmove) {
    si->follow_pv = false;
    for (int i=0; i<nmoves; i++) {
        if ((mp+i)->move == pvmove) {
            si->follow_pv = true;
            (mp+i)->score = SCORE_PV;
            return;
        }
    }
}

static inline void score_move(board_t* board, search_info_t* si, move_t* mp, U16 pvmove) {
    int score;
    U16 move = mp->move;
    if (move == pvmove) {
        score = SCORE_HASH;
    } else if (IS_CAPTURE(move)) {
        score = (good_capture(board, move) ? SCORE_CAPTURE : SCORE_LCAPTURE);
        score += MVV_LVA[board->squares[MOVE_TO(move)]][board->squares[MOVE_FROM(move)]];

    } else if (IS_PROMOTION(move)) {
        if (PROMOTE_TO(move) == 0x3) {score = SCORE_PROMOTE;}
        else {score = SCORE_UNDERPROMOTE;}
    } else {
        if (move == si->killers[0][si->ply]) {score = SCORE_KILLER1;}
        else if (move == si->killers[1][si->ply]) {score = SCORE_KILLER2;}
        else {score = history_heuristic[board->side][MOVE_FROM(move)][MOVE_TO(move)];}
    }

    mp->score = score;
}

void score_moves(board_t* board, search_info_t* si, move_t* msp, int nmoves, U16 pvmove) {
    for (int i=0; i<nmoves; i++) {
        score_move(board, si, (msp + i), pvmove);
    }
}

U16 pick_move(move_t* mp, int nmoves) {
    int bestidx, bestscore, score;
    if (!nmoves) {return NOMOVE;}

    bestidx = -1;
    bestscore = -1;

    for (int i=0; i<nmoves; i++) {
        if ((score = (mp+i)->score) > bestscore) {
            bestscore = score;
            bestidx = i;
        }
    }

    if (bestidx != 0) {
        swap_moves(mp, (mp + bestidx));
    }
    return mp->move;
}