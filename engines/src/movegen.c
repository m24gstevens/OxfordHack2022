#include "movegen.h"
#include "move.h"
#include "bitboard.h"
#include "board.h"

void print_moves(move_t* mp, int nmoves) {
    for (int i=0; i<nmoves; i++) {
        print_move((mp+i)->move);
        printf("\n");
    }
}

const U64 seventhRank[2] = {C64(0x00FF000000000000), C64(0xFF00)};
const U64 kingside_castle_empty[2] = {C64(0x60), C64(0x6000000000000000)};
const U64 queenside_castle_empty[2] = {C64(0xE), C64(0x0E00000000000000)};

int generate_moves(board_t* board, move_t* mp) {
    int side, from, to, pc, sq, nmoves, flags, prom;
    U64 bb, attacks, blockers, pieces;

    blockers = board->occupancies[BOTH];
    nmoves = 0;
    side = board->side;

    // King moves

    from = Bitscan(board->bitboards[K + 6*side]);
    attacks = kingAttackTable[from] & ~board->occupancies[side];
    while (attacks) {
        to = Bitscan(attacks);
        POP_LS1B(attacks);
        flags = (board->squares[to] == _ ? 0 : 4);
        (mp+(nmoves++))->move = ENCODE_MOVE(from,to,flags);
    }

    // Pawn moves (excl. EP)

    pieces = board->bitboards[P + 6*side];

    // Home rank pawns
    bb = pieces & seventhRank[1^side];
    while (bb) {
        from = Bitscan(bb);
        POP_LS1B(bb);
        // Pawn captures
        attacks = pawnAttackTable[side][from] & board->occupancies[1^side];
        while (attacks) {
            to = Bitscan(attacks);
            POP_LS1B(attacks);
            (mp+(nmoves++))->move = ENCODE_MOVE(from,to,4);
        }

        // Pawn pushes, including double pushes
        to = from + 8*(1-2*side);
        if (!TEST_BIT(blockers,to)) {
            (mp+(nmoves++))->move = ENCODE_MOVE(from,to,0);
            to += 8*(1-2*side);
            if (!TEST_BIT(blockers,to)) {
                (mp+(nmoves++))->move = ENCODE_MOVE(from,to,1);
            }
        }
    }

    // Non-seventh rank pawns
    bb = pieces & ~seventhRank[1^side] & ~seventhRank[side];

    while (bb) {
        from = Bitscan(bb);
        POP_LS1B(bb);
        // Pawn captures
        attacks = pawnAttackTable[side][from] & board->occupancies[1^side];
        while (attacks) {
            to = Bitscan(attacks);
            POP_LS1B(attacks);
            (mp+(nmoves++))->move = ENCODE_MOVE(from,to,4);
        }

        // Pawn pushes
        to = from + 8*(1-2*side);
        if (!TEST_BIT(blockers,to)) {
            (mp+(nmoves++))->move = ENCODE_MOVE(from,to,0);
        }
    }

    // Promoting pawns
    bb = pieces & seventhRank[side];

    while (bb) {
        from = Bitscan(bb);
        POP_LS1B(bb);
        // Capture promotions
        attacks = pawnAttackTable[side][from] & board->occupancies[1^side];
        while (attacks) {
            to = Bitscan(attacks);
            POP_LS1B(attacks);
            for (prom=0; prom<4; prom++) {
                (mp+(nmoves++))->move = ENCODE_MOVE(from,to,12+prom);
            }
        }

        // Push promotions
        to = from + 8*(1-2*side);
        if (!TEST_BIT(blockers,to)) {
            for (prom=0; prom<4; prom++) {
                (mp+(nmoves++))->move = ENCODE_MOVE(from,to,8+prom);
            }
        }
    }

    // Special moves
    // EP
    if ((to = board->ep_square) != -1) {
        pieces = pawnAttackTable[1^side][to] & board->bitboards[P + 6*side];
        while (pieces) {
            from = Bitscan(pieces);
            POP_LS1B(pieces);
            (mp+(nmoves++))->move = ENCODE_MOVE(from,to,5);
        }
    }

    // O-O
    from = e1 + 56*side;
    if ((board->castle_flags & (1<<(2*side))) && !(board->occupancies[BOTH] & kingside_castle_empty[side])) {
        if ((!is_square_attacked(board,from,1^side)) && (!is_square_attacked(board,from+1,1^side))) {
            (mp+(nmoves++))->move = ENCODE_MOVE(from, from+2, 2);
        } 
    }
    // O-O-O
    if ((board->castle_flags & (1<<(2*side + 1))) && !(board->occupancies[BOTH] & queenside_castle_empty[side])) {
        if ((!is_square_attacked(board,from,1^side)) && (!is_square_attacked(board,from-1,1^side))) {
            (mp+(nmoves++))->move = ENCODE_MOVE(from, from-2, 3);
        } 
    }

    // Knight moves

    pieces = board->bitboards[N + 6*side];
    while (pieces) {
        from = Bitscan(pieces);
        POP_LS1B(pieces);
        attacks = knightAttackTable[from] & ~board->occupancies[side];
        while (attacks) {
            to = Bitscan(attacks);
            POP_LS1B(attacks);
            flags = (board->squares[to] == _ ? 0 : 4);
            (mp+(nmoves++))->move = ENCODE_MOVE(from,to,flags);
        }
    }

    // Bishop (+ Queen) moves

    pieces = board->bitboards[B + 6*side] | board->bitboards[Q + 6*side];
    while (pieces) {
        from = Bitscan(pieces);
        POP_LS1B(pieces);
        attacks = bishopAttacks(from, blockers) & ~board->occupancies[side];
        while (attacks) {
            to = Bitscan(attacks);
            POP_LS1B(attacks);
            flags = (board->squares[to] == _ ? 0 : 4);
            (mp+(nmoves++))->move = ENCODE_MOVE(from,to,flags);
        }
    }

    // Rook (+ Queen) moves

    pieces = board->bitboards[R + 6*side] | board->bitboards[Q + 6*side];
    while (pieces) {
        from = Bitscan(pieces);
        POP_LS1B(pieces);
        attacks = rookAttacks(from, blockers) & ~board->occupancies[side];
        while (attacks) {
            to = Bitscan(attacks);
            POP_LS1B(attacks);
            flags = (board->squares[to] == _ ? 0 : 4);
            (mp+(nmoves++))->move = ENCODE_MOVE(from,to,flags);
        }
    }

    return nmoves;
}

int generate_captures(board_t* board, move_t* mp) {
    int side, from, to, pc, sq, nmoves, flags, prom;
    U64 bb, attacks, blockers, pieces;

    blockers = board->occupancies[BOTH];
    nmoves = 0;
    side = board->side;

    // King moves

    from = Bitscan(board->bitboards[K + 6*side]);
    attacks = kingAttackTable[from] & board->occupancies[1^side];
    while (attacks) {
        to = Bitscan(attacks);
        POP_LS1B(attacks);
        (mp+(nmoves++))->move = ENCODE_MOVE(from,to,4);
    }

    // Pawn moves (excl. EP)

    pieces = board->bitboards[P + 6*side];

    // Non promotions
    bb = pieces & ~seventhRank[side];
    while (bb) {
        from = Bitscan(bb);
        POP_LS1B(bb);
        // Pawn captures
        attacks = pawnAttackTable[side][from] & board->occupancies[1^side];
        while (attacks) {
            to = Bitscan(attacks);
            POP_LS1B(attacks);
            (mp+(nmoves++))->move = ENCODE_MOVE(from,to,4);
        }
    }

    // Promoting pawns
    bb = pieces & seventhRank[side];

    while (bb) {
        from = Bitscan(bb);
        POP_LS1B(bb);
        // Capture promotions
        attacks = pawnAttackTable[side][from] & board->occupancies[1^side];
        while (attacks) {
            to = Bitscan(attacks);
            POP_LS1B(attacks);
            for (prom=0; prom<4; prom++) {
                (mp+(nmoves++))->move = ENCODE_MOVE(from,to,12+prom);
            }
        }
    }

    // Special moves
    // EP
    if ((to = board->ep_square) != -1) {
        pieces = pawnAttackTable[1^side][to] & board->bitboards[P + 6*side];
        while (pieces) {
            from = Bitscan(pieces);
            POP_LS1B(pieces);
            (mp+(nmoves++))->move = ENCODE_MOVE(from,to,5);
        }
    }

    // Knight moves

    pieces = board->bitboards[N + 6*side];
    while (pieces) {
        from = Bitscan(pieces);
        POP_LS1B(pieces);
        attacks = knightAttackTable[from] & board->occupancies[1^side];
        while (attacks) {
            to = Bitscan(attacks);
            POP_LS1B(attacks);
            flags = (board->squares[to] == _ ? 0 : 4);
            (mp+(nmoves++))->move = ENCODE_MOVE(from,to,flags);
        }
    }

    // Bishop (+ Queen) moves

    pieces = board->bitboards[B + 6*side] | board->bitboards[Q + 6*side];
    while (pieces) {
        from = Bitscan(pieces);
        POP_LS1B(pieces);
        attacks = bishopAttacks(from, blockers) & board->occupancies[1^side];
        while (attacks) {
            to = Bitscan(attacks);
            POP_LS1B(attacks);
            flags = (board->squares[to] == _ ? 0 : 4);
            (mp+(nmoves++))->move = ENCODE_MOVE(from,to,flags);
        }
    }

    // Rook (+ Queen) moves

    pieces = board->bitboards[R + 6*side] | board->bitboards[Q + 6*side];
    while (pieces) {
        from = Bitscan(pieces);
        POP_LS1B(pieces);
        attacks = rookAttacks(from, blockers) & board->occupancies[1^side];
        while (attacks) {
            to = Bitscan(attacks);
            POP_LS1B(attacks);
            flags = (board->squares[to] == _ ? 0 : 4);
            (mp+(nmoves++))->move = ENCODE_MOVE(from,to,flags);
        }
    }

    return nmoves;
}