#include <janet.h>
#include <libpq-fe.h>
#include "pg.h"

#define FLAG_CLOSED 1

typedef struct {
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

    connection_close(*(Connection **)p);

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
    NULL,
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
    connection->info = info;
    connection->handle = handle;
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
    PGresult* handle;
    int n_tuples;
    int n_fields;
} Result;

static int result_gc(void *p, size_t size) {
    (void) size;

    PQclear(((Result *)p)->handle);

    return 0;
}

static struct JanetAbstractType Result_jt = {
    "pg/result",
    result_gc,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

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
    result->handle = pgres;
    result->n_tuples = PQntuples(pgres);
    result->n_fields = PQnfields(pgres);

    return janet_wrap_abstract(result);
}

static Janet cfun_collect_row(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 2);

    Result* result = janet_getabstract(argv, 0, &Result_jt);
    int row_idx = janet_getinteger(argv, 1);

    if (row_idx < 0 || row_idx > result->n_tuples) {
        janet_panic("Row index is out of bounds");
    }

    JanetKV *row = janet_struct_begin(result->n_fields);

    for (int field_idx = 0; field_idx < result->n_fields; field_idx++) {
        char* k = PQfname(result->handle, field_idx);
        char* v = PQgetvalue(result->handle, row_idx, field_idx);

        janet_struct_put(row, janet_ckeywordv(k), janet_cstringv(v));
    }

    return janet_wrap_struct(janet_struct_end(row));
}

static Janet cfun_collect_all(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);

    Result* result = janet_getabstract(argv, 0, &Result_jt);

    JanetArray* rows = janet_array(result->n_tuples);

    for (int row_idx = 0; row_idx < result->n_tuples; row_idx++) {
        JanetKV *row = janet_struct_begin(result->n_fields);

        for (int field_idx = 0; field_idx < result->n_fields; field_idx++) {
            char* k = PQfname(result->handle, field_idx);
            char* v = PQgetvalue(result->handle, row_idx, field_idx);

            janet_struct_put(row, janet_ckeywordv(k), janet_cstringv(v));
        }

        janet_array_push(rows, janet_wrap_struct(janet_struct_end(row)));
    }

    return janet_wrap_array(rows);
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
    {"collect-row", cfun_collect_row, "(pg/collect-row)\n\nCollects a single result of a query"},
    {"collect-all", cfun_collect_all, "(pg/collect-all)\n\nCollects all results of a query"},
    {"escape-literal", cfun_escape_literal, "(pg/escape-literal)\n\nEscapes a literal string"},
    {"escape-identifier", cfun_escape_identifier, "(pg/escape-identifier)\n\nEscapes an identifier"},
    {NULL, NULL, NULL}
};

JANET_MODULE_ENTRY(JanetTable *env) {
    janet_cfuns(env, "staab.pg/connection", cfuns);
}
