CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -I./include -I./src
SRC_DIR = src
TEST_DIR = tests
BENCH_DIR = benchmark

SQL_TARGET = sqlparser
SQL_TEST_TARGET = test_runner
BPTREE_TARGET = bptree
STRUCTURE_TEST_TARGET = test_structure
INSERT_TEST_TARGET = test_insert
SEARCH_TEST_TARGET = test_search
EDGE_TEST_TARGET = test_edge_cases
TEST_TARGETS = $(STRUCTURE_TEST_TARGET) $(INSERT_TEST_TARGET) $(SEARCH_TEST_TARGET) $(EDGE_TEST_TARGET)
BPTREE_BENCH_TARGET = bench_search

SQL_SRCS = $(SRC_DIR)/main.c \
           $(SRC_DIR)/parser.c \
           $(SRC_DIR)/ast_print.c \
           $(SRC_DIR)/json_out.c \
           $(SRC_DIR)/sql_format.c \
           $(SRC_DIR)/executor.c \
           $(SRC_DIR)/storage.c

SQL_TEST_SRCS = $(TEST_DIR)/test_parser.c \
                $(TEST_DIR)/test_executor.c \
                $(SRC_DIR)/parser.c \
                $(SRC_DIR)/ast_print.c \
                $(SRC_DIR)/json_out.c \
                $(SRC_DIR)/sql_format.c \
                $(SRC_DIR)/executor.c \
                $(SRC_DIR)/storage.c

STORAGE_TEST_TARGETS = test_storage_insert test_storage_delete test_storage_update test_storage_select_result
STORAGE_TEST_DEPS = $(SRC_DIR)/storage.c
SELECT_RESULT_DEPS = $(SRC_DIR)/storage.c $(SRC_DIR)/parser.c

.PHONY: all clean test bench sqlparser test-sqlparser valgrind test_storage_all run json

all: $(BPTREE_TARGET)

$(BPTREE_TARGET): $(SRC_DIR)/bptree_main.c $(SRC_DIR)/bptree.c
	$(CC) $(CFLAGS) -o $@ $^

$(STRUCTURE_TEST_TARGET): test/test_structure.c $(SRC_DIR)/bptree.c
	$(CC) $(CFLAGS) -o $@ $^

$(INSERT_TEST_TARGET): test/test_insert.c $(SRC_DIR)/bptree.c
	$(CC) $(CFLAGS) -o $@ $^

$(SEARCH_TEST_TARGET): test/test_search.c $(SRC_DIR)/bptree.c
	$(CC) $(CFLAGS) -o $@ $^

$(EDGE_TEST_TARGET): test/test_edge_cases.c $(SRC_DIR)/bptree.c
	$(CC) $(CFLAGS) -o $@ $^

test: $(TEST_TARGETS)
	./$(STRUCTURE_TEST_TARGET)
	./$(INSERT_TEST_TARGET)
	./$(SEARCH_TEST_TARGET)
	./$(EDGE_TEST_TARGET)

$(BPTREE_BENCH_TARGET): $(BENCH_DIR)/bench_search.c $(SRC_DIR)/bptree.c
	$(CC) $(CFLAGS) -o $@ $^

bench: $(BPTREE_BENCH_TARGET)
	./$(BPTREE_BENCH_TARGET)

sqlparser: $(SQL_TARGET)

$(SQL_TARGET): $(SQL_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

$(SQL_TEST_TARGET): $(SQL_TEST_SRCS)
	$(CC) $(CFLAGS) -o $(SQL_TEST_TARGET) $^

test_storage_insert: $(TEST_DIR)/test_storage_insert.c $(STORAGE_TEST_DEPS)
	$(CC) $(CFLAGS) -o $@ $^
	./$@

test_storage_delete: $(TEST_DIR)/test_storage_delete.c $(STORAGE_TEST_DEPS)
	$(CC) $(CFLAGS) -o $@ $^
	./$@

test_storage_update: $(TEST_DIR)/test_storage_update.c $(STORAGE_TEST_DEPS)
	$(CC) $(CFLAGS) -o $@ $^
	./$@

test_storage_select_result: $(TEST_DIR)/test_storage_select_result.c $(SELECT_RESULT_DEPS)
	$(CC) $(CFLAGS) -o $@ $^
	./$@

test_storage_all: $(STORAGE_TEST_TARGETS)

test-sqlparser: $(SQL_TEST_TARGET)
	./$(SQL_TEST_TARGET)
	$(MAKE) test_storage_all

valgrind: $(SQL_TARGET)
	valgrind --leak-check=full --error-exitcode=1 ./$(SQL_TARGET) $(SQL)

clean:
	rm -f $(BPTREE_TARGET) $(TEST_TARGETS) $(BPTREE_BENCH_TARGET)
	rm -f $(SQL_TARGET) $(SQL_TEST_TARGET) $(STORAGE_TEST_TARGETS)
	rm -f data/*.csv data/*.schema
	rm -f data/schema/*.schema data/tables/*.csv data/tables/*.csv.tmp

run: $(SQL_TARGET)
	./$(SQL_TARGET) $(SQL)

json: $(SQL_TARGET)
	./$(SQL_TARGET) $(SQL) --json
