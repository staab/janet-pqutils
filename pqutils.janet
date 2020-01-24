(import pq)
(import codec)

# Re-export some stuff
(def *decoders* pq/*decoders*)
(def json pq/json)
(def jsonb pq/jsonb)

(defn get-connection []
  (let [conn (dyn :pqutils/global-conn)]
    (when (not= (type conn) :pq/context)
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

(defn exec [query & params]
  (if (= :pq/result (type query))
    query
    (pq/exec (get-connection) query ;params)))

(defn count [& args] (pq/result-ntuples (exec ;args)))

(defn all [& args]
  (pq/result-unpack (exec ;args) *decoders*))

(defn row [& args]
  (def rows (all ;args))
  (if (empty? rows) nil (first rows)))

(defn col [& args]
  (map |(first (values $)) (all ;args)))

(defn val [& args]
  (if-let [r (row ;args)
           v (values r)]
    (when (not (empty? v))
      (first v))))

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
