(use staab.assert/assert)
(use _pg)
(use ./util)

# String representations and escapings should work

(let [c (connect "dbname = postgres")]
  (assert= "<pg/connection dbname = postgres>" (string/format "%q" c))
  (assert= "'hello ''there'''" (escape-literal c "hello 'there'"))
  (assert= "\"x'y\"\"z\"" (escape-identifier c "x'y\"z")))

# Test basic query building and collection

(let [c (connect "dbname = postgres")
      t (escape-identifier c "pg_tables")
      t_col (escape-identifier c "tablename")
      s_col (escape-identifier c "schemaname")
      pattern (escape-literal c "pg_auth%")
      query ["select" t_col "," s_col "from" t "where" t_col "LIKE" pattern]
      result (exec c (string/join query " "))]
  (assert=
   {:oid :name :value :pg_authid :name :tablename}
   (->immut (first (collect-row-meta result))))
  (assert=
   [{:tablename :pg_authid :schemaname :pg_catalog}
    {:tablename :pg_auth_members :schemaname :pg_catalog}]
    (->immut (collect-all result))))

# Make sure various data types are coerced properly

(let [c (connect "dbname = postgres")]
  (assert= {:x 1} (->immut (first (collect-all (exec c "select 1 as x")))))
  (assert= {:x 1.1} (->immut (first (collect-all (exec c "select 1.1 as x")))))
  (assert= {:x true} (->immut (first (collect-all (exec c "select true as x")))))
  (assert= {:x false} (->immut (first (collect-all (exec c "select false as x"))))))

# Closed connections should throw appropriate errors

(let [c (connect "dbname = postgres")]
  (disconnect c)
  (assert-err (exec c "select 1"))
  (assert=
   :caught
   (try
    (exec c "select 1")
    ([e]
     (assert
      (or (= e "connection not open\n") (= e "no connection to the server\n"))
      "Connection should be closed")
     :caught))))
