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
    uint8_t castling_rights;
    uint64_t zobrist;
} Position;

void add_piece(int index, Position *position, char piece);

int piece_at(int index, Position *position);

void load_position(const char fen[], Position *position);

char* unload_position(Position* position);
#endif