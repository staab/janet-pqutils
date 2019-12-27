# staab.pg/exec

This is the primary interface for running queries against postgres. In general, it manages your connection by placing it into a single `:pg/global-conn` dynamic binding.

## Connection

**`get-connection -> Connection`**

Checks the type of `(dyn :pg/global-conn)` and returns it.

**`connect : string {:no-global bool}? -> Connection`**

Takes a connection string, connects to the database, and returns a connection. By default, it sets the connection to `:pg/global-conn`, but this can be disabled by passing `{:no-global true}` as an option.

**`disconnect -> nil`**

## Escaping

Disconnects the global connection and unsets `:pg/global-conn`.

**`literal : string -> string`**

Escapes a string for use in a query as a literal using the global connection object.

**`identifier : string -> string`**

## Query Execution

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

**`all : string|Result -> @[@{keyword any}]`**

Takes a query and returns all rows.

**`nth : string|Result -> @{keyword any}`**

Takes a query and returns the nth row. Panics if the row is out of bounds.

**`one : string|Result -> @{keyword any}`**

Takes a query and returns the first row. Returns nil if there are no results.

**`scalar : string|Result -> any`**

Takes a query and returns some value in the first row of the result set. Which field it chooses when there are multiple is undefined, so only use this with a
query that returns a single field.

**`col : string|Result keyword -> @[any]`**

Takes a query and a key and returns a tuple of the values for that key.

## Customization

**`defcast : keyword (any -> any) -> nil`**

Takes a keyword corresponding to the name of a postgres data type, and a function for coercing a sql value to a janet value for that column. This will automatically be called by all functions in the `exec` namespace.

For example, you can easily add jsonb support to janet-pg:

```
(defcast :jsonb json/decode)
```

## Options

For convenience, everything in this namespace that accepts a query string also accepts a Result, and an optional map of options.

**`unpack : {keyword any} -> nil`**

A function that takes a row and mutates it to implement post-processing logic.

The purpose of this is to convert sql results to something more apropos to your application in a way that is not determined by type alone, without having to re-iterate over the results. In most cases, you're better off doing this kind of thing in sql since you can take advantage of postgres' features (views, window functions, computations in non-select clauses).

However, there are times when this might be useful, like when merging another data source with your postgres results. Suppose you had a redis datastore that contained rate limit information for an api key, but your key data is stored in postgres. You might do something like the following:

```
# Define an unpacker that merges in information about how much capacity has been used
# and whether the api key should be rate limited by using the id from postgres to
# retrieve up to date information from redis.
(defn unpack-rate-limit-info [row]
  (let [{:capacity cap :id id} row
        used (get-capacity-used id)]
    (put row :capacity-used used)
    (put row :rate-limited? (> used cap))))

# Use it by selecting the id and capacity, and passing :api-key/rate-limit-info as an unpacker.
# This might yield something like {:id 1 :capacity 100 :capacity-used 30 :rate-limited? false}
(one "select id, capacity from api_key" {:unpack unpack-rate-limit-info})
```
