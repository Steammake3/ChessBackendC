#include "zobrist.h"

static uint64_t xorshift64_state = 88172645463325252ULL;

uint64_t xorshift64() {
    uint64_t x = xorshift64_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    xorshift64_state = x;
    return x;
}

void init_zobrist(){
    for (int piece=0; piece<12; piece++){
        for (int sq=0; sq<64; sq++){
            pieces[piece][sq] = xorshift64();
        }
    }

    for (int castle=0; castle<4; castle++){castles[castle]=xorshift64();}
    for (int file=0; file<8; file++){ep_file[file]=xorshift64();}

    zh_side = xorshift64();
}