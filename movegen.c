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

bool ep_is_legal(Position *pos, int from, int ep_to){
    Undo undid; LegalData legs;
    bool side = pos->side_to_move;
    make_move(pos, MAKE_MOVE(from, ep_to, 0), &undid);
    pos->side_to_move=side; compute_pins_n_checks(pos, &legs); pos->side_to_move=side^1;
    unmake_move(pos, MAKE_MOVE(from, ep_to, 0), &undid);
    return legs.checkers==0;
}

uint64_t generate_knight_attacks(uint8_t sq) {
    //ChatGPT Generated, I couldn't care less, it's free optimization
    uint64_t b = BBd(sq);

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
    uint64_t b = BBd(sq);

    if (side == WHITE) {
        return ((b << 7) & NOT_H_FILE) |
               ((b << 9) & NOT_A_FILE);
    } else {
        return ((b >> 7) & NOT_A_FILE) |
               ((b >> 9) & NOT_H_FILE);
    }
}

uint64_t generate_king_attacks(uint8_t sq) {
    uint64_t b = BBd(sq);

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
        uint64_t bit = BBd(s);
        attacks |= bit;
        if (occ & bit) break;
    }
    // South
    for (int rr = r - 1; rr >= 0; rr--) {
        int s = rr * 8 + f;
        uint64_t bit = BBd(s);
        attacks |= bit;
        if (occ & bit) break;
    }
    // East
    for (int ff = f + 1; ff <= 7; ff++) {
        int s = r * 8 + ff;
        uint64_t bit = BBd(s);
        attacks |= bit;
        if (occ & bit) break;
    }
    // West
    for (int ff = f - 1; ff >= 0; ff--) {
        int s = r * 8 + ff;
        uint64_t bit = BBd(s);
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
        uint64_t bit = BBd(s);
        attacks |= bit;
        if (occ & bit) break;
    }
    // NW
    for (int rr = r + 1, ff = f - 1; rr <= 7 && ff >= 0; rr++, ff--) {
        int s = rr * 8 + ff;
        uint64_t bit = BBd(s);
        attacks |= bit;
        if (occ & bit) break;
    }
    // SE
    for (int rr = r - 1, ff = f + 1; rr >= 0 && ff <= 7; rr--, ff++) {
        int s = rr * 8 + ff;
        uint64_t bit = BBd(s);
        attacks |= bit;
        if (occ & bit) break;
    }
    // SW
    for (int rr = r - 1, ff = f - 1; rr >= 0 && ff >= 0; rr--, ff--) {
        int s = rr * 8 + ff;
        uint64_t bit = BBd(s);
        attacks |= bit;
        if (occ & bit) break;
    }

    return attacks;
}

void compute_pins_n_checks(Position *pos, LegalData *legals){
    legals->pinned = 0ULL;
    legals->checkers = 0ULL;
    memset(legals->pin_dir, 0xFF, sizeof(legals->pin_dir));

    bool side = pos->side_to_move;
    bool enemy = side^1;

    uint64_t king_bb = pos->bitboards[side==BLACK ? BK:WK];
    uint8_t king = __builtin_ctzll(king_bb);

    uint64_t e_queens = pos->bitboards[enemy?BQ:WQ];
    uint64_t e_bishops = pos->bitboards[enemy?BB:WB];
    uint64_t e_rooks = pos->bitboards[enemy?BR:WR];

    for (uint8_t dir=0; dir<8; dir++){
        uint8_t candidate = NO_SQ;

        for (uint8_t extent=1; extent<=EDGEDISTS[king][dir]; extent++){
            uint8_t cur_sq = king + DIRECTIONS[dir]*extent;
            uint64_t bb = BBd(cur_sq);

            if (bb&pos->occupancies[side]){ //Friend ;)
                if (candidate==NO_SQ) candidate = cur_sq;
                else break;
            } else if (bb&pos->occupancies[enemy]){ //Fiend >:]

                if (dir<4 && (bb&(e_queens|e_bishops))) { //Diag case
                    if (candidate==NO_SQ) legals->checkers |= bb;
                    else {
                        legals->pinned |= BBd(candidate);

                        uint64_t ray = 0ULL;
                        for (int e = 1; e <= extent; e++){
                            int sq = king + DIRECTIONS[dir]*e;
                            ray |= BBd(sq);
                        }

                        legals->pin_dir[candidate] = ray;
                    }
                
                } else if (dir>=4 && (bb&(e_queens|e_rooks))){ //manhattan
                    if (candidate==NO_SQ) legals->checkers |= bb;
                    else {
                        legals->pinned |= BBd(candidate);

                        uint64_t ray = 0ULL;
                        for (int e = 1; e <= extent; e++){
                            int sq = king + DIRECTIONS[dir]*e;
                            ray |= BBd(sq);
                        }

                        legals->pin_dir[candidate] = ray;
                    }
                }
                break;
            }
        }
    }

    uint64_t e_knights = pos->bitboards[enemy?BN:WN];
    legals->checkers |= knight_attacks[king]&e_knights;

    uint64_t e_pawns = pos->bitboards[enemy?BP:WP];
    legals->checkers |= pawn_attacks[side][king]&e_pawns;

    legals->block_mask = 0ULL;

    if (__builtin_popcountll(legals->checkers) == 1) {
        int checker_sq = __builtin_ctzll(legals->checkers);

        //If the checker is a knight or pawn, only the checker square is relevant
        if (legals->checkers & (e_pawns | e_knights)) {
            legals->block_mask = legals->checkers; //can only capture the checker
        } else {
            // sliding piece: rook, bishop, queen
            // calculate ray from king to checker
            int dir = -1;

            // find the direction of the checker relative to the king
            for (int d = 0; d < 8; d++) {
                for (int dist = 1; dist <= EDGEDISTS[king][d]; dist++) {
                    int sq = king + DIRECTIONS[d] * dist;
                    if (sq == checker_sq) {
                        dir = d;
                        break;
                    }
                    //if (BBd(sq) & pos->occupancies[BOTH]) break; // blocked
                }
                if (dir != -1) break;
            }

            if (dir != -1) {
                // build the block mask along the ray
                for (int dist = 1; dist <= EDGEDISTS[king][dir]; dist++) {
                    int sq = king + DIRECTIONS[dir] * dist;
                    legals->block_mask |= BBd(sq);
                    if (sq == checker_sq) break; // stop at the checker
                    if (BBd(sq) & pos->occupancies[BOTH]) break; // blocked by piece
                }
            }
        }
    }
    legals->enemy_attack_maps = compute_attack_map(pos, enemy, king_bb);
}

void generate_pawn_moves(Position *pos, MoveList *moves, LegalData *legals){
    bool side = pos->side_to_move;
    uint64_t pawns = pos->bitboards[6*side + WP];
    uint64_t occ = pos->occupancies[BOTH];
    uint64_t enemy_occ = pos->occupancies[side^1];

    int forward = side ? -8 : 8;
    int start_rank = side ? 6 : 1;

    while (pawns){
        int from = pop_lsb(&pawns);
        uint64_t from_bb = BBd(from);

        // Determine the allowed ray for pinned pawns
        uint64_t pin_mask = (legals->pinned & from_bb) ? legals->pin_dir[from] : ~0ULL;

        // Single push
        int to = from + forward;
        uint64_t to_bb = BBd(to);
        if (!(occ & to_bb) && (to_bb & pin_mask) &&
            (!legals->checkers || (to_bb & legals->block_mask))) {
            
            moves->moves[moves->count++] = MAKE_MOVE(from, to, 0);
            if (to/8==(side?0:7)){ //Promo
                moves->moves[moves->count++] = MAKE_MOVE(from, to, 1);
                moves->moves[moves->count++] = MAKE_MOVE(from, to, 2);
                moves->moves[moves->count++] = MAKE_MOVE(from, to, 3);
            }

        }
        // Double push
        int dbl = from + 2*forward;
        uint64_t dbl_bb = BBd(dbl);
        if ((from/8 == start_rank) && !(occ & dbl_bb) && !(occ & to_bb) &&
            (dbl_bb & pin_mask) &&
            (!legals->checkers || (dbl_bb & legals->block_mask))) {
            moves->moves[moves->count++] = MAKE_MOVE(from, dbl, 0);
        }

        // Captures
        uint64_t attacks = pawn_attacks[side][from] & enemy_occ;
        while (attacks){
            int cap_to = pop_lsb(&attacks);
            uint64_t cap_bb = BBd(cap_to);

            // Respect pin and check
            if ((!(legals->pinned & from_bb) || (cap_bb & pin_mask)) &&
                (!legals->checkers || (cap_bb & legals->block_mask))) {
                moves->moves[moves->count++] = MAKE_MOVE(from, cap_to, 0);
                if (to/8==(side?0:7)){ //Promo
                    moves->moves[moves->count++] = MAKE_MOVE(from, cap_to, 1);
                    moves->moves[moves->count++] = MAKE_MOVE(from, cap_to, 2);
                    moves->moves[moves->count++] = MAKE_MOVE(from, cap_to, 3);
                }
            }
        }

        // En passant
        if (pos->en_passant != NO_SQ){
            int ep_to = pos->en_passant;
            uint64_t ep_bb = BBd(ep_to);

            // Is this pawn able to capture EP?
            if ((pawn_attacks[side][from] & ep_bb) &&
                (!(legals->pinned && from_bb) || (ep_bb & pin_mask))){
                // Must simulate EP legality (king not left in check)
                if (ep_is_legal(pos, from, ep_to)) {
                    moves->moves[moves->count++] = MAKE_MOVE(from, ep_to, 0);
                }
            }
        }
    }
}

void generate_knight_moves(Position *pos, MoveList *moves, LegalData *legals){
    bool side = pos->side_to_move;
    uint64_t knights = pos->bitboards[side?BN:WN];

    while (knights){
        uint8_t knight = pop_lsb(&knights);
        uint64_t k_bb = BBd(knight);
        uint64_t moves_rn = knight_attacks[knight] & ~pos->occupancies[side];

        if (legals->pinned & k_bb) continue;
        while (moves_rn) {
            uint8_t move_rn = pop_lsb(&moves_rn);

            if (legals->checkers && !(BBd(move_rn) & legals->block_mask)) continue;
            moves->moves[moves->count++] = MAKE_MOVE(knight, move_rn, 0);
        }
    }
}

void generate_king_moves(Position *pos, MoveList *moves, LegalData *legals){
    bool side = pos->side_to_move;
    uint64_t king = __builtin_ctzll(pos->bitboards[side?BK:WK]);
    uint64_t k_bb = pos->bitboards[side?BK:WK];
    uint64_t moves_rn = king_attacks[king] & ~pos->occupancies[side];
    uint8_t castles = (pos->castling_rights >> ((side^1)*2)) & 0b11;

    //General
    while (moves_rn) {
        uint8_t move_rn = pop_lsb(&moves_rn);

        if (square_attacked(move_rn, legals)) continue;
        moves->moves[moves->count++] = MAKE_MOVE(king, move_rn, 0);
    }
    //Castling (ChatGPT)
    uint64_t occ = pos->occupancies[BOTH];

    if (side == WHITE) {

        // Kingside
        if (pos->castling_rights & 8) {
            if (!(occ & (BBd(5) | BBd(6))) &&
                !(legals->enemy_attack_maps & (BBd(4) | BBd(5) | BBd(6))))
                moves->moves[moves->count++] = MAKE_MOVE(4,6,0);
        }

        // Queenside
        if (pos->castling_rights & 4) {
            if (!(occ & (BBd(1) | BBd(2) | BBd(3))) &&
                !(legals->enemy_attack_maps & (BBd(4) | BBd(3) | BBd(2))))
                moves->moves[moves->count++] = MAKE_MOVE(4,2,0);
        }

    } else {

        // Kingside
        if (pos->castling_rights & 2) {
            if (!(occ & (BBd(61) | BBd(62))) &&
                !(legals->enemy_attack_maps & (BBd(60) | BBd(61) | BBd(62))))
                moves->moves[moves->count++] = MAKE_MOVE(60,62,0);
        }

        // Queenside
        if (pos->castling_rights & 1) {
            if (!(occ & (BBd(57) | BBd(58) | BBd(59))) &&
                !(legals->enemy_attack_maps & (BBd(60) | BBd(59) | BBd(58))))
                moves->moves[moves->count++] = MAKE_MOVE(60,58,0);
        }

    }
}

void generate_sliding_moves(Position *pos, MoveList *moves, LegalData *legals){
    bool side = pos->side_to_move;
    uint64_t occ = pos->occupancies[BOTH];
    uint64_t enemy_occ = pos->occupancies[side^1];

    // --- ROOKS ---
    uint64_t rooks = pos->bitboards[side ? BR : WR];
    while (rooks){
        int from = pop_lsb(&rooks);
        uint64_t attacks = generate_rook_attacks(from, occ) & ~pos->occupancies[side];

        // respect pins and checks
        uint64_t from_bb = BBd(from);
        uint64_t pin_mask = (legals->pinned & from_bb) ? legals->pin_dir[from] : ~0ULL;
        attacks &= pin_mask;
        if (legals->checkers) attacks &= legals->block_mask;

        while (attacks){
            int to = pop_lsb(&attacks);
            moves->moves[moves->count++] = MAKE_MOVE(from, to, 0);
        }
    }

    // --- BISHOPS ---
    uint64_t bishops = pos->bitboards[side ? BB : WB];
    while (bishops){
        int from = pop_lsb(&bishops);
        uint64_t attacks = generate_bishop_attacks(from, occ) & ~pos->occupancies[side];

        uint64_t from_bb = BBd(from);
        uint64_t pin_mask = (legals->pinned & from_bb) ? legals->pin_dir[from] : ~0ULL;
        attacks &= pin_mask;
        if (legals->checkers) attacks &= legals->block_mask;

        while (attacks){
            int to = pop_lsb(&attacks);
            moves->moves[moves->count++] = MAKE_MOVE(from, to, 0);
        }
    }

    // --- QUEENS ---
    uint64_t queens = pos->bitboards[side ? BQ : WQ];
    while (queens){
        int from = pop_lsb(&queens);
        uint64_t attacks = (generate_rook_attacks(from, occ) | generate_bishop_attacks(from, occ)) & ~pos->occupancies[side];

        uint64_t from_bb = BBd(from);
        uint64_t pin_mask = (legals->pinned & from_bb) ? legals->pin_dir[from] : ~0ULL;
        attacks &= pin_mask;
        if (legals->checkers) attacks &= legals->block_mask;

        while (attacks){
            int to = pop_lsb(&attacks);
            moves->moves[moves->count++] = MAKE_MOVE(from, to, 0);
        }
    }
}

uint64_t compute_attack_map(Position *pos, int by_side, uint64_t exclude){
    uint64_t pawns = pos->bitboards[by_side?BP:WP]&~exclude;
    uint64_t knights = pos->bitboards[by_side?BN:WN]&~exclude;
    uint64_t king = pos->bitboards[by_side?BK:WK]&~exclude;
    uint64_t rooks = pos->bitboards[by_side?BR:WR]&~exclude;
    uint64_t bishops = pos->bitboards[by_side?BB:WB]&~exclude;
    uint64_t queens = pos->bitboards[by_side?BQ:WQ]&~exclude;
    uint64_t attack_mask = 0ULL;
    uint64_t occ = pos->occupancies[BOTH]&~exclude;

    uint8_t cur_sq = NO_SQ;

    while (pawns) {
        cur_sq = pop_lsb(&pawns);
        attack_mask |= pawn_attacks[by_side][cur_sq];
    } while (knights) {
        cur_sq = pop_lsb(&knights);
        attack_mask |= knight_attacks[cur_sq];
    } attack_mask |= king_attacks[pop_lsb(&king)];

    while (rooks){
        cur_sq = pop_lsb(&rooks);
        attack_mask |= generate_rook_attacks(cur_sq, occ);
    } while (bishops){
        cur_sq = pop_lsb(&bishops);
        attack_mask |= generate_bishop_attacks(cur_sq, occ);
    } while (queens){
        cur_sq = pop_lsb(&queens);
        attack_mask |= generate_rook_attacks(cur_sq, occ) | generate_bishop_attacks(cur_sq, occ);
    }

    return attack_mask;
}

bool square_attacked(int sq, LegalData *legals){
    return (BBd(sq)&legals->enemy_attack_maps)!=0;
}

void generate_moves(Position *pos, MoveList *moves){
    LegalData legals;
    compute_pins_n_checks(pos, &legals);

    generate_king_moves(pos, moves, &legals);
    if (__builtin_popcountll(legals.checkers) == 2) return;
    generate_pawn_moves(pos, moves, &legals);
    generate_knight_moves(pos, moves, &legals);
    generate_sliding_moves(pos, moves, &legals);

    /* uint64_t p = legals.pinned;
    while (p) {
        int sq = pop_lsb(&p);
        printf("Pinned: %d\n", sq);
    }
    printf("King: %d\n", __builtin_ctzll(pos->bitboards[pos->side_to_move?BK:WK]));
    printf("Pinned mask: %llx\n", legals.pinned);
    */
}