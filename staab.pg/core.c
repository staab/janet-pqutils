#include <janet.h>
#include <libpq-fe.h>
#include "pg.h"

#define FLAG_CLOSED 1

typedef struct {
    JanetTable* oids;
    PGconn* handle;
    const uint8_t* info;
    int flags;
} Connection;

static void connection_close(Connection *connection) {
    if (!(connection->flags & FLAG_CLOSED)) {
        connection->flags |= FLAG_CLOSED;
        PQfinish(connection->handle);
    }
}

static int connection_gc(void *p, size_t size) {
    (void) size;

    connection_close((Connection*)p);

    return 0;
}

static int connection_mark(void *p, size_t size) {
    (void) size;

    Connection* connection = (Connection*)p;

    janet_mark(janet_wrap_table(connection->oids));
    janet_mark(janet_wrap_string(connection->info));

    return 0;
}

static void connection_tostring(void *p, JanetBuffer *buffer) {
    Connection* connection = (Connection *)p;
    char repr[strlen((char *)connection->info) + 16];

    sprintf(repr, "<pg/connection %s>", connection->info);

    janet_buffer_push_cstring(buffer, repr);
}

static struct JanetAbstractType Connection_jt = {
    "pg/connection",
    connection_gc,
    connection_mark,
    NULL,
    NULL,
    NULL,
    NULL,
    connection_tostring
};

static Janet cfun_connect(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);

    const uint8_t *info = janet_getstring(argv, 0);
    PGconn *handle = PQconnectdb((char *)info);

    if (PQstatus(handle) != CONNECTION_OK) {
        janet_panicf("Connection to database failed: %s", PQerrorMessage(handle));
    }

    // Set always-secure search path, so malicious users can't take control.
    PGresult *res = PQexec(handle, "SELECT pg_catalog.set_config('search_path', '', false)");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        janet_panicf("SET failed: %s", PQerrorMessage(handle));
    } else {
        PQclear(res);
    }

    Connection *connection = janet_abstract(&Connection_jt, sizeof(Connection));
    connection->oids = janet_table(0);
    connection->handle = handle;
    connection->info = info;
    connection->flags = 0;

    return janet_wrap_abstract(connection);
}

static Janet cfun_disconnect(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);

    Connection *connection = janet_getabstract(argv, 0, &Connection_jt);

    connection_close(connection);

    return janet_wrap_nil();
}

typedef struct {
    Connection* connection;
    PGresult* handle;
    int n_tuples;
    int n_fields;
} Result;

static int result_gc(void *p, size_t size) {
    (void) size;

    PQclear(((Result *)p)->handle);

    return 0;
}

static int result_mark(void *p, size_t size) {
    (void) size;

    Result* result = (Result*)p;

    janet_mark(janet_wrap_abstract(result->connection));

    return 0;
}

static struct JanetAbstractType Result_jt = {
    "pg/result",
    result_gc,
    result_mark,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

void populate_oids(Result* result) {
    for (int col_idx = 0; col_idx < result->n_fields; col_idx++) {
        int oid = PQftype(result->handle, col_idx);
        Janet match = janet_table_get(result->connection->oids, janet_wrap_integer(oid));

        if (janet_equals(match, janet_wrap_nil())) {
            int oid_length = snprintf(NULL, 0, "%d", oid);
            char* query = malloc(oid_length + 23);

            sprintf(query, "SELECT %i::oid::regtype", oid);

            PGresult* pgres = PQexec(result->connection->handle, query);

            free(query);

            if (PQresultStatus(pgres) != PGRES_TUPLES_OK) {
                PQclear(pgres);

                return janet_panic("Failed to populate oid table (this is a bug)");
            }

            char* oid_name = PQgetvalue(pgres, 0, 0);

            PQclear(pgres);

            janet_table_put(result->connection->oids, janet_wrap_integer(oid), janet_cstringv(oid_name));
        }
    }
}

static Janet result_get_value(Result *result, int row_idx, int col_idx) {
    if (PQgetisnull(result->handle, row_idx, col_idx)) {
        return janet_wrap_nil();
    }

    char* v = PQgetvalue(result->handle, row_idx, col_idx);
    int oid = PQftype(result->handle, col_idx);
    Janet oid_name = janet_table_get(result->connection->oids, janet_wrap_integer(oid));

    if (janet_equals(oid_name, janet_wrap_nil())) {
        janet_panic("Failed to find oid (this is a bug)");
    }

    uint8_t* oid_str = (uint8_t*)janet_unwrap_string(oid_name);

    if (
        janet_cstrcmp(oid_str, "integer") == 0     ||
        janet_cstrcmp(oid_str, "numeric") == 0     ||
        janet_cstrcmp(oid_str, "bigserial") == 0   ||
        janet_cstrcmp(oid_str, "bigint") == 0      ||
        janet_cstrcmp(oid_str, "double") == 0      ||
        janet_cstrcmp(oid_str, "real") == 0        ||
        janet_cstrcmp(oid_str, "smallint") == 0    ||
        janet_cstrcmp(oid_str, "smallserial") == 0 ||
        janet_cstrcmp(oid_str, "serial") == 0
    ) {
        double number;
        janet_scan_number((const uint8_t*)v, strlen(v), &number);
        return janet_wrap_number(number);
    }

    if (janet_cstrcmp(oid_str, "boolean") == 0) {
        return strcmp(v, "t") == 0 ? janet_wrap_true() : janet_wrap_false();
    }

    if (janet_cstrcmp(oid_str, "name") == 0) {
        return janet_ckeywordv(v);
    }

    return janet_cstringv(v);
}

static Janet cfun_exec(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 2);

    Connection* connection = janet_getabstract(argv, 0, &Connection_jt);
    char* command = (char*)janet_getstring(argv, 1);

    PGresult* pgres = PQexecParams(connection->handle, command, 0, NULL, NULL, NULL, NULL, 0);
    ExecStatusType status = PQresultStatus(pgres);
    char* error = PQerrorMessage(connection->handle);

    switch (status) {
        case PGRES_FATAL_ERROR:
        case PGRES_BAD_RESPONSE:
            PQclear(pgres);
            janet_panic(error);
            break;
        case PGRES_NONFATAL_ERROR:
            fprintf(stderr, "%s", error);
            break;
        default:
            break;
    }

    if (status != PGRES_TUPLES_OK) {
        PQclear(pgres);

        return janet_wrap_nil();
    }

    Result *result = janet_abstract(&Result_jt, sizeof(Result));
    result->connection = connection;
    result->handle = pgres;
    result->n_tuples = PQntuples(pgres);
    result->n_fields = PQnfields(pgres);

    populate_oids(result);

    return janet_wrap_abstract(result);
}

static Janet cfun_collect_count(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);

    Result* result = janet_getabstract(argv, 0, &Result_jt);

    return janet_wrap_integer(result->n_tuples);
}

static Janet cfun_collect_row(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 2);

    Result* result = janet_getabstract(argv, 0, &Result_jt);
    int row_idx = janet_getinteger(argv, 1);

    if (row_idx < 0 || row_idx > result->n_tuples) {
        janet_panic("Row index is out of bounds");
    }

    JanetKV *row = janet_struct_begin(result->n_fields);

    for (int col_idx = 0; col_idx < result->n_fields; col_idx++) {
        char* k = PQfname(result->handle, col_idx);
        Janet v = result_get_value(result, row_idx, col_idx);

        janet_struct_put(row, janet_ckeywordv(k), v);
    }

    return janet_wrap_struct(janet_struct_end(row));
}

static Janet cfun_collect_all(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);

    Result* result = janet_getabstract(argv, 0, &Result_jt);

    Janet* rows = janet_tuple_begin(result->n_tuples);

    for (int row_idx = 0; row_idx < result->n_tuples; row_idx++) {
        JanetKV *row = janet_struct_begin(result->n_fields);

        for (int32_t col_idx = 0; col_idx < result->n_fields; col_idx++) {
            char* k = PQfname(result->handle, col_idx);
            Janet v = result_get_value(result, row_idx, col_idx);

            janet_struct_put(row, janet_ckeywordv(k), v);
        }

        rows[row_idx] = janet_wrap_struct(janet_struct_end(row));
    }

    return janet_wrap_tuple(janet_tuple_end(rows));
}

static Janet cfun_escape_literal(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 2);

    Connection* connection = janet_getabstract(argv, 0, &Connection_jt);
    char* input = (char*)janet_getstring(argv, 1);
    char* output = PQescapeLiteral(connection->handle, input, strlen(input));
    const uint8_t* result = janet_string((uint8_t*)output, strlen(output));

    PQfreemem(output);

    return janet_wrap_string(result);
}

static Janet cfun_escape_identifier(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 2);

    Connection* connection = janet_getabstract(argv, 0, &Connection_jt);
    char* input = (char*)janet_getstring(argv, 1);
    char* output = PQescapeIdentifier(connection->handle, input, strlen(input));
    const uint8_t* result = janet_string((uint8_t*)output, strlen(output));

    PQfreemem(output);

    return janet_wrap_string(result);
}

static const JanetReg cfuns[] = {
    {"connect", cfun_connect, "(pg/connect)\n\nReturns a postgresql connection."},
    {"disconnect", cfun_disconnect, "(pg/disconnect)\n\nCloses a postgresql connection"},
    {"exec", cfun_exec, "(pg/exec)\n\nExecutes a query with optional parameters"},
    {"collect-count", cfun_collect_count, "(pg/collect-count)\n\nReturns the number of rows for a query result"},
    {"collect-row", cfun_collect_row, "(pg/collect-row)\n\nCollects a single result of a query"},
    {"collect-all", cfun_collect_all, "(pg/collect-all)\n\nCollects all results of a query"},
    {"escape-literal", cfun_escape_literal, "(pg/escape-literal)\n\nEscapes a literal string"},
    {"escape-identifier", cfun_escape_identifier, "(pg/escape-identifier)\n\nEscapes an identifier"},
    {NULL, NULL, NULL}
};

JANET_MODULE_ENTRY(JanetTable *env) {
    janet_cfuns(env, "staab.pg/connection", cfuns);
}
