(defn pg-config [opt]
  (with [f (file/popen (string "pg_config " opt))]
    (string/slice (file/read f :all) 0 -2)))

(def cflags
  @[(string "-I" (pg-config "--includedir"))
    (string "-L" (pg-config "--libdir"))
    "-fsanitize=undefined"
    "-lpq"])

(declare-project
 :name "janet-pg"
 :description "A libpq wrapper for Janet")

(declare-native
 :name "core"
 :source @["staab.pg/core.c"]
 :cflags cflags)

(declare-source
  :source ["staab.pg"])
