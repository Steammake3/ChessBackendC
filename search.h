#ifndef SEARCH_H
#define SEARCH_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "position.h"
#include "tt.h"
#include "movegen.h"

#define STALEMATE 1
#define THREEFOLD 2
#define FIFTYMOVES 3
#define CHECKMATE 4
#define CONTEMPT 0 //At some point I will add/fix this but for rn it breaks symmetry
#define FLIP(sq) sq^0b111000

extern time_t start;
extern float bot_time_control;
extern float elapsed_time;
extern uint64_t nodes;
static const int phase_weights[6] = {
    0, // pawn
    1, // knight
    1, // bishop
    2, // rook
    4, // queen
    0  // king
};
//TODO: This is from BLACK's perspective
const int8_t middle_psts[6][64];
const int8_t endgame_psts[2][64]; //Pawn then King!

int evaluate(Position *pos);
int be_referee(Position *pos, MoveList *moves, LegalData *legs, int depth);
int get_draw_type(Position *pos, MoveList *moves, LegalData *legs);

int quiesence_search(Position *pos, int alpha, int beta);
int search(Position *pos, uint8_t depth, int alpha, int beta, uint16_t *move);

//Helpers
int material_eval(Position *pos);
int mobility_eval(Position *pos);
int pst_eval(Position *pos, uint8_t endgame_weight);
int king_safety_eval(Position *pos);
int pawn_structure_eval(Position *pos);

uint16_t get_best_move(Position *pos, uint8_t depth);

void order_moves(MoveList *ml, Position *pos, LegalData *legs);
int guess_move_priority(uint16_t move, Position *pos, LegalData *legs);

static inline int lerp(int a, int b, uint8_t t) {
    return a + (t * (b - a) + 127) / 255; //+127 permits better rounding
}

static inline int compute_phase(Position *pos) {
    int phase = 0;

    for (int piece = WN; piece <= WQ; piece++) {
        phase += phase_weights[piece] *
            __builtin_popcountll(pos->bitboards[piece]);

        phase += phase_weights[piece] *
            __builtin_popcountll(pos->bitboards[piece + 6]);
    }

    return phase;
}
static inline uint8_t phase_to_eg_weight(int phase) {
    if (phase < 0) phase = 0;
    if (phase > 24) phase = 24;
    return (uint8_t)((24 - phase) * 255 / 24);
}

#endif