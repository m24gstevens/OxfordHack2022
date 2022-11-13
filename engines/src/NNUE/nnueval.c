#include "nnueval.h"
#include "./Probe/nnue.h"

//const int nnue_pieces[12] = {6,5,4,3,2,1,12,11,10,9,8,7};
const int nnue_pieces[12] = {1,6,5,4,3,2,7,12,11,10,9,8};

void init_nnue(char* filename) {
    nnue_init(filename);
}

int evaluate_nnue(int player, int *pieces, int *squares) {
    return nnue_evaluate(player, pieces, squares);
}

int nevaluate(board_t* board) {
    U64 bb;
    int piece, square;
    int pieces[33];
    int squares[33];
    int index = 2;
    for (int bb_piece = 0; bb_piece < 12; bb_piece++) {
        bb = board->bitboards[bb_piece];
        while (bb) {
            piece = bb_piece;
            square = Bitscan(bb);
            if (piece == K) {
                pieces[0] = nnue_pieces[piece];
                squares[0] = square;
            } else if (piece == k) {
                pieces[1] = nnue_pieces[piece];
                squares[1] = square;
            } else {
                pieces[index] = nnue_pieces[piece];
                squares[index] = square;
                index++;
            }
            POP_LS1B(bb);
        }
    }
    pieces[index] = 0;
    squares[index] = 0;

    return (evaluate_nnue(board->side, pieces, squares) * (100 - board->rule50) / 100);
}