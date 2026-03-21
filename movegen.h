#ifndef MOVEGEN_H
#define MOVEGEN_H

#include <stdint.h>
#include <stdbool.h>
#include "position.h"

#define GEN_ALL 0
#define GEN_QSN 1

extern uint64_t knight_attacks[64];
extern uint64_t king_attacks[64];
extern uint64_t pawn_attacks[2][64]; // [side][square]

typedef struct {
    uint16_t moves[256];
    uint16_t count;
} MoveList;

typedef struct {
    uint64_t pinned;
    uint64_t checkers;
    uint64_t block_mask;
    uint64_t pin_dir[64];
    uint64_t enemy_attack_maps;
    uint64_t king_danger_map;
} LegalData;

void init_attack_tables();

bool ep_is_legal(Position *pos, int from, int ep_to);

void generate_moves(Position *pos, MoveList *moves, LegalData *legals, bool qsn);

void generate_sliding_moves(Position *pos, MoveList *moves, LegalData *legals, bool qsn);

void generate_pawn_moves(Position *pos, MoveList *moves, LegalData *legals , bool qsn);

void generate_knight_moves(Position *pos, MoveList *moves, LegalData *legals, bool qsn);

void generate_king_moves(Position *pos, MoveList *moves, LegalData *legals, bool qsn);

static inline int pop_lsb(uint64_t *b) {
    //if (*b == 0) return NO_SQ;
    int sq = __builtin_ctzll(*b);
    *b &= *b - 1;
    return sq;
}

//Attack calcs functions for knp unnecessary to call, me just use lookup :)
uint64_t generate_king_attacks(uint8_t sq);
uint64_t generate_knight_attacks(uint8_t sq);
uint64_t generate_pawn_attacks(uint8_t sq, int side);
//No need for queen, it's just a rook|bishop
uint64_t generate_rook_attacks(uint8_t sq, uint64_t occ);
uint64_t generate_bishop_attacks(uint8_t sq, uint64_t occ);

bool square_attacked(int sq, LegalData *legals);
uint64_t compute_attack_map(Position *pos, int by_side, uint64_t exclude);

//Pin & Checks
void compute_pins_n_checks(Position *pos, LegalData *legals);

#endif