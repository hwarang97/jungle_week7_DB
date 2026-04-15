#include "types.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define dup _dup
#define dup2 _dup2
#define close _close
#ifndef fileno
#define fileno _fileno
#endif
#define make_dir(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define make_dir(path) mkdir(path, 0775)
#endif

#define DATA_DIR "data"
#define TEST_OUTPUT_PATH DATA_DIR "/test_sql_index_output.txt"
#define MEMBERS_SCHEMA_PATH DATA_DIR "/schema/members.schema"
#define MEMBERS_TABLE_PATH DATA_DIR "/tables/members.csv"

static void fail_test(const char *message)
{
    fprintf(stderr, "[SQL INDEX INTEGRATION] FAIL: %s\n", message);
}

static void cleanup_fixture(void)
{
    remove(TEST_OUTPUT_PATH);
    remove(MEMBERS_TABLE_PATH);
    remove(MEMBERS_SCHEMA_PATH);
}

static int ensure_data_dirs(void)
{
    if (make_dir(DATA_DIR) != 0 && errno != EEXIST) {
        return 1;
    }
    if (make_dir(DATA_DIR "/schema") != 0 && errno != EEXIST) {
        return 1;
    }
    if (make_dir(DATA_DIR "/tables") != 0 && errno != EEXIST) {
        return 1;
    }
    return 0;
}

static char *read_text_file(const char *path)
{
    FILE *file;
    long size;
    size_t read_size;
    char *buffer;

    file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }

    if (fseek(file, 0L, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    size = ftell(file);
    if (size < 0) {
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0L, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    buffer = (char *)malloc((size_t)size + 1U);
    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }

    read_size = fread(buffer, 1U, (size_t)size, file);
    fclose(file);

    if (read_size != (size_t)size) {
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';
    return buffer;
}

static int contains_text(const char *haystack, const char *needle)
{
    return haystack != NULL && needle != NULL && strstr(haystack, needle) != NULL;
}

static int execute_sql_text(const char *sql_text)
{
    ParsedSQL *sql = parse_sql(sql_text);
    if (sql == NULL) {
        return 1;
    }

    execute(sql);
    free_parsed(sql);
    return 0;
}

static int capture_select_output(const char *sql_text, char **output)
{
    ParsedSQL *sql;
    int saved_stdout;
    FILE *redirected;

    *output = NULL;
    sql = parse_sql(sql_text);
    if (sql == NULL) {
        return 1;
    }

    fflush(stdout);
    saved_stdout = dup(fileno(stdout));
    if (saved_stdout < 0) {
        free_parsed(sql);
        return 1;
    }

    redirected = freopen(TEST_OUTPUT_PATH, "w", stdout);
    if (redirected == NULL) {
        close(saved_stdout);
        free_parsed(sql);
        return 1;
    }

    execute(sql);
    fflush(stdout);
    free_parsed(sql);

    if (dup2(saved_stdout, fileno(stdout)) < 0) {
        close(saved_stdout);
        return 1;
    }
    close(saved_stdout);

    *output = read_text_file(TEST_OUTPUT_PATH);
    remove(TEST_OUTPUT_PATH);
    return (*output == NULL) ? 1 : 0;
}

static int test_auto_id_insert_and_indexed_select(void)
{
    char *output = NULL;

    if (execute_sql_text("CREATE TABLE members (id INT, name VARCHAR, age INT);") != 0 ||
        execute_sql_text("INSERT INTO members (name, age) VALUES (Alice, 20);") != 0 ||
        execute_sql_text("INSERT INTO members (name, age) VALUES (Bob, 31);") != 0) {
        fail_test("CREATE/INSERT SQL execution failed.");
        return 1;
    }

    if (capture_select_output("SELECT id, name FROM members WHERE id = 2;", &output) != 0) {
        fail_test("Failed to capture indexed SELECT output.");
        return 1;
    }

    if (!contains_text(output, "id | name") ||
        !contains_text(output, "2 | Bob") ||
        !contains_text(output, "(1 rows)")) {
        free(output);
        fail_test("Indexed SELECT did not return the expected row.");
        return 1;
    }

    free(output);
    return 0;
}

static int test_non_id_select_still_works(void)
{
    char *output = NULL;

    if (capture_select_output("SELECT name, age FROM members WHERE name = Alice;", &output) != 0) {
        fail_test("Failed to capture fallback SELECT output.");
        return 1;
    }

    if (!contains_text(output, "name | age") ||
        !contains_text(output, "Alice | 20") ||
        !contains_text(output, "(1 rows)")) {
        free(output);
        fail_test("Fallback SELECT did not return the expected row.");
        return 1;
    }

    free(output);
    return 0;
}

int main(void)
{
    int status = 0;

    cleanup_fixture();
    if (ensure_data_dirs() != 0) {
        fail_test("Failed to prepare data directories.");
        return 1;
    }

    if (test_auto_id_insert_and_indexed_select() != 0) {
        status = 1;
        goto cleanup;
    }

    if (test_non_id_select_still_works() != 0) {
        status = 1;
        goto cleanup;
    }

cleanup:
    cleanup_fixture();
    if (status == 0) {
        fprintf(stderr, "[SQL INDEX INTEGRATION] passed\n");
    }
    return status;
}
