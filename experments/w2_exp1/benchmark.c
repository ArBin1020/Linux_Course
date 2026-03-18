#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>

#include "list.h"

/* ================================================================
 *  Timing
 * ================================================================ */
static inline uint64_t ns_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

/* ============================
 *  Median of REPS doubles
 * ============================ */
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

static void sub1_alloc(void) {
    puts("[SUB1] Allocation Strategy  (1e4 / 1e6 / 1e8)");

    static const long sizes[] = {10000L, 100000L, 1000000L, 10000000L, 100000000L};
    const int nsz = (int)(sizeof(sizes) / sizeof(sizes[0]));

    FILE *f = fopen("exp2_sub1_alloc.csv", "w");
    if (!f) { perror("fopen sub1"); return; }
    fprintf(f, "N,"
               "Malloc_Total_ns,Malloc_Avg_ns,"
               "Pool_Total_ns,Pool_Avg_ns\n");

    for (int si = 0; si < nsz; si++) {
        long n = sizes[si];
        double tm_total[REPS], tm_avg[REPS];
        double tp_total[REPS], tp_avg[REPS];
        volatile long long sink = 0;

        for (int r = 0; r < REPS; r++) {
            uint64_t t0, t1, elapsed;

            /* M1 — per-node malloc */
            {
                struct list_node *head = NULL, *tail = NULL;
                t0 = ns_now();
                for (long j = 0; j < n; j++)
                    list_append_known_tail(&head, &tail, (int)(j & 0x7fffffff));
                t1 = ns_now();
                elapsed      = t1 - t0;
                tm_total[r]  = (double)elapsed;
                tm_avg[r]    = (double)elapsed / (double)n;
                sink        += head ? head->value : 0;
                list_free(head);
            }

            /* M2 — memory pool */
            {
                list_pool_t pool;
                pool_init(&pool, (size_t)n);
                struct list_node *head = NULL, *tail = NULL;
                t0 = ns_now();
                for (long j = 0; j < n; j++)
                    list_append_known_tail_pool(&head, &tail,
                                               (int)(j & 0x7fffffff), &pool);
                t1 = ns_now();
                elapsed      = t1 - t0;
                tp_total[r]  = (double)elapsed;
                tp_avg[r]    = (double)elapsed / (double)n;
                sink        += head ? head->value : 0;
                pool_free(&pool);
            }
        }

        fprintf(f, "%ld,%.0f,%.3f,%.0f,%.3f\n",
                n,
                median(tm_total), median(tm_avg),
                median(tp_total), median(tp_avg));
        printf("  SUB1  N=%-12ld  malloc_avg=%.1f ns  pool_avg=%.1f ns\n",
               n, median(tm_avg), median(tp_avg));
        (void)sink;
    }

    fclose(f);
    puts("  -> exp2_sub1_alloc.csv\n");
}

/* =========================================
 *  SUB-EXP 2 — Spatial Locality Effect
 * ========================================= */
static void sub2_locality(void) {
    puts("[SUB2] Spatial Locality  (1e4 ~ 1e7)");

    static const long sizes[] = {10000L, 100000L, 1000000L, 10000000L, 100000000L};
    const int nsz = (int)(sizeof(sizes) / sizeof(sizes[0]));

    FILE *f = fopen("exp2_sub2_locality.csv", "w");
    if (!f) { perror("fopen sub2"); return; }
    fprintf(f, "N,Sequential_Pool_Scan_ns,Shuffled_Pool_Scan_ns\n");

    srand(20250315);   /* 種子 */

    for (int si = 0; si < nsz; si++) {
        long n = sizes[si];
        double ts[REPS], tu[REPS];
        volatile long long sink = 0;

        /* Pool 只配置一次 兩種 scan 都在同一個記憶體上操作 */
        list_pool_t pool;
        pool_init(&pool, (size_t)n);
        for (long j = 0; j < n; j++) {
            pool.nodes[j].value = (int)(j & 0x7fffffff);
            pool.nodes[j].next  = NULL;
        }
        pool.used = (size_t)n;

        /* 索引洗牌陣列（不計時） */
        long *idx = malloc((size_t)n * sizeof(long));
        if (!idx) { perror("malloc idx"); exit(1); }

        for (int r = 0; r < REPS; r++) {
            uint64_t t0, t1;

            /* M1 — Sequential */
            for (long j = 0; j < n - 1; j++)
                pool.nodes[j].next = &pool.nodes[j + 1];
            pool.nodes[n - 1].next = NULL;

            t0 = ns_now();
            sink += list_scan(&pool.nodes[0]);
            t1 = ns_now();
            ts[r] = (double)(t1 - t0) / (double)n;

            /* M2 — Shuffled：Fisher-Yates 洗牌後重建 next 指標 */
            for (long j = 0; j < n; j++) idx[j] = j;
            for (long j = n - 1; j > 0; j--) {
                long k   = (long)rand() % (j + 1);
                long tmp = idx[j]; idx[j] = idx[k]; idx[k] = tmp;
            }
            for (long j = 0; j < n - 1; j++)
                pool.nodes[idx[j]].next = &pool.nodes[idx[j + 1]];
            pool.nodes[idx[n - 1]].next = NULL;

            t0 = ns_now();
            sink += list_scan(&pool.nodes[idx[0]]);
            t1 = ns_now();
            tu[r] = (double)(t1 - t0) / (double)n;
        }

        free(idx);
        pool_free(&pool);

        fprintf(f, "%ld,%.3f,%.3f\n", n, median(ts), median(tu));
        printf("  SUB2  N=%-10ld  sequential=%.1f ns  shuffled=%.1f ns\n",
               n, median(ts), median(tu));
        (void)sink;
    }

    fclose(f);
    puts("  -> exp2_sub2_locality.csv\n");
}

int main(void) {
    puts("=== w1_exp2: Allocation Strategy & Spatial Locality ===\n");
    sub1_alloc();
    sub2_locality();
    puts("=== Done ===");
    return EXIT_SUCCESS;
}