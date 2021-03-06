#define CH_HASH_CAPACITY_INIT (32)
#define CH_HASH_CAPACITY_MULT (2)
#define CH_HASH_GROWTH (1)

typedef struct ch_key_ops_s {
    uint32_t (*hash)(const void *data, void *arg);
    void* (*cp)(const void *data, void *arg);
    void (*free)(void *data, void *arg);
    bool (*eq)(const void *data1, const void *data2, void *arg);
    void *arg;
} ch_key_ops;

typedef struct ch_val_ops_s {
    void* (*cp)(const void *data, void *arg);
    void (*free)(void *data, void *arg);
    bool (*eq)(const void *data1, const void *data2, void *arg);
    void *arg;
} ch_val_ops;

typedef struct ch_node_s {
    uint32_t hash;
    void *key;
    void *val;
    struct ch_node_s *next;
} ch_node;

typedef struct ch_hash_s {
    size_t capacity;
    size_t size;
    ch_node **buckets;
    ch_key_ops key_ops;
    ch_val_ops val_ops;
} ch_hash;


// Creates a new hash table
ch_hash *ch_hash_new(ch_key_ops k_ops, ch_val_ops v_ops);

// Free the memory associated with the hash (and all of its contents)
void ch_hash_free(ch_hash *hash);

// Gets the value coresponding to a key
// If the key is not found returns NULL
void* ch_hash_get(ch_hash *hash, const void *k);

// Checks if a key exists or not in the hash table
bool ch_hash_contains(ch_hash *hash, const void *k);

// Adds a <key, value> pair to the table
void ch_hash_put(ch_hash *hash, const void *k, const void *v);

// Prints the contents of the hash table 
void ch_hash_print(ch_hash *hash, void (*print_key)(const void *k), void (*print_val)(const void *v));

// Get the total number of collisions
uint32_t ch_hash_numcol(ch_hash *hash);

// String operations

uint32_t ch_string_hash(const void *data, void *arg);
void* ch_string_cp(const void *data, void *arg);
bool ch_string_eq(const void *data1, const void *data2, void *arg);
void ch_string_free(void *data, void *arg);
void ch_string_print(const void *data);

extern ch_key_ops ch_key_ops_string;
extern ch_val_ops ch_val_ops_string;