#include <stdio.h>

#define p(s) fprintf(stderr, "|| DEBUG: %s\n", s)

static Janet cfun_connect(int32_t argc, Janet *argv);
static Janet cfun_disconnect(int32_t argc, Janet *argv);

static Janet cfun_sql_unsafe(int32_t argc, Janet *argv);
static Janet cfun_sql_literal(int32_t argc, Janet *argv);
static Janet cfun_sql_identifier(int32_t argc, Janet *argv);
static Janet cfun_sql_composite(int32_t argc, Janet *argv);
