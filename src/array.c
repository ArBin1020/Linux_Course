#include "array.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void array_init(dynamic_array_t *arr, size_t initial_capacity) {
    arr->data     = malloc(initial_capacity * sizeof(int));
    arr->size     = 0;
    arr->capacity = initial_capacity;
    if (!arr->data) {
        fprintf(stderr, "array_init: malloc failed\n");
        exit(EXIT_FAILURE);
    }
}

static void array_grow(dynamic_array_t *arr) {
    arr->capacity *= 2;
    arr->data = realloc(arr->data, arr->capacity * sizeof(int));
    if (!arr->data) {
        fprintf(stderr, "array_grow: realloc failed\n");
        exit(EXIT_FAILURE);
    }
}

void array_append(dynamic_array_t *arr, int value) {
    if (arr->size >= arr->capacity) array_grow(arr);
    arr->data[arr->size++] = value;
}

void array_front_insert(dynamic_array_t *arr, int value) {
    if (arr->size >= arr->capacity) array_grow(arr);
    /* Shift all existing elements one position to the right. */
    memmove(arr->data + 1, arr->data, arr->size * sizeof(int));
    arr->data[0] = value;
    arr->size++;
}

long long array_scan(const dynamic_array_t *arr) {
    long long sum = 0;
    for (size_t i = 0; i < arr->size; i++) sum += arr->data[i];
    return sum;
}

void array_free(dynamic_array_t *arr) {
    free(arr->data);
    arr->data     = NULL;
    arr->size     = 0;
    arr->capacity = 0;
}