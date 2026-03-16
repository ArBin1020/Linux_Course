#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>

#include "array.h"
#include "list.h"

/* ================================================================
 *  Timing
 * ================================================================ */
static inline uint64_t ns_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

/* ================================================================
 *  Helpers
 * ================================================================ */
static int cmp_u64(const void *a, const void *b) {
    uint64_t x = *(const uint64_t *)a;
    uint64_t y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}

/*
 * Return the median of REPS doubles via insertion sort.
 * REPS is expected to be small (≤ 7).
 */
#define REPS 5
static double median(double v[REPS]) {
    double s[REPS];
    memcpy(s, v, REPS * sizeof(double));
    for (int i = 1; i < REPS; i++) {
        double key = s[i];
        int j = i - 1;
        while (j >= 0 && s[j] > key) { s[j + 1] = s[j]; j--; }
        s[j + 1] = key;
    }
    return s[REPS / 2];
}

static uint64_t percentile(uint64_t *sorted, int n, double p) {
    int idx = (int)(p / 100.0 * n);
    if (idx >= n) idx = n - 1;
    return sorted[idx];
}

/* ================================================================
 *  EXP 1 — Tail Append & System Overhead
 * ================================================================ */
static void exp1_run(void) {
    puts("[EXP1] Tail Append & System Overhead");

    static const int sizes[] = {1000, 5000, 10000, 20000, 50000};
    const int        nsz     = (int)(sizeof(sizes) / sizeof(sizes[0]));

    FILE *f = fopen("exp1_tail_append.csv", "w");
    if (!f) { perror("fopen exp1"); return; }
    fprintf(f, "N,LL_Unknown_Malloc,LL_Known_Malloc,"
               "LL_Unknown_Pool,LL_Known_Pool,Array_Append\n");

    for (int si = 0; si < nsz; si++) {
        int    n = sizes[si];
        double t[5][REPS];
        volatile long long sink = 0;

        for (int r = 0; r < REPS; r++) {
            uint64_t t0, t1;

            /* M1 — LL Unknown Tail + Malloc (O(N) traversal + syscall) */
            {
                struct list_node *head = NULL;
                t0 = ns_now();
                for (int j = 0; j < n; j++) list_append_unknown_tail(&head, j);
                t1 = ns_now();
                t[0][r] = (double)(t1 - t0) / n;
                sink   += head ? head->value : 0;
                list_free(head);
            }

            /* M2 — LL Known Tail + Malloc (O(1) insert + syscall) */
            {
                struct list_node *head = NULL, *tail = NULL;
                t0 = ns_now();
                for (int j = 0; j < n; j++) list_append_known_tail(&head, &tail, j);
                t1 = ns_now();
                t[1][r] = (double)(t1 - t0) / n;
                sink   += head ? head->value : 0;
                list_free(head);
            }

            /* M3 — LL Unknown Tail + Pool (O(N) traversal, no syscall) */
            {
                list_pool_t pool;
                pool_init(&pool, n);
                struct list_node *head = NULL;
                t0 = ns_now();
                for (int j = 0; j < n; j++) list_append_unknown_tail_pool(&head, j, &pool);
                t1 = ns_now();
                t[2][r] = (double)(t1 - t0) / n;
                sink   += head ? head->value : 0;
                pool_free(&pool);
            }

            /* M4 — LL Known Tail + Pool (O(1) insert, no syscall) */
            {
                list_pool_t       pool;
                pool_init(&pool, n);
                struct list_node *head = NULL, *tail = NULL;
                t0 = ns_now();
                for (int j = 0; j < n; j++) list_append_known_tail_pool(&head, &tail, j, &pool);
                t1 = ns_now();
                t[3][r] = (double)(t1 - t0) / n;
                sink   += head ? head->value : 0;
                pool_free(&pool);
            }

            /* M5 — Array Append (amortised O(1)) */
            {
                dynamic_array_t arr;
                array_init(&arr, 16);
                t0 = ns_now();
                for (int j = 0; j < n; j++) array_append(&arr, j);
                t1 = ns_now();
                t[4][r] = (double)(t1 - t0) / n;
                sink   += (long long)arr.size;
                array_free(&arr);
            }
        }

        fprintf(f, "%d,%.3f,%.3f,%.3f,%.3f,%.3f\n",
                n,
                median(t[0]), median(t[1]), median(t[2]),
                median(t[3]), median(t[4]));
        printf("  EXP1  N=%-8d done\n", n);
        (void)sink;
    }

    fclose(f);
    puts("  -> exp1_tail_append.csv\n");
}

/* ================================================================
 *  EXP 2 — Worst-case Insertion & Memmove
 * ================================================================ */
static void exp2_run(void) {
    puts("[EXP2] Worst-case Insertion & Memmove");

    static const int sizes[] = {1000, 5000, 10000, 20000, 50000};
    const int        nsz     = (int)(sizeof(sizes) / sizeof(sizes[0]));

    FILE *f = fopen("exp2_worst_insert.csv", "w");
    if (!f) { perror("fopen exp2"); return; }
    fprintf(f, "N,Array_Front_Insert,LL_Head_Prepend_Pool\n");

    for (int si = 0; si < nsz; si++) {
        int    n = sizes[si];
        double t[2][REPS];
        volatile long long sink = 0;

        for (int r = 0; r < REPS; r++) {
            uint64_t t0, t1;

            /* M1 — Array Front Insert  (O(N) memmove per call) */
            {
                dynamic_array_t arr;
                array_init(&arr, n);   /* pre-allocate: no realloc during test */
                t0 = ns_now();
                for (int j = 0; j < n; j++) array_front_insert(&arr, j);
                t1 = ns_now();
                t[0][r] = (double)(t1 - t0) / n;
                sink   += (long long)arr.size;
                array_free(&arr);
            }

            /* M2 — LL Head Prepend + Pool  (O(1) pointer swap) */
            {
                list_pool_t       pool;
                pool_init(&pool, n);
                struct list_node *head = NULL;
                t0 = ns_now();
                for (int j = 0; j < n; j++) list_head_prepend_pool(&head, j, &pool);
                t1 = ns_now();
                t[1][r] = (double)(t1 - t0) / n;
                sink   += head ? head->value : 0;
                pool_free(&pool);
            }
        }

        fprintf(f, "%d,%.3f,%.3f\n", n, median(t[0]), median(t[1]));
        printf("  EXP2  N=%-8d done\n", n);
        (void)sink;
    }

    fclose(f);
    puts("  -> exp2_worst_insert.csv\n");
}

/* ================================================================
 *  EXP 3 — Tail Latency & Real-time Stability
 * ================================================================ */
#define EXP3_N          200000
#define EXP3_SERIES_LEN 5000

static void exp3_run(void) {
    puts("[EXP3] Tail Latency & Real-time Stability");

    uint64_t *lat_arr  = malloc(EXP3_N * sizeof(uint64_t));
    uint64_t *lat_pool = malloc(EXP3_N * sizeof(uint64_t));
    if (!lat_arr || !lat_pool) { perror("malloc exp3"); return; }

    /* --- Warm-up (avoids cold-start artefacts on first measurement) --- */
    {
        dynamic_array_t arr; array_init(&arr, 16);
        for (int j = 0; j < 8192; j++) array_append(&arr, j);
        array_free(&arr);
    }

    /* M1 — Array Append, individual timings */
    {
        dynamic_array_t arr;
        array_init(&arr, 16);
        for (int j = 0; j < EXP3_N; j++) {
            uint64_t t0    = ns_now();
            array_append(&arr, j);
            lat_arr[j] = ns_now() - t0;
        }
        array_free(&arr);
    }
    puts("  EXP3  Array done");

    /* M2 — LL Known Tail + Pool, individual timings */
    {
        list_pool_t       pool;
        pool_init(&pool, EXP3_N);
        struct list_node *head = NULL, *tail = NULL;
        for (int j = 0; j < EXP3_N; j++) {
            uint64_t t0     = ns_now();
            list_append_known_tail_pool(&head, &tail, j, &pool);
            lat_pool[j] = ns_now() - t0;
        }
        pool_free(&pool);
    }
    puts("  EXP3  Pool-LL done");

    /* --- Compute percentiles --- */
    uint64_t *sarr  = malloc(EXP3_N * sizeof(uint64_t));
    uint64_t *spool = malloc(EXP3_N * sizeof(uint64_t));
    memcpy(sarr,  lat_arr,  EXP3_N * sizeof(uint64_t));
    memcpy(spool, lat_pool, EXP3_N * sizeof(uint64_t));
    qsort(sarr,  EXP3_N, sizeof(uint64_t), cmp_u64);
    qsort(spool, EXP3_N, sizeof(uint64_t), cmp_u64);

    FILE *fs = fopen("exp3_latency_stats.csv", "w");
    if (!fs) { perror("fopen exp3_stats"); goto cleanup; }
    fprintf(fs, "Method,P50_ns,P90_ns,P95_ns,P99_ns,P999_ns,Max_ns\n");
    fprintf(fs, "Array_Append,%"PRIu64",%"PRIu64",%"PRIu64",%"PRIu64",%"PRIu64",%"PRIu64"\n",
            percentile(sarr,  EXP3_N, 50.0),
            percentile(sarr,  EXP3_N, 90.0),
            percentile(sarr,  EXP3_N, 95.0),
            percentile(sarr,  EXP3_N, 99.0),
            percentile(sarr,  EXP3_N, 99.9),
            sarr[EXP3_N - 1]);
    fprintf(fs, "LL_Known_Pool,%"PRIu64",%"PRIu64",%"PRIu64",%"PRIu64",%"PRIu64",%"PRIu64"\n",
            percentile(spool, EXP3_N, 50.0),
            percentile(spool, EXP3_N, 90.0),
            percentile(spool, EXP3_N, 95.0),
            percentile(spool, EXP3_N, 99.0),
            percentile(spool, EXP3_N, 99.9),
            spool[EXP3_N - 1]);
    fclose(fs);
    puts("  -> exp3_latency_stats.csv");

    /* --- Save time-series (first SERIES_LEN insertions) --- */
    FILE *fr = fopen("exp3_latency_series.csv", "w");
    if (!fr) { perror("fopen exp3_series"); goto cleanup; }
    fprintf(fr, "Index,Array_ns,PoolLL_ns\n");
    for (int j = 0; j < EXP3_SERIES_LEN; j++)
        fprintf(fr, "%d,%"PRIu64",%"PRIu64"\n", j, lat_arr[j], lat_pool[j]);
    fclose(fr);
    puts("  -> exp3_latency_series.csv\n");

cleanup:
    free(sarr); free(spool);
    free(lat_arr); free(lat_pool);
}

/* ================================================================
 *  EXP 4 — Sequential Scan & Cache Locality
 * ================================================================ */
static void exp4_run(void) {
    puts("[EXP4] Sequential Scan & Cache Locality");

    static const int sizes[] = {10000, 50000, 100000, 500000};
    const int        nsz     = (int)(sizeof(sizes) / sizeof(sizes[0]));

    FILE *f = fopen("exp4_cache_scan.csv", "w");
    if (!f) { perror("fopen exp4"); return; }
    fprintf(f, "N,Array_Scan,Fragmented_LL_Scan,Continuous_LL_Scan\n");

    srand(42); /* reproducible shuffle */

    for (int si = 0; si < nsz; si++) {
        int    n = sizes[si];
        double t[3][REPS];
        volatile long long sink = 0;

        for (int r = 0; r < REPS; r++) {
            uint64_t t0, t1;

            /* M1 — Array Scan */
            {
                dynamic_array_t arr;
                array_init(&arr, n);
                for (int j = 0; j < n; j++) array_append(&arr, j);
                t0 = ns_now();
                sink += array_scan(&arr);
                t1 = ns_now();
                t[0][r] = (double)(t1 - t0) / n;
                array_free(&arr);
            }

            /* M2 — Fragmented LL Scan */
            {
                struct list_node **ptrs = malloc(n * sizeof(struct list_node *));
                if (!ptrs) { perror("malloc ptrs"); exit(1); }
                for (int j = 0; j < n; j++) {
                    ptrs[j]        = malloc(sizeof(struct list_node));
                    ptrs[j]->value = j;
                    ptrs[j]->next  = NULL;
                }
                /* Fisher-Yates shuffle */
                for (int j = n - 1; j > 0; j--) {
                    int k              = rand() % (j + 1);
                    struct list_node *tmp = ptrs[j];
                    ptrs[j] = ptrs[k];
                    ptrs[k] = tmp;
                }
                for (int j = 0; j < n - 1; j++) ptrs[j]->next = ptrs[j + 1];
                ptrs[n - 1]->next = NULL;
                struct list_node *head = ptrs[0];
                free(ptrs);

                t0 = ns_now();
                sink += list_scan(head);
                t1 = ns_now();
                t[1][r] = (double)(t1 - t0) / n;

                list_free(head);
            }

            /* M3 — Continuous LL Scan (pool, sequential addresses) */
            {
                list_pool_t       pool;
                pool_init(&pool, n);
                struct list_node *head = NULL, *tail = NULL;
                for (int j = 0; j < n; j++)
                    list_append_known_tail_pool(&head, &tail, j, &pool);

                t0 = ns_now();
                sink += list_scan(head);
                t1 = ns_now();
                t[2][r] = (double)(t1 - t0) / n;

                pool_free(&pool);
            }
        }

        fprintf(f, "%d,%.3f,%.3f,%.3f\n",
                n, median(t[0]), median(t[1]), median(t[2]));
        printf("  EXP4  N=%-8d done\n", n);
        (void)sink;
    }

    fclose(f);
    puts("  -> exp4_cache_scan.csv\n");
}

int main(void) {
    puts("=== Array vs Linked List — Benchmark Suite ===\n");
    exp1_run();
    exp2_run();
    exp3_run();
    exp4_run();
    puts("=== All experiments complete ===");
    return EXIT_SUCCESS;
}