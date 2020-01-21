(import _pq)
(import pq)
(import codec)

(def *decoders* pq/*decoders*)
(def json pq/json)
(def jsonb pq/jsonb)

(put *decoders* 19 keyword)
(put *decoders* 1700 scan-number)

(defn get-connection []
  (let [conn (dyn :pqutils/global-conn)]
    (when (not= (type conn) :pq.context)
      (error "dyn :pqutils/global-conn is not a connection object"))
    conn))

(defn disconnect []
  (if-let [conn (dyn :pqutils/global-conn)]
    (pq/close conn))
  (setdyn :pqutils/global-conn nil))

(defn connect [info &opt opts]
  (default opts {})
  (let [{:no-global no-global?} opts
        conn (pq/connect info)]
    (when (not no-global?)
      (disconnect)
      (setdyn :pqutils/global-conn conn))
    conn))

(defmacro with-connection [connection & body]
  ~(with-dyns [:pqutils/global-conn ,;connection] ,;body))

(defmacro with-connect [info & body]
  ~(with-dyns [:pqutils/global-conn (,pq/connect ,;info)] ,;body))

(defn literal [s]
  (if (number? s) s (pq/escape-literal (get-connection) (string s))))

(defn identifier [s] (pq/escape-identifier (get-connection) (string s)))

(defn composite [& s] (string/join (map string s) " "))

(defn exec
  "Takes a string and executes it with the current connection, returing
   a result. If a result is passed instead, it passes the result through.
   This allows passing a result wherever you would otherwise pass a string
   and re-use most of the query execution api."
  [query & params]
  (def result (if (= :pq.result (type query))
                query
                (_pq/exec (get-connection) query ;params)))
  (if (_pq/error? result) (error result) result))

(defn count [& args] (pq/result-ntuples (exec ;args)))

(defn- map-keys [f d]
  (def ctor (if (= (type d) :table) table struct))
  (ctor ;(mapcat (fn [[k v]] [(f k) v]) (pairs d))))

(defn all [query & params]
  (map
   |(map-keys keyword $)
   (_pq/result-unpack (exec query ;params) pq/*decoders*)))

(defn one [query & params]
  (def result (all query ;params))
  (if (empty? result) nil (first result)))

(defn scalar [query & params]
  (if-let [row (one query ;params)]
    (first (values row))))

(defn col [query & params]
  (map |(first (values $)) (all query ;params)))

(defn iter [query &opt opts & params]
  (let [chunk-size (literal (get opts :chunk-size 100))
        cur (identifier (codec/encode (string (os/cryptorand 10))))
        get-chunk |(all (composite "FETCH FORWARD" chunk-size cur))]
    (var done? false)
    (exec (composite "DECLARE" cur "CURSOR FOR" query) ;params)
    (loop [chunk :iterate (get-chunk) :until (empty? chunk)]
      (each row chunk (yield row)))
    (exec (composite "CLOSE" cur))))

(defn generator [query &opt opts & params]
  (fiber/new |(iter query opts ;params) :iy))
