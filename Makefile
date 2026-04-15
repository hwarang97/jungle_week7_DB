CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -finput-charset=UTF-8 -fexec-charset=UTF-8 -I./include -I./src
SRC_DIR = src
TEST_DIR = tests
BENCH_DIR = benchmark

SQL_TARGET = sqlparser
SQL_TEST_TARGET = test_runner
BPTREE_TARGET = bptree
STRUCTURE_TEST_TARGET = test_structure
SEARCH_TEST_TARGET = test_search
TEST_TARGETS = $(STRUCTURE_TEST_TARGET) $(SEARCH_TEST_TARGET)
BPTREE_BENCH_TARGET = bench_search

SQL_SRCS = $(SRC_DIR)/main.c \
           $(SRC_DIR)/parser.c \
           $(SRC_DIR)/ast_print.c \
           $(SRC_DIR)/json_out.c \
           $(SRC_DIR)/sql_format.c \
           $(SRC_DIR)/executor.c \
           $(SRC_DIR)/storage.c \
           $(SRC_DIR)/bptree.c

SQL_TEST_SRCS = $(TEST_DIR)/test_parser.c \
                $(TEST_DIR)/test_executor.c \
                $(SRC_DIR)/parser.c \
                $(SRC_DIR)/ast_print.c \
                $(SRC_DIR)/json_out.c \
                $(SRC_DIR)/sql_format.c \
                $(SRC_DIR)/executor.c \
                $(SRC_DIR)/storage.c \
                $(SRC_DIR)/bptree.c

STORAGE_TEST_TARGETS = test_storage_insert test_storage_delete test_storage_update test_storage_select_result test_sql_index_integration
STORAGE_TEST_DEPS = $(SRC_DIR)/storage.c $(SRC_DIR)/bptree.c
SELECT_RESULT_DEPS = $(SRC_DIR)/storage.c $(SRC_DIR)/bptree.c $(SRC_DIR)/parser.c

ifeq ($(OS),Windows_NT)
EXEEXT = .exe
RUN_BPTREE_TARGET = powershell -NoProfile -Command "& '.\\$(BPTREE_TARGET)$(EXEEXT)'"
RUN_STRUCTURE_TEST_TARGET = powershell -NoProfile -Command "& '.\\$(STRUCTURE_TEST_TARGET)$(EXEEXT)'"
RUN_SEARCH_TEST_TARGET = powershell -NoProfile -Command "& '.\\$(SEARCH_TEST_TARGET)$(EXEEXT)'"
RUN_BPTREE_BENCH_TARGET = powershell -NoProfile -Command "& '.\\$(BPTREE_BENCH_TARGET)$(EXEEXT)'"
RUN_SQL_TARGET = powershell -NoProfile -Command "& '.\\$(SQL_TARGET)$(EXEEXT)'"
RUN_SQL_TEST_TARGET = powershell -NoProfile -Command "& '.\\$(SQL_TEST_TARGET)$(EXEEXT)'"
RUN_CURRENT_TARGET = powershell -NoProfile -Command "& '.\\$@$(EXEEXT)'"
else
EXEEXT =
RUN_BPTREE_TARGET = ./$(BPTREE_TARGET)$(EXEEXT)
RUN_STRUCTURE_TEST_TARGET = ./$(STRUCTURE_TEST_TARGET)$(EXEEXT)
RUN_SEARCH_TEST_TARGET = ./$(SEARCH_TEST_TARGET)$(EXEEXT)
RUN_BPTREE_BENCH_TARGET = ./$(BPTREE_BENCH_TARGET)$(EXEEXT)
RUN_SQL_TARGET = ./$(SQL_TARGET)$(EXEEXT)
RUN_SQL_TEST_TARGET = ./$(SQL_TEST_TARGET)$(EXEEXT)
RUN_CURRENT_TARGET = ./$@$(EXEEXT)
endif

.PHONY: all clean test bench sqlparser test-sqlparser valgrind test_storage_all run json

all: $(BPTREE_TARGET)

$(BPTREE_TARGET): $(SRC_DIR)/bptree_main.c $(SRC_DIR)/bptree.c
	$(CC) $(CFLAGS) -o $@ $^

$(STRUCTURE_TEST_TARGET): test/test_structure.c $(SRC_DIR)/bptree.c
	$(CC) $(CFLAGS) -o $@ $^

$(SEARCH_TEST_TARGET): test/test_search.c $(SRC_DIR)/bptree.c
	$(CC) $(CFLAGS) -o $@ $^

test: $(TEST_TARGETS)
	$(RUN_STRUCTURE_TEST_TARGET)
	$(RUN_SEARCH_TEST_TARGET)

$(BPTREE_BENCH_TARGET): $(BENCH_DIR)/bench_search.c $(SRC_DIR)/bptree.c
	$(CC) $(CFLAGS) -o $@ $^

bench: $(BPTREE_BENCH_TARGET)
	$(RUN_BPTREE_BENCH_TARGET)

sqlparser: $(SQL_TARGET)

$(SQL_TARGET): $(SQL_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

$(SQL_TEST_TARGET): $(SQL_TEST_SRCS)
	$(CC) $(CFLAGS) -o $(SQL_TEST_TARGET) $^

test_storage_insert: $(TEST_DIR)/test_storage_insert.c $(STORAGE_TEST_DEPS)
	$(CC) $(CFLAGS) -o $@ $^
	$(RUN_CURRENT_TARGET)

test_storage_delete: $(TEST_DIR)/test_storage_delete.c $(STORAGE_TEST_DEPS)
	$(CC) $(CFLAGS) -o $@ $^
	$(RUN_CURRENT_TARGET)

test_storage_update: $(TEST_DIR)/test_storage_update.c $(STORAGE_TEST_DEPS)
	$(CC) $(CFLAGS) -o $@ $^
	$(RUN_CURRENT_TARGET)

test_storage_select_result: $(TEST_DIR)/test_storage_select_result.c $(SELECT_RESULT_DEPS)
	$(CC) $(CFLAGS) -o $@ $^
	$(RUN_CURRENT_TARGET)

test_sql_index_integration: $(TEST_DIR)/test_sql_index_integration.c $(SRC_DIR)/parser.c $(SRC_DIR)/executor.c $(SELECT_RESULT_DEPS)
	$(CC) $(CFLAGS) -o $@ $^
	$(RUN_CURRENT_TARGET)

test_storage_all: $(STORAGE_TEST_TARGETS)

test-sqlparser: $(SQL_TEST_TARGET)
	$(RUN_SQL_TEST_TARGET)
	$(MAKE) test_storage_all

valgrind: $(SQL_TARGET)
	valgrind --leak-check=full --error-exitcode=1 $(RUN_SQL_TARGET) $(SQL)

clean:
	rm -f $(BPTREE_TARGET) $(TEST_TARGETS) $(BPTREE_BENCH_TARGET)
	rm -f $(SQL_TARGET) $(SQL_TEST_TARGET) $(STORAGE_TEST_TARGETS)
	rm -f data/*.csv data/*.schema
	rm -f data/schema/*.schema data/tables/*.csv data/tables/*.csv.tmp

run: $(SQL_TARGET)
	$(RUN_SQL_TARGET) $(SQL)

json: $(SQL_TARGET)
	$(RUN_SQL_TARGET) $(SQL) --json
