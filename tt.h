#ifndef TT_H
#define TT_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define TT_BUCKET_SIZE 4

typedef enum {
    TT_EXACT,
    TT_LOWERBOUND,
    TT_UPPERBOUND
} TTFlag;

typedef struct {
    uint64_t key;
    int depth;
    int score;
    TTFlag flag;
    uint16_t move;
    bool valid;
} TTEntry;

extern TTEntry *tt_list;
extern size_t tt_size;
extern size_t tt_used;

void tt_init(size_t size_mb);

void tt_free();
void tt_clear();

bool tt_probe(uint64_t key, TTEntry *out_entry);
TTEntry* tt_probe_ptr(uint64_t key);

void tt_store(uint64_t key, int depth, int score, uint8_t flag, uint16_t move);
double tt_fullness();
#endif