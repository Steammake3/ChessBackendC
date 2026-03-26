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

extern time_t start;
extern float bot_time_control;
extern float elapsed_time;
extern uint64_t nodes;
//TODO: This is from BLACK's perspective
const int8_t psts[6][64];

int evaluate(Position *pos);
int be_referee(Position *pos, MoveList *moves, LegalData *legs, int depth);
int get_draw_type(Position *pos, MoveList *moves, LegalData *legs);

int quiesence_search(Position *pos, int alpha, int beta);
int search(Position *pos, uint8_t depth, int alpha, int beta, uint16_t *move);

//Helpers
int material_eval(Position *pos);
int mobility_eval(Position *pos);
int pst_eval(Position *pos);
int king_safety_eval(Position *pos);
int pawn_structure_eval(Position *pos);

uint16_t get_best_move(Position *pos, uint8_t depth);

void order_moves(MoveList *ml, Position *pos, LegalData *legs);
int guess_move_priority(uint16_t move, Position *pos, LegalData *legs);

#endif