#include <stdio.h>
#include <janet.h>
#include <libpq-fe.h>

#define p(s) fprintf(stderr, "%s\n", s)

static int connection_gc(void *p, size_t size) {
    (void) size;

    PQfinish(*(PGconn **)p);

    return 0;
}

static struct JanetAbstractType Connection_jt = {
    "pg.connection",
    connection_gc,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static Janet cfun_connect(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);

    const uint8_t *conninfo = janet_getstring(argv, 0);
    PGconn *conn = PQconnectdb((char *)conninfo);
    void *jconn = janet_abstract(&Connection_jt, sizeof(PGconn*));

    return janet_wrap_abstract(jconn);
}

static Janet cfun_disconnect(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);

    PGconn* *conn = janet_getabstract(argv, 0, &Connection_jt);

    PQfinish(*conn);

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
