(defn pg-config [opt]
  (with [f (file/popen (string "pg_config " opt))]
    (string/slice (file/read f :all) 0 -2)))

(declare-project
 :name "janet-pg"
 :description "A libpq wrapper for Janet")

(declare-native
 :name "pg"
 :source @["staab.pg/pg.c"]
 :cflags @[(string "-I" (pg-config "--includedir"))
           (string "-L" (pg-config "--libdir"))
           "-lpq"])
