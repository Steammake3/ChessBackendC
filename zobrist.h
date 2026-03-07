#ifndef ZOBRIST_H
#define ZOBRIST_H

#include <stdint.h>
#include <stdbool.h>

extern uint64_t zh_pieces[12][64];
extern uint64_t zh_castles[16];
extern uint64_t zh_ep_file[8];
extern uint64_t zh_side;

void init_zobrist();

#endif