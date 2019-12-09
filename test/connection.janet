(use build/connection)
(use staab.assert/assert)

(def connection (connect "dbname = postgres"))

(assert= "<pg/connection dbname = postgres>" (string/format "%q" connection))
(assert= "'hello ''there'''" (escape-literal connection "hello 'there'"))
(assert= "\"x'y\"\"z\"" (escape-identifier connection "x'y\"z"))

(assert=
 [{:tablename "pg_authid" :schemaname "pg_catalog"}
  {:tablename "pg_auth_members" :schemaname "pg_catalog"}]
 (let [t (escape-identifier connection "pg_tables")
       t_col (escape-identifier connection "tablename")
       s_col (escape-identifier connection "schemaname")
       pattern (escape-literal connection "pg_auth%")
       query ["select" t_col "," s_col "from" t "where" t_col "LIKE" pattern]]
   (tuple ;(exec connection (string/join query " ")))))

(disconnect connection)

(assert-err (exec connection "select 1"))

(assert=
 :caught
 (try
  (exec connection "select 1")
  ([e] (assert= e "no connection to the server\n") :caught)))
