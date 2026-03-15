#ifndef POSITION_H
#define POSITION_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

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
} Position;

typedef struct {
    uint8_t captured;
    uint8_t castle;
    uint8_t ep;
    uint16_t halfmove;
    uint16_t fullmove;
    uint64_t hash;
    uint8_t flags; //0 Normal, 1 EP, 2 Castle, 3 Promotion
} Undo;

extern uint8_t CHEBYSHEV[64][64];
extern uint8_t EDGEDISTS[64][8];
extern const int8_t DIRECTIONS[8]; 

void add_piece(int index, Position *position, char piece);

int piece_at(int index, Position *position);

void load_position(const char fen[], Position *position);

char* unload_position(Position* position);

void init_hash(Position *pos);

void make_move(Position *position, uint16_t move, Undo *undo);

void unmake_move(Position *position, uint16_t move, Undo *undo);

void precompute_chebyshev();

void precompute_edgedists();
#endif