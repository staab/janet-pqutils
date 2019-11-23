(import build/pg :as pg)
(use staab.assert/assert)

(let [c (pg/connect "dbname = postgres")]
  (pp c))
