#include <stdio.h>
#include <janet.h>
#include <libpq-fe.h>

#define p(s) fprintf(stderr, "|| DEBUG: %s\n", s)
#define FLAG_CLOSED 1

typedef struct {
    PGconn* handle;
    int flags;
} Conn;

static void connection_close(Conn *conn) {
    if (!(conn->flags & FLAG_CLOSED)) {
        conn->flags |= FLAG_CLOSED;
        PQfinish(conn->handle);
    }
}

static int connection_gc(void *p, size_t size) {
    (void) size;

    connection_close(*(Conn **)p);

    return 0;
}

static void connection_tostring(void *p, JanetBuffer *buffer) {
    Conn* conn = *(Conn **)p;
    char* dbname = PQdb(conn->handle);
    char repr[32];

    // TODO: I can't get dbname to be anything but (null)
    sprintf(repr, "<pg/connection %s>", dbname);

    janet_buffer_push_cstring(buffer, repr);
}

static struct JanetAbstractType Conn_jt = {
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

    const uint8_t *conninfo = janet_getstring(argv, 0);
    PGconn *handle = PQconnectdb((char *)conninfo);

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

    Conn *conn = janet_abstract(&Conn_jt, sizeof(Conn*));
    conn->handle = handle;
    conn->flags = 0;

    return janet_wrap_abstract(conn);
}

static Janet cfun_disconnect(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);

    Conn* *conn = janet_getabstract(argv, 0, &Conn_jt);

    connection_close(*conn);

    return janet_wrap_nil();
}

static const JanetReg cfuns[] = {
    {"connect", cfun_connect, "(pg/connect)\n\nReturns a postgresql connection."},
    {"disconnect", cfun_disconnect, "(pg/disconnect)\n\nCloses a postgresql connection"},
    {NULL, NULL, NULL}
};

JANET_MODULE_ENTRY(JanetTable *env) {
    janet_cfuns(env, "staab.pg", cfuns);
}
