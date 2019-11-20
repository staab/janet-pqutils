(declare-project
 :name "janet-pg"
 :description "A libpq wrapper for Janet")

(declare-native
 :name "pg"
 :source @["staab.pg/pg.c"]
 :cflags @["-Ipostgresql/src/interfaces/libpq"
           "-Ipostgresql/src/include"])
