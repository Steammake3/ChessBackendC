#include <stdint.h>
#include <string.h>
#include "position.h"
#include "tt.h"

TTEntry *tt_list;
size_t tt_size;

void tt_init(size_t size_mb){
    size_t bytes = size_mb * 1024 * 1024;
    size_t total_entries = bytes / sizeof(TTEntry);

    // number of buckets
    tt_size = total_entries / TT_BUCKET_SIZE;

    //force power of two (IMPORTANT for & masking)
    size_t pow2 = 1;
    while (pow2 <= tt_size) pow2 <<= 1;
    tt_size = pow2 >> 1;

    tt_list = malloc(tt_size * TT_BUCKET_SIZE * sizeof(TTEntry));
    memset(tt_list, 0, tt_size * TT_BUCKET_SIZE * sizeof(TTEntry));
}

void tt_free(){
    free(tt_list);
    tt_list = NULL;
}
void tt_clear(){
    if (tt_list) memset(tt_list, 0, tt_size * TT_BUCKET_SIZE * sizeof(TTEntry));
}

bool tt_probe(uint64_t key, TTEntry *out_entry){
    size_t index = key & (tt_size - 1);

    TTEntry *bucket = &tt_list[index * TT_BUCKET_SIZE];

    for (int i = 0; i < TT_BUCKET_SIZE; i++){
        TTEntry *entry = &bucket[i];

        if (entry->key == key){
            *out_entry = *entry;
            return true;
        }
    }

    return false;
}

TTEntry* tt_probe_ptr(uint64_t key) { //BE CAREFUL, DONT DESTROY TT, PLZ
    size_t index = key & (tt_size - 1);
    TTEntry *bucket = &tt_list[index * TT_BUCKET_SIZE];

    for (int i = 0; i < TT_BUCKET_SIZE; i++) {
        TTEntry *entry = &bucket[i];
        if (entry->key == key)
            return entry;
    }

    return NULL;
}

void tt_store(uint64_t key, int depth, int score, uint8_t flag, uint16_t move){
    size_t index = key & (tt_size - 1);
    TTEntry *bucket = &tt_list[index * TT_BUCKET_SIZE];

    TTEntry *best = NULL;
    TTEntry *shallowest = &bucket[0];

    for (int i = 0; i < TT_BUCKET_SIZE; i++){
        TTEntry *entry = &bucket[i];

        // overwrite if same key and exact
        if (entry->key == key){
            if (depth >= entry->depth || flag == TT_EXACT){
                best = entry; break;
            } else {
                return; //don't overwrite better info (rookie mistake bruh)
            }
        }

        // empty slot
        if (entry->key == 0 && best == NULL){
            best = entry;
        }

        // track shallowest
        if (entry->depth < shallowest->depth){
            shallowest = entry;
        }
    }

    if (best == NULL){
        best = shallowest;
    }

    best->key = key;
    best->depth = depth;
    best->score = score;
    best->flag = flag;
    best->move = move;
}