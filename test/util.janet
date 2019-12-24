(defn ->immut [x]
  (cond
   (indexed? x) (tuple ;(map ->immut x))
   (table? x) (struct ;(kvs x))
   x))
