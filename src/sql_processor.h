#ifndef SQL_PROCESSOR_H
#define SQL_PROCESSOR_H

#include "types.h"

typedef struct SQLProcessorOptions {
    int debug_mode;
    int json_mode;
    int tokens_mode;
    int format_mode;
} SQLProcessorOptions;

/* TODO: 기존 SQL 처리기 코드를 여기에 복사 */

int sql_processor_run_file(const char *path, SQLProcessorOptions options);
int sql_processor_run_string(const char *input, SQLProcessorOptions options);

#endif
