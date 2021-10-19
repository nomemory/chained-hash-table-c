#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include "chained_hashv.h"

ch_hashv *ch_hashv_new(ch_key_ops k_ops, ch_val_ops v_ops) {

    ch_hashv *hash;
    hash = malloc(sizeof(*hash));

    if(NULL == hash) {
        fprintf(stderr,"malloc() failed in file %s at line # %d", __FILE__,__LINE__);
        exit(EXIT_FAILURE);
    }

    hash->size = 0;
    hash->capacity = CH_HASH_CAPACITY_INIT;
    hash->key_ops = k_ops;
    hash->val_ops = v_ops;
    hash->buckets = malloc(hash->capacity * sizeof(*(hash->buckets)));

    if (NULL == hash->buckets) {
        fprintf(stderr,"malloc() failed in file %s at line # %d", __FILE__,__LINE__);
        exit(EXIT_FAILURE);
    }
    for(int i = 0; i < hash->capacity; i++) {
        // Initially all the buckets are NULL
        // Memory will be allocated for them when needed
        hash->buckets[i] = NULL;
    }

    return hash;
}

void ch_hash_free(ch_hashv *htable) {
    ch_vect *crt;
    ch_node *crt_el;
    for(int i = 0; i < htable->capacity; ++i) {
        // Free memory for each bucket
        crt = htable->buckets[i];
        if (NULL!=crt) {
            for(int j = 0; j < crt->size; j++) {
                crt_el = crt->array[j];
                htable->key_ops.free(crt_el->key, htable->key_ops.arg);
                htable->val_ops.free(crt_el->val, htable->val_ops.arg);
                free(crt_el);
            }
        }
        ch_vect_free(crt);
    }
    // Free the buckets and the hash structure itself
    free(htable->buckets);
    free(htable);
}

static ch_node* ch_hashv_get_node(ch_hashv *htable, const void *key) {

    ch_node *result = NULL;
    ch_node *crt_node = NULL;
    ch_vect *crt_bucket = NULL;

    uint32_t computed_hash;
    size_t bucket_idx;
    
    computed_hash = htable->key_ops.hash(key, htable->key_ops.arg);
    bucket_idx = computed_hash % htable->capacity;
    crt_bucket = htable->buckets[bucket_idx];
    
    if (NULL!=crt_bucket) {
        for(int i = 0; i < crt_bucket->size; ++i) {        
            crt_node = crt_bucket->array[i];
            if (crt_node->hash == computed_hash) {
                if (htable->key_ops.eq(crt_node->key, key, htable->key_ops.arg)) {
                    result = crt_node;
                    break;
                }
            }
        }
    }
    return result;
}

void* ch_hashv_get(ch_hashv *htable, const void *k) {
    ch_node *result = ch_hashv_get_node(htable, k);

    if (NULL!=result) {
        return result->val;
    }

    return NULL;
}

static void ch_hash_grow(ch_hashv *htable) {
    
    ch_vect **new_buckets;
    ch_vect *crt_bucket;
    ch_node *crt_element;
    size_t new_capacity;
    size_t new_idx;

    new_capacity = htable->capacity * CH_HASH_CAPACITY_MULT;
    new_buckets = malloc(sizeof(*new_buckets) * new_capacity);

    if (NULL==new_buckets) {
        fprintf(stderr, "Cannot resize buckets array. Hash table won't be resized.\n");
        return;
    }

    for(int i = 0; i < new_capacity; ++i) {
        new_buckets[i] = NULL;
    }
    
    // Rehash 
    // For each (old) bucket
    for(int i = 0; i < htable->capacity; i++) {
        crt_bucket = htable->buckets[i];
        // For each element from the old bucket
        if (NULL!=crt_bucket) {
            for(int j = 0; j < crt_bucket->size; j++) {
                // For each element from the 
                crt_element = crt_bucket->array[j];
                // Compute the new id for the new bucket
                new_idx = crt_element->hash % new_capacity;
                // If the bucket doesn't exist yet, we create yet
                if (NULL==new_buckets[new_idx]) {
                    new_buckets[new_idx] = ch_vect_new_default();
                }
                // Add the element to the corresponding bucket
                ch_vect_append(new_buckets[new_idx], crt_element);   
            }
        }
    }

    htable->capacity = new_capacity;

    // Free the old buckets
    free(htable->buckets);
    
    // Update with the new buckets
    htable->buckets = new_buckets;
}

void ch_hashv_put(ch_hashv *htable, const void *k, const void *v) {

    ch_node *crt;
    size_t bucket_idx;

    crt = ch_hashv_get_node(htable, k);

    if (NULL!=crt) {
        // Key already exists
        // We need to update the value
        htable->val_ops.free(crt->val, htable->val_ops.arg);
        crt->val = v ? htable->val_ops.cp(v, htable->val_ops.arg) : 0;        
    }

    else {
        // Key doesn't exist
        // - We create a node
        // - We add a node to the correspoding bucket
        crt = malloc(sizeof(*crt));
        if (NULL == crt) {
            fprintf(stderr,"malloc() failed in file %s at line # %d", __FILE__,__LINE__);
            exit(EXIT_FAILURE);
        }   
        crt->hash = htable->key_ops.hash(k, htable->key_ops.arg);
        crt->key = htable->key_ops.cp(k, htable->key_ops.arg);
        crt->val = htable->val_ops.cp(v, htable->val_ops.arg);

        bucket_idx = crt->hash % htable->capacity;
        if (NULL==htable->buckets[bucket_idx]) {
            htable->buckets[bucket_idx] = ch_vect_new_default();
        }
        ch_vect_append(htable->buckets[bucket_idx], crt);
        
        // Element has been added successfully
        htable->size++;

        // Grow if needed
        if (htable->size > htable->capacity * CH_HASH_GROWTH) {
            ch_hash_grow(htable);
        }
    }
}

bool ch_hashv_contains(ch_hashv *htable, const void *k) {
    return ch_hashv_get_node(htable, k) ? true : false;
}

static uint32_t ch_node_numcol(ch_vect* bucket) {
    return (bucket->size == 0) ? 0 : bucket->size-1;
}

uint32_t ch_hashv_numcol(ch_hashv *htable) {
    uint32_t result = 0;
    for(int i = 0; i < htable->capacity; ++i) {
        result += ch_node_numcol(htable->buckets[i]);
    }
    return result;
}

void ch_hashv_print(ch_hashv *htable, void (*print_key)(const void *k), void (*print_val)(const void *v)) {

    ch_vect *crt_bucket;
    ch_node *crt_el;

    printf("Hash Capacity: %lu\n", htable->capacity);
    printf("Hash Size: %lu\n", htable->size);

    printf("Hash Buckets:\n");
    for(int i = 0; i < htable->capacity; i++) {
        crt_bucket = htable->buckets[i];
        printf("\tbucket[%d]:\n", i);
        if (NULL!=crt_bucket) {
            for(int j = 0; j < crt_bucket->size; j++) {
                crt_el = crt_bucket->array[j];
                printf("\t\thash=%" PRIu32 ", key=", crt_el->hash);
                print_key(crt_el->key);
                printf(", value=");
                print_val(crt_el->val);
                printf("\n");
            }
        }
    }
}

// String operations

static uint32_t ch_hashv_fmix32(uint32_t h) {
    h ^= h >> 16;
    h *= 0x3243f6a9U;
    h ^= h >> 16;
    return h;
}

uint32_t ch_string_hash(const void *data, void *arg) {
    //djb2
    uint32_t hash = (const uint32_t) 5381;
    const char *str = (const char*) data;
    char c;
    while((c=*str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return ch_hashv_fmix32(hash);
}


void* ch_string_cp(const void *data, void *arg) {
    const char *input = (const char*) data;
    size_t input_length = strlen(input) + 1;
    char *result;
    result = malloc(sizeof(*result) * input_length);
    if (NULL==result) {
        fprintf(stderr,"malloc() failed in file %s at line # %d", __FILE__,__LINE__);
        exit(EXIT_FAILURE);
    }
    strcpy(result, input);
    return result;
}

bool ch_string_eq(const void *data1, const void *data2, void *arg) {
    const char *str1 = (const char*) data1;
    const char *str2 = (const char*) data2;
    return !(strcmp(str1, str2)) ? true : false;    
}

void ch_string_free(void *data, void *arg) {
    free(data);
}

void ch_string_print(const void *data) {
    printf("%s", (const char*) data);
}

ch_key_ops ch_key_ops_string = { ch_string_hash, ch_string_cp, ch_string_free, ch_string_eq, NULL};
ch_val_ops ch_val_ops_string = { ch_string_cp, ch_string_free, ch_string_eq, NULL };