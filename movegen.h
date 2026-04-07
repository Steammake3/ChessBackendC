#ifndef MOVEGEN_H
#define MOVEGEN_H

#include <stdint.h>
#include <stdbool.h>
#include "position.h"

#define GEN_ALL 0
#define GEN_QSN 1

#define FILE_MASK 0x0101010101010101ULL
#define RANK_MASK 0xFFULL
#define DIAG_MASK 0x8040201008040201ULL
#define ANTI_MASK 0x0102040810204080ULL

extern uint64_t knight_attacks[64];
extern uint64_t king_attacks[64];
extern uint64_t pawn_attacks[2][64]; // [side][square]
extern uint64_t hq_masks[4][64]; //Axis, Square

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

typedef enum {
    FILE_, RANK, DIAG, ANTI
} Axes;

void init_attack_tables();

bool ep_is_legal(Position *pos, int from, int ep_to);

void generate_moves(Position *pos, MoveList *moves, LegalData *legals, bool qsn);

void generate_sliding_moves(Position *pos, MoveList *moves, LegalData *legals, bool qsn);

void generate_pawn_moves(Position *pos, MoveList *moves, LegalData *legals , bool qsn);

void generate_knight_moves(Position *pos, MoveList *moves, LegalData *legals, bool qsn);

void generate_king_moves(Position *pos, MoveList *moves, LegalData *legals, bool qsn);

//Attack calcs functions for knp unnecessary to call, me just use lookup :)
uint64_t generate_king_attacks(uint8_t sq);
uint64_t generate_knight_attacks(uint8_t sq);
uint64_t generate_pawn_attacks(uint8_t sq, int side);
//No need for queen, it's just a rook|bishop
uint64_t generate_rook_attacks(uint8_t sq, uint64_t occ);
uint64_t generate_bishop_attacks(uint8_t sq, uint64_t occ);

static inline bool square_attacked(int sq, LegalData *legals){
    return (BBd(sq)&legals->enemy_attack_maps)!=0;
}
uint64_t compute_attack_map(Position *pos, int by_side, uint64_t exclude);

//Pin & Checks
void compute_pins_n_checks(Position *pos, LegalData *legals);

//HQ (helpers rn)!
static inline uint64_t reverse_per_byte(uint64_t x) { //Flips each byte
    x = ((x >> 1)  & 0x5555555555555555ULL) | ((x & 0x5555555555555555ULL) << 1);
    x = ((x >> 2)  & 0x3333333333333333ULL) | ((x & 0x3333333333333333ULL) << 2);
    x = ((x >> 4)  & 0x0F0F0F0F0F0F0F0FULL) | ((x & 0x0F0F0F0F0F0F0F0FULL) << 4);
    return x;
}
static inline uint64_t reverse_bytes(uint64_t x){ //Retains each byte, but flip their order around
    /* x = ((x >> 8)  & 0x00FF00FF00FF00FFULL) | ((x & 0x00FF00FF00FF00FFULL) << 8);
    x = ((x >> 16) & 0x0000FFFF0000FFFFULL) | ((x & 0x0000FFFF0000FFFFULL) << 16);
    x = ( x >> 32                         ) | ( x                          << 32);
    return x; //Slight older implementation */
    return __builtin_bswap64(x);
}
static inline uint64_t reverse_u64(uint64_t x) { //Reverses uint64_t
    return reverse_bytes(reverse_per_byte(x));
}
//True HQ!
static inline uint64_t HQ(uint8_t sq, uint64_t occ, uint64_t mask) {
    uint64_t r = BBd(sq); uint64_t rp = reverse_u64(r);
    uint64_t o = mask&occ; uint64_t op = reverse_u64(o);
    return mask&((o^(o-(r+r)))^reverse_u64(op^(op-(rp+rp))));
}

void precompute_hq_masks();

bool move_is_check(Position *pos, uint16_t move);

#endif