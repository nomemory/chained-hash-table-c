#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include "chained_hash.h"

ch_hash *ch_hash_new(ch_key_ops k_ops, ch_val_ops v_ops) {
    ch_hash *hash;
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

void ch_hash_free(ch_hash *hash) {
    
    ch_node *crt;
    ch_node *next;

    for(int i = 0; i < hash->capacity; ++i) {
        // Free memory for each bucket
        crt = hash->buckets[i];
        while(NULL!=crt) {
            next = crt->next;
            
            // Free memory for key and value
            hash->key_ops.free(crt->key, hash->key_ops.arg);
            hash->val_ops.free(crt->val, hash->val_ops.arg);

            // Free the node
            free(crt);
            crt = next;
        }
    }
    // Free the buckets and the hash structure itself
    free(hash->buckets);
    free(hash);
}

static ch_node* ch_hash_get_node(ch_hash *hash, const void *key) {

    ch_node *result = NULL;
    ch_node *crt = NULL;
    uint32_t h;
    size_t bucket_idx;
    
    h = hash->key_ops.hash(key, hash->key_ops.arg);
    bucket_idx = h % hash->capacity;
    crt = hash->buckets[bucket_idx];

    while(NULL!=crt) {
        // Iterated through the linked list to determine if the element is present
        if (crt->hash == h && hash->val_ops.eq(crt->key, key, hash->val_ops.arg)) {
            result = crt;
            break;
        }
        crt = crt->next;
    }

    return result;
}

void* ch_hash_get(ch_hash *hash, const void *k) {
    ch_node *result = NULL;
    if (NULL!=(result=ch_hash_get_node(hash, k))) {
        return result->val;
    }
    return NULL;
}

static void ch_hash_grow(ch_hash *hash) {
    
    ch_node **new_buckets;
    ch_node *crt;
    size_t new_capacity;
    size_t new_idx;

    new_capacity = hash->capacity * CH_HASH_CAPACITY_MULT;
    new_buckets = malloc(sizeof(*new_buckets) * new_capacity);
    if (NULL==new_buckets) {
        fprintf(stderr, "Cannot resize buckets array. Hash table won't be resized.\n");
        return;
    }
    for(int i = 0; i < new_capacity; ++i) {
        new_buckets[i] = NULL;   
    }
    
    // Rehash 
    // For each bucket
    for(int i = 0; i < hash->capacity; i++) {
        // For each linked list
        crt = hash->buckets[i];
        while(NULL!=crt) {
            // Finding the new bucket
            new_idx = crt->hash % new_capacity;
            ch_node *cur = crt;
            crt = crt->next;
            cur->next = new_buckets[new_idx];
            new_buckets[new_idx] = cur;
        }
    }

    hash->capacity = new_capacity;

    // Free the old buckets
    free(hash->buckets);
    
    // Update with the new buckets
    hash->buckets = new_buckets;
}

void ch_hash_put(ch_hash *hash, const void *k, const void *v) {
    ch_node *crt;
    size_t bucket_idx;
    crt = ch_hash_get_node(hash, k);
    if (crt) {
        // Key already exists
        // We need to update the value
        hash->val_ops.free(crt->val, hash->val_ops.arg);
        crt->val = v ? hash->val_ops.cp(v, hash->val_ops.arg) : 0;
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
        crt->hash = hash->key_ops.hash(k, hash->key_ops.arg);
        crt->key = hash->key_ops.cp(k, hash->key_ops.arg);
        crt->val = hash->val_ops.cp(v, hash->val_ops.arg);

        bucket_idx = crt->hash % hash->capacity;
        crt->next = hash->buckets[bucket_idx];
        hash->buckets[bucket_idx] = crt;
        
        // Element has been added succesfuly
        hash->size++;

        // Grow if needed
        if (hash->size > hash->capacity * CH_HASH_GROWTH) {
            ch_hash_grow(hash);
        }
    }
}

bool ch_hash_contains(ch_hash *hash, const void *k) {
    return ch_hash_get_node(hash, k) ? true : false;
}

static uint32_t ch_node_numcol(ch_node* node) {
    uint32_t result = 0;
    if (node) {
        while(node->next!=NULL) {
            result++;
            node = node->next;
        }
    }
    return result;
}

uint32_t ch_hash_numcol(ch_hash *hash) {
    uint32_t result = 0;
    for(int i = 0; i < hash->capacity; ++i) {
        result += ch_node_numcol(hash->buckets[i]);
    }
    return result;
}

void ch_hash_print(ch_hash *hash, void (*print_key)(const void *k), void (*print_val)(const void *v)) {

    ch_node *crt;

    printf("Hash Capacity: %lu\n", hash->capacity);
    printf("Hash Size: %lu\n", hash->size);

    printf("Hash Buckets:\n");
    for(int i = 0; i < hash->capacity; i++) {
        crt = hash->buckets[i];
        printf("\tbucket[%d]:\n", i);
        while(NULL!=crt) {
            printf("\t\thash=%" PRIu32 ", key=", crt->hash);
            print_key(crt->key);
            printf(", value=");
            print_val(crt->val);
            printf("\n");
            crt=crt->next;
        }
    }
}

// String operations

static uint32_t ch_hash_fmix32(uint32_t h) {
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

    return ch_hash_fmix32(hash);
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

ch_key_ops ch_key_ops_string = { ch_string_hash, ch_string_cp, ch_string_free, NULL};
ch_val_ops ch_val_ops_string = { ch_string_cp, ch_string_free, ch_string_eq, NULL};