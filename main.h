#ifndef MAIN_H
#define MAIN_H

#include "position.h"
#include "movegen.h"
#include "tt.h"
#include "zobrist.h"

uint16_t str2move(const char move[]);
char* move2str(uint16_t move);
uint16_t best_move(float time_control);
void init_consts();

#endif