#include <stdio.h>
#include <janet.h>
#include <libpq-fe.h>

#define p(s) fprintf(stderr, "|| DEBUG: %s\n", s)
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

    Connection *connection = janet_abstract(&Connection_jt, sizeof(Connection*));
    connection->info = info;
    connection->handle = handle;
    connection->flags = 0;

    return janet_wrap_abstract(connection);
}

static Janet cfun_disconnect(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);

    Connection* *connection = janet_getabstract(argv, 0, &Connection_jt);

    connection_close(*connection);

    return janet_wrap_nil();
}

static int escaped_gc(void *p, size_t size) {
    (void) size;

    // PGfreemem(*(char **)p);

    return 0;
}

static void literal_tostring(void *p, JanetBuffer *buffer) {
    char* literal = (char *)p;
    char repr[strlen(literal) + 20];

    sprintf(repr, "<pg/literal %s>", literal);

    janet_buffer_push_cstring(buffer, repr);
}

static struct JanetAbstractType Literal_jt = {
    "pg/literal",
    escaped_gc,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    literal_tostring
};

static Janet cfun_literal(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 2);

    Connection* connection = janet_getabstract(argv, 0, &Connection_jt);
    char* str = (char *)janet_getstring(argv, 1);
    char* safe = PQescapeLiteral(connection->handle, str, strlen(str));
    char* literal = janet_abstract(&Literal_jt, strlen(safe));

    sprintf(literal, "%s", safe);

    return janet_wrap_abstract(literal);
}

static void identifier_tostring(void *p, JanetBuffer *buffer) {
    char* identifier = (char *)p;
    char repr[strlen(identifier) + 9];

    sprintf(repr, "<pg/identifier %s>", identifier);

    janet_buffer_push_cstring(buffer, repr);
}

static struct JanetAbstractType Identifier_jt = {
    "pg/identifier",
    escaped_gc,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    identifier_tostring
};

static Janet cfun_identifier(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 2);

    Connection* connection = janet_getabstract(argv, 0, &Connection_jt);
    char* str = (char *)janet_getstring(argv, 1);
    char* safe = PQescapeIdentifier(connection->handle, str, strlen(str));
    char* ident = janet_abstract(&Identifier_jt, strlen(safe));

    sprintf(ident, "%s", safe);

    return janet_wrap_abstract(ident);
}

static const JanetReg cfuns[] = {
    {"connect", cfun_connect, "(pg/connect)\n\nReturns a postgresql connection."},
    {"disconnect", cfun_disconnect, "(pg/disconnect)\n\nCloses a postgresql connection"},
    {"literal", cfun_literal, "(pg/literal)\n\nEscapes a string as a postgresql literal"},
    {"identifier", cfun_identifier, "(pg/identifier)\n\nEscapes a string as a postgresql identifier"},
    {NULL, NULL, NULL}
};

JANET_MODULE_ENTRY(JanetTable *env) {
    janet_cfuns(env, "staab.pg", cfuns);
}
