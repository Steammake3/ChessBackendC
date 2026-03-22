#include <stdint.h>
#include "search.h"
#include "movegen.h"
#include "tt.h"

#define INF 999999
#define popcount(x) __builtin_popcountll(x)

#define pawnValue 100
#define knightValue 300
#define bishopValue 310
#define rookValue 500
#define queenValue 900

int values[6] = {pawnValue, knightValue, bishopValue, rookValue, queenValue, 0}; //King should take

//Helpers
int material_eval(Position *pos){
    int pawns = (popcount(pos->bitboards[WP])-popcount(pos->bitboards[BP]))*pawnValue;
    int knights = (popcount(pos->bitboards[WN])-popcount(pos->bitboards[BN]))*knightValue;
    int bishops = (popcount(pos->bitboards[WB])-popcount(pos->bitboards[BB]))*bishopValue;
    int rooks = (popcount(pos->bitboards[WR])-popcount(pos->bitboards[BR]))*rookValue;
    int queens = (popcount(pos->bitboards[WQ])-popcount(pos->bitboards[BQ]))*queenValue;

    int perspective = pos->side_to_move ? -1 : 1;
    return (pawns+knights+bishops+rooks+queens) * perspective;
}

//GGGOOOOOO!!!!
int evaluate(Position *pos){
    return material_eval(pos);
}

int search(Position *pos, uint8_t depth, int alpha, int beta){
    if (depth==0) return evaluate(pos); //Quiesence comes later

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
    generate_moves(pos, &moves, &legs, GEN_ALL);

    if (moves.count==0){
        if (legs.checkers){ //checkmate );
            return -INF;
        }
        return 0; //Stalemate
    }
    order_moves(&moves, pos, &legs);

    int best_score = -INF; uint16_t best_move = 0;
    //Search
    for (uint8_t i=0; i<moves.count; i++){
        make_move(pos, moves.moves[i], &undoer);
        int evalution = -search(pos, depth-1, -beta, -alpha);
        unmake_move(pos, moves.moves[i], &undoer);

        if (evalution > best_score) {
            best_score = evalution;
            best_move = moves.moves[i];
        }

        if (evalution >= beta){ //Move was so good that the opponent prolly wants to avoid ts
            tt_store(pos->zobrist, depth, beta, TT_LOWERBOUND, moves.moves[i]);
            return beta; //Stop this engine line, go *snip* MWAHAHA
        } if (evalution > alpha){
            alpha = evalution;
        }
    }

    //Write to TT
    uint8_t flag;
    if (best_score<=alpha) flag = TT_UPPERBOUND;
    else if (best_score>=beta) flag = TT_LOWERBOUND;
    else flag = TT_EXACT;

    tt_store(pos->zobrist, depth, best_score, flag, best_move);
    //Return
    return alpha;
}

uint16_t get_best_move(Position *pos, uint8_t depth){
    search(pos, depth, -INF, INF);
    TTEntry *entry = tt_probe_ptr(pos->zobrist);

    MoveList moves; LegalData legs; moves.count = 0;
    generate_moves(pos, &moves, &legs, GEN_ALL);
    return (entry ? entry->move : (legs.checkers ? UINT16_MAX : 0));
}

//Move ordering
void order_moves(MoveList *ml, Position *pos, LegalData *legs){
    const uint8_t k = 4; //How many moves to actually sort
    int n = ml->count;
    for (int i = 0; i < k && i < n; i++) {
        int best_idx = i;
        for (int j = i + 1; j < n; j++) {
            if (guess_move_priority(ml->moves[j], pos, legs)
                > guess_move_priority(ml->moves[best_idx], pos, legs))
                best_idx = j;
        }
        // Swap
        uint16_t temp = ml->moves[best_idx];
        ml->moves[best_idx] = ml->moves[i];
        ml->moves[i] = temp;
    }
}

int guess_move_priority(uint16_t move, Position *pos, LegalData *legs){
    TTEntry *entry = tt_probe_ptr(pos->zobrist);
    if (entry && move == entry->move) return INF; //Hashed move

    int guess_rn = 0;

    uint8_t from = MOVE_FROM(move); uint8_t to = MOVE_TO(move); uint8_t promo = MOVE_FLAGS(move);
    uint8_t mover = piece_at(from, pos); uint8_t mover_type = PIECEtype(mover);
    uint8_t victim = piece_at(to, pos); uint8_t victim_type = PIECEtype(victim);

    if (victim!=NO_SQ){ //MVV-LVA
        guess_rn += 250 + 10 * values[victim_type] - values[mover_type];
    }

    if (mover_type==WP && (to%8==0 || to%8==7)){ //Promo
        guess_rn += 100 + values[4-PIECEtype(promo)];
    }

    if (mover_type==WK && ABS(to-from)==2){ //Castling
        int base = 900;
        int decay = pos->fullmove * 50;
        int bonus = base - decay;
        if (bonus < 50) bonus = 50;
        guess_rn += bonus;
    }

    if (BBd(to)&legs->enemy_attack_maps){ //Don't be attacked
        guess_rn -= values[mover_type];
    }

    return guess_rn;
}