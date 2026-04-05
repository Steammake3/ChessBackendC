#ifndef POSITION_H
#define POSITION_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "types.h"
#include "eval.h"
#define MAX_HISTORY 1024 //defined in position.h


extern uint64_t repetition_tableaus[MAX_HISTORY]; extern uint16_t rep_idx;
extern uint16_t last_irreversible;
extern uint8_t CHEBYSHEV[64][64];
extern uint8_t EDGEDISTS[64][8];
extern const int8_t DIRECTIONS[8];
extern uint64_t BETWEEN[64][64]; //BETWEEN[start_square(excluded btw)][end_square(included btw)]

void add_piece(int index, Position *position, char piece);

int piece_at(int index, Position *position);

void load_position(const char fen[], Position *position);

char* unload_position(Position* position);

void init_hash(Position *pos);

void make_move(Position *position, uint16_t move, Undo *undo);

void unmake_move(Position *position, uint16_t move, Undo *undo);

void precompute_chebyshev();

void precompute_edgedists();

void precompute_betweens();

char* ascii_repr(Position *pos);
#endif