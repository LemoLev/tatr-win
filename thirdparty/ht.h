#ifndef HT_H_
#define HT_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// The Hash Table.
//
// You can define it like this:
// ```c
// Ht(int, int) ht = {0}; // Zero-initialized Hash Table is a valid Hash Table!
// ```
//
// You should probably typedef it if you want to pass it to multiple places. Because anonymous structs
// are not particularly compatible with each other even if they have literally the same definition:
//
// ```c
// typedef Ht(const char*, int) Word_Count;
//
// void count_words(Word_Count *wc, const char *text);
// ```
#define Ht(Key, Value)                                           \
    struct {                                                     \
        /* .count - amount of unique items in the Hash Table. */ \
        size_t    count;                                         \
        /* .hasheq - key Hashing and Equality function.          \
         * NULL means direct byte representation of the key      \
         * will be hashed and compared.                          \
         */                                                      \
        Ht_Hasheq  hasheq;                                       \
        /* .impl_* - these fields are subject to change.         \
         * Do not access any impl_* fields if you just need      \
         * to iterate the Table. Use ht_foreach() instead.       \
         */                                                      \
        Key        *impl_keys;                                   \
        Value      *impl_values;                                 \
        uint32_t   *impl_hashes;                                 \
        Ht__Status *impl_status;                                 \
        size_t      impl_filled_slots;                           \
        size_t      impl_capacity;                               \
        /* .default_value - ht_put() and friends will use        \
         * .default_value as the default value for all the newly \
         * inserted values. Leave it empty for zero-initialized  \
         * values by default.                                    \
         */                                                      \
        Value       default_value;                               \
    }

// By default the Hash Table compares and hashes the keys by their byte representation. For the C string types,
// you must specify .hasheq. Ht_Hasheq is a function that combines together Hashing and Equality operations on the key.
// We provide default implementation for the C string types:
//
// ```c
// Ht(const char *, int) ht = {
//     .hasheq = ht_cstr_hasheq,
// };
// ```
//
// This covers 90% of the cases. If you need to treat your keys in a different way, create
// your custom Ht_Hasheq function:
//
// ```c
// typedef struct {
//     const char *data;
//     int count;
// } String_View;
//
// uint32_t sv_hasheq(Ht_Op op, void const* a_, void const* b_, size_t n);
//
// Ht(String_View, int) ht = {
//     .hasheq = sv_hasheq,
// };
//
// uint32_t sv_hasheq(Ht_Op op, void const* a_, void const* b_, size_t n)
// {
//     (void) n; // `n` is the size of the key in bytes. We pass it in case your hasheq
//               // doesn't know for some reason. ht_mem_hasheq for instance uses it.
//     String_View const* a = (String_View const*)a_;
//     String_View const* b = (String_View const*)b_;
//     switch (op) {
//     case HT_HASH: return ht_default_hash(a->data, a->count);
//     case HT_EQ:   return a->count == b->count && memcmp(*a, *b, a->count) = 0;
//     }
//     return 0;
// }
//
// ```
//
// To make it easy to remember to provide necessary .hasheq-s
// we recommend on top of typedef-ing your custom Hash Tables also defining global
// variables for their default values:
//
// ```c
// typedef Ht(const char*, int) Word_Count;
// static const Word_Count default_word_count = {
//     .hasheq = ht_cstr_hasheq,
// };
//
// Word_Count wc = default_word_count;
// ```
//
// If you need to specify .hasheq for an inner Ht, use the .default_value field:
//
// ```c
// Ht(const char *, Ht(String_View, int)) ht = {
//     .hasheq = ht_cstr_hasheq,
//     .default_value = {
//         .hasheq = ht_cstr_hasheq
//     },
// };
// ```
typedef enum {
    HT_HASH,
    HT_EQ,
} Ht_Op;
typedef uint32_t (*Ht_Hasheq)(Ht_Op op, void const *a, void const *b, size_t n);

// The default .hasheq implementation for C-strings.
uint32_t ht_cstr_hasheq(Ht_Op op, void const *a, void const *b, size_t n);
// The default .hasheq implementation for when .hasheq == NULL.
uint32_t ht_mem_hasheq(Ht_Op op, void const *a, void const *b, size_t n);

#ifdef NOB_H_
// If you are using nob.h we may automatically provide hasheq for its String_View
// type. We detect nob.h presence by NOB_H_ being defined. It is also checked under
// HT_IMPLEMENTATION. If you are compiling ht.h as a separate translation unit do
// not forget to defined NOB_H_ manually to make sure ht_sv_hasheq implementation
// is compiled:
// ```console
// $ gcc -DHT_IMPLEMENTATION -DNOB_H_ -x c -c ht.h
// ```
uint32_t ht_sv_hasheq(Ht_Op op, void const *a, void const *b, size_t n);
#endif // NOB_H_

// Value *ht_put(Ht(Key, Value) *ht, Key key)
//
// Puts a the key with the value initialized with ht->default_value.
// Returns the pointer to the inserted value.
//
// ```c
// Ht(const char *, int) ht = {
//     .hasheq = ht_cstr_hasheq
// };
// *ht_put(&ht, "foo") = 69;
// *ht_put(&ht, "bar") = 420;
// *ht_put(&ht, "baz") = 1337;
// ```
#if defined(__cplusplus)
    auto ht_put(auto *ht, auto key)
    {
        decltype(*ht->impl_keys) key_ = key;
        return (decltype(ht->impl_values))
            ht__put(
                ht,
                &key_,
                sizeof(*ht->impl_keys),
                sizeof(*ht->impl_values));
    }
#else
    #define ht_put(ht, key)                                \
        ((__typeof__((ht)->impl_values)) ht__put(          \
            (ht),                                          \
            (__typeof__(*(ht)->impl_keys)[]){key},         \
            sizeof(*(ht)->impl_keys),                      \
            sizeof(*(ht)->impl_values)                     \
        ))
#endif

// Value *ht_find(Ht(Key, Value) *ht, Key key)
//
// Tries to find a value by the key. If the value is found returns the pointer to the value,
// otherwise returns NULL.
//
// ```c
// int n = 5;
// const char *words[n] = {"foo", "bar", "foo", "baz", "aboba"};
// Ht(const char *, int) ht = { .hasheq = ht_cstr_hasheq };
// for (int i = 0; i < n; ++i) {
//     int *count = ht_find(&ht, words[i]);
//     if (count) {
//         *count += 1;
//     } else {
//         *ht_put(&ht, words[i]) = 1;
//     }
// }
// ```
#if defined(__cplusplus)
    auto ht_find(auto *ht, auto key)
    {
        decltype(*ht->impl_keys) key_ = key;
        return (decltype(ht->impl_values)) ht__find(
            ht,
            &key_,
            sizeof(*ht->impl_keys),
            sizeof(*ht->impl_values));
    }
#else
    #define ht_find(ht, key)                              \
        ((__typeof__((ht)->impl_values)) ht__find(        \
            (ht),                                         \
            (__typeof__(*(ht)->impl_keys)[]){key},        \
            sizeof(*(ht)->impl_keys),                     \
            sizeof(*(ht)->impl_values)                    \
        ))
#endif // __cplusplus

// Value *ht_find_or_put(Ht(Key, Value) *ht, Key key)
//
// Tries to find a value by the key, if not found inserts the key with the value initialized as ht->default_value.
// Never fails. Always returns either the pointer to the found value or the newly added value.
//
// ```c
// int n = 5;
// const char *words[n] = {"foo", "bar", "foo", "baz", "aboba"};
// Ht(const char *, int) ht = { .hasheq = ht_cstr_hasheq };
// for (int i = 0; i < n; ++i) {
//     *ht_find_or_put(&ht, words[i]) += 1;
// }
// ```
#if defined(__cplusplus)
    auto ht_find_or_put(auto *ht, auto key)
    {
        decltype(*ht->impl_keys) key_ = key;
        return (decltype(ht->impl_values)) ht__find_or_put(
            ht,
            &key_,
            sizeof(*ht->impl_keys),
            sizeof(*ht->impl_values));
    }
#else
    #define ht_find_or_put(ht, key)                       \
        ((__typeof__((ht)->impl_values)) ht__find_or_put( \
            (ht),                                         \
            (__typeof__(*(ht)->impl_keys)[]){key},        \
            sizeof(*(ht)->impl_keys),                     \
            sizeof(*(ht)->impl_values)                    \
        ))
#endif // __cplusplus

// void ht_delete(Ht(Key, Value) *ht, Value *value)
//
// Delete the element by the pointer to its value slot. You can
// get the value pointer via ht_find() or ht_foreach(). NULL is
// a valid value pointer and will be simply ignored.
//
// ```c
// Ht(const char *, int) ht = { .hasheq = ht_cstr_hasheq };
// ...
// int *count = ht_find(&ht, "foo");
// if (count) {
//     ht_delete(&ht, ht_find(&ht, "foo"));
//     printf("`foo` has been deleted!\n");
// } else {
//     printf("`foo` doesn't exist!\n");
// }
// ```
#define ht_delete(ht, value) ht__delete(ht, value, sizeof(*(ht)->impl_values))

// bool ht_find_and_delete(Ht(Key, Value) *ht, Key key)
//
// Combines together ht_find() and ht_delete() enabling you to delete the elements
// by the keys. Returns true when the element was deleted, returns false when the
// element doesn’t exist
//
// ```c
// Ht(const char *, int) ht = { .hasheq = ht_cstr_hasheq };
// ...
// if (ht_find_and_delete(&ht, "foo")) {
//     printf("`foo` has been deleted!\n");
// } else {
//     printf("`foo` doesn't exist!\n");
// }
// ```
#if defined(__cplusplus)
    bool ht_find_and_delete(auto *ht, auto key)
    {
        decltype(*ht->impl_keys) key_ = key;
        return ht__find_and_delete(
            ht,
            &key_,
            sizeof(*ht->impl_keys),
            sizeof(*ht->impl_values));
    }
#else
    #define ht_find_and_delete(ht, key)                \
        ht__find_and_delete(                           \
            (ht),                                      \
            (__typeof__(*(ht)->impl_keys)[]){key},     \
            sizeof(*(ht)->impl_keys),                  \
            sizeof(*(ht)->impl_values))
#endif // __cplusplus

// Key ht_key(Ht(Key, Value) *ht, Value *value)
//
// Returns the key of the element by its value pointer. Useful in conjunction with ht_foreach()
#if defined(__cplusplus)
    #define ht_key(ht, value) \
        (*(decltype((ht)->impl_keys))ht__key(ht, value, sizeof(*(ht)->impl_keys), sizeof(*(ht)->impl_values)))
#else
    #define ht_key(ht, value) \
        (*(__typeof__((ht)->impl_keys))ht__key(ht, value, sizeof(*(ht)->impl_keys), sizeof(*(ht)->impl_values)))
#endif // __cplusplus

// A foreach macro that iterates the values of the Hash Table.
//
// ```c
// Ht(const char *, int) ht = { .hasheq = ht_cstr_hasheq };
// ht_foreach(value, &ht) {
//     printf("%s => %d\n", ht_key(value), *value);
// }
// ```
#if defined(__cplusplus)
    #define ht_foreach(iter, ht)                                         \
        for (decltype((ht)->impl_values) iter = NULL;                  \
             ht__next((ht), sizeof(*(ht)->impl_values), (void **)&iter);)
#else
    #define ht_foreach(iter, ht)                                         \
        for (__typeof__((ht)->impl_values) iter = NULL;                  \
             ht__next((ht), sizeof(*(ht)->impl_values), (void **)&iter);)
#endif // __cplusplus

// void ht_reset(Ht(Key, Value) *ht)
//
// Removes all the elements from the hash table, but does not deallocate any memory, making the hash table
// ready to be reused again.
#define ht_reset ht__reset

// void ht_free(Ht(Key, Value) *ht)
//
// Deallocates all the memory associated with the hash table and completely resets its state.
#define ht_free ht__free

// The initial capacity of the Ht. Always rounded up to the nearest
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

// http://www.cse.yorku.ca/~oz/hash.html#djb2
uint32_t ht_djb2(void const *data, size_t size);

#if !defined(HT_ASSERT)
    #include <assert.h>
    #define HT_ASSERT assert
#endif

#if !defined(HT_FREE) && !defined(HT_CALLOC)
    #include <stdlib.h>
    #define HT_FREE   free
    #define HT_CALLOC calloc
#elif !defined(HT_FREE) || !defined(HT_CALLOC)
    #error "Both HT_FREE and HT_CALLOC must be defined together"
#endif

// Private names. Do not use directly. //////////////////////////////

typedef enum {
    HT__EMPTY,
    HT__OCCUPIED,
    HT__DELETED,
} Ht__Status;

typedef struct {
    size_t      count;
    Ht_Hasheq   hasheq;
    void       *impl_keys;
    void       *impl_values;
    uint32_t   *impl_hashes;
    Ht__Status *impl_status;
    size_t      impl_filled_slots;
    size_t      impl_capacity;
    uint8_t     default_value;
} Ht__Abstract;

static void *ht__put(void *ht, void *key, size_t key_size, size_t value_size);
static void *ht__find(void *ht, void *key, size_t key_size, size_t value_size);
static void *ht__find_or_put(void *ht, void *key, size_t key_size, size_t value_size);
static void ht__delete(void *ht, void *value, size_t value_size);
static bool ht__find_and_delete(void *ht, void *key, size_t key_size, size_t value_size);
static void *ht__key(void *ht, void *value, size_t key_size, size_t value_size);
static bool ht__next(void *ht, size_t value_size, void **value);
static void ht__reset(void *ht);
static void ht__free(void *ht);
static void *ht__put_no_expand(void *ht, void *key, size_t key_size, size_t value_size);
static void ht__expand(void *ht, size_t key_size, size_t value_size);
static size_t ht__strlen(const char *s);
static int ht__strcmp(const char *l, const char *r);
static void *ht__memcpy(void *dest, const void *src, size_t n);
static int ht__memcmp(const void *vl, const void *vr, size_t n);

#endif // HT_H_

#ifdef HT_IMPLEMENTATION

static void *ht__put(void *ht, void *key, size_t key_size, size_t value_size)
{
    ht__expand(ht, key_size, value_size);
    return ht__put_no_expand(ht, key, key_size, value_size);
}

static void *ht__find(void *ht, void *key, size_t key_size, size_t value_size)
{
    Ht__Abstract *aht = (Ht__Abstract*)ht;

    if (aht->impl_capacity == 0) return NULL;

    HT_ASSERT(key_size > 0);
    HT_ASSERT(value_size > 0);

    uint8_t *keys   = (uint8_t*)aht->impl_keys;
    uint8_t *values = (uint8_t*)aht->impl_values;

    Ht_Hasheq hasheq = aht->hasheq ? aht->hasheq : ht_mem_hasheq;
    uint32_t hash = hasheq(HT_HASH, key, NULL, key_size);
    uint32_t index = hash%aht->impl_capacity;
    uint32_t step = 1;
    while (
        aht->impl_status[index] == HT__DELETED ||
        (aht->impl_status[index] == HT__OCCUPIED &&
         !(aht->impl_hashes[index] == hash &&
           hasheq(HT_EQ, keys + index*key_size, key, key_size)))
    ) {
        index = (index + step)%aht->impl_capacity;
        step += 1;
    }

    if (aht->impl_status[index] == HT__OCCUPIED) {
        return values + index*value_size;
    }
    return NULL;
}

static void *ht__find_or_put(void *ht, void *key, size_t key_size, size_t value_size)
{
    void *value = ht__find(ht, key, key_size, value_size);
    if (value) return value;
    return ht__put(ht, key, key_size, value_size);
}

static void ht__delete(void *ht, void *value, size_t value_size)
{
    Ht__Abstract *aht = (Ht__Abstract*) ht;
    if (value == NULL) return;
    HT_ASSERT(value_size > 0);
    HT_ASSERT(aht->impl_values <= value);
    size_t index = ((uint8_t*)value - (uint8_t*)aht->impl_values)/value_size;
    HT_ASSERT(index < aht->impl_capacity);
    HT_ASSERT(aht->impl_status[index] == HT__OCCUPIED);
    aht->impl_status[index] = HT__DELETED;
    aht->count -= 1;
}

static bool ht__find_and_delete(void *ht, void *key, size_t key_size, size_t value_size)
{
    void *value = ht__find(ht, key, key_size, value_size);
    if (value == NULL) return false;
    ht__delete(ht, value, value_size);
    return true;
}

static void *ht__key(void *ht, void *value, size_t key_size, size_t value_size)
{
    Ht__Abstract *aht = (Ht__Abstract*) ht;
    HT_ASSERT(aht->impl_values <= value);
    size_t index = ((uint8_t*)value - (uint8_t*)aht->impl_values)/value_size;
    return (uint8_t*)aht->impl_keys + index*key_size;
}

static bool ht__next(void *ht, size_t value_size, void **value)
{
    Ht__Abstract *aht = (Ht__Abstract*) ht;

    HT_ASSERT(value_size > 0);

    uint8_t *values = (uint8_t*)aht->impl_values;

    if (*value == NULL) *value = aht->impl_values;

    HT_ASSERT(aht->impl_values <= *value);
    size_t index = ((uint8_t*)*value - values)/value_size + 1;
    while (index < aht->impl_capacity && aht->impl_status[index] != HT__OCCUPIED) {
        index += 1;
    }
    if (index >= aht->impl_capacity) return false;
    *value = values + index*value_size;
    return true;
}

static void ht__reset(void *ht)
{
    Ht__Abstract *aht = (Ht__Abstract*) ht;
    for (size_t i = 0; i < aht->impl_capacity; ++i) {
        aht->impl_status[i] = HT__EMPTY;
    }
    aht->impl_filled_slots = 0;
    aht->count = 0;
}

static void ht__free(void *ht)
{
    Ht__Abstract *aht = (Ht__Abstract*) ht;
    HT_FREE(aht->impl_keys);   aht->impl_keys   = NULL;
    HT_FREE(aht->impl_values); aht->impl_values = NULL;
    HT_FREE(aht->impl_status); aht->impl_status = NULL;
    HT_FREE(aht->impl_hashes); aht->impl_hashes = NULL;
    aht->impl_filled_slots = 0;
    aht->count = 0;
    aht->impl_capacity = 0;
}

uint32_t ht_cstr_hasheq(Ht_Op op, void const* a_, void const *b_, size_t n)
{
    (void) n; // not used
    char const* const* a = (char const* const*)a_;
    char const* const* b = (char const* const*)b_;
    switch (op) {
    case HT_HASH: return ht_default_hash(*a, ht__strlen(*a));
    case HT_EQ:   return ht__strcmp(*a, *b) == 0;
    }
    return 0;
}

uint32_t ht_mem_hasheq(Ht_Op op, void const* a_, void const *b_, size_t n)
{
    uint8_t const* a = (uint8_t const*)a_;
    uint8_t const* b = (uint8_t const*)b_;
    switch (op) {
    case HT_HASH: return ht_default_hash(a, n);
    case HT_EQ:   return ht__memcmp(a, b, n) == 0;
    }
    return 0;
}

#ifdef NOB_H_
uint32_t ht_sv_hasheq(Ht_Op op, void const *a_, void const *b_, size_t n)
{
    (void) n; // not used
    Nob_String_View const* a = (Nob_String_View const*)a_;
    Nob_String_View const* b = (Nob_String_View const*)b_;
    switch (op) {
    case HT_HASH: return ht_djb2(a->data, a->count);
    case HT_EQ:   return nob_sv_eq(*a, *b);
    }
    return 0;
}
#endif // NOB_H_

uint32_t ht_djb2(void const *data, size_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t hash = 5381;
    for (size_t i = 0; i < size; ++i) {
        hash += ((hash << 5) + hash) + (uint32_t)bytes[i];
    }
    return hash;
}

static void *ht__put_no_expand(void *ht, void *key, size_t key_size, size_t value_size)
{
    Ht__Abstract *aht = (Ht__Abstract*) ht;

    HT_ASSERT(key_size > 0);
    HT_ASSERT(value_size > 0);

    uint8_t *keys   = (uint8_t*)aht->impl_keys;
    uint8_t *values = (uint8_t*)aht->impl_values;

    Ht_Hasheq hasheq = aht->hasheq ? aht->hasheq : ht_mem_hasheq;
    uint32_t hash = hasheq(HT_HASH, key, NULL, key_size);
    uint32_t index = hash%aht->impl_capacity;
    uint32_t step = 1;
    while (
        aht->impl_status[index] == HT__OCCUPIED &&
        !(aht->impl_hashes[index] == hash &&
          hasheq(HT_EQ, keys + index*key_size, key, key_size))
     ) {
        index = (index + step)%aht->impl_capacity;
        step += 1;
    }
    if (aht->impl_status[index] != HT__OCCUPIED) {
        if (aht->impl_status[index] == HT__EMPTY) {
            aht->impl_filled_slots += 1;
        }
        aht->impl_status[index] = HT__OCCUPIED;
        ht__memcpy(keys + index*key_size, key, key_size);
        ht__memcpy(values + index*value_size, &aht->default_value, value_size);
        aht->impl_hashes[index] = hash;
        aht->count += 1;
    } else {
        ht__memcpy(values + index*value_size, &aht->default_value, value_size);
    }
    return values + index*value_size;
}

static void ht__expand(void *ht, size_t key_size, size_t value_size)
{
    Ht__Abstract *aht = (Ht__Abstract*) ht;
    if (aht->impl_capacity == 0 || aht->impl_filled_slots*100 >= HT_LOAD_FACTOR_PERCENT*aht->impl_capacity) {
        size_t      old_impl_capacity = aht->impl_capacity;
        void       *old_impl_keys     = aht->impl_keys;
        void       *old_impl_values   = aht->impl_values;
        Ht__Status *old_impl_status   = aht->impl_status;
        uint32_t   *old_impl_hashes   = aht->impl_hashes;

        aht->impl_capacity = 0;
        if (old_impl_capacity == 0) {
            aht->impl_capacity = 1;
            while (aht->impl_capacity < HT_INIT_CAP) {
                aht->impl_capacity <<= 1;
            }
        } else {
            aht->impl_capacity = old_impl_capacity << 1;
        }
        aht->impl_filled_slots = 0;
        aht->count             = 0;
        aht->impl_keys         = HT_CALLOC(aht->impl_capacity, key_size);
        aht->impl_values       = HT_CALLOC(aht->impl_capacity, value_size);
        aht->impl_status       = (Ht__Status*)HT_CALLOC(aht->impl_capacity, sizeof(*aht->impl_status));
        aht->impl_hashes       = (uint32_t*)HT_CALLOC(aht->impl_capacity, sizeof(*aht->impl_hashes));

        HT_ASSERT(key_size > 0);
        HT_ASSERT(value_size > 0);

        uint8_t *keys   = (uint8_t*)old_impl_keys;
        uint8_t *values = (uint8_t*)old_impl_values;
        for (size_t i = 0; i < old_impl_capacity; ++i) {
            if (old_impl_status[i] == HT__OCCUPIED) {
                void *slot = ht__put_no_expand(ht, keys + i*key_size, key_size, value_size);
                ht__memcpy(slot, values + i*value_size, value_size);
            }
        }

        HT_FREE(old_impl_keys);
        HT_FREE(old_impl_values);
        HT_FREE(old_impl_status);
        HT_FREE(old_impl_hashes);
    }
}

static size_t ht__strlen(const char *s)
{
    const char *a = s;
    for (; *s; s++);
    return s-a;
}

static int ht__strcmp(const char *l, const char *r)
{
    for (; *l==*r && *l; l++, r++);
    return *(uint8_t *)l - *(uint8_t *)r;
}

static void *ht__memcpy(void *dest, const void *src, size_t n)
{
    uint8_t *d = (uint8_t*)dest;
    const uint8_t *s = (const uint8_t *)src;
    for (; n; n--) *d++ = *s++;
    return dest;
}

static int ht__memcmp(const void *vl, const void *vr, size_t n)
{
    const uint8_t *l=(const uint8_t *)vl, *r=(const uint8_t *)vr;
    for (; n && *l == *r; n--, l++, r++);
    return n ? *l-*r : 0;
}

#endif // HT_IMPLEMENTATION
