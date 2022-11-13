#include "search.h"
#include "evaluate.h"
#include "movegen.h"
#include "move.h"
#include "uci.h"
#include "order.h"
#include "board.h"
#include "tt.h"

void prepare_search(search_info_t* si, move_t* movestack) {
    si->ply = 0;
    si->msp[0] = movestack;
    si->nodes = 0;
    memset(si->pv,0,sizeof(U16)*MAXPLY*MAXPLY);
    age_history();
}

void update_pv(search_info_t* si, U16 move, int ply) {
    int i;
    U16 pvm;
    si->pv[ply][ply] = move;
    for (i=1; (i + ply <MAXPLY) && ((pvm = si->pv[ply+1][ply+i]) != NOMOVE); i++) {
        si->pv[ply][ply + i] = pvm;
    }
    if (!ply) {
        si->bestmove = move;
    }
}

void print_pv(search_info_t* si, int depth) {
    int i;
    U16 line;
    for (i=0; i<depth && ((line = si->pv[0][i]) != NOMOVE); i++) {
        print_move(line);
        printf(" ");
    }
}

int qsearch(board_t* board, search_info_t* si, int alpha, int beta) {
    int score, ply, nmoves, i, stand_pat;
    U16 move;
    move_t* mp;
    hist_t undo;

    if (alpha >= beta) {return alpha;}

    stand_pat = evaluate(board);

    if (stand_pat >= beta) {
        return beta;
    }
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    if (!(si->nodes & 2047)) {
        communicate(si);
    }
    if (time_control.stop) {return 0;}

    si->nodes++;

    ply = si->ply;
    mp = si->msp[ply];

    nmoves = generate_captures(board, mp);
    si->msp[ply+1] = mp + nmoves;

    score_moves(board,si,mp,nmoves,NOMOVE);

    while ((move = pick_move(mp++, nmoves--)) != NOMOVE) {
        if (!make_move(board, move, &undo)) {continue;}

        si->ply++;

        score = -qsearch(board, si, -beta, -alpha);

        si->ply--;
        
        unmake_move(board, move, &undo);

        if (score > alpha) {
            alpha = score;

            if (score >= beta) {
                return beta;
            }
        }
    }

    return alpha;
}

int search(board_t* board, search_info_t* si, int depth, int alpha, int beta) {
    int score, ply, nmoves, searched, legal, i, new_depth, reduction, eval;
    U16 move, pvmove = NOMOVE, hashmv = NOMOVE;
    U8 hashflag = HASH_FLAG_ALPHA;
    bool in_check, pvnode, found_pv=false;
    move_t* mp;
    hist_t undo;

    if (alpha >= beta) {return alpha;}

    ply = si->ply;
    pvnode = ((beta - alpha) > 1);

    if (ply && is_draw(board,si)) {
        si->nodes++;
        return 0;
    }

    if (ply && (score=probe_tt(board, ply, depth, alpha, beta, &pvmove)) != INVALID) {
        return score;
    }

    if (!(si->nodes & 2047)) {
        communicate(si);
    }
    if (time_control.stop) {return 0;}


    if (depth == 0) {return qsearch(board, si, alpha, beta);}

    if (depth >= MAXPLY) {
        si->nodes++;
        return evaluate(board);
    }

    si->nodes++;

    in_check = is_square_attacked(board, Bitscan(board->bitboards[K + 6*board->side]), 1^board->side);
    if (in_check) {depth++;}

    eval = evaluate(board);

    // Razoring

    if (depth == 1 && eval <= alpha - 300) {
        return qsearch(board, si, alpha, beta);
    }

    // Static Nullmove

    if (depth <= 6 && abs(beta) < 19000) {
        int margin = 120*depth;
        if (eval - margin >= beta) {
            return beta;
        }
    }

    // Nullmove pruning

    if (ply && !in_check && depth >= 3) {
        new_depth = depth-1;
        reduction = (depth >= 8) ? 3 : 2;
        new_depth -= reduction;
        make_nullmove(board, &undo);
        si->msp[ply+1] = si->msp[ply];
        si->ply++;
        score = -search(board, si, new_depth, -beta, -beta+1);
        si->ply--;
        unmake_nullmove(board, &undo);

        if (score >= beta) {
            return beta;
        }
    }

    // Internal IID
    if ((pvmove == NOMOVE) && pvnode && depth >= 6) {
        new_depth = depth - (depth / 4) - 1;
        score = search(board, si, new_depth, alpha, beta);
        if ((score > alpha) && (score < beta)) {
            pvmove = si->pv[ply][ply];
        }
    }
    
    mp = si->msp[ply];
    nmoves = generate_moves(board, mp);
    si->msp[ply+1] = mp + nmoves;

    score_moves(board,si,mp,nmoves, pvmove);
    if (si->follow_pv) {
        score_pv(si, mp, nmoves, si->pv[ply][ply]);
    }


    searched = 0;

    while ((move = pick_move(mp++, nmoves--)) != NOMOVE) {
        if (!make_move(board, move, &undo)) {continue;}

        si->ply++;
        searched++;

        new_depth = depth-1;
        reduction = 0;
        if (found_pv
            && new_depth > 3
            && searched > 3
            && !IS_CAPTURE(move)
            && !IS_PROMOTION(move)
            && !in_check) {
            
            reduction = 1;
            if (searched > 6) {reduction++;}
            if ((move == si->killers[0][ply]) || (move == si->killers[1][ply])) {reduction--;}
            new_depth -= reduction;
        }

        // PVS

        if (!found_pv) {
            score = -search(board,si,new_depth,-beta,-alpha);
        } else {
            score = -search(board, si, new_depth, -alpha-1, -alpha);
            
            if (reduction && (score > alpha)) {
                new_depth += reduction;
                score = -search(board, si, new_depth, -alpha-1, -alpha);
            }

            if ((score > alpha) && (score < beta)) {
                score = -search(board,si,depth-1, -beta, -alpha);
            }
        }

        score = -search(board, si, depth-1, -beta, -alpha);

        si->ply--;
        
        unmake_move(board, move, &undo);

        if (score > alpha) {
            alpha = score;
            hashflag = HASH_FLAG_EXACT;
            hashmv = move;
            found_pv = true;
            update_pv(si,move,ply);

            if (score >= beta) {
                hashflag = HASH_FLAG_BETA;
                alpha = beta;
                if (!IS_TACTICAL(move)) {
                    score_cutoff(board,si,move,ply,depth);
                }
                break;
            }
        }
    }

    if (!searched) {
        return (in_check ? -MATE + ply : 0);
    }

    store_tt(board, depth, ply, alpha, hashflag, hashmv);

    return alpha;
}

int aspiration_window(board_t* board, search_info_t* si, int depth, int score) {
    int alpha, beta;

    alpha = score - 75;
    beta = score + 75;
    score = search(board, si, depth, alpha, beta);

    if ((score <= alpha) || (score >= beta)) {
        score = search(board, si, depth, -INF, INF);
    }
    return score;
}

void iterative_deepening(board_t* board, search_info_t* si) {
    int depth, score;
    U16 bestmove;

    si->sdepth = 1;
    score = search(board,si,1,-INF,INF);
    bestmove = si->bestmove;

    for (depth = 2; depth <= MAXPLY; depth++) {
        if (time_control.stop || time_stop_root(si)) {break;}
        bestmove = si->bestmove;

        printf("info score cp %d depth %d nodes %ld pv ", score, depth-1, si->nodes);
        print_pv(si, depth);
        printf("\n");

        si->sdepth = depth;
        score = aspiration_window(board,si,depth,score);
    }

    send_move(bestmove);
}

void search_position(board_t* board) {
    search_info_t si;
    move_t movestack[MAXMOVES];
    prepare_search(&si, movestack);
    time_calc(board);

    int score;
    iterative_deepening(board, &si);
}