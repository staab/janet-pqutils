(import build/pg :as pg)

(pp (pg/disconnect (pg/connect "dbname = ccapi")))
