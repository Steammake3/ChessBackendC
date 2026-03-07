#ifndef ZOBRIST_H
#define ZOBRIST_H

#include <stdint.h>

extern uint64_t pieces[12][64];
extern uint64_t castles[4];
extern uint64_t ep_file[8];
extern uint64_t zh_side;

void init_zobrist();

#endif