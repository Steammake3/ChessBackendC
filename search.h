#ifndef SEARCH_H
#define SEARCH_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "types.h"
#include "position.h"
#include "tt.h"
#include "movegen.h"

#define STALEMATE 1
#define THREEFOLD 2
#define FIFTYMOVES 3
#define CHECKMATE 4
#define CONTEMPT 0 //At some point I will add/fix this but for rn it breaks symmetry

#define INF 999999
#define TIMEOUT 99999999
#define TIMEOUT_MOVE 0xfff7

extern clock_t start;
extern float bot_time_control;
extern float elapsed_time;
extern uint64_t nodes;
extern uint64_t qnodes;
#ifdef LOG
    extern FILE *log;
#endif

int be_referee(Position *pos, MoveList *moves, LegalData *legs, int depth);
int get_draw_type(Position *pos, MoveList *moves, LegalData *legs);

int quiesence_search(Position *pos, int alpha, int beta);
int search(Position *pos, uint8_t depth, int alpha, int beta, uint16_t *move);

uint8_t extensions(Position *pos, MoveList *moves, LegalData *legs);

uint16_t get_best_move(Position *pos, uint8_t depth);

void order_moves(MoveList *ml, Position *pos, LegalData *legs);

static inline int guess_move_priority(uint16_t move, Position *pos, LegalData *legs, TTEntry *entry){
    if (entry && move == entry->move) return INF; //Hashed move

    int guess_rn = 0;

    uint8_t from = MOVE_FROM(move); uint8_t to = MOVE_TO(move); uint8_t promo = MOVE_FLAGS(move);
    uint8_t mover = piece_at(from, pos); uint8_t mover_type = PIECEtype(mover);
    uint8_t victim = piece_at(to, pos); uint8_t victim_type = PIECEtype(victim);
    bool side = pos->side_to_move;

    if (victim!=NO_SQ){ //MVV-LVA
        guess_rn += 250 + 10 * values[victim_type] - values[mover_type];
    }

    if (mover_type==WP && (to%8==0 || to%8==7)){ //Promo
        guess_rn += 100 + values[4-PIECEtype(promo)];
    }

    if (BBd(to)&legs->enemy_attack_maps){ //Don't be attacked
        guess_rn -= values[mover_type];
    }

    //if (move_is_check(pos, move)) guess_rn += 5000;

    return guess_rn;
};

#endif