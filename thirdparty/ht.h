#ifndef HT_H_
#define HT_H_

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// The Hash Table.
//
// You can define it like this:
// ```c
// Hash_Table(int, int) ht = {0}; // Zero initialized Hash Table is a valid Hash Table!
// ```
//
// You should probably typedef it if you want to pass it to multiple places. Because anonymous structs
// are not particularly compatible with each other even if the have literally the same definition:
//
// ```c
// typedef Hash_Table(const char*, int) Word_Count;
//
// void count_words(Word_Count *wc, const char *text);
// ```
#define Hash_Table(Key, Value)                                 \
    struct {                                                   \
        /* Amount of items in the Hash Table */                \
        size_t    count;                                       \
        /* Key hashing function.                               \
         * NULL means a default hashing function will be used. \
         */                                                    \
        Key_Hash  key_hash;                                    \
        /* Key equality function.                              \
         * NULL means the key will be compared by bytes.       \
         */                                                    \
        Key_Eq    key_eq;                                      \
        /* This fields are subject to change.                  \
         * Do not access any of these fields if you just need  \
         * to iterate the Table. Use ht_foreach() instead.     \
         */                                                    \
        struct {                                               \
            Key       temp_key;                                \
            Value    *temp_value;                              \
            Key      *keys;                                    \
            Value    *values;                                  \
            uint32_t *hashes;                                  \
            Ht_Slot  *slots;                                   \
            size_t    filled_slots;                            \
            size_t    capacity;                                \
        } impl;                                                \
    }

// Usually the Hash Table will work fine with the majority of the Key types. By default it compares and
// hashes the keys by their byte representation. For the C stringy types:
// - const char *
// - char *
// - const unsigned char *
// - unsigned char *
// it makes an exception and treats them as NULL-terminated strings.
//
// This works for like 90% of the cases. If you need to treat your keys in a different way, create
// custom Key_Hash and Key_Eq functions and put them into .key_hash and .key_eq fields of the Hash_Table
//
// ```c
// typedef struct {
//     const char *data;
//     int count;
// } String_View;
//
// uint32_t sv_key_hash(void const* a);
// bool sv_key_eq(void const* a, void const* b);
//
// Hash_Table(String_View, int) ht = {
//     .key_hash = sv_key_hash,
//     .key_eq   = sv_key_eq,
// };
//
// uint32_t sv_key_hash(void const* key)
// {
//     String_View const* key_typed = key;
//     return ht_default_hash(key_typed->data, key_typed->count);
// }
//
// bool sv_key_eq(void const* a, void const* b)
// {
//     String_View const* a_typed = a;
//     String_View const* b_typed = b;
//     if (a_typed->count != b_typed->count) return false;
//     return memcmp(a_typed->data, b_typed->data, a_typed->count) == 0;
// }
// ```
typedef uint32_t (*Key_Hash)(void const *key);
typedef bool (*Key_Eq)(void const *a, void const *b);

// Value *ht_put(Hash_Table(Key, Value) *ht, Key key)
//
// Puts a the key with the value zero-initialized.
// Returns the pointer to the inserted value.
//
// ```c
// Hash_Table(const char *, int) ht = {0};
// *ht_put(&ht, "foo") = 69;
// *ht_put(&ht, "bar") = 420;
// *ht_put(&ht, "baz") = 1337;
// ```
#define ht_put(ht, key)                                                                 \
    ((ht)->impl.temp_key = (key),                                                       \
     ht__expand((void**)&(ht)->impl.keys, sizeof(*(ht)->impl.keys),                     \
                ht__default_key_hash(ht), ht__default_key_eq(ht),                       \
                (void**)&(ht)->impl.values, sizeof(*(ht)->impl.values),                 \
                &(ht)->impl.slots, &(ht)->impl.hashes,                                  \
                &(ht)->impl.capacity, &(ht)->impl.filled_slots, &(ht)->count),          \
     (ht)->impl.temp_value =                                                            \
         ht__decltype_cast((ht)->impl.temp_value)                                       \
         ht__put_no_expand((ht)->impl.keys, sizeof(*(ht)->impl.keys),                   \
                           ht__default_key_hash(ht), ht__default_key_eq(ht),            \
                           (ht)->impl.values, sizeof(*(ht)->impl.values),               \
                           (ht)->impl.slots, (ht)->impl.hashes,                         \
                           (ht)->impl.capacity, &(ht)->impl.filled_slots, &(ht)->count, \
                           &(ht)->impl.temp_key))

// Value *ht_find(Hash_Table(Key, Value) *ht, Key key)
//
// Tries to find a value by they key. If found returns the pointer to the value,
// otherwise returns NULL.
//
// ```c
// int n = 5;
// const char *words[n] = {"foo", "bar", "foo", "baz", "aboba"};
// Hash_Table(const char*, int) ht = {0};
// for (int i = 0; i < n; ++i) {
//     int *count = ht_find(&ht, words[i]);
//     if (count) {
//         *count += 1;
//     } else {
//         *ht_put(&ht, words[i]) = 1;
//     }
// }
// ```
#define ht_find(ht, key)                                            \
    ((ht)->impl.temp_key = (key),                                   \
     (ht)->impl.temp_value =                                        \
         ht__decltype_cast((ht)->impl.temp_value)                   \
         ht__find((ht)->impl.keys, sizeof(*(ht)->impl.keys),        \
                  ht__default_key_hash(ht), ht__default_key_eq(ht), \
                  (ht)->impl.values, sizeof(*(ht)->impl.values),    \
                  (ht)->impl.slots, (ht)->impl.hashes,              \
                  (ht)->impl.capacity,                              \
                  &(ht)->impl.temp_key))

// Value *ht_find_or_put(Hash_Table(Key, Value) *ht, Key key)
//
// Tries to find a value by the key, if not found inserts the key with the value implicitly zero-initialized.
// Never fails. Always returns either the pointer to the found value or the newly added value.
//
// ```c
// int n = 5;
// const char *words[n] = {"foo", "bar", "foo", "baz", "aboba"};
// Hash_Table(const char*, int) ht = {0};
// for (int i = 0; i < n; ++i) {
//     *ht_find_or_put(&ht, words[i]) += 1;
// }
// ```
#define ht_find_or_put(ht, key)                                                             \
    ((ht)->impl.temp_key = (key),                                                           \
     (ht)->impl.temp_value =                                                                \
         ht__decltype_cast((ht)->impl.temp_value)                                           \
         ht__find((ht)->impl.keys, sizeof(*(ht)->impl.keys),                                \
                  ht__default_key_hash(ht), ht__default_key_eq(ht),                         \
                  (ht)->impl.values, sizeof(*(ht)->impl.values),                            \
                  (ht)->impl.slots, (ht)->impl.hashes,                                      \
                  (ht)->impl.capacity,                                                      \
                  &(ht)->impl.temp_key),                                                    \
     (ht)->impl.temp_value ? (ht)->impl.temp_value : (                                      \
         ht__expand((void**)&(ht)->impl.keys, sizeof(*(ht)->impl.keys),                     \
                    ht__default_key_hash(ht), ht__default_key_eq(ht),                       \
                    (void**)&(ht)->impl.values, sizeof(*(ht)->impl.values),                 \
                    &(ht)->impl.slots, &(ht)->impl.hashes,                                  \
                    &(ht)->impl.capacity, &(ht)->impl.filled_slots, &(ht)->count),          \
         (ht)->impl.temp_value =                                                            \
             ht__decltype_cast((ht)->impl.temp_value)                                       \
             ht__put_no_expand((ht)->impl.keys, sizeof(*(ht)->impl.keys),                   \
                               ht__default_key_hash(ht), ht__default_key_eq(ht),            \
                               (ht)->impl.values, sizeof(*(ht)->impl.values),               \
                               (ht)->impl.slots, (ht)->impl.hashes,                         \
                               (ht)->impl.capacity, &(ht)->impl.filled_slots, &(ht)->count, \
                               &(ht)->impl.temp_key)))

// void ht_delete(Hash_Table(Key, Value) *ht, Value *value)
//
// Delete the element by the pointer to its value slot. You can
// get the value pointer via ht_find() or ht_foreach(). NULL is
// a valid value pointer and will be simply ignored.
//
// ```c
// Hash_Table(const char *, int) ht = {0};
// ...
// int *count = ht_find(&ht, "foo");
// if (count) {
//     ht_delete(&ht, ht_find(&ht, "foo"));
//     printf("`foo` has been deleted!\n");
// } else {
//     printf("`foo` doesn't exist!\n");
// }
// ```
#define ht_delete(ht, value) \
    ht__delete((ht)->impl.values, sizeof(*(ht)->impl.values), (ht)->impl.slots, (ht)->impl.capacity, &(ht)->count, value)

// bool ht_find_and_delete(Hash_Table(Key, Value) *ht, Key key)
//
// Combines together ht_find() and ht_delete() enabling you to delete the elements
// by the keys. Returns true when the element was deleted, returns false when the
// element doesn’t exist
//
// ```c
// Hash_Table(const char *, int) ht = {0};
// ...
// if (ht_find_and_delete(&ht, "foo")) {
//     printf("`foo` has been deleted!\n");
// } else {
//     printf("`foo` doesn't exist!\n");
// }
// ```
#define ht_find_and_delete(ht, key) \
    (ht_find((ht), (key)), (ht)->impl.temp_value ? (ht_delete((ht), (ht)->impl.temp_value), true) : false)

// Key ht_key(Hash_Table(Key, Value) *ht, Value *value)
//
// Returns the key of the element by its value pointer. Useful in conjunction with ht_foreach()
#define ht_key(ht, value) (assert((ht)->impl.values <= (value)), (ht)->impl.keys[(value) - (ht)->impl.values])

// A foreach macro that iterates the values of the Hash Table.
//
// ```c
// Hash_Table(const char*, int) ht = {0};
// ht_foreach(int, value, &ht) {
//     printf("%s => %d\n", ht_key(value), *value);
// }
// ```
#define ht_foreach(Type, iter, ht)                               \
    for (Type *iter = NULL;                                      \
         ht__next((ht)->impl.slots, (ht)->impl.capacity,         \
                  (ht)->impl.values, sizeof(*(ht)->impl.values), \
                  (void**)&iter);)

// void ht_reset(Hash_Table(Key, Value) *ht)
//
// Removes all the elements from the hash table, but does not deallocate any memory, making the hash table
// ready to be reused again.
#define ht_reset(ht)                                                             \
    (memset((ht)->impl.slots, 0, sizeof(*(ht)->impl.slots)*(ht)->impl.capacity), \
     (ht)->impl.filled_slots = 0,                                                \
     (ht)->count = 0,                                                            \
     (void)0)

// void ht_free(Hash_Table(Key, Value) *ht)
//
// Deallocates all the memory associated with the hash table and completely resets its state.
#define ht_free(ht)                                     \
    (free((ht)->impl.keys),   (ht)->impl.keys   = NULL, \
     free((ht)->impl.values), (ht)->impl.values = NULL, \
     free((ht)->impl.slots),  (ht)->impl.slots  = NULL, \
     free((ht)->impl.hashes), (ht)->impl.hashes = NULL, \
     (ht)->impl.filled_slots = 0,                       \
     (ht)->count = 0,                                   \
     (ht)->impl.capacity = 0,                           \
     (void)0)

// The initial capacity of the Hash_Table. Always rounded up to the nearest
// power of two. You can redefine it.
#ifndef HT_INIT_CAP
#define HT_INIT_CAP 256
#endif // HT_INIT_CAP

#define HT_LOAD_FACTOR_PERCENT 70

// The default hash function. It's the hash function that is used by default
// throughout the library if your .key_hash is set to NULL. You can redefine it.
#ifndef ht_default_hash
#define ht_default_hash ht_djb2
#endif // ht_default_hash

// The default .key_hash and .key_eq implementation for C-strings.
// We use those when .key_hash and .key_eq are set to NULL and the
// Key is C stringy (char*, const char*, unsigned char*, or
// const unsigned char *).
uint32_t ht_default_cstr_key_hash(void const *key);
bool ht_default_cstr_key_eq(void const *a, void const *b);

// http://www.cse.yorku.ca/~oz/hash.html#djb2
uint32_t ht_djb2(void const *data, size_t size);

typedef enum {
    HT_EMPTY,
    HT_OCCUPIED,
    HT_DELETED,
} Ht_Slot;

// Private functions. Do not use directly. //////////////////////////////

static bool ht__next(Ht_Slot *ht_slots, size_t ht_capacity,
                     void *ht_values, size_t ht_value_size,
                     void **value);
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

#if defined(__cplusplus)
    #define ht__decltype_cast(expr) (decltype(expr))
#else
    #define ht__decltype_cast(...)
#endif // __cplusplus

#if defined(__cplusplus)
    template<typename T> Key_Hash ht__generic_key_hash()                       { return NULL;                     }
    template<>           Key_Hash ht__generic_key_hash<const char*>()          { return ht_default_cstr_key_hash; }
    template<>           Key_Hash ht__generic_key_hash<char*>()                { return ht_default_cstr_key_hash; }
    template<>           Key_Hash ht__generic_key_hash<const unsigned char*>() { return ht_default_cstr_key_hash; }
    template<>           Key_Hash ht__generic_key_hash<unsigned char*>()       { return ht_default_cstr_key_hash; }
    #define ht__default_key_hash(ht)                              \
        ((ht)->key_hash ?                                         \
         (ht)->key_hash :                                         \
         ht__generic_key_hash<decltype(*(ht)->impl.keys)>())
#else
    #define ht__default_key_hash(ht)                              \
        ((ht)->key_hash ?                                         \
         (ht)->key_hash :                                         \
         _Generic(*(ht)->impl.keys,                               \
                  char*:                ht_default_cstr_key_hash, \
                  const char*:          ht_default_cstr_key_hash, \
                  unsigned char*:       ht_default_cstr_key_hash, \
                  const unsigned char*: ht_default_cstr_key_hash, \
                  default:              (ht)->key_hash))
#endif // __cplusplus

#if defined(__cplusplus)
    template<typename T> Key_Eq ht__generic_key_eq()                       { return NULL;                   }
    template<>           Key_Eq ht__generic_key_eq<const char*>()          { return ht_default_cstr_key_eq; }
    template<>           Key_Eq ht__generic_key_eq<char*>()                { return ht_default_cstr_key_eq; }
    template<>           Key_Eq ht__generic_key_eq<const unsigned char*>() { return ht_default_cstr_key_eq; }
    template<>           Key_Eq ht__generic_key_eq<unsigned char*>()       { return ht_default_cstr_key_eq; }
    #define ht__default_key_eq(ht)                              \
        ((ht)->key_eq ?                                         \
         (ht)->key_eq :                                         \
         ht__generic_key_eq<decltype(*(ht)->impl.keys)>())
#else
    #define ht__default_key_eq(ht)                              \
        ((ht)->key_eq ?                                         \
         (ht)->key_eq :                                         \
         _Generic(*(ht)->impl.keys,                             \
                  char*:                ht_default_cstr_key_eq, \
                  const char*:          ht_default_cstr_key_eq, \
                  unsigned char*:       ht_default_cstr_key_eq, \
                  const unsigned char*: ht_default_cstr_key_eq, \
                  default:              (ht)->key_eq))
#endif // __cplusplus

#endif // HT_H_

#ifdef HT_IMPLEMENTATION

uint32_t ht_default_cstr_key_hash(void const* key)
{
    char const* const* key_typed = (char const* const*)key;
    return ht_default_hash(*key_typed, strlen(*key_typed));
}

bool ht_default_cstr_key_eq(void const* a, void const* b)
{
    char const* const* a_typed = (char const* const*)a;
    char const* const* b_typed = (char const* const*)b;
    return strcmp(*a_typed, *b_typed) == 0;
}

uint32_t ht_djb2(void const *data, size_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t hash = 5381;
    for (size_t i = 0; i < size; ++i) {
        hash += ((hash << 5) + hash) + (uint32_t)bytes[i];
    }
    return hash;
}

static bool ht__next(Ht_Slot *ht_slots, size_t ht_capacity,
                     void *ht_values, size_t ht_value_size,
                     void **value)
{
    assert(ht_value_size > 0);

    uint8_t *values = (uint8_t*)ht_values;

    if (*value == NULL) *value = ht_values;

    assert(ht_values <= *value);
    size_t index = ((uint8_t*)*value - values)/ht_value_size + 1;
    while (index < ht_capacity && ht_slots[index] != HT_OCCUPIED) {
        index += 1;
    }
    if (index >= ht_capacity) return false;
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

    uint8_t *keys   = (uint8_t*)ht_keys;
    uint8_t *values = (uint8_t*)ht_values;

    uint32_t hash = key_hash ? key_hash(key) : ht_default_hash(key, ht_key_size);
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
            new_ht_capacity = 1;
            while (new_ht_capacity < HT_INIT_CAP) {
                new_ht_capacity *= 2;
            }
        } else {
            new_ht_capacity = (*ht_capacity)*2;
        }
        size_t new_ht_filled_slots = 0;
        size_t new_ht_count = 0;

        void     *new_ht_keys   = calloc(new_ht_capacity, ht_key_size);
        void     *new_ht_values = calloc(new_ht_capacity, ht_value_size);
        Ht_Slot  *new_ht_slots  = (Ht_Slot*)calloc(new_ht_capacity, sizeof(*new_ht_slots));
        uint32_t *new_ht_hashes = (uint32_t*)calloc(new_ht_capacity, sizeof(*new_ht_hashes));

        assert(ht_key_size > 0);
        assert(ht_value_size > 0);

        uint8_t *keys   = (uint8_t*)*ht_keys;
        uint8_t *values = (uint8_t*)*ht_values;
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

    uint8_t *keys   = (uint8_t*)ht_keys;
    uint8_t *values = (uint8_t*)ht_values;

    uint32_t hash = key_hash ? key_hash(key) : ht_default_hash(key, ht_key_size);
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
