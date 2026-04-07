#ifndef EVAL_H
#define EVAL_H

#include "types.h"
#include "movegen.h"

#define FLIP(sq) sq^0b111000

extern int values[6];

static const int phase_weights[6] = {
    0, // pawn
    1, // knight
    1, // bishop
    2, // rook
    4, // queen
    0  // king
};
//TODO: This is from BLACK's perspective
extern const int8_t middle_psts[6][64];
extern const int8_t endgame_psts[2][64]; //Pawn then King!

//Helpers
int material_eval(Position *pos);
int mobility_eval(Position *pos);
int pst_eval(Position *pos, uint8_t endgame_weight);
int king_safety_eval(Position *pos);
int pawn_structure_eval(Position *pos);

//GGGOOOOOO!!!!
int evaluate(Position *pos);

int init_evaluate(Position *pos);

static inline int lerp(int a, int b, uint8_t t) {
    int64_t diff = (int64_t)b - (int64_t)a; //This shall save any overflows
    return a + (t * diff + 127) / 255; //+127 permits better rounding
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
    return (uint8_t)((24 - phase) * 255 + 12) / 24;
}


#endif