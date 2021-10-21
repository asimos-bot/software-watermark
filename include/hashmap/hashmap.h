#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct HASHMAP_NODE {
    struct HASHMAP_NODE* prev;
    struct HASHMAP_NODE* next;
    void* data;
    unsigned long data_len;
    void* key;
    unsigned long key_len;

} HASHMAP_NODE;

typedef struct HASHMAP {
    unsigned long n;
    HASHMAP_NODE** hashmap;
    unsigned long (*hash_function)(void* key, unsigned long key_len);
    uint8_t copy_key, copy_data;
} HASHMAP;

#include "utils/utils.h"

HASHMAP* hashmap_create(unsigned long n, uint8_t copy_key, uint8_t copy_data, unsigned long (*hash)(void*, unsigned long));
void* hashmap_get(HASHMAP* hashmap, void* key, unsigned long* len);
void hashmap_set(HASHMAP* hashmap, void* key, unsigned long key_len, void* data, unsigned long data_len);
void hashmap_destroy(HASHMAP* hashmap, void* key, unsigned long key_len);
void hashmap_free(HASHMAP* hashmap);

#endif
