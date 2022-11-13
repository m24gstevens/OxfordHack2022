#include "move.h"
#include "board.h"
#include "bitboard.h"
#include "movegen.h"
#include "tt.h"

void print_move(U16 move) {
    if (IS_PROMOTION(move)) {
        printf("%s%s%c", square_strings[MOVE_FROM(move)], square_strings[MOVE_TO(move)],promoted_pieces[PROMOTE_TO(move)]);
    } else {
        printf("%s%s", square_strings[MOVE_FROM(move)], square_strings[MOVE_TO(move)]);
    }
}

bool is_square_attacked(board_t* board, int sq, int side) {
    if (kingAttackTable[sq] & board->bitboards[K + 6*side]) {return true;}    // King attacks
    if (knightAttackTable[sq] & board->bitboards[N + 6*side]) {return true;}   // Knight attacks
    if (pawnAttackTable[1^side][sq] & board->bitboards[P + 6*side]) {return true;}  // Pawn attacks
    if (bishopAttacks(sq, board->occupancies[BOTH]) & (board->bitboards[B + 6*side] | board->bitboards[Q + 6*side])) {return true;}   // Diagonal attacks
    if (rookAttacks(sq, board->occupancies[BOTH]) & (board->bitboards[R + 6*side] | board->bitboards[Q + 6*side])) {return true;}   // Rank/File attacks
    return false;
}

const U8 castling_rights_update[64] = {
    13,15,15,15,12,15,15,14,
    15, 15,15,15,15,15,15,15,
    15, 15,15,15,15,15,15,15,
    15, 15,15,15,15,15,15,15,
    15, 15,15,15,15,15,15,15,
    15, 15,15,15,15,15,15,15,
    15, 15,15,15,15,15,15,15,
    7,15,15,15,3,15,15,11
};

bool make_move(board_t* board, U16 move, hist_t* undo) {
    int king, square, from, to, captured, flags, side, moved, epsq, prom, rook_home, rook_castles;
    U64 from_bb, to_bb, from_to_bb, epbb, hash;

    side = board->side;
    from = MOVE_FROM(move);
    to = MOVE_TO(move);
    flags = MOVE_FLAGS(move);
    from_bb = C64(1) << from;
    to_bb = C64(1) << to;
    from_to_bb = from_bb | to_bb;
    moved = board->squares[from];
    captured = board->squares[to];
    hash = board->hash;

    // Save irreversible information
    undo->rule50 = board->rule50;
    undo->castle_flags = board->castle_flags;
    undo->ep_square = board->ep_square;
    undo->captured = captured;
    undo->hash = board->hash;

    // Update occupancies and bitboards
    board->bitboards[moved] ^= from_to_bb;
    board->occupancies[side] ^= from_to_bb;
    board->occupancies[BOTH] ^= from_to_bb;
    board->squares[from] = _;
    board->squares[to] = moved;

    hash ^= piece_keys[moved][from];
    hash ^= piece_keys[moved][to];
    hash ^= piece_keys[captured][to];

    // Captures (incl. EP)
    if (IS_CAPTURE(move)) {
        if (flags == 5) {
            epsq = (to + 16*side - 8);
            epbb = C64(1) << epsq;
            board->squares[epsq] = _;
            board->bitboards[p-6*side] ^= epbb;
            board->occupancies[BOTH] ^= epbb;
            board->occupancies[1^side] ^= epbb;
            hash ^= piece_keys[p - (6*side)][epsq];
        } else {
            board->bitboards[captured] ^= to_bb;
            board->occupancies[BOTH] ^= to_bb;
            board->occupancies[1^side] ^= to_bb;
        }
    }

    // Promotions
    if (IS_PROMOTION(move)) {
        prom = N + 6*side + PROMOTE_TO(move);
        board->squares[to] = prom;
        board->bitboards[prom] ^= to_bb;
        board->bitboards[P + 6*side] ^= to_bb;
        hash ^= piece_keys[moved][to];
        hash ^= piece_keys[prom][to];
    }

    // Castling
    if (flags == 2) {
        rook_home = h1 + 56*side;
        rook_castles = f1 + 56*side;
        from_to_bb = (C64(1) << rook_home) | (C64(1) << rook_castles);
        board->squares[rook_castles] = R + 6*side;
        board->squares[rook_home] = _;
        board->occupancies[side] ^= from_to_bb;
        board->occupancies[BOTH] ^= from_to_bb;
        board->bitboards[R + 6*side] ^= from_to_bb;
        hash ^= piece_keys[R + 6*side][rook_home];
        hash ^= piece_keys[R + 6*side][rook_castles];
    } else if (flags == 3) {
        rook_home = a1 + 56*side;
        rook_castles = d1 + 56*side;
        from_to_bb = (C64(1) << rook_home) | (C64(1) << rook_castles);
        board->squares[rook_castles] = R + 6*side;
        board->squares[rook_home] = _;
        board->occupancies[side] ^= from_to_bb;
        board->occupancies[BOTH] ^= from_to_bb;
        board->bitboards[R + 6*side] ^= from_to_bb;
        hash ^= piece_keys[R + 6*side][rook_home];
        hash ^= piece_keys[R + 6*side][rook_castles];
    }

    // Update board state
    hash ^= side_key;
    if (board->ep_square != -1) {
        hash ^= ep_keys[side][FILE(board->ep_square)];
    }
    board->side ^= 1;

    hash ^= castle_keys[board->castle_flags];
    board->castle_flags &= castling_rights_update[from];
    board->castle_flags &= castling_rights_update[to];
    hash ^= castle_keys[board->castle_flags];
    
    if ((flags & 0x4) || (PIECE_TYPE(moved) == P)) {board->rule50 = 0;}
    else {board->rule50++;}

    if (flags == 1) { 
        board->ep_square = to + 16*side - 8;
        hash ^= ep_keys[1^side][FILE(to)];
    }
    else {board->ep_square = -1;}

    board->hash = hash; 

    // Check if king is attacked
    if (is_square_attacked(board,Bitscan(board->bitboards[K + 6*side]),1^side)) {
        unmake_move(board, move, undo);
        return false;
    }
    return true;
}

void unmake_move(board_t* board, U16 move, hist_t* undo) {
    int king, square, from, to, captured, flags, side, moved, epsq, prom, rook_home, rook_castles;
    U64 from_bb, to_bb, from_to_bb, epbb;

    side = 1^board->side;
    from = MOVE_FROM(move);
    to = MOVE_TO(move);
    flags = MOVE_FLAGS(move);
    from_bb = C64(1) << from;
    to_bb = C64(1) << to;
    from_to_bb = from_bb | to_bb;
    moved = board->squares[to];
    captured = undo->captured;

    // Occupancies and Bitboards
    board->bitboards[moved] ^= from_to_bb;
    board->occupancies[side] ^= from_to_bb;
    board->occupancies[BOTH] ^= from_to_bb;
    board->squares[from] = moved;
    board->squares[to] = _;

    // Captures

    if (IS_CAPTURE(move)) {
        if (flags == 5) {
            epsq = (to + 16*side - 8);
            epbb = C64(1) << epsq;
            board->squares[epsq] = p - 6*side;
            board->bitboards[p-6*side] ^= epbb;
            board->occupancies[BOTH] ^= epbb;
            board->occupancies[1^side] ^= epbb;
        } else {
            board->squares[to] = captured;
            board->bitboards[captured] ^= to_bb;
            board->occupancies[BOTH] ^= to_bb;
            board->occupancies[1^side] ^= to_bb;
        }

    }

    // Promotions

    if (IS_PROMOTION(move)) {
        board->bitboards[moved] ^= from_bb;
        board->bitboards[P + 6*side] ^= from_bb;
        board->squares[from] = P + 6*side;
    }

    // Castles

    if (flags == 2) {
        rook_home = h1 + 56*side;
        rook_castles = f1 + 56*side;
        from_to_bb = (C64(1) << rook_home) | (C64(1) << rook_castles);
        board->squares[rook_castles] = _;
        board->squares[rook_home] = R + 6*side;
        board->occupancies[side] ^= from_to_bb;
        board->occupancies[BOTH] ^= from_to_bb;
        board->bitboards[R + 6*side] ^= from_to_bb;
    } else if (flags == 3) {
        rook_home = a1 + 56*side;
        rook_castles = d1 + 56*side;
        from_to_bb = (C64(1) << rook_home) | (C64(1) << rook_castles);
        board->squares[rook_castles] = _;
        board->squares[rook_home] = R + 6*side;
        board->occupancies[side] ^= from_to_bb;
        board->occupancies[BOTH] ^= from_to_bb;
        board->bitboards[R + 6*side] ^= from_to_bb;
    }

    // Restore irreversibles

    board->rule50 = undo->rule50;
    board->castle_flags = undo->castle_flags;
    board->ep_square = undo->ep_square;
    board->side ^= 1;
    board->hash = undo->hash;
}

void make_nullmove(board_t* board, hist_t* undo) {
    undo->captured = _;
    undo->ep_square = board->ep_square;
    undo->castle_flags = board->castle_flags;
    undo->rule50 = board->rule50;
    undo->hash = board->hash;

    if (board->ep_square != -1) {
        board->hash ^= ep_keys[board->side][FILE(board->ep_square)];
    }
    board->hash ^= side_key;
    board->ep_square = -1;
    board->side ^= 1;
}

void unmake_nullmove(board_t* board, hist_t* undo) {
    board->ep_square = undo->ep_square;
    board->castle_flags = undo->castle_flags;
    board->rule50 = undo->rule50;
    board->hash = undo->hash;
    board->side ^= 1;
}

long perft(board_t* board, int depth) {
    move_t movelist[256];
    hist_t undo;
    int nmoves, i;
    long nodes;

    if (!depth) {return 1;}
    nodes = 0;
    nmoves = generate_moves(board, &movelist[0]);

    for (i=0; i<nmoves; i++) {
        if (make_move(board, movelist[i].move, &undo)) {
            nodes += perft(board, depth-1);
            unmake_move(board, movelist[i].move, &undo);
        }
    }
    return nodes;
}

void perft_test(board_t* board, int depth) {
    int i;
    long tim, nodes;
    printf("Perft Test\n");
    for (i=1; i<=depth; i++) {
        tim = get_time_ms();
        nodes = perft(board, i);
        printf("Depth %d Nodes %ld Time %ldms\n", i, nodes, get_time_ms() - tim);
    }
}

void divide(board_t* board, int depth) {
    print_board(board);
    // Depth >= 1
    move_t movelist[256];
    hist_t undo;
    int nmoves, i;
    long nodes, total_nodes, tim;
    printf("Divide (perft) test\n");
    tim = get_time_ms();
    nmoves = generate_moves(board, &movelist[0]);
    total_nodes = 0;
    for (i=0; i<nmoves; i++) {
        if (make_move(board, movelist[i].move, &undo)) {
            nodes = perft(board, depth-1);
            printf("Move ");
            print_move(movelist[i].move);
            printf(" nodes %ld\n", nodes);
            total_nodes += nodes;
            unmake_move(board, movelist[i].move, &undo);
        }
    }
    printf("Total nodes %ld time %ldms", total_nodes, get_time_ms() - tim);
}