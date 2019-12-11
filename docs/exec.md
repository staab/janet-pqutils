# staab.pg/exec

This is the primary interface for running queries against postgres. In general, it manages your connection by placing it into a single `:pg/global-conn` dynamic binding.

For convenience, everything in this namespace that accepts a query string also accepts a Result.

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

**`exec : string|Result -> Result`**

Runs a query against the global connection and returns the result.

**`count : string|Result -> int`**

Returns the number of rows in a result.

**`iter : string|Result -> nil`**

Takes a query string and yields rows one by one to the current fiber. Prefer `generator` to `iter`.

**`generator : string|Result -> fiber`**

Wraps `iter` in a fiber that inherits the current environment and captures rows yielded. This is the best way to lazily iterate over rows in a result set, e.g.:

```
(let [rows @[]]
  (loop [row :generate (exec/generator "select * from my_table")]
    (array/push rows row))
  (pp rows))
```

**`all : string|Result -> [{keyword any}]`**

Takes a query and returns all rows.

**`nth : string|Result -> {keyword any}`**

Takes a query and returns the nth row. Panics if the row is out of bounds.

**`one : string|Result -> {keyword any}`**

Takes a query and returns the first row. Returns nil if there are no results.

**`scalar : string|Result -> any`**

Takes a query and returns some value in the first row of the result set. Which field it chooses when there are multiple is undefined, so only use this with a
query that returns a single field.

**`col : string|Result keyword -> [any]`**

Takes a query and a key and returns a tuple of the values for that key.
