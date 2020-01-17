(import _pq)
(import pq)

(def *decoders* pq/*decoders*)
(def json pq/json)
(def jsonb pq/jsonb)

(put *decoders* 19 keyword)

(defn get-connection []
  (let [conn (dyn :pqutils/global-conn)]
    (when (not= (type conn) :pq.context)
      (error "dyn :pqutils/global-conn is not a connection object"))
    conn))

(defn connect [info &opt opts]
  (default opts {})
  (let [{:no-global no-global?} opts
        conn (pq/connect info)]
    (if no-global? conn (setdyn :pqutils/global-conn conn))))

(defmacro with-connection [connection & body]
  ~(with-dyns [:pqutils/global-conn ,;connection] ,;body))

(defmacro with-connect [info & body]
  ~(with-dyns [:pqutils/global-conn (,pq/connect ,;info)] ,;body))

(defn disconnect []
  (if-let [conn (dyn :pqutils/global-conn)]
    (pq/close conn))
  (setdyn :pqutils/global-conn nil))

(defn literal [s] (pq/escape-literal (get-connection) (string s)))

(defn identifier [s] (pq/escape-identifier (get-connection) (string s)))

(defn composite [& s] (string/join (map string s) " "))

(defn exec
  "Takes a string and executes it with the current connection, returing
   a result. If a result is passed instead, it passes the result through.
   This allows passing a result wherever you would otherwise pass a string
   and re-use most of the query execution api."
  [query & params]
  (if (= :pq.result (type query))
    query
    (_pq/exec (get-connection) query ;params)))

(defn count [& args] (pq/result-ntuples (exec ;args)))

(defn all [query & params]
  (def result (exec query ;params))
  (when (_pq/error? result) (error result))
  (_pq/result-unpack result pq/*decoders*))

(defn one [query & params]
  (def result (all query ;params))
  (if (empty? result) nil (first result)))

(defn scalar [query & params]
  (if-let [row (one query ;params)]
    (first (values row))))

(defn col [query & params]
  (map |(first (values $)) (all query ;params)))

(defn iter [query &opt opts & params]
  (let [chunk-size (get opts :chunk-size 100)
        cur (identifier (gensym))]
    (var done? false)
    (exec (composite "DECLARE" cur "CURSOR FOR" query) ;params)
    (while (not done?)
      (try
       (each row (all "FETCH FORWARD $1 $2" chunk-size cur)
         (yield row))
       ([e] (set done? true))))
    (exec "CLOSE" cur)))

(defn generator [query &opt opts & params]
  (fiber/new |(iter query opts ;params) :iy))
