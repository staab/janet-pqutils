#include <janet.h>
#include "pg.h"

int string_in_array(char *val, int32_t argc, char *argv[]) {
    int i;
    for(i = 0; i < argc; i++) {
        if(*argv[i] == *val) {
            return 1;
        }
    }

    return 0;
}

typedef struct {
    char* type;
    char* contents;
} SQLFragment;

static void fragment_tostring(void *p, JanetBuffer *buffer) {
    SQLFragment* fragment = (SQLFragment *)p;
    char repr[strlen(fragment->type) + strlen(fragment->contents)];

    sprintf(repr, "<pg/%s %s>", fragment->type, fragment->contents);

    janet_buffer_push_cstring(buffer, repr);
}

static struct JanetAbstractType SQLFragment_jt = {
    "pg.sql/fragment",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    fragment_tostring
};

static Janet cfun_sql_unsafe(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);

    char* contents = (char *)janet_getstring(argv, 0);

    SQLFragment *fragment = janet_abstract(&SQLFragment_jt, sizeof(SQLFragment*));
    fragment->type = "unsafe";
    fragment->contents = contents;

    return janet_wrap_abstract(fragment);
}

static Janet cfun_sql_literal(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);

    char* contents = (char *)janet_getstring(argv, 0);

    SQLFragment *fragment = janet_abstract(&SQLFragment_jt, sizeof(SQLFragment*));
    fragment->type = "literal";
    fragment->contents = contents;

    return janet_wrap_abstract(fragment);
}

static Janet cfun_sql_identifier(int32_t argc, Janet *argv) {
    janet_fixarity(argc, 1);

    char* contents = (char *)janet_getstring(argv, 0);

    SQLFragment *fragment = janet_abstract(&SQLFragment_jt, sizeof(SQLFragment*));
    fragment->type = "identifier";
    fragment->contents = contents;

    return janet_wrap_abstract(fragment);
}

typedef struct {
    JanetArray* children;
} SQLComposite;

static int composite_mark(void *p, size_t size) {
    (void) size;

    SQLComposite *composite = (SQLComposite *)p;

    janet_mark(janet_wrap_array(composite->children));

    return 0;
}

static struct JanetAbstractType SQLComposite_jt = {
    "pg.sql/composite",
    NULL,
    composite_mark,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static Janet cfun_sql_composite(int32_t argc, Janet *argv) {
    janet_arity(argc, 1, -1);

    JanetArray *children = janet_array_n(argv, argc);

    SQLComposite *composite = janet_abstract(&SQLComposite_jt, sizeof(SQLComposite*));
    composite->children = children;

    return janet_wrap_abstract(composite);
}

static const JanetReg sql_cfuns[] = {
    {"unsafe", cfun_sql_unsafe, "(pg/unsafe)\n\nMarks an arbitrary string as sql. Use with caution, contents are not escaped."},
    {"literal", cfun_sql_literal, "(pg/literal)\n\nCreates a sql literal."},
    {"identifier", cfun_sql_identifier, "(pg/identifier)\n\nCreates a sql identifier."},
    {"composite", cfun_sql_composite, "(pg/composite)\n\nCreates a sql composite from fragments."},
    {NULL, NULL, NULL}
};

JANET_MODULE_ENTRY(JanetTable *env) {
    janet_cfuns(env, "staab.pg/sql", sql_cfuns);
}
