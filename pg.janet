(import _pg)

(defn get-connection []
  (let [conn (dyn :pg/global-conn)]
    (when (not= (type conn) :pg/connection)
      (error "dyn :pg/global-conn is not a connection object"))
    conn))

(defn connect [info &opt opts]
  (default opts {})
  (let [{:no-global no-global?} opts
        conn (_pg/connect info)]
    (if no-global? conn (setdyn :pg/global-conn conn))))

(defmacro with-connection [connection & body]
  ~(with-dyns [:pg/global-conn ,;connection] ,;body))

(defmacro with-connect [info & body]
  ~(with-dyns [:pg/global-conn (,_pg/connect ,;info)] ,;body))

(defn disconnect []
  (if-let [conn (dyn :pg/global-conn)]
    (_pg/disconnect conn))
  (setdyn :pg/global-conn nil))

(defn literal [s] (_pg/escape-literal (get-connection) (string s)))

(defn identifier [s] (_pg/escape-identifier (get-connection) (string s)))

(defn composite [& s] (string/join (map string s) " "))

(defn exec
  "Takes a string and executes it with the current connection, returing
   a result. If a result is passed instead, it passes the result through.
   This allows passing a result wherever you would otherwise pass a string
   and re-use most of the query execution api."
  [q] (if (= :pg/result (type q)) q (_pg/exec (get-connection) q)))

(defn count [q] (_pg/collect-count (exec q)))

(def- casters @{})

(defn defcast
  "Defines a function for casting a sql value to janet based on the
   keywordized string representation of the corresponding postgres oid."
  [oid f]
  (put casters oid f))

(defn cast [oid x]
  ((get casters oid identity) x))

(defn post-process [meta row &opt opts]
  # Cast sql types to janet types
  (each {:name k :oid oid} meta (update row k |(cast oid $)))
  # Allow caller to post-process each row while it's still mutable
  ((get opts :unpack identity) row)
  row)

(defn all [q &opt opts]
  (def r (exec q))
  (def m (_pg/collect-row-meta r))
  (map |(post-process m $ opts) (_pg/collect-all r)))

(defn one [q &opt opts]
  (def r (exec q))
  (when (> (count r) 0)
    (let [m (_pg/collect-row-meta r)
          row (first (_pg/collect-all r))]
      (post-process m row opts))))

(defn scalar [q &opt opts]
  (if-let [row (one q opts)]
    (first (values row))))

(defn col [q &opt opts]
  (map |(first (values $)) (all q opts)))

(defn iter [q &opt opts]
  (let [chunk-size (get opts :chunk-size 100)
        cur (identifier (gensym))]
    (var done? false)
    (exec (composite "DECLARE" cur "CURSOR FOR" q))
    (while (not done?)
      (let [q (composite "FETCH FORWARD" chunk-size cur)]
        (try
         (each row (all q) (yield row))
         ([e] (set done? true)))))
    (exec (composite "CLOSE" cur))))

(defn generator [q &opt opts]
  (fiber/new |(iter q opts) :iy))
