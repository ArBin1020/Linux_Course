#include "list.h"

#include <stdlib.h>
#include <stdio.h>

/* ── Pool ─────────────────────────────────────────────────────── */

void pool_init(list_pool_t *pool, size_t capacity) {
    pool->nodes = malloc(capacity * sizeof(struct list_node));
    if (!pool->nodes) {
        fprintf(stderr, "pool_init: malloc failed\n");
        exit(EXIT_FAILURE);
    }
    pool->capacity = capacity;
    pool->used     = 0;
}

void pool_reset(list_pool_t *pool) {
    pool->used = 0;
}

void pool_free(list_pool_t *pool) {
    free(pool->nodes);
    pool->nodes    = NULL;
    pool->capacity = 0;
    pool->used     = 0;
}

struct list_node *pool_alloc(list_pool_t *pool, int value) {
    if (pool->used >= pool->capacity) {
        fprintf(stderr, "pool_alloc: pool exhausted\n");
        exit(EXIT_FAILURE);
    }
    struct list_node *node = &pool->nodes[pool->used++];
    node->value = value;
    node->next  = NULL;
    return node;
}

/* ── Malloc ────────────────────────────────────────────── */

static struct list_node *create_node(int value) {
    struct list_node *node = malloc(sizeof(struct list_node));
    if (!node) {
        fprintf(stderr, "create_node: malloc failed\n");
        exit(EXIT_FAILURE);
    }
    node->value = value;
    node->next  = NULL;
    return node;
}

void list_append_unknown_tail(struct list_node **head, int value) {
    struct list_node *node = create_node(value);
    if (*head == NULL) { *head = node; return; }
    struct list_node *cur = *head;
    while (cur->next) cur = cur->next;
    cur->next = node;
}

void list_append_known_tail(struct list_node **head, struct list_node **tail, int value) {
    struct list_node *node = create_node(value);
    if (*head == NULL) { *head = node; *tail = node; return; }
    (*tail)->next = node;
    *tail         = node;
}

void list_free(struct list_node *head) {
    while (head) {
        struct list_node *tmp = head->next;
        free(head);
        head = tmp;
    }
}

/* ── Pool ─────────────────────────────────────────────── */

void list_append_unknown_tail_pool(struct list_node **head, int value, list_pool_t *pool) {
    struct list_node *node = pool_alloc(pool, value);
    if (*head == NULL) { *head = node; return; }
    struct list_node *cur = *head;
    while (cur->next) cur = cur->next;
    cur->next = node;
}

void list_append_known_tail_pool(struct list_node **head, struct list_node **tail,
                                 int value, list_pool_t *pool) {
    struct list_node *node = pool_alloc(pool, value);
    if (*head == NULL) { *head = node; *tail = node; return; }
    (*tail)->next = node;
    *tail         = node;
}

void list_head_prepend_pool(struct list_node **head, int value, list_pool_t *pool) {
    struct list_node *node = pool_alloc(pool, value);
    node->next = *head;
    *head      = node;
}

/* ── Scan ────────────────────────────────────────────────────── */

long long list_scan(struct list_node *head) {
    long long sum = 0;
    for (struct list_node *cur = head; cur; cur = cur->next) sum += cur->value;
    return sum;
}