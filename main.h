#ifndef MAIN_H
#define MAIN_H

#include "position.h"
#include "movegen.h"
#include "tt.h"
#include "zobrist.h"
#include <time.h>

uint16_t id_best_move(float time_control);
void init_consts();

#endif