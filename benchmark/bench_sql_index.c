#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/types.h"
#include "../src/bptree.h"

#define BENCH_TABLE "bench_users"
#define ID_LOOKUPS 500
#define NAME_LOOKUPS 500

static int parse_row_count_arg(const char *text, int *out_value)
{
    char *end = NULL;
    long value;

    if (text == NULL || out_value == NULL) {
        return -1;
    }

    value = strtol(text, &end, 10);
    if (end == text || *end != '\0' || value <= 0 || value > INT_MAX) {
        return -1;
    }

    *out_value = (int)value;
    return 0;
}

static const char *ansi_reset(void)
{
    return "\033[0m";
}

static const char *ansi_bold(void)
{
    return "\033[1m";
}

static const char *ansi_cyan(void)
{
    return "\033[36m";
}

static const char *ansi_green(void)
{
    return "\033[32m";
}

static const char *ansi_yellow(void)
{
    return "\033[33m";
}

static const char *ansi_red(void)
{
    return "\033[31m";
}

static void cleanup_fixture(void)
{
    remove("data/schema/" BENCH_TABLE ".schema");
    remove("data/tables/" BENCH_TABLE ".csv");
}

static ParsedSQL *make_select_equals(const char *column, const char *value)
{
    ParsedSQL *sql = calloc(1, sizeof(*sql));

    if (sql == NULL) {
        return NULL;
    }

    sql->type = QUERY_SELECT;
    strncpy(sql->table, BENCH_TABLE, sizeof(sql->table) - 1U);
    sql->limit = -1;
    sql->col_count = 1;
    sql->columns = calloc(1, sizeof(char *));
    sql->where = calloc(1, sizeof(WhereClause));
    if (sql->columns == NULL || sql->where == NULL) {
        free(sql->columns);
        free(sql->where);
        free(sql);
        return NULL;
    }

    sql->columns[0] = strdup("*");
    if (sql->columns[0] == NULL) {
        free(sql->columns);
        free(sql->where);
        free(sql);
        return NULL;
    }

    strncpy(sql->where[0].column, column, sizeof(sql->where[0].column) - 1U);
    strcpy(sql->where[0].op, "=");
    strncpy(sql->where[0].value, value, sizeof(sql->where[0].value) - 1U);
    sql->where_count = 1;
    return sql;
}

static void free_simple_select(ParsedSQL *sql)
{
    if (sql == NULL) {
        return;
    }

    free(sql->columns[0]);
    free(sql->columns);
    free(sql->where);
    free(sql);
}

static double elapsed_ms(clock_t start, clock_t end)
{
    return ((double)(end - start) * 1000.0) / (double)CLOCKS_PER_SEC;
}

static int prepare_fixture(int row_count)
{
    char *col_defs[] = {"id INT", "name VARCHAR", "age INT"};
    char *columns[] = {"name", "age"};
    int i;

    cleanup_fixture();
    if (storage_create(BENCH_TABLE, col_defs, 3) != 0) {
        fprintf(stderr,
                "[bench] storage_create failed for table='%s'\n",
                BENCH_TABLE);
        return -1;
    }

    if (storage_bulk_begin(BENCH_TABLE) != 0) {
        fprintf(stderr,
                "[bench] storage_bulk_begin failed for table='%s'\n",
                BENCH_TABLE);
        return -1;
    }

    for (i = 1; i <= row_count; ++i) {
        char name_text[64];
        char age_text[32];
        char *values[2];

        snprintf(name_text, sizeof(name_text), "user%d", i);
        snprintf(age_text, sizeof(age_text), "%d", 20 + (i % 30));

        values[0] = name_text;
        values[1] = age_text;

        if (storage_insert(BENCH_TABLE, columns, values, 2) != 0) {
            fprintf(stderr,
                    "[bench] storage_insert failed at row=%d table='%s' name='%s' age='%s'\n",
                    i, BENCH_TABLE, name_text, age_text);
            storage_bulk_end(BENCH_TABLE);
            return -1;
        }

        if (i % 100000 == 0 || i == row_count) {
            printf("[bench] fixture progress rows=%d inserted=%d\n", row_count, i);
            fflush(stdout);
        }
    }

    if (storage_bulk_end(BENCH_TABLE) != 0) {
        fprintf(stderr,
                "[bench] storage_bulk_end failed for table='%s'\n",
                BENCH_TABLE);
        return -1;
    }

    return 0;
}

static double benchmark_id_lookup(int row_count)
{
    int i;
    clock_t start;
    clock_t end;

    start = clock();
    for (i = 0; i < ID_LOOKUPS; ++i) {
        char value[32];
        ParsedSQL *sql;
        RowSet *rs = NULL;
        int target = (i % row_count) + 1;

        snprintf(value, sizeof(value), "%d", target);
        sql = make_select_equals("id", value);
        if (sql == NULL) {
            return -1.0;
        }

        if (storage_select_result(BENCH_TABLE, sql, &rs) != 0) {
            rowset_free(rs);
            free_simple_select(sql);
            return -1.0;
        }

        rowset_free(rs);
        free_simple_select(sql);
    }
    end = clock();

    return elapsed_ms(start, end);
}

static double benchmark_name_lookup(int row_count)
{
    int i;
    clock_t start;
    clock_t end;

    start = clock();
    for (i = 0; i < NAME_LOOKUPS; ++i) {
        char value[64];
        ParsedSQL *sql;
        RowSet *rs = NULL;
        int target = (i % row_count) + 1;

        snprintf(value, sizeof(value), "'user%d'", target);
        sql = make_select_equals("name", value);
        if (sql == NULL) {
            return -1.0;
        }

        if (storage_select_result(BENCH_TABLE, sql, &rs) != 0) {
            rowset_free(rs);
            free_simple_select(sql);
            return -1.0;
        }

        rowset_free(rs);
        free_simple_select(sql);
    }
    end = clock();

    return elapsed_ms(start, end);
}

static int run_case(int row_count)
{
    double id_ms;
    double name_ms;
    double speedup;

    if (prepare_fixture(row_count) != 0) {
        printf("%s%s[bench]%s fixture setup failed for rows=%s%d%s\n",
               ansi_bold(), ansi_red(), ansi_reset(),
               ansi_bold(), row_count, ansi_reset());
        cleanup_fixture();
        return -1;
    }

    id_ms = benchmark_id_lookup(row_count);
    name_ms = benchmark_name_lookup(row_count);
    if (id_ms < 0.0 || name_ms < 0.0) {
        fprintf(stderr,
                "[bench] lookup benchmark failed for rows=%d id_ms=%.3f name_ms=%.3f\n",
                row_count, id_ms, name_ms);
        cleanup_fixture();
        return -1;
    }
    speedup = (id_ms > 0.0) ? (name_ms / id_ms) : 0.0;

    printf("\n%s%s[CASE]%s rows=%s%d%s order=%s%d%s\n",
           ansi_bold(), ansi_cyan(), ansi_reset(),
           ansi_bold(), row_count, ansi_reset(),
           ansi_bold(), BPTREE_ORDER, ansi_reset());
    printf("  %s[id index]%s    total=%s%.3fms%s avg=%s%.6fms%s\n",
           ansi_green(), ansi_reset(),
           ansi_bold(), id_ms, ansi_reset(),
           ansi_bold(), id_ms / (double)ID_LOOKUPS, ansi_reset());
    printf("  %s[name scan]%s   total=%s%.3fms%s avg=%s%.6fms%s\n",
           ansi_yellow(), ansi_reset(),
           ansi_bold(), name_ms, ansi_reset(),
           ansi_bold(), name_ms / (double)NAME_LOOKUPS, ansi_reset());
    printf("  %s[speedup]%s     name scan is %s%.2fx%s slower than id index\n",
           ansi_bold(), ansi_reset(),
           ansi_bold(), speedup, ansi_reset());

    cleanup_fixture();
    return 0;
}

int main(int argc, char **argv)
{
    int row_counts[] = {10000, 100000, 1000000};
    int i;
    int single_row_count = 0;

    printf("%s%s[bench-sql-index]%s current_bptree_order=%s%d%s\n",
           ansi_bold(), ansi_cyan(), ansi_reset(),
           ansi_bold(), BPTREE_ORDER, ansi_reset());
    printf("%s%s[goal]%s indexed %sWHERE id = ?%s vs linear %sWHERE name = ?%s\n",
           ansi_bold(), ansi_cyan(), ansi_reset(),
           ansi_green(), ansi_reset(),
           ansi_yellow(), ansi_reset());
    printf("%s%s[insert]%s fixture uses actual %sstorage_insert()%s with auto-generated ids\n",
           ansi_bold(), ansi_cyan(), ansi_reset(),
           ansi_bold(), ansi_reset());

    if (argc >= 2) {
        if (parse_row_count_arg(argv[1], &single_row_count) != 0) {
            fprintf(stderr, "[bench] invalid row count: %s\n", argv[1]);
            return 1;
        }

        if (run_case(single_row_count) != 0) {
            fprintf(stderr,
                    "[bench] aborted after first failure at rows=%d\n",
                    single_row_count);
            return 1;
        }

        return 0;
    }

    for (i = 0; i < (int)(sizeof(row_counts) / sizeof(row_counts[0])); ++i) {
        if (run_case(row_counts[i]) != 0) {
            fprintf(stderr,
                    "[bench] aborted after first failure at rows=%d\n",
                    row_counts[i]);
            return 1;
        }
    }

    return 0;
}
