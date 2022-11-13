#include "board.h"
#include "tt.h"

U64 game_history[MAXHIST];
int hply;

char *starting_position = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
char *kiwipete = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ";
char *capture_position = "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1 ";
char *unsafe_king = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";

char *piece_characters = "KPNBRQkpnbrq.";

enumPiece char_to_piece_code[] = {
    ['K'] = K,
    ['P'] = P,
    ['N'] = N,
    ['B'] = B,
    ['R'] = R,
    ['Q'] = Q,
    ['k'] = k,
    ['p'] = p,
    ['n'] = n,
    ['b'] = b,
    ['q'] = q,
    ['r'] = r,
    ['.'] = _
};

char *square_strings[64] = {
    "a1","b1","c1","d1","e1","f1","g1","h1",
    "a2","b2","c2","d2","e2","f2","g2","h2",
    "a3","b3","c3","d3","e3","f3","g3","h3",
    "a4","b4","c4","d4","e4","f4","g4","h4",
    "a5","b5","c5","d5","e5","f5","g5","h5",
    "a6","b6","c6","d6","e6","f6","g6","h6",
    "a7","b7","c7","d7","e7","f7","g7","h7",
    "a8","b8","c8","d8","e8","f8","g8","h8"
};

char *promoted_pieces= "nbrq";

void print_board(board_t* board) {
    int i,j;
    for (j=7;j>=0;j--) {
        printf(" %d  ",j+1);
        for (i=0; i<8; i++) {
            printf("%c ", piece_characters[board->squares[8*j+i]]);
        }
        printf("\n");
    }
    printf("\n    A B C D E F G H\n\n");
    printf("Side: %d\n", board->side);
    printf("Castling rights: %d\n", board->castle_flags);
    printf("Fifty move clock: %d\n", board->rule50);
    if (board->ep_square != -1) {
        printf("En Passent Square: %s\n", square_strings[board->ep_square]);
    }
}

void parse_fen(board_t* board,char* fen) {
    memset(&board->bitboards,0,sizeof(board->bitboards));
    memset(&board->occupancies,0,sizeof(board->occupancies));
    memset(&board->squares,_,sizeof(board->squares));

    board->ep_square = 0;
    board->castle_flags = 0;
    board->side = WHITE;

    for (int r=7; r >= 0; r--) {
        for (int f=0; f <= 8; f++) {
            if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z')) {
                char piece = char_to_piece_code[*fen++];
                board->bitboards[piece] |= ((U64)1 << (8*r + f));
                board->squares[8*r + f] = piece;
                board->occupancies[piece/6] |= ((U64)1 << (8*r + f));
            }
            if (*fen >= '0' && *fen <= '9') {
                int offset = *fen++ - '0';
                if (board->squares[8*r + f] == _) f--;
                f += offset;
            }
            if (*fen == '/') {
                fen++;
                break;
            }
        }
    }
    /* Side to move */
    if (*(++fen) == 'b') {board->side = BLACK;};
    fen += 2;
    /* Castling rights */
    while (*fen != ' ') {
        switch(*fen) {
            case 'K': board->castle_flags |= WCK; break;
            case 'Q': board->castle_flags |= WCQ; break;
            case 'k': board->castle_flags |= BCK; break;
            case 'q': board->castle_flags |= BCQ; break;
        }
        fen++;
    }
    fen++;
    /* en passent square */
    if (*fen != '-') {
        int fl = *fen++ - 'a';
        int rk = *fen - '1';
        board->ep_square = 8 * rk + fl;
    } else {board->ep_square = -1;}

    /* Fifty move clock */
    board->rule50 = atoi(fen + 2);
    /* occupancies */
    board->occupancies[BOTH] = board->occupancies[WHITE] | board->occupancies[BLACK];

    board->hash = hash_position(board);

    hply = 0;
}

bool is_draw(board_t* board, search_info_t* si) {
    // Draw by repetition or 50 move rule
    if (board->rule50 >= 100) {return true;}
    game_history[hply + si->ply] = board->hash;
    
    for (int x = hply + si->ply - 2; x >= MAX(0,hply + si->ply - board->rule50); x-=2) {
        if (board->hash == game_history[x]) {return true;}
    }
    return false;
}
