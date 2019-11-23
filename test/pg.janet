(import build/pg :as pg)
(use staab.assert/assert)

(assert= 1 (pg/disconnect (pg/connect "dbname = ccapi"))))
