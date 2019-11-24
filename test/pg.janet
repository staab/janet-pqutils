(import build/pg :as pg)
(use staab.assert/assert)

(let [c (pg/connect "dbname = postgres")]
  (assert=
    "<pg/connection dbname = postgres>"
   (string/format "%q" c))
  (assert=
   "<pg/literal 'that''s'>"
   (string/format "%q" (pg/literal c "that's")))
  (assert=
   "<pg/identifier \"Royal \"\"we\"\"\">"
   (string/format "%q" (pg/identifier c "Royal \"we\""))))

