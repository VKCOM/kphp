// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef KDB_COMMON_SMART_PTRS_TAGGED_PTR_H
#define KDB_COMMON_SMART_PTRS_TAGGED_PTR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TAGGED_PTR_PTR_MASK ((1ULL << 48ULL) - 1)
#define TAGGED_PTR_TAG_INDEX (3)

struct tagged_ptr {
  uintptr_t value;
};
typedef struct tagged_ptr tagged_ptr_t;

union tagged_ptr_cast_unit {
  uintptr_t value;
  uint16_t tag[4];
};
typedef union tagged_ptr_cast_unit tagged_ptr_cast_unit_t;

static inline void *tagged_ptr_get_ptr(const tagged_ptr_t *tagged_ptr) {
  return (void *)(tagged_ptr->value & TAGGED_PTR_PTR_MASK);
}

static inline uint16_t tagged_ptr_get_tag(const tagged_ptr_t *tagged_ptr) {
  tagged_ptr_cast_unit_t cast_unit;
  cast_unit.value = tagged_ptr->value;

  return cast_unit.tag[TAGGED_PTR_TAG_INDEX];
}

static inline uint16_t tagged_ptr_get_next_tag(const tagged_ptr_t *tagged_ptr) {
  return (uint16_t)((tagged_ptr_get_tag(tagged_ptr) + 1) & UINT16_MAX);
}

static inline void tagged_ptr_pack(tagged_ptr_t *tagged_ptr, void *ptr, uint16_t tag) {
  tagged_ptr_cast_unit_t cast_unit;
  cast_unit.value = (uintptr_t)ptr;
  cast_unit.tag[TAGGED_PTR_TAG_INDEX] = tag;
  tagged_ptr->value = cast_unit.value;
}

static inline void tagged_ptr_set_ptr(tagged_ptr_t *tagged_ptr, void *ptr) {
  const uint16_t tag = tagged_ptr_get_tag(tagged_ptr);
  tagged_ptr_pack(tagged_ptr, ptr, tag);
}

static inline void tagged_ptr_set_tag(tagged_ptr_t *tagged_ptr, uint16_t tag) {
  void *ptr = tagged_ptr_get_ptr(tagged_ptr);
  tagged_ptr_pack(tagged_ptr, ptr, tag);
}

static inline tagged_ptr_t tagged_ptr_from_uintptr(uintptr_t ptr) {
  tagged_ptr_t tagged_ptr;
  tagged_ptr.value = ptr;

  return tagged_ptr;
}

static inline uintptr_t tagged_ptr_to_uintptr(tagged_ptr_t *tagged_ptr) {
  return tagged_ptr->value;
}

static inline bool tagged_ptr_cas(tagged_ptr_t *tagged_ptr, uintptr_t old_value, uintptr_t new_value) {
  return __sync_bool_compare_and_swap(&tagged_ptr->value, old_value, new_value);
}

#endif // KDB_COMMON_SMART_PTRS_TAGGED_PTR_H
