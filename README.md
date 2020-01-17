# janet-pqutils

A postgres utility library for Janet and [janet-pq](https://github.com/andrewchambers/janet-pq). To get started, check out the example program below:

```
(import pqutils :as sql)

# By default, connect sets the connection to a global dynamic binding
(sql/connect "postgres://localhost:5432/mydb")

# Lazily iterate over results. Parameters are always supported
(loop [row :generate (sql/generator "select a, b from mytable where c > $1" 3)]
  (pp (+ ;(values row))))

# Temporarily use a different database, disconnecting when done
(sql/with-connect ["postgres://localhost:5432/myotherdb"]
  # Return a tuple of values for column "a"
  (pp (sql/col "select a from mytable")))

# Back at mydb again
(let [res (sql/exec "select * from mytable")]
  # Count reads metadata, it doesn't re-execute the query. In cases
  # like this, it's useful to pass a result rather than a query string.
  (when (>= (sql/count res) 10) (in (all res) 9)))

# Returns @{"x" 1}
(sql/scalar "select jsonb_build_object('x', 1)")

# Manually disconnect
(sql/disconnect)
```

# Disclaimer

This is pre-alpha software. Please open an issue if you'd like to use it in a project.
