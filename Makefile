CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./src -I./include

TARGET = bptree
TEST_TARGET = test_bptree
BENCH_TARGET = bench_search

SQL_SRCS = src/parser.c \
	src/ast_print.c \
	src/json_out.c \
	src/sql_format.c \
	src/executor.c \
	src/storage.c \
	src/sql_processor.c

BPTREE_SRCS = src/main.c src/bptree.c

.PHONY: all test bench clean

all: $(TARGET)

$(TARGET): $(BPTREE_SRCS) $(SQL_SRCS)
	$(CC) $(CFLAGS) -o $@ $(BPTREE_SRCS) $(SQL_SRCS)

$(TEST_TARGET): test/test_bptree.c src/bptree.c
	$(CC) $(CFLAGS) -o $@ test/test_bptree.c src/bptree.c

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(BENCH_TARGET): benchmark/bench_search.c src/bptree.c
	$(CC) $(CFLAGS) -o $@ benchmark/bench_search.c src/bptree.c

bench: $(BENCH_TARGET)
	./$(BENCH_TARGET)

clean:
	rm -f $(TARGET) $(TEST_TARGET) $(BENCH_TARGET)
