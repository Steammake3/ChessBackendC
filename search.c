#include <stdint.h>
#include "search.h"
#include "movegen.h"
#include "tt.h"

#define INF 999999
#define TIMEOUT 99999999
#define popcount(x) __builtin_popcountll(x)

#define pawnValue 100
#define knightValue 300
#define bishopValue 310
#define rookValue 500
#define queenValue 900

int values[6] = {pawnValue, knightValue, bishopValue, rookValue, queenValue, 0}; //King should take

time_t start;
int bot_time_control = 1;
int elapsed_time;
uint64_t nodes;
const int8_t middle_psts[6][64] = {
    {0, 0, 0, 0, 0, 0, 0, 0, //Black Pawns
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
    5, 5, 10, 25, 25, 10, 5, 5,
    0, 0, 0, 20, 20, 0, 0, 0,
    5, -5, -10, 0, 0, -10, -5, 5,
    5, 10, 10, -20, -20, 10, 10, 5,
    0, 0, 0, 0, 0, 0, 0, 0},
    {-50, -40, -30, -30, -30, -30, -40, -50, //Black Knights
    -40, -20, 0, 0, 0, 0, -20, -40,
    -30, 0, 10, 15, 15, 10, 0, -30,
    -30, 5, 15, 20, 20, 15, 5, -30,
    -30, 5, 15, 20, 20, 15, 5, -30,
    -30, 5, 10, 15, 15, 10, 5, -30,
    -40, -20, 0, 5, 5, 0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50},
    {-20, -10, -10, -10, -10, -10, -10, -20, //Black Bishops
    -10, 0, 0, 0, 0, 0, 0, -10,
    -10, 0, 5, 10, 10, 5, 0, -10,
    -10, 5, 5, 10, 10, 5, 5, -10,
    -10, 0, 10, 10, 10, 10, 0, -10,
    -10, 10, 10, 10, 10, 10, 10, -10,
    -10, 5, 0, 0, 0, 0, 5, -10,
    -20, -10, -10, -10, -10, -10, -10, -20},
    {0, 0, 0, 0, 0, 0, 0, 0, //Black Rooks
    5, 10, 10, 10, 10, 10, 10, 5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    0, 0, 0, 5, 5, 0, 0, 0},
    {-20, -10, -10, -5, -5, -10, -10, -20, //Black Queens
    -10, 0, 0, 0, 0, 0, 0, -10,
    -10, 0, 5, 5, 5, 5, 0, -10,
    -5, 0, 5, 5, 5, 5, 0, -5,
    0, 0, 5, 5, 5, 5, 0, -5,
    -10, 5, 5, 5, 5, 5, 0, -10,
    -10, 0, 5, 0, 0, 0, 0, -10,
    -20, -10, -10, -5, -5, -10, -10, -20},
    {-30,-40,-40,-50,-50,-40,-40,-30, //Black King
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
    20, 20,  0,  0,  0,  0, 20, 20,
    20, 30, 10,  0,  0, 10, 30, 20}
};
const int8_t endgame_psts[2][64] = {
    {0, 0, 0, 0, 0, 0, 0, 0,//Pawn
    80, 80, 80, 80, 80, 80, 80, 80,
    50, 50, 50, 50, 50, 50, 50, 50,
    30, 30, 30, 30, 30, 30, 30, 30,
    20, 20, 20, 20, 20, 20, 20, 20,
    10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10,
    0, 0, 0, 0, 0, 0, 0, 0},
    {-50,-40,-30,-20,-20,-30,-40,-50, //King
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50}
};

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

int pst_eval(Position *pos, uint8_t endgame_weight){
    bool side_rn = pos->side_to_move; int perspective;
    int retval = 0;
    for (uint8_t side = WHITE; side<=BLACK; side++){
        perspective = side ? -1 : 1;
        for (uint8_t i = WP; i<=WK; i++){
            uint64_t current_pieces = pos->bitboards[side ? i+6 : i];
            while (current_pieces)
            {
                uint8_t cur_sq = pop_lsb(&current_pieces);
                if (i==WP || i==WK){
                    retval += perspective * lerp(middle_psts[i][side==WHITE? FLIP(cur_sq) : cur_sq],
                        endgame_psts[i==WK ? 1 : 0][side==WHITE? FLIP(cur_sq) : cur_sq],
                        endgame_weight);
                }
                else retval += perspective * middle_psts[i][side==WHITE? FLIP(cur_sq) : cur_sq];
            }
        }
    }
    perspective = side_rn ? -1 : 1;
    return retval * perspective;
}

//GGGOOOOOO!!!!
int evaluate(Position *pos){
    uint8_t eg_weight = phase_to_eg_weight(compute_phase(pos));
    return material_eval(pos)+ pst_eval(pos, eg_weight); //TODO: game phase rn
}

int quiesence_search(Position *pos, int alpha, int beta){
    //nodes++; //DEBUG
    MoveList moves; LegalData legs; compute_pins_n_checks(pos, &legs);
    moves.count = 0;
    generate_moves(pos, &moves, &legs, legs.checkers ? GEN_ALL : GEN_QSN);

    int stand_pat = evaluate(pos);
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
        elapsed_time = time(NULL) - start;
        if (elapsed_time >= bot_time_control){
            return TIMEOUT;
        }
    }

    return best_score;
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
            return alpha;   // cutoff
    }

    Undo undoer; MoveList moves; LegalData legs;
    moves.count = 0; bool timed_out = false; bool searched_any = false;
    generate_moves(pos, &moves, &legs, GEN_ALL);

    int ref = be_referee(pos, &moves, &legs, depth);
    if (ref!=1) return ref;
    order_moves(&moves, pos, &legs);

    int best_score = -INF; uint16_t best_move = 0;
    //Search
    for (uint8_t i=0; i<moves.count; i++){
        make_move(pos, moves.moves[i], &undoer);
        int evalution = -search(pos, depth-1, -beta, -alpha, NULL);
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
            if (!move){
                tt_store(pos->zobrist, depth, beta, TT_LOWERBOUND, moves.moves[i]);
                return beta; //Stop this engine line, go *snip* MWAHAHA
            }

            break;
        } if (evalution > alpha){
            alpha = evalution;
        }

        //Time check
        elapsed_time = time(NULL) - start;
        if (elapsed_time >= bot_time_control){
            if (move) break;
            else if (searched_any) return TIMEOUT;
        }
        searched_any = true;
    }

    //Write to TT
    if (!timed_out && !move) {
        // store TT only if search was fully completed
        uint8_t flag = (best_score <= alpha_orig) ? TT_UPPERBOUND :
                   (best_score >= beta) ? TT_LOWERBOUND : TT_EXACT;
        tt_store(pos->zobrist, depth, best_score, flag, best_move);
    }

    if (move) *move = best_move;
    //Return
    return best_score;
}

uint16_t get_best_move(Position *pos, uint8_t depth){
    nodes = 0;
    uint16_t move = 0;
    search(pos, depth, -INF, INF, &move);

    LegalData legs;
    compute_pins_n_checks(pos, &legs);

    MoveList movers;
    generate_moves(pos, &movers, &legs, GEN_ALL);

    int draw_type = get_draw_type(pos, &movers, &legs);
    if(draw_type == STALEMATE || draw_type == FIFTYMOVES || draw_type == THREEFOLD)
        return 0;
    if(draw_type == CHECKMATE)
        return UINT16_MAX;

    return move ? move : UINT16_MAX-34; //This is only to prevent broken things
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