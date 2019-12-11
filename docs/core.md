# staab.pg/core

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

