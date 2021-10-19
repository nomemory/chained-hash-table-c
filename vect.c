#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stddef.h>

#include "vect.h"

ch_vect* ch_vect_new(size_t capacity) {
    ch_vect *result;
    result = malloc(sizeof(*result));
    if (NULL==result) {
        fprintf(stderr,"malloc() failed in file %s at line # %d", __FILE__,__LINE__);
        exit(EXIT_FAILURE);  
    }
    result->capacity = capacity;
    result->size = 0;
    result->array = malloc(result->capacity * sizeof(*(result->array)));
    if (NULL == result->array) {
        fprintf(stderr,"malloc() failed in file %s at line # %d", __FILE__,__LINE__);
        exit(EXIT_FAILURE);  
    }
    return result;
}

ch_vect* ch_vect_new_default() {
    return ch_vect_new(VECT_INIT_CAPACITY);
}

void ch_vect_free(ch_vect *vect) {
    free(vect->array);
    free(vect);
}

void* ch_vect_get(ch_vect *vect, size_t idx) {
    if (idx >= vect->size) {
        fprintf(stderr, "cannot get index %lu from vector.\n", idx);
        exit(EXIT_FAILURE);  
    }   
    return vect->array[idx];
}

void ch_vect_set(ch_vect *vect, size_t idx, void *data) {
    if (idx >= vect->size) {
        fprintf(stderr, "cannot get index %lu from vector.\n", idx);
        exit(EXIT_FAILURE);  
    }
    vect->array[idx] = data;
}

void ch_vect_append(ch_vect *vect, void *data) {
    if (!(vect->size < vect->capacity)) { 
        // Check for a potential overflow
        uint64_t tmp = (uint64_t) VECT_GROWTH_MULTI * (uint64_t) vect->capacity;
        if (tmp > SIZE_MAX) {
            fprintf(stderr, "size overflow\n");
            exit(EXIT_FAILURE);
        }
        size_t new_capacity = (size_t) tmp;
        //void *new_array = malloc(new_capacity * sizeof(*(vect->array)));
        vect->array = realloc(vect->array, new_capacity * sizeof(*(vect->array)));
        if (NULL==vect->array) {
            fprintf(stderr,"realloc() failed in file %s at line # %d", __FILE__,__LINE__);
            exit(EXIT_FAILURE);  
        }   
        vect->capacity = new_capacity;
    }
    vect->array[vect->size] = data;
    vect->size++;
}