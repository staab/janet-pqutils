(def JANET_PG_DEBUG (os/getenv "JANET_PG_DEBUG"))

(defn pg-config [opt]
  (with [f (file/popen (string "pg_config " opt))]
    (string/slice (file/read f :all) 0 -2)))

(def cflags
  (let [includedir (string "-I" (pg-config "--includedir"))
        libdir (string "-L" (pg-config "--libdir"))
        flags @[includedir libdir "-lpq"]]
    (when JANET_PG_DEBUG
      (array/push flags "-fsanitize=undefined")
      (array/push flags "-g"))
    flags))

(def lflags
  (if JANET_PG_DEBUG @["-g"] @[]))

(declare-project
 :name "janet-pg"
 :description "A libpq wrapper for Janet")

(declare-native
 :name "core"
 :source @["staab.pg/core.c"]
 :lflags lflags
 :cflags cflags)

(declare-source
  :source ["staab.pg"])
