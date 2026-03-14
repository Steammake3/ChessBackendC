#include <stdbool.h>
#include "zobrist.h"

uint64_t zh_pieces[12][64];
uint64_t zh_castles[16];
uint64_t zh_ep_file[8];
uint64_t zh_side;

static bool initialized = false;

static uint64_t seed = 88172645463325252ULL;
static uint64_t xorshift64_state;

uint64_t xorshift64() {
    uint64_t x = xorshift64_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    xorshift64_state = x;
    return x;
}

void init_zobrist(){
    if (initialized) {return;}
    initialized = true;

    xorshift64_state = seed;
    for (int piece=0; piece<12; piece++){
        for (int sq=0; sq<64; sq++){
            zh_pieces[piece][sq] = xorshift64();
        }
    }

    for (int castle=0; castle<16; castle++){zh_castles[castle]=xorshift64();}
    for (int file=0; file<8; file++){zh_ep_file[file]=xorshift64();}

    zh_side = xorshift64();
}