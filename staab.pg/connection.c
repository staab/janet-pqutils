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

static Janet cfun_exec(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 2);

    Connection* connection = janet_getabstract(argv, 0, &Connection_jt);
    char* command = (char*)janet_getstring(argv, 1);

    PGresult* res = PQexecParams(connection->handle, command, 0, NULL, NULL, NULL, NULL, 0);
    ExecStatusType status = PQresultStatus(res);
    char* error = PQerrorMessage(connection->handle);

    switch (status) {
        case PGRES_FATAL_ERROR:
        case PGRES_BAD_RESPONSE:
            PQclear(res);
            janet_panic(error);
            break;
        case PGRES_NONFATAL_ERROR:
            fprintf(stderr, "%s", error);
            break;
        default:
            break;
    }

    if (status != PGRES_TUPLES_OK) {
        PQclear(res);

        return janet_wrap_nil();
    }

    int n_results = PQntuples(res);
    int n_fields = PQnfields(res);
    JanetArray* rows = janet_array(n_results);

    for (int i = 0; i < n_results; i++) {
        JanetKV *row = janet_struct_begin(n_fields);

        for (int j = 0; j < n_fields; j++) {
            char* k = PQfname(res, j);
            char* v = PQgetvalue(res, i, j);

            janet_struct_put(row, janet_ckeywordv(k), janet_cstringv(v));
        }

        janet_array_push(rows, janet_wrap_struct(janet_struct_end(row)));
    }

    PQclear(res);

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
    {"escape-literal", cfun_escape_literal, "(pg/escape-literal)\n\nEscapes a literal string"},
    {"escape-identifier", cfun_escape_identifier, "(pg/escape-identifier)\n\nEscapes an identifier"},
    {NULL, NULL, NULL}
};

JANET_MODULE_ENTRY(JanetTable *env) {
    janet_cfuns(env, "staab.pg/connection", cfuns);
}
