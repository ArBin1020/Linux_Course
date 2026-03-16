#ifndef ARRAY_H
#define ARRAY_H

#include <stddef.h>

typedef struct {
    int    *data;
    size_t  size;
    size_t  capacity;
} dynamic_array_t;

void      array_init(dynamic_array_t *arr, size_t initial_capacity);
void      array_append(dynamic_array_t *arr, int value);
void      array_front_insert(dynamic_array_t *arr, int value);
long long array_scan(const dynamic_array_t *arr);
void      array_free(dynamic_array_t *arr);

#endif /* ARRAY_H */