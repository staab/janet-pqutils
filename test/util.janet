(defn ->immut [x]
  (cond
   (array? x) (tuple ;(map ->immut x))
   (table? x) (struct ;(kvs x))
   x))
