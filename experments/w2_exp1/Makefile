CC = gcc
CFLAGS = -O2 -I../../include

SRC_BENCH = benchmark.c ../../src/list.c ../../src/array.c
TARGET_BENCH = bench

SRC_PERF = perf_benchmark.c ../../src/list.c ../../src/array.c ../../src/perf_benchmark.c
TARGET_PERF = perf_runner

RESULT = *.csv *.png

.PHONY: all bench perf_runner clean

all: $(TARGET_BENCH) $(TARGET_PERF)

$(TARGET_BENCH): $(SRC_BENCH)
    $(CC) $(CFLAGS) $(SRC_BENCH) -o $(TARGET_BENCH)

$(TARGET_PERF): $(SRC_PERF)
    $(CC) $(CFLAGS) $(SRC_PERF) -o $(TARGET_PERF)

clean:
    rm -f $(TARGET_BENCH) $(TARGET_PERF) $(RESULT)
