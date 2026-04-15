#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/types.h"
#include "../src/bptree.h"

#define BENCH_TABLE "bench_users"
#define ID_LOOKUPS 1000
#define NAME_LOOKUPS 1000

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
    int i;

    cleanup_fixture();
    if (storage_create(BENCH_TABLE, col_defs, 3) != 0) {
        return -1;
    }

    for (i = 1; i <= row_count; ++i) {
        char id_text[32];
        char name_text[64];
        char age_text[32];
        char *values[3];

        snprintf(id_text, sizeof(id_text), "%d", i);
        snprintf(name_text, sizeof(name_text), "user%d", i);
        snprintf(age_text, sizeof(age_text), "%d", 20 + (i % 30));

        values[0] = id_text;
        values[1] = name_text;
        values[2] = age_text;

        if (storage_insert(BENCH_TABLE, NULL, values, 3) != 0) {
            return -1;
        }
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

static void run_case(int row_count)
{
    double id_ms;
    double name_ms;
    double speedup;

    if (prepare_fixture(row_count) != 0) {
        printf("%s%s[bench]%s fixture setup failed for rows=%s%d%s\n",
               ansi_bold(), ansi_red(), ansi_reset(),
               ansi_bold(), row_count, ansi_reset());
        cleanup_fixture();
        return;
    }

    id_ms = benchmark_id_lookup(row_count);
    name_ms = benchmark_name_lookup(row_count);
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
}

int main(void)
{
    int row_counts[] = {100, 1000, 5000};
    int i;

    printf("%s%s[bench-sql-index]%s current_bptree_order=%s%d%s\n",
           ansi_bold(), ansi_cyan(), ansi_reset(),
           ansi_bold(), BPTREE_ORDER, ansi_reset());
    printf("%s%s[goal]%s indexed %sWHERE id = ?%s vs linear %sWHERE name = ?%s\n",
           ansi_bold(), ansi_cyan(), ansi_reset(),
           ansi_green(), ansi_reset(),
           ansi_yellow(), ansi_reset());

    for (i = 0; i < (int)(sizeof(row_counts) / sizeof(row_counts[0])); ++i) {
        run_case(row_counts[i]);
    }

    printf("\n%s%s[hint]%s compare another order with %smake bench-sql-index CFLAGS=\"... -DBPTREE_ORDER=32\"%s\n",
           ansi_bold(), ansi_cyan(), ansi_reset(),
           ansi_bold(), ansi_reset());
    return 0;
}
