(import staab.pg/exec :as x)
(use staab.assert/assert)

(def test-info "dbname = postgres")

# Connection dyn var is required when doing anything. Connecting
# sets the connection to pg/connection by default

(assert= :pg/connection (type (x/connect test-info {:no-global true})))
(assert-err (x/get-connection))
(assert= :pg/connection (type (x/connect test-info)))
(assert= :pg/connection (type (x/get-connection)))
(x/disconnect)
(assert-err (x/get-connection))
(x/with-connection [(x/connect test-info {:no-global true})]
  (assert= :pg/connection (type (x/get-connection))))
(assert-err (x/get-connection))
(x/with-connect [test-info]
  (assert= :pg/connection (type (x/get-connection))))
(assert-err (x/get-connection))

# Query functions

(x/connect test-info)

(assert= :pg/result (type (x/exec "select 1")))

(def text-query
  (string/join
   ["select" (x/identifier :tablename) ","
    "rank() over (order by tablename) as int, 1.2 as float,"
    "false as false, true as true, null as nil"
    "from" (x/identifier :pg_tables)
    "where" (x/identifier :tablename) "like" (x/literal "pg_auth%")]
   " "))

(def text-query-result
  [{:tablename :pg_auth_members :int 1 :float 1.2 :false false :true true :nil nil}
   {:tablename :pg_authid :int 2 :float 1.2 :false false :true true :nil nil}])

(let [result @[]]
  (loop [row :generate (x/generator text-query)]
    (array/push result row))
  (assert= text-query-result (tuple ;result)))

# Everything should work on both a string and a result as input

(assert= 2 (x/count text-query))
(assert= 2 (x/count (x/exec text-query)))

(assert= text-query-result (x/all text-query))
(assert= text-query-result (x/all (x/exec text-query)))

(assert= (get text-query-result 1) (x/nth text-query 1))
(assert= (get text-query-result 1) (x/nth (x/exec text-query) 1))

(assert-err (x/nth text-query 10))
(assert-err (x/nth (x/exec text-query) 10))

(assert= (first text-query-result) (x/one text-query))
(assert= (first text-query-result) (x/one (x/exec text-query)))

(assert= nil (x/one "select 1 where false = true"))
(assert= nil (x/one (x/exec "select 1 where false = true")))

(assert= 3 (x/scalar "select 3"))
(assert= 3 (x/scalar (x/exec "select 3")))

(assert= nil (x/scalar "select 3 where false = true"))
(assert= nil (x/scalar (x/exec "select 3 where false = true")))

(assert= [1 2] (x/col text-query :int))
(assert= [1 2] (x/col (x/exec text-query) :int))


