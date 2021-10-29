#include "vector.h"

#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

double default_value = 0;

const char *vectorlib_version = "0.4.2";

void vector2_abs(struct Vector2 *vec) {
  vec->x = fabs(vec->x);
  vec->y = fabs(vec->y);
}

void vector2f_abs(struct Vector2f *vec) {
  vec->x = fabs(vec->x);
  vec->y = fabs(vec->y);
}

void vector2_array_fill(struct Vector2Array *array, ...) {
  va_list args;
  va_start(args, array);

  for (int i = 0; i < array->len; i++) {
    array->data[i].x = va_arg(args, double);
    array->data[i].y = va_arg(args, double);
  }

  va_end(args);
}

void vector2_array_set(struct Vector2Array *array, int index, struct Vector2 val) {
  array->data[index] = val;
}

struct Vector2 vector2_array_get(struct Vector2Array *array, int index) {
  return array->data[index];
}

void vector2_array_init(struct Vector2Array *array, int size, ...) {
  va_list args;
  va_start(args, size);

  array->data = calloc(size, sizeof(struct Vector2));
  array->len = size;
  for (int i = 0; i < size; i++) {
    array->data[i] = va_arg(args, struct Vector2);
  }

  va_end(args);
}

void vector2_array_init_ptr(struct Vector2Array *array, int size, ...) {
  va_list args;
  va_start(args, size);

  array->data = calloc(size, sizeof(struct Vector2));
  array->len = size;
  for (int i = 0; i < size; i++) {
    struct Vector2 *ptr = va_arg(args, struct Vector2*);
    array->data[i] = *ptr;
  }

  va_end(args);
}

void vector2_array_alloc(struct Vector2Array *array, int size) {
  array->data = calloc(size, sizeof(struct Vector2));
  array->len = size;
  if (default_value != 0) {
    for (int i = 0; i < size; i++) {
      array->data[i].x = default_value;
      array->data[i].y = default_value;
    }
  }
}

void vector2_array_free(struct Vector2Array *array) {
  free(array->data);
}

struct Vector2 *vector2_new_data(int size) {
  return calloc(size, sizeof(struct Vector2));
}

void vector2_data_free(struct Vector2 *data) {
  free(data);
}
