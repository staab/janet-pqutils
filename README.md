# janet-pg

A libpq wrapper for Janet

# Installation

This library depends on libpq being installed. At the very least, [libpq](https://www.postgresql.org/docs/9.5/install-procedure.html#INSTALL) must be installed on your system, and pg_config needs to be availabe. If both those prerequisites are met, `jpm build` should do the rest.

# Usage

janet-pg comes with two layers; the first is written in c, and lives in `staab.pg/core`. This provides a very basic (and incomplete) glue layer between janet and libpq. If you need lots of control, or need to tune performance, you can call this layer directly, but `staab.pg/exec` is a janet wrapper that is more convenient and provides more functionality, including:

- A number of convenience wrappers, including `nth`, `col`, `scalar`, and `generate`.
- Extensible sql -> janet datatype casting (builtins are handled in the core layer).
- Extensible table/column oriented unpacking

To get started, check out the example program below:

```
(import staab.pg/exec :as x)

# By default, connect sets the connection to a global dynamic binding
(x/connect "postgres://localhost:5432/mydb")

# Lazily iterate over results
(loop [row :generate (x/generator "select a, b from mytable")]
  (pp (+ ;(values row))))

# Temporarily use a different database, disconnecting when done
(x/with-connect ["postgres://localhost:5432/myotherdb"]
  # Return a tuple of values for column "a"
  (pp (x/col "select a from mytable" :a)))

# Back at mydb again
(let [res (x/exec "select * from mytable")]
  # Count reads metadata, it doesn't re-execute the query. In cases
  # like this, it's useful to pass a result rather than a query string.
  # Everything in exec supports this overloading.
  (when (>= (x/count res) 10) (x/nth res 9)))

# Add rules to cast sql values to janet values
(x/defcast :json json/decode)
(x/defcast :jsonb json/decode)

# Returns @{"x" 1}
(x/scalar "select jsonb_build_object('x', 1)")

# Add rules to post-process results from certain tables. Read
# more about how this works at docs/exec.md.
(x/defcast :integer inc)
(x/defunpack :increment-x (fn [row] (update row :x inc)))

# Returns {:x 3 :y 2}, per above unpack rules.
(x/one "select 1 as x, 1 as y" {:unpack [:increment-x]})

# Manually disconnect
(x/disconnect)
```

# Documentation

- [staab.pg/core](/docs/core.md)
- [staab.pg/exec](/docs/exec.md)

# Disclaimer

This is pre-alpha software. Please open an issue if you'd like to use it in a project.
