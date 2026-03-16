CC = gcc
CFLAGS = -O2 -I../../include
SRC = benchmark.c ../../src/list.c ../../src/array.c
TARGET = bench
RESULT = *.csv *.png

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

.PHONY: clean

clean:
	rm -f $(TARGET) $(RESULT)