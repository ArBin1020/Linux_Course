#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node {
    int          value;
    struct node *next;
};

__attribute__((noinline))
static long long do_scan(struct node *head) {
    long long sum = 0;
    for (struct node *cur = head; cur; cur = cur->next)
        sum += cur->value;
    return sum;
}


__attribute__((noinline))
static void build_malloc_list(struct node **ptrs, long n) {
    for (long i = 0; i < n; i++) {
        ptrs[i]        = malloc(sizeof(struct node));
        ptrs[i]->value = (int)(i & 0x7fffffff);
        ptrs[i]->next  = (i > 0) ? ptrs[i - 1] : NULL;
    }
}

__attribute__((noinline))
static void free_malloc_list(struct node **ptrs, long n) {
    for (long i = 0; i < n; i++) free(ptrs[i]);
}

/* ── Exp 1：Allocation Strategy ──────────────────────────────── */
static void run_exp1(long n, int mode, int reps) {
    for (int r = 0; r < reps; r++) {

        if (mode == 0) {
            /* Mode 0 — Per-node malloc */
            struct node **ptrs = malloc((size_t)n * sizeof(struct node *));
            build_malloc_list(ptrs, n);
            free_malloc_list(ptrs, n);
            free(ptrs);

        } else {
            /* Mode 1 — Memory Pool（單次 malloc + 循序寫入） */
            struct node *pool = malloc((size_t)n * sizeof(struct node));
            for (long i = 0; i < n; i++) {
                pool[i].value = (int)(i & 0x7fffffff);
                pool[i].next  = (i < n - 1) ? &pool[i + 1] : NULL;
            }
            free(pool);
        }
    }
}

/* ── Exp 2：Spatial Locality Scan ────────────────────────────── */
static void run_exp2(long n, int mode, int reps) {
    struct node *pool = malloc((size_t)n * sizeof(struct node));
    for (long i = 0; i < n; i++) {
        pool[i].value = (int)(i & 0x7fffffff);
        pool[i].next  = NULL;
    }

    struct node *head;

    if (mode == 0) {
        /* Mode 0 — Sequential Pool Scan */
        for (long i = 0; i < n - 1; i++)
            pool[i].next = &pool[i + 1];
        pool[n - 1].next = NULL;
        head = &pool[0];

    } else {
        /* Mode 1 — Shuffled Pool Scan */
        long *idx = malloc((size_t)n * sizeof(long));
        for (long i = 0; i < n; i++) idx[i] = i;
        srand(2026);
        for (long i = n - 1; i > 0; i--) {
            long k = (long)rand() % (i + 1);
            long t = idx[i]; idx[i] = idx[k]; idx[k] = t;
        }
        for (long i = 0; i < n - 1; i++)
            pool[idx[i]].next = &pool[idx[i + 1]];
        pool[idx[n - 1]].next = NULL;
        head = &pool[idx[0]];
        free(idx);
    }

    volatile long long sink = 0;
    for (int r = 0; r < reps; r++)
        sink += do_scan(head);

    free(pool);
    (void)sink;
}

int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr,
                "Usage: %s <exp: 1|2> <N> <mode: 0|1> <reps>\n"
                "  exp=1: Allocation  (mode 0=malloc, 1=pool)\n"
                "  exp=2: Scan        (mode 0=sequential, 1=shuffled)\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    int  exp  = atoi(argv[1]);
    long n    = atol(argv[2]);
    int  mode = atoi(argv[3]);
    int  reps = atoi(argv[4]);

    if      (exp == 1) run_exp1(n, mode, reps);
    else if (exp == 2) run_exp2(n, mode, reps);
    else {
        fprintf(stderr, "Unknown exp: %d\n", exp);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}