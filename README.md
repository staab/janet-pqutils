# janet-pg

A libpq wrapper for Janet

# Installation

This library depends on libpq being installed. At the very least, [libpq](https://www.postgresql.org/docs/9.5/install-procedure.html#INSTALL) must be installed on your system, and pg_config needs to be availabe. If both those prerequisites are met, `jpm build` should do the rest.

# Usage

janet-pg comes with two layers; the first is written in c, and lives in `staab.pg/core`. This provides a very basic (and incomplete) glue layer between janet and libpq. If you need lots of control, you can call this layer directly, but `staab.pg/exec` is a janet wrapper that is more convenient and provides more functionality.

To get you started, check out the example program before

```
(import staab.pg/exec as x)

(x/connect "postgres://localhost:5432/mydb")

# Lazily iterate over results
(loop [{:a a :b b} :generate (x/generator "select a, b from mytable")]
  (pp (+ a b)))

# Temporarily use a different database, disconnectin when done
(x/with-connect ["postgres://localhost:5432/myotherdb"]
  # Return a tuple of values for column "a"
  (pp (x/col "select a from mytable" :a)))

# Back at mydb again
(let [res (x/exec "select * from mytable")]
  # Count reads metadata, it doesn't re-execute the query
  (when (> (x/count res) 10) (x/nth res 9)))

# Manually disconnect
(x/disconnect)
```

# Documentation

- [staab.pg/core](/docs/core.md)
- [staab.pg/exec](/docs/exec.md)

# Disclaimer

This is pre-alpha software. Please open an issue if you'd like to use it in a project.
