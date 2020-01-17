(use staab.assert/assert)
(use pg)
(use ./util)

(def test-info "dbname = postgres")

# Connection dyn var is required when doing anything. Connecting
# sets the connection to pg/connection by default

(assert= :pg/connection (type (connect test-info {:no-global true})))
(assert-err (get-connection))
(assert= :pg/connection (type (connect test-info)))
(assert= :pg/connection (type (get-connection)))
(disconnect)
(assert-err (get-connection))
(with-connection [(connect test-info {:no-global true})]
  (assert= :pg/connection (type (get-connection))))
(assert-err (get-connection))
(with-connect [test-info]
  (assert= :pg/connection (type (get-connection))))
(assert-err (get-connection))

# Query functions

(connect test-info)

(assert= :pg/result (type (exec "select 1")))

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

(assert= (first text-query-result) (->immut (one text-query)))
(assert= (first text-query-result) (->immut (one (exec text-query))))

(assert= text-query-result (->immut (all text-query)))
(assert= text-query-result (->immut (all (exec text-query))))

(assert= nil (->immut (one "select 1 where false = true")))
(assert= nil (->immut (one (exec "select 1 where false = true"))))

(assert= 3 (scalar "select 3"))
(assert= 3 (scalar (exec "select 3")))

(assert= nil (scalar "select 3 where false = true"))
(assert= nil (scalar (exec "select 3 where false = true")))

(assert= [1 2] (->immut (col text-query :int)))
(assert= [1 2] (->immut (col (exec text-query) :int)))

# Test iteration

(assert=
 :caught
 (try
  (resume (generator text-query))
  ([e]
   (assert= e "ERROR:  DECLARE CURSOR can only be used in transaction blocks\n")
   :caught)))

(let [result @[]]
  (exec "BEGIN")
  (loop [row :generate (generator text-query)]
    (array/push result row))
  (exec "COMMIT")
  (assert= text-query-result (->immut result)))

# Test that stuff gets unpacked/casted properly

(defcast :integer inc)

(defn unpack [row]
  (update row :x inc)
  (put row :z (+ (row :x) (row :y))))

(assert= {:x 3 :y 2 :z 5} (->immut (one "select 1 as x, 1 as y" {:unpack unpack})))

