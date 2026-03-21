#ifndef SEARCH_H
#define SEARCH_H

#include <stdint.h>
#include <stdbool.h>
#include "position.h"
#include "tt.h"
#include "movegen.h"

int evaluate(Position *pos);

int quiesence_search(Position *pos, int alpha, int beta);
int search(Position *pos, uint8_t depth, int alpha, int beta);

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