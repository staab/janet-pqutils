(use build/connection)
(use staab.assert/assert)

(def connection (connect "dbname = postgres"))

(assert= "<pg/connection dbname = postgres>" (string/format "%q" connection))
(disconnect connection)

(assert= "'hello ''there'''" (escape-literal connection "hello 'there'"))
(assert= "\"x'y\"\"z\"" (escape-identifier connection "x'y\"z"))

