#ifndef LIST_H
#define LIST_H

#include <stddef.h>

/* ── Node ─────────────────────────────────────────────────────── */
struct list_node {
    int               value;
    struct list_node *next;
};

/* ── Memory Pool ──────────────────────────────────────────────── */
typedef struct {
    struct list_node *nodes;
    size_t            capacity;
    size_t            used;
} list_pool_t;

void              pool_init(list_pool_t *pool, size_t capacity);
void              pool_reset(list_pool_t *pool);
void              pool_free(list_pool_t *pool);
struct list_node *pool_alloc(list_pool_t *pool, int value);

/* ── Malloc ────────────────────────────────────────────── */
void list_append_unknown_tail(struct list_node **head, int value);
void list_append_known_tail(struct list_node **head, struct list_node **tail, int value);
void list_free(struct list_node *head);

/* ── Pool ─────────────────────────────────────────────── */
void list_append_unknown_tail_pool(struct list_node **head, int value, list_pool_t *pool);
void list_append_known_tail_pool(struct list_node **head, struct list_node **tail,
                                 int value, list_pool_t *pool);
void list_head_prepend_pool(struct list_node **head, int value, list_pool_t *pool);

/* ── Scan ─────── */
long long list_scan(struct list_node *head);

#endif /* LIST_H */