#ifndef HT_H_
#define HT_H_

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define HT_INIT_CAP 1024        // Must be power of 2
#define HT_LOAD_FACTOR_PERCENT 70

typedef enum {
    HT_EMPTY,
    HT_OCCUPIED,
    HT_DELETED,
} Ht_Slot;

#define Hash_Table(Key, Value)  \
    struct {                    \
        Key       key;          \
        Value    *value;        \
        Key      *keys;         \
        Value    *values;       \
        uint32_t *hashes;       \
        Ht_Slot  *slots;        \
        size_t    count;        \
        size_t    filled_slots; \
        size_t    capacity;     \
        Key_Hash  key_hash;     \
        Key_Eq    key_eq;       \
    }

typedef uint32_t (*Key_Hash)(void const *key);
typedef bool (*Key_Eq)(void const *a, void const *b);

// TASK(20260303-184629): add more different hash functions
uint32_t ht_djb2(void const *data, size_t size);

uint32_t ht_cstr_hash_djb2(void const *key);
bool ht_cstr_eq(void const *a, void const *b);

// Value *ht_find(Hash_Table(Key, Value) *ht, Key key)
//
// Tries to find a value by they key. If found returns the pointer to the value,
// otherwise returns NULL.
#define ht_find(ht, key_)                                                     \
    ((ht)->key = (key_),                                                      \
     (ht)->value = ht__find((ht)->keys, sizeof(*(ht)->keys),                  \
                            ht__default_key_hash(ht), ht__default_key_eq(ht), \
                            (ht)->values, sizeof(*(ht)->values),              \
                            (ht)->slots, (ht)->hashes,                        \
                            (ht)->capacity,                                   \
                            &(ht)->key))

// Value *ht_find_or_put(Hash_Table(Key, Value) *ht, Key key)
//
// Tries to find a value by the key, if not found inserts the key with the value implicitly zero-initialized.
// Never fails. Always returns either the pointer to the found value or the newly added value.
#define ht_find_or_put(ht, key_)                                                            \
    ((ht)->key = (key_),                                                                    \
     (ht)->value = ht__find((ht)->keys, sizeof(*(ht)->keys),                                \
                            ht__default_key_hash(ht), ht__default_key_eq(ht),               \
                            (ht)->values, sizeof(*(ht)->values),                            \
                            (ht)->slots, (ht)->hashes,                                      \
                            (ht)->capacity,                                                 \
                            &(ht)->key),                                                    \
     (ht)->value ? (ht)->value : (                                                          \
         ht__expand((void**)&(ht)->keys, sizeof(*(ht)->keys),                               \
                    ht__default_key_hash(ht), ht__default_key_eq(ht),                       \
                    (void**)&(ht)->values, sizeof(*(ht)->values),                           \
                    &(ht)->slots, &(ht)->hashes,                                            \
                    &(ht)->capacity, &(ht)->filled_slots, &(ht)->count),                    \
         (ht)->value = ht__put_no_expand((ht)->keys, sizeof(*(ht)->keys),                   \
                                         ht__default_key_hash(ht), ht__default_key_eq(ht),  \
                                         (ht)->values, sizeof(*(ht)->values),               \
                                         (ht)->slots, (ht)->hashes,                         \
                                         (ht)->capacity, &(ht)->filled_slots, &(ht)->count, \
                                         &(ht)->key)))

// Value *ht_put(Hash_Table(Key, Value) *ht, Key key)
//
// Puts a the key with the value zero-initialized.
// Returns the pointer to the inserted value.
#define ht_put(ht, key_)                                                                \
    ((ht)->key = (key_),                                                                \
     ht__expand((void**)&(ht)->keys, sizeof(*(ht)->keys),                               \
                ht__default_key_hash(ht), ht__default_key_eq(ht),                       \
                (void**)&(ht)->values, sizeof(*(ht)->values),                           \
                &(ht)->slots, &(ht)->hashes,                                            \
                &(ht)->capacity, &(ht)->filled_slots, &(ht)->count),                    \
     (ht)->value = ht__put_no_expand((ht)->keys, sizeof(*(ht)->keys),                   \
                                     ht__default_key_hash(ht), ht__default_key_eq(ht),  \
                                     (ht)->values, sizeof(*(ht)->values),               \
                                     (ht)->slots, (ht)->hashes,                         \
                                     (ht)->capacity, &(ht)->filled_slots, &(ht)->count, \
                                     &(ht)->key))

// void ht_delete(Hash_Table(Key, Value) *ht, Value *value)
#define ht_delete(ht, value_) \
    ht__delete((ht)->values, sizeof(*(ht)->values), (ht)->slots, (ht)->capacity, &(ht)->count, value_)

#define ht_foreach(ht)                                \
    for (                                             \
        (ht)->value = NULL;                           \
        ht__next((ht)->slots, (ht)->capacity,         \
                 (ht)->keys, sizeof(*(ht)->keys),     \
                 (ht)->values, sizeof(*(ht)->values), \
                 &(ht)->key, (void**)&(ht)->value);   \
    )

// void ht_reset(Hash_Table(Key, Value) *ht)
//
// Removes all the elements from the hash table, but does not deallocate any memory, making the hash table
// ready to be reused again.
#define ht_reset(ht)                                              \
    (memset((ht)->slots, 0, sizeof(*(ht)->slots)*(ht)->capacity), \
     (ht)->filled_slots = 0,                                      \
     (ht)->count = 0,                                             \
     (void)0)

// void ht_free(Hash_Table(Key, Value) *ht)
//
// Deallocates all the memory associated with the hash table and completely resets its state.
#define ht_free(ht)                              \
    (                                            \
        free((ht)->keys),   (ht)->keys = NULL,   \
        free((ht)->values), (ht)->values = NULL, \
        free((ht)->slots),  (ht)->slots = NULL,  \
        free((ht)->hashes), (ht)->hashes = NULL, \
        (ht)->filled_slots = 0,                  \
        (ht)->count = 0,                         \
        (ht)->capacity = 0,                      \
        (void)0                                  \
    )

// TASK(20260303-184648): Do not allocate more memory if after rehashing filled_slots don't exceed HT_LOAD_FACTOR_PERCENT
// TASK(20260303-184659): ht_find and ht_foreach may conflict with each other since they use the same ht->value.
// TASK(20260303-184712): find some stress testing benchmarks for hash tables online and test ht.h on them

// Private functions. Do not use directly. //////////////////////////////

static bool ht__next(Ht_Slot *ht_slots, size_t ht_capacity,
                     void *ht_keys, size_t ht_key_size,
                     void *ht_values, size_t ht_value_size,
                     void *key, void **value);
static void *ht__put_no_expand(void *ht_keys, size_t ht_key_size, Key_Hash key_hash, Key_Eq key_eq,
                               void *ht_values, size_t ht_value_size,
                               Ht_Slot *ht_slots, uint32_t *ht_hashes,
                               size_t ht_capacity, size_t *ht_filled_slots, size_t *ht_count,
                               void *key);
static void ht__expand(void **ht_keys, size_t ht_key_size, Key_Hash key_hash, Key_Eq key_eq,
                       void **ht_values, size_t ht_value_size,
                       Ht_Slot **ht_slots, uint32_t **ht_hashes,
                       size_t *ht_capacity, size_t *ht_filled_slots, size_t *ht_count);
static void *ht__find(void *ht_keys, size_t ht_key_size, Key_Hash key_hash, Key_Eq key_eq,
                      void *ht_values, size_t ht_value_size,
                      Ht_Slot *ht_slots, uint32_t *ht_hashes,
                      size_t ht_capacity,
                      void *key);
static void ht__delete(void *ht_values, size_t ht_value_size,
                       Ht_Slot *ht_slots,
                       size_t ht_capacity, size_t *ht_count,
                       void *value);

#ifdef ht_user_key_hash
#error "ht_user_key_hash() has been removed. Use .key_hash field of Hash_Table instead"
#endif
#define ht__default_key_hash(ht)                       \
    ((ht)->key_hash ?                                  \
     (ht)->key_hash :                                  \
     _Generic(*(ht)->keys,                             \
              char*:                ht_cstr_hash_djb2, \
              const char*:          ht_cstr_hash_djb2, \
              unsigned char*:       ht_cstr_hash_djb2, \
              const unsigned char*: ht_cstr_hash_djb2, \
              default:              (ht)->key_hash))

#ifdef ht_user_key_hash
#error "ht_user_key_hash() has been removed. Use .key_hash field of Hash_Table instead"
#endif
#define ht__default_key_eq(ht)                  \
    ((ht)->key_eq ?                             \
     (ht)->key_eq :                             \
     _Generic(*(ht)->keys,                      \
              char*:                ht_cstr_eq, \
              const char*:          ht_cstr_eq, \
              unsigned char*:       ht_cstr_eq, \
              const unsigned char*: ht_cstr_eq, \
              default:              (ht)->key_eq))

#endif // HT_H_

#ifdef HT_IMPLEMENTATION

uint32_t ht_cstr_hash_djb2(void const* key)
{
    char const* const* key_typed = key;
    return ht_djb2(*key_typed, strlen(*key_typed));
}

bool ht_cstr_eq(void const* a, void const* b)
{
    char const* const* a_typed = a;
    char const* const* b_typed = b;
    return strcmp(*a_typed, *b_typed) == 0;
}

uint32_t ht_djb2(void const *data, size_t size)
{
    const uint8_t *bytes = data;
    uint32_t hash = 5381;
    for (size_t i = 0; i < size; ++i) {
        hash += ((hash << 5) + hash) + (uint32_t)bytes[i];
    }
    return hash;
}

static bool ht__next(Ht_Slot *ht_slots, size_t ht_capacity,
                     void *ht_keys, size_t ht_key_size,
                     void *ht_values, size_t ht_value_size,
                     void *key, void **value)
{
    assert(ht_key_size > 0);
    assert(ht_value_size > 0);

    uint8_t *keys   = ht_keys;
    uint8_t *values = ht_values;

    if (*value == NULL) *value = ht_values;

    assert(ht_values <= *value);
    size_t index = ((uint8_t*)*value - values)/ht_value_size + 1;
    while (index < ht_capacity && ht_slots[index] != HT_OCCUPIED) {
        index += 1;
    }
    if (index >= ht_capacity) return false;
    memcpy(key, keys + index*ht_key_size, ht_key_size);
    *value = values + index*ht_value_size;
    return true;
}

static void *ht__put_no_expand(void *ht_keys, size_t ht_key_size, Key_Hash key_hash, Key_Eq key_eq,
                               void *ht_values, size_t ht_value_size,
                               Ht_Slot *ht_slots, uint32_t *ht_hashes,
                               size_t ht_capacity, size_t *ht_filled_slots, size_t *ht_count,
                               void *key)
{
    assert(ht_key_size > 0);
    assert(ht_value_size > 0);

    uint8_t *keys   = ht_keys;
    uint8_t *values = ht_values;

    uint32_t hash = key_hash ? key_hash(key) : ht_djb2(key, ht_key_size);
    uint32_t index = hash%ht_capacity;
    uint32_t step = 1;
    if (key_eq) {
        while (
            ht_slots[index] == HT_OCCUPIED &&
            !(ht_hashes[index] == hash &&
              key_eq(keys + index*ht_key_size, key))
         ) {
            index = (index + step)%ht_capacity;
            step += 1;
        }
    } else {
        while (
            ht_slots[index] == HT_OCCUPIED &&
            !(ht_hashes[index] == hash &&
             memcmp(keys + index*ht_key_size, key, ht_key_size) == 0)
         ) {
            index = (index + step)%ht_capacity;
            step += 1;
        }
    }
    if (ht_slots[index] != HT_OCCUPIED) {
        if (ht_slots[index] == HT_EMPTY) {
            *ht_filled_slots += 1;
        }
        ht_slots[index] = HT_OCCUPIED;
        memcpy(keys + index*ht_key_size, key, ht_key_size);
        memset(values + index*ht_value_size, 0, ht_value_size);
        ht_hashes[index] = hash;
        *ht_count += 1;
    } else {
        memset(values + index*ht_value_size, 0, ht_value_size);
    }
    return values + index*ht_value_size;
}

static void ht__expand(void **ht_keys, size_t ht_key_size, Key_Hash key_hash, Key_Eq key_eq,
                       void **ht_values, size_t ht_value_size,
                       Ht_Slot **ht_slots, uint32_t **ht_hashes,
                       size_t *ht_capacity, size_t *ht_filled_slots, size_t *ht_count)
{
    if ((*ht_capacity) == 0 || (*ht_filled_slots)*100 >= HT_LOAD_FACTOR_PERCENT*(*ht_capacity)) {
        size_t new_ht_capacity = 0;
        if ((*ht_capacity) == 0) {
            new_ht_capacity = HT_INIT_CAP;
        } else {
            new_ht_capacity = (*ht_capacity)*2;
        }
        size_t new_ht_filled_slots = 0;
        size_t new_ht_count = 0;

        void     *new_ht_keys   = calloc(new_ht_capacity, ht_key_size);
        void     *new_ht_values = calloc(new_ht_capacity, ht_value_size);
        Ht_Slot  *new_ht_slots  = calloc(new_ht_capacity, sizeof(*new_ht_slots));
        uint32_t *new_ht_hashes = calloc(new_ht_capacity, sizeof(*new_ht_hashes));

        assert(ht_key_size > 0);
        assert(ht_value_size > 0);

        uint8_t *keys   = *ht_keys;
        uint8_t *values = *ht_values;
        for (size_t i = 0; i < (*ht_capacity); ++i) {
            if ((*ht_slots)[i] == HT_OCCUPIED) {
                void *slot = ht__put_no_expand(new_ht_keys, ht_key_size, key_hash, key_eq,
                                               new_ht_values, ht_value_size,
                                               new_ht_slots, new_ht_hashes,
                                               new_ht_capacity,
                                               &new_ht_filled_slots, &new_ht_count,
                                               keys + i*ht_key_size);
                memcpy(slot, values + i*ht_value_size, ht_value_size);
            }
        }

        free(*ht_keys);
        free(*ht_values);
        free(*ht_slots);
        free(*ht_hashes);
        *ht_keys         = new_ht_keys;
        *ht_values       = new_ht_values;
        *ht_slots        = new_ht_slots;
        *ht_hashes       = new_ht_hashes;
        *ht_filled_slots = new_ht_filled_slots;
        *ht_count        = new_ht_count;
        *ht_capacity     = new_ht_capacity;
    }
}

static void *ht__find(void *ht_keys, size_t ht_key_size, Key_Hash key_hash, Key_Eq key_eq,
                      void *ht_values, size_t ht_value_size,
                      Ht_Slot *ht_slots, uint32_t *ht_hashes,
                      size_t ht_capacity,
                      void *key)
{
    if (ht_capacity == 0) return NULL;

    assert(ht_key_size > 0);
    assert(ht_value_size > 0);

    uint8_t *keys   = ht_keys;
    uint8_t *values = ht_values;

    uint32_t hash = key_hash ? key_hash(key) : ht_djb2(key, ht_key_size);
    uint32_t index = hash%ht_capacity;
    uint32_t step = 1;
    if (key_eq) {
        while (
            ht_slots[index] == HT_DELETED ||
            (ht_slots[index] == HT_OCCUPIED &&
             !(ht_hashes[index] == hash &&
               key_eq(keys + index*ht_key_size, key)))
        ) {
            index = (index + step)%ht_capacity;
            step += 1;
        }
    } else {
        while (
            ht_slots[index] == HT_DELETED ||
            (ht_slots[index] == HT_OCCUPIED &&
             !(ht_hashes[index] == hash &&
               memcmp(keys + index*ht_key_size, key, ht_key_size) == 0))
        ) {
            index = (index + step)%ht_capacity;
            step += 1;
        }
    }

    if (ht_slots[index] == HT_OCCUPIED) {
        return values + index*ht_value_size;
    }
    return NULL;
}

static void ht__delete(void *ht_values, size_t ht_value_size,
                       Ht_Slot *ht_slots,
                       size_t ht_capacity,
                       size_t *ht_count, void *value)
{
    if (value == NULL) return;
    assert(ht_value_size > 0);
    assert(ht_values <= value);
    size_t index = ((char*)value - (char*)ht_values)/ht_value_size;
    assert(index < ht_capacity);
    assert(ht_slots[index] == HT_OCCUPIED);
    ht_slots[index] = HT_DELETED;
    *ht_count -= 1;
}

#endif // HT_IMPLEMENTATION
