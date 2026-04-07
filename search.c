#include <stdint.h>
#include "search.h"
#include "movegen.h"
#include "tt.h"
#include "eval.h"


clock_t start;
float bot_time_control = 1.0;
float elapsed_time;
uint64_t nodes;

int quiesence_search(Position *pos, int alpha, int beta){
    nodes++; //DEBUG
    MoveList moves; LegalData legs; compute_pins_n_checks(pos, &legs);
    moves.count = 0;
    generate_moves(pos, &moves, &legs, legs.checkers ? GEN_ALL : GEN_QSN);

    int stand_pat = (bot_time_control==3.14159f) ? 314 : evaluate(pos);
    if (stand_pat >= beta) return beta;
    if (stand_pat > alpha) alpha = stand_pat;

    int ref = be_referee(pos, &moves, &legs, 0);
    if (ref!=1) return ref ? ref : stand_pat;

    order_moves(&moves, pos, &legs); int best_score = -INF; Undo undoer;

    for (uint8_t i=0; i<moves.count; i++){
        make_move(pos, moves.moves[i], &undoer);
        int evalution = -quiesence_search(pos, -beta, -alpha);
        unmake_move(pos, moves.moves[i], &undoer);

        if (evalution == -TIMEOUT) return TIMEOUT;

        if (evalution > best_score) {
            best_score = evalution;
        }

        if (evalution >= beta){ //Move was so good that the opponent prolly wants to avoid ts
            return beta; //Stop this engine line, go *snip* MWAHAHA
        } if (evalution > alpha){
            alpha = evalution;
        }
        
        //Time check
        if ((nodes&0x3FF) <= 0x7F){
            elapsed_time = (float)(clock()-start) / CLOCKS_PER_SEC;
            if (elapsed_time >= bot_time_control){
                return TIMEOUT;
            }
        }
    }

    return alpha;
}

int search(Position *pos, uint8_t depth, int alpha, int beta, uint16_t *move){
    int alpha_orig = alpha;
    if (depth==0) return quiesence_search(pos, alpha, beta); //Quiesence comes later

    //TT!
    TTEntry *entry = tt_probe_ptr(pos->zobrist);
    if (entry && entry->depth >= depth) {
        if (entry->flag == TT_EXACT)
            return entry->score;  // exact score
        if (entry->flag == TT_LOWERBOUND && entry->score > alpha)
            alpha = entry->score;  // improve alpha
        if (entry->flag == TT_UPPERBOUND && entry->score < beta)
            beta = entry->score;   // reduce beta

        if (alpha >= beta)
            return entry->score;   // cutoff
    }

    Undo undoer; MoveList moves; LegalData legs;
    moves.count = 0; bool timed_out = false; bool searched_any = false;
    generate_moves(pos, &moves, &legs, GEN_ALL);

    int ref = be_referee(pos, &moves, &legs, depth);
    if (ref!=1) return ref;
    order_moves(&moves, pos, &legs);
    uint8_t extender = extensions(pos, &moves, &legs);

    int best_score = -INF; uint16_t best_move = 0; bool beta_cutoff = false;
    //Search
    for (uint8_t i=0; i<moves.count; i++){
        make_move(pos, moves.moves[i], &undoer);
        int evalution = -search(pos, depth-1 + extender, -beta, -alpha, NULL);
        unmake_move(pos, moves.moves[i], &undoer);

        if (evalution == -TIMEOUT){
            timed_out = true;
            if (move) break;
            else if (searched_any) return TIMEOUT;
        }

        if (evalution > best_score) {
            best_score = evalution;
            best_move = moves.moves[i];
        }

        if (evalution >= beta){ //Move was so good that the opponent prolly wants to avoid ts
            beta_cutoff = true;
            if (!move){
                tt_store(pos->zobrist, depth, evalution, TT_LOWERBOUND, moves.moves[i]);
                return evalution; //Stop this engine line, go *snip* MWAHAHA
            }

            break;
        } if (evalution > alpha){
            alpha = evalution;
        }

        //Time check
        if ((nodes&0x3FF) <= 0x7F){
            elapsed_time = (float)(clock()-start) / CLOCKS_PER_SEC;
            if (elapsed_time >= bot_time_control){
                if (move) break;
                else if (searched_any) return TIMEOUT;
            }
            searched_any = true;
        }
    }

    //Write to TT
    if (!timed_out && !move) {
        // store TT only if search was fully completed
        uint8_t flag;
        if (best_score <= alpha_orig) flag = TT_UPPERBOUND;
        else if (beta_cutoff) flag =  TT_LOWERBOUND;
        else flag = TT_EXACT;
        
        tt_store(pos->zobrist, depth, best_score, flag, best_move);
    }

    if (move) *move = best_move;
    //Return
    return best_score;
}

uint8_t extensions(Position *pos, MoveList *moves, LegalData *legs){
    uint8_t retval = 0;
    if (legs->checkers) retval = 1;
    if (moves->count==1) retval = 1;

    return retval;
}

uint16_t get_best_move(Position *pos, uint8_t depth){
    nodes = 0;
    uint16_t move = 0;
    search(pos, depth, -INF, INF, &move);

    LegalData legs;
    compute_pins_n_checks(pos, &legs);

    //if (bot_time_control != 3.14159f)
    //printf("Nodes: %llu\n", nodes);

    MoveList movers;
    generate_moves(pos, &movers, &legs, GEN_ALL);

    int draw_type = get_draw_type(pos, &movers, &legs);
    if(draw_type == STALEMATE || draw_type == FIFTYMOVES || draw_type == THREEFOLD)
        return 0;
    if(draw_type == CHECKMATE)
        return UINT16_MAX;

    return move ? move : TIMEOUT_MOVE; //This is only to prevent broken things
}
//Move ordering
void order_moves(MoveList *ml, Position *pos, LegalData *legs){
    const uint8_t k = 8; //How many moves to actually sort
    int n = ml->count;

    int scores[n];
    for (uint16_t i=0; i<n; i++){
        TTEntry *entry = tt_probe_ptr(pos->zobrist);
        scores[i] = guess_move_priority(ml->moves[i], pos, legs, entry);
    }

    uint8_t lim = k<n ? k : n;

    for (int i = 0; i < lim; i++) {
        int best_idx = i;
        for (int j = i + 1; j < n; j++) {
            if (scores[j] > scores[best_idx])
                best_idx = j;
        }
        // Swap
        uint16_t temp = ml->moves[best_idx];
        ml->moves[best_idx] = ml->moves[i];
        ml->moves[i] = temp;

        //Score swap
        int temp_score = scores[best_idx];
        scores[best_idx] = scores[i];
        scores[i] = temp_score;
    }
}

//I moved the guess_move_priority to search.h for inline reasons

//Hey you! Yes, you! Oh, you don't wanna be the referee? Well, he can be!
int be_referee(Position *pos, MoveList *moves, LegalData *legs, int depth){
    int draw_type = get_draw_type(pos, moves, legs);
    switch (draw_type){
        case STALEMATE: return CONTEMPT; break;
        case FIFTYMOVES: return CONTEMPT; break;
        case THREEFOLD: return CONTEMPT; break;
        case CHECKMATE: return -INF-depth; break; //Bcz what can be worse than losing a game of chess?
    }
    return 1;
}

int get_draw_type(Position *pos, MoveList *moves, LegalData *legs){
    if (pos->halfmove>=100) return FIFTYMOVES; //50 move rule

    if (moves->count == 0){
        if (legs->checkers){ //CHECKMATE!!
            return CHECKMATE;
        }
        return STALEMATE; //Stalemate
    }

    uint8_t repetition_counter = 1;
    for (int i=last_irreversible; i<rep_idx-1; i++){
        if (repetition_tableaus[i]==pos->zobrist){
            if (++repetition_counter==3) return THREEFOLD; //Threefold repetition
        }
    }

    return 0; //Game continues
}