#include "oxchess.h"
#include "board.h"
#include "bitboard.h"
#include "search.h"
#include "uci.h"
#include "tt.h"

void init_all() {
    initAttackTables();
    initRandomKeys();
    initTT();
}

long get_time_ms() {
    return GetTickCount();
}

int main() {
    init_all();
    board_t board;
    int debug = 0;
    if (debug) {
        parse_position(&board, "position startpos");
        parse_go(&board, "go depth 7");
        return 0;
    }
    uci_loop(&board);
}