#include <stdint.h>
#include <string.h>

#include "pointers.h"

static void *unmanaged_data;

SomeData *nullptr_data(void) {
  return (SomeData*)0;
}

void uint64_memset(uint64_t* ptr, uint8_t v) {
  memset(ptr, v, 8);
}

uint64_t intptr_addr_value(int *ptr) {
  return (uint64_t)(ptr);
}

uint64_t voidptr_addr_value(void *ptr) {
  return (uint64_t)(ptr);
}

void write_int64(int64_t *dst, int64_t value) {
  *dst = value;
}

int64_t read_int64(const int64_t *ptr) {
  return *ptr;
}

int64_t read_int64_from_void(const void *ptr) {
  return *(const int64_t*)ptr;
}

int64_t* read_int64p(int64_t **ptr) {
  return *ptr;
}

uint8_t bytes_array_get(uint8_t *arr, int offset) {
  return arr[offset];
}

void bytes_array_set(uint8_t *arr, int offset, uint8_t val) {
  arr[offset] = val;
}

void* ptr_to_void(void *ptr) {
  return ptr;
}

const void* ptr_to_const_void(const void *ptr) {
  return ptr;
}

int void_strlen(const void *s) {
  return strlen(s);
}

void cstr_out_param(const char **out) {
  *out = "example result";
}

void set_void_ptr(void **dst, void *ptr) {
  *dst = ptr;
}

int strlen_safe(const char *s) {
  if (s == NULL) {
    return 0;
  }
  return strlen(s);
}

const char* nullptr_cstr() {
  return NULL;
}

const char* empty_cstr() {
  return "";
}

void set_unmanaged_data(void *data) {
  unmanaged_data = data;
}

void *get_unmanaged_data() {
  return unmanaged_data;
}

uint64_t alloc_callback_test(void* (*alloc_func) (int)) {
  void *mem = alloc_func(8);
  uint64_t *as_uint = (uint64_t*)(mem);
  *as_uint = 0xffff;
  return *as_uint;
}

uint64_t alloc_callback_test2(AllocFunc f) {
  return alloc_callback_test(f);
}
