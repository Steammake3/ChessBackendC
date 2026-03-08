#ifndef POSITION_H
#define POSITION_H

#include <stdbool.h>
#include <stdint.h>

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
    uint64_t hash;
} Undo;

extern uint8_t CHEBYSHEV[64][64];

void add_piece(int index, Position *position, char piece);

int piece_at(int index, Position *position);

void load_position(const char fen[], Position *position);

char* unload_position(Position* position);

void init_hash(Position *pos);

void make_move(Position *position, uint16_t move, Undo *undo);

void unmake_move(Position *position, uint16_t move, Undo *undo);

void precompute_chebyshev();
#endif