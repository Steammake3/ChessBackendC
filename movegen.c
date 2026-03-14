#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "position.h"
#include "movegen.h"

#define NO_SQ 64
#define WHITE 0
#define BLACK 1
#define BOTH 2

#define MOVE_FROM(m)   ((m) & 0x3F)          // 0b111111
#define MOVE_TO(m)     (((m) >> 6) & 0x3F)   // next 6 bits
#define MOVE_FLAGS(m)  (((m) >> 12) & 0xF)   // top 4 bits

#define NOT_A_FILE 0xfefefefefefefefeULL
#define NOT_H_FILE 0x7f7f7f7f7f7f7f7fULL
#define NOT_1_RANK 0xffffffffffffff00ULL
#define NOT_8_RANK 0x00ffffffffffffffULL

#define MAKE_MOVE(f,t,fl)  (uint16_t) ((f) | ((t)<<6) | ((fl)<<12))

uint64_t knight_attacks[64];
uint64_t king_attacks[64];
uint64_t pawn_attacks[2][64]; // [side][square]

void init_attack_tables() {
    for (int sq = 0; sq < 64; sq++) {
        knight_attacks[sq] = generate_knight_attacks(sq);
        king_attacks[sq] = generate_king_attacks(sq);

        pawn_attacks[WHITE][sq] = generate_pawn_attacks(sq, WHITE);
        pawn_attacks[BLACK][sq] = generate_pawn_attacks(sq, BLACK);
    }
}

uint64_t generate_knight_attacks(uint8_t sq) {
    //ChatGPT Generated, I couldn't care less, it's free optimization
    uint64_t b = 1ULL << sq;

    uint64_t l1 = (b >> 1) & 0x7f7f7f7f7f7f7f7fULL;
    uint64_t l2 = (b >> 2) & 0x3f3f3f3f3f3f3f3fULL;
    uint64_t r1 = (b << 1) & 0xfefefefefefefefeULL;
    uint64_t r2 = (b << 2) & 0xfcfcfcfcfcfcfcfcULL;

    uint64_t h1 = l2 | r2;
    uint64_t h2 = l1 | r1;

    return (h1 << 8) | (h1 >> 8) | (h2 << 16) | (h2 >> 16);
}

uint64_t generate_pawn_attacks(uint8_t sq, int side) {
    //ChatGPT again
    uint64_t b = 1ULL << sq;

    if (side == WHITE) {
        return ((b << 7) & NOT_H_FILE) |
               ((b << 9) & NOT_A_FILE);
    } else {
        return ((b >> 7) & NOT_A_FILE) |
               ((b >> 9) & NOT_H_FILE);
    }
}

uint64_t generate_king_attacks(uint8_t sq) {
    uint64_t b = 1ULL << sq;

    uint64_t attacks =
        (b & NOT_A_FILE) >> 1 |
        (b & NOT_H_FILE) << 1 |
        (b >> 8) |
        (b << 8) |
        (b & NOT_A_FILE) >> 9 |
        (b & NOT_A_FILE) << 7 |
        (b & NOT_H_FILE) >> 7 |
        (b & NOT_H_FILE) << 9;

    return attacks;
}

uint64_t generate_rook_attacks(uint8_t sq, uint64_t occ) {
    uint64_t attacks = 0ULL;
    int r = sq / 8;
    int f = sq % 8;
    // North
    for (int rr = r + 1; rr <= 7; rr++) {
        int s = rr * 8 + f;
        uint64_t bit = 1ULL << s;
        attacks |= bit;
        if (occ & bit) break;
    }
    // South
    for (int rr = r - 1; rr >= 0; rr--) {
        int s = rr * 8 + f;
        uint64_t bit = 1ULL << s;
        attacks |= bit;
        if (occ & bit) break;
    }
    // East
    for (int ff = f + 1; ff <= 7; ff++) {
        int s = r * 8 + ff;
        uint64_t bit = 1ULL << s;
        attacks |= bit;
        if (occ & bit) break;
    }
    // West
    for (int ff = f - 1; ff >= 0; ff--) {
        int s = r * 8 + ff;
        uint64_t bit = 1ULL << s;
        attacks |= bit;
        if (occ & bit) break;
    }

    return attacks;
}

uint64_t generate_bishop_attacks(uint8_t sq, uint64_t occ) {
    //ChatGPT modified my rook code to do this
    uint64_t attacks = 0ULL;
    int r = sq / 8;
    int f = sq % 8;
    // NE
    for (int rr = r + 1, ff = f + 1; rr <= 7 && ff <= 7; rr++, ff++) {
        int s = rr * 8 + ff;
        uint64_t bit = 1ULL << s;
        attacks |= bit;
        if (occ & bit) break;
    }
    // NW
    for (int rr = r + 1, ff = f - 1; rr <= 7 && ff >= 0; rr++, ff--) {
        int s = rr * 8 + ff;
        uint64_t bit = 1ULL << s;
        attacks |= bit;
        if (occ & bit) break;
    }
    // SE
    for (int rr = r - 1, ff = f + 1; rr >= 0 && ff <= 7; rr--, ff++) {
        int s = rr * 8 + ff;
        uint64_t bit = 1ULL << s;
        attacks |= bit;
        if (occ & bit) break;
    }
    // SW
    for (int rr = r - 1, ff = f - 1; rr >= 0 && ff >= 0; rr--, ff--) {
        int s = rr * 8 + ff;
        uint64_t bit = 1ULL << s;
        attacks |= bit;
        if (occ & bit) break;
    }

    return attacks;
}

void compute_pins_n_checks(Position *pos, uint64_t *pinned, uint64_t *checkers, uint64_t *block_mask, uint64_t pin_dir[64]){
    *pinned = 0ULL;
    *checkers = 0ULL;
    memset(pin_dir, 0xFF, sizeof(pin_dir));

    bool side = pos->side_to_move;
    bool enemy = side^1;

    uint64_t king_bb = pos->bitboards[side==BLACK ? BK:WK];
    uint8_t king = __builtin_ctzll(king_bb);

    uint64_t e_queens = pos->bitboards[enemy?BQ:WQ];
    uint64_t e_bishops = pos->bitboards[enemy?BB:WB];
    uint64_t e_rooks = pos->bitboards[enemy?BR:WR];

    for (uint8_t dir=0; dir<8; dir++){
        uint8_t candidate = NO_SQ;
        uint64_t rn_pin = 0ULL;
        for (uint8_t extent=1; extent<=EDGEDISTS[king][dir]; extent++){
            uint8_t cur_sq = king + DIRECTIONS[dir]*extent;
            uint64_t bb = BBd(cur_sq);
            rn_pin |= bb;
            if (bb&pos->occupancies[side]){ //Friend ;)
                if (candidate==NO_SQ) candidate = cur_sq;
                else break;
            } else if (bb&pos->occupancies[enemy]){ //Fiend >:]

                if (dir<4 && (bb&(e_queens|e_bishops))) { //Diag case
                    if (candidate==NO_SQ) *checkers |= bb;
                    else {
                        *pinned |= BBd(candidate); pin_dir[candidate] = rn_pin;
                    }
                } else if (dir>=4 && (bb&(e_queens|e_rooks))){ //manhattan
                    if (candidate==NO_SQ) *checkers |= bb;
                    else {
                        *pinned |= BBd(candidate); pin_dir[candidate] = rn_pin;
                    }
                }
                break;
            }
        }
    }

    uint64_t e_knights = pos->bitboards[enemy?BN:WN];
    *checkers |= knight_attacks[king]&e_knights;

    uint64_t e_pawns = pos->bitboards[enemy?BP:WP];
    *checkers |= pawn_attacks[side][king]&e_pawns;

    *block_mask = 0ULL;

    uint8_t checker_sq;
    if (__builtin_popcountll(*checkers) == 1) {
        checker_sq = __builtin_ctzll(*checkers);
        if (*checkers & (e_pawns|e_knights)){
            *block_mask = *checkers;
        } else {
            for (uint8_t dir=0; dir<8; dir++) {
                for (uint8_t dist=1; dist<=EDGEDISTS[king][dir]; dist++) {
                    uint8_t sq = king + DIRECTIONS[dir]*dist;
                    *block_mask |= BBd(sq);
                    if (sq == checker_sq) break; // found the checker along this ray
                    if (BBd(sq) & pos->occupancies[BOTH]) break; // blocked by something else
                }
                if (*block_mask & *checkers) break; // ray found
            }
        }
    }
}

void generate_pawn_moves(Position *pos, MoveList *moves){
    uint64_t pawns = pos->bitboards[6*pos->side_to_move+WP];
    while (pawns){
        int cpawn_sq = pop_lsb(&pawns);
        int sing_push = cpawn_sq + (pos->side_to_move ? -8 : 8);
        int doub_push = cpawn_sq + (pos->side_to_move ? -16 : 16);
        if (!(pos->occupancies[BOTH]&sing_push)) moves->moves[moves->count++] = 4;//TODO
    }
}