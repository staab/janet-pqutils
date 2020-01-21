(use staab.assert/assert)
(use pqutils)

(defn ->immut [x]
  (cond
   (indexed? x) (tuple ;(map ->immut x))
   (table? x) (struct ;(kvs x))
   x))

(def test-info "dbname = postgres")

# Connection dyn var is required when doing anything. Connecting
# sets the connection to pq.context by default

(assert= :pq.context (type (connect test-info {:no-global true})))
(assert-err (get-connection))
(assert= :pq.context (type (connect test-info)))
(assert= :pq.context (type (get-connection)))
(disconnect)
(assert-err (get-connection))
(with-connection [(connect test-info {:no-global true})]
  (assert= :pq.context (type (get-connection))))
(assert-err (get-connection))
(with-connect [test-info]
  (assert= :pq.context (type (get-connection))))
(assert-err (get-connection))

# Query functions

(connect test-info)

(assert= :pq.result (type (exec "select 1")))

(def text-query
  (string/join
   ["select" (identifier :tablename) ","
    "(rank() over (order by tablename))::numeric as int, 1.2 as float,"
    "false as false, true as true, null as nil"
    "from" (identifier :pg_tables)
    "where" (identifier :tablename) "like" (literal "pg_auth%")]
   " "))

(def text-query-result
  [{:tablename :pg_auth_members :int 1 :float 1.2
    :false false :true true :nil nil}
   {:tablename :pg_authid :int 2 :float 1.2
    :false false :true true :nil nil}])

# Everything should work on both a string and a result as input

(assert= 2 (count text-query))
(assert= 2 (count (exec text-query)))

(assert= (first text-query-result) (->immut (-> (one text-query))))
(assert= (first text-query-result) (->immut (one (exec text-query))))

(assert= text-query-result (->immut (all text-query)))
(assert= text-query-result (->immut (all (exec text-query))))

(assert= nil (one "select 1 where false = true"))
(assert= nil (one (exec "select 1 where false = true")))

(assert= 3 (scalar "select 3"))
(assert= 3 (scalar (exec "select 3")))

(assert= nil (scalar "select 3 where false = true"))
(assert= nil (scalar (exec "select 3 where false = true")))

(assert-deep= @["x" "y"] (col "select jsonb_array_elements_text(jsonb_build_array('x', 'y'))"))
(assert-deep= @["x" "y"] (col (exec "select jsonb_array_elements_text(jsonb_build_array('x', 'y'))")))

# Test iteration

(assert=
 :caught
 (try
  (resume (generator text-query))
  ([e]
   (assert=
    (string e)
    "PGResult: ERROR:  DECLARE CURSOR can only be used in transaction blocks\n")
   :caught)))

(let [result @[]]
  (exec "BEGIN")
  (loop [row :generate (generator text-query)]
    (array/push result row))
  (exec "COMMIT")
  (assert= text-query-result (->immut result)))
