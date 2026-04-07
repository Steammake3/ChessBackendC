#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#define NO_SQ 64
#define WHITE 0
#define BLACK 1
#define BOTH 2

#define MOVE_FROM(m)   ((m) & 0x3F)          // 0b111111
#define MOVE_TO(m)     (((m) >> 6) & 0x3F)   // next 6 bits
#define MOVE_FLAGS(m)  (((m) >> 12) & 0xF)   // top 4 bits

#define MAKE_MOVE(f,t,fl)  (uint16_t) ((f) | ((t)<<6) | ((fl)<<12))
#define ABS(a) (((a)>0) ? (a) : (-(a)))
#define MIN(a,b) (a<b?a:b)
#define BBd(sq) (1ULL<<(sq))
#define PIECEtype(p) ((p)%6)

#define popcount(x) __builtin_popcountll(x)

typedef enum {
    WP, WN, WB, WR, WQ, WK, BP, BN, BB, BR, BQ, BK
} Pieces;

typedef struct {
    uint64_t bitboards[12]; //White Pawn, Knight, Bishop ... Black Pawn... (PNBRQK)
    uint64_t occupancies[3]; //White, Black, Both
    uint8_t en_passant; //0-63 for actual EP, if en_passant>>6 then no en_passant
    uint16_t halfmove;
    uint16_t fullmove;
    bool side_to_move;
    uint8_t castling_rights; //KQkq White then Black
    uint64_t zobrist;
    int simple_eval_mid; //Always from White POV
    int simple_eval_end; //Always from White POV
    uint8_t phase; //24 (Opening) to 0 (endgame)
} Position;

typedef struct {
    uint8_t captured;
    uint8_t castle;
    uint8_t ep;
    uint16_t halfmove;
    uint16_t fullmove;
    uint64_t hash;
    uint8_t flags; //0 Normal, 1 EP, 2 Castle, 3 Promotion
    uint16_t last_irreversible;
    int simple_eval_mid;
    int simple_eval_end;
    uint8_t phase;
} Undo;

static inline int pop_lsb(uint64_t *b) {
    //if (*b == 0) return NO_SQ;
    int sq = __builtin_ctzll(*b);
    *b &= *b - 1;
    return sq;
}

#endif