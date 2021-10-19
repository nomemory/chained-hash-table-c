#include <stddef.h>

#define VECT_INIT_CAPACITY (32)
#define VECT_GROWTH_MULTI (2)

typedef struct ch_vect_s {
	size_t capacity;
	size_t size;
	void **array;
} ch_vect;

ch_vect* ch_vect_new(size_t capacity);
ch_vect* ch_vect_new_default();
void ch_vect_free(ch_vect *vect);
void* ch_vect_get(ch_vect *vect, size_t idx);
void ch_vect_set(ch_vect *vect, size_t idx, void *data);
void ch_vect_append(ch_vect *vect, void *data);