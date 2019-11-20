#include <janet.h>
#include <libpq-fe.h>

static Janet myfun(int32_t argc, Janet *argv) {
    PGconn *conn;

    conn = PQconnectdb("dbname = ccapi");

    PQfinish(conn);

    return janet_wrap_nil();
}

static const JanetReg cfuns[] = {
    {"myfun", myfun, "(mymod/myfun)\n\nPrints a hello message."},
    {NULL, NULL, NULL}
};

JANET_MODULE_ENTRY(JanetTable *env) {
    janet_cfuns(env, "staab.pg", cfuns);
}
