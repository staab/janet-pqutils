(use build/connection)
(use staab.assert/assert)

(assert= "<pg/connection dbname = postgres>" (string/format "%q" (connect "dbname = postgres")))

