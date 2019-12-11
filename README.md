# janet-pg

A libpq wrapper for Janet

# Installation

This library depends on libpq being installed. At the very least, [libpq](https://www.postgresql.org/docs/9.5/install-procedure.html#INSTALL) must be installed on your system, and pg_config needs to be availabe. If both those prerequisites are met, `jpm build` should do the rest.

# Usage

janet-pg comes with two layers; the first is written in c, and lives in `staab.pg/core`. This provides a very basic (and incomplete) glue layer between janet and libpq. If you need lots of control, you can call this layer directly, but `staab.pg/exec` and `staab.pg/sql` are provided for convenience.

## staab.pg/core

If you're just getting started, skip down to staab.pg/exec, which is a more user- friendly wrapper.

**`connect : string -> Connection`**

Takes a [connection string](https://www.postgresql.org/docs/current/libpq-connect.html#LIBPQ-CONNSTRING) and returns a connection abstract type.

**`disconnect : Connection -> nil`**

Takes a connection and closes it. An error will be raised if the connection is used subsequently.

**`exec : Connection string -> Result`**

Takes a connection and a string of sql and returns a Result abstract type. This will panic if the query fails.

Under the hood, this function uses PQexecParams, but doesn't provide a way to pass parameters currently. See [issue #3](https://github.com/staab/janet-pg/issues/3).

**`collect-count : Result -> int`**

Takes a result and returns the number of rows in the result set.

**`collect-row : Connection Result int -> {keyword any}`**

Takes a connection and a result and returns the nth row of the result set. Panics if the row is out of bounds.

**`collect-all : Connection Result int -> [{keyword any}]`**

Takes a connection and a result and returns all rows.

**`escape-literal : Connection string -> string`**

Takes a connection and a string and returns the string escaped for use as a literal in a query.

**`escape-identifier : Connection string -> string`**

Takes a connection and a string and returns the string escaped for use as an identifier in a query.

## staab.pg/exec

This is the primary interface for running queries against postgres. In general, it manages your connection by placing it into a single `:pg/global-conn` dynamic binding.

**`get-connection -> Connection`**

Checks the type of `(dyn :pg/global-conn)` and returns it.

**`connect : string {:no-global bool}? -> Connection`**

Takes a connection string, connects to the database, and returns a connection. By default, it sets the connection to `:pg/global-conn`, but this can be disabled by passing `{:no-global true}` as an option.

**`disconnect -> nil`**

Disconnects the global connection and unsets `:pg/global-conn`.

**`literal : string -> string`**

Escapes a string for use in a query as a literal using the global connection object.

**`identifier : string -> string`**

Escapes a string for use in a query as an identifer using the global connection object.

**`exec : string -> Result`**

Runs a query against the global connection and returns the result.

**`count : Result -> int`**

Returns the number of rows in a result.

**`iter : string -> nil`**

Takes a query string and yields rows one by one to the current fiber. Prefer `generator` to `iter`.

**`generator : string -> fiber`**

Wraps `iter` in a fiber that inherits the current environment and captures rows yielded. This is the best way to lazily iterate over rows in a result set, e.g.:

```
(let [rows @[]]
  (loop [row :generate (exec/generator "select * from my_table")]
    (array/push rows row))
  (pp rows))
```

**`all : string -> [{keyword any}]`**

Takes a query and returns all rows.

**`nth : string -> {keyword any}`**

Takes a query and returns the nth row. Panics if the row is out of bounds.

**`one : string -> {keyword any}`**

Takes a query and returns the first row. Returns nil if there are no results.

**`scalar : string -> any`**

Takes a query and returns some value in the first row of the result set. Which field it chooses when there are multiple is undefined, so only use this with a
query that returns a single field.

**`col : string keyword -> [any]`**

Takes a query and a key and returns a tuple of the values for that key.

# Disclaimer

This is pre-alpha software. Please open an issue if you'd like to use it in a project.
