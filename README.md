# janet-pqutils

A postgres utility library for Janet and [janet-pq](https://github.com/andrewchambers/janet-pq). To get started, check out the example program below:

```
(import pqutils :as sql)

# By default, connect sets the connection to a global dynamic binding
(sql/connect "postgres://localhost:5432/mydb")

# Use parameterized queries
(def param-q "select a, b from mytable where c > $1")

# Or dynamically constructed queries (identifiers and literals are escaped)
(defn dyn-q [table col & where]
  (sql/composite "select" (identifier col) "from" (identifier table) "where" ;where))

(dyn-q "jim's coffee" :a (identifier :c) "=" (literal "starbuck's"))
# => "select "jim's coffee" from "mytable" where "c" = 'starbuck''s'"

# Retrieve all, a row, a column, or a single value from results. Parameters are always supported.
(sql/all param-q 2) # => [{:a 1 :b 2} {:a 2 :b 3}]
(sql/row param-q 2) # => {:a 1 :b 2}
(sql/col param-q 2) # => [1 2]
(sql/one param-q 2) # => 1

# Lazily iterate over results
(loop [row :generate (sql/generator param-q 2)]
  (pp (+ ;(values row)))) # => 3, 5

# Temporarily use a different database, disconnecting when done
(sql/with-connect ["postgres://localhost:5432/myotherdb"]
  # Return a tuple of values for column "a"
  (pp (sql/col "select a from mytable")))

# Back at mydb again
(let [res (sql/exec "select * from mytable")]
  # Count reads metadata, it doesn't re-execute the query. In cases
  # like this, it's useful to pass a result rather than a query string.
  (when (>= (sql/count res) 10) (in (all res) 9)))

# Handles json decoding (encoding has to be explicitly opted into)
(sql/one "select jsonb_build_object('x', 1)") # => @{"x" 1}

# Manually disconnect
(sql/disconnect)
```

# Disclaimer

This is pre-alpha software. Please open an issue if you'd like to use it in a project.
