(declare-project
 :name "janet-pg"
 :description "A libpq wrapper for Janet")

(declare-native
 :name "pg"
 :source @["staab.pg/pg.c"]
 :cflags @["-I./postgresql/src/interfaces/libpq"
           "-I./postgresql/src/include"])
