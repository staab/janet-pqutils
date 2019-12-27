(import build/core :as core)

(defn get-connection []
  (let [conn (dyn :pg/global-conn)]
    (when (not= (type conn) :pg/connection)
      (error "dyn :pg/global-conn is not a connection object"))
    conn))

(defn connect [info &opt opts]
  (default opts {})
  (let [{:no-global no-global?} opts
        conn (core/connect info)]
    (if no-global? conn (setdyn :pg/global-conn conn))))

(defmacro with-connection [connection & body]
  ~(with-dyns [:pg/global-conn ,;connection] ,;body))

(defmacro with-connect [info & body]
  ~(with-dyns [:pg/global-conn (,core/connect ,;info)] ,;body))

(defn disconnect []
  (if-let [conn (dyn :pg/global-conn)]
    (core/disconnect conn))
  (setdyn :pg/global-conn nil))

(defn literal [s] (core/escape-literal (get-connection) (string s)))

(defn identifier [s] (core/escape-identifier (get-connection) (string s)))

(defn exec
  "Takes a string and executes it with the current connection, returing
   a result. If a result is passed instead, it passes the result through.
   This allows passing a result wherever you would otherwise pass a string
   and re-use most of the query execution api."
  [q] (if (= :pg/result (type q)) q (core/exec (get-connection) q)))

(defn count [q] (core/collect-count (exec q)))

(def- casters @{})

(defn defcast
  "Defines a function for casting a sql value to janet based on the
   keywordized string representation of the corresponding postgres oid."
  [oid f]
  (put casters oid f))

(defn cast [oid x]
  ((get casters oid identity) x))

(def- unpackers @{})

(defn defunpack
  "Defines an unpacker function for a dispatch value. When unpacking results,
   the function is used to collect the value from the resulting row, so it should
   take a key and a row, rather than the value at that location. Example:

   (defunpack :a+b |(+ ($ :a) ($ :b)))
   (unpack {:a 2 :b 1} {:unpack [:a+b]}) # => {:a+b 3 :a 2 :b 1}"
  [k f]
  (put unpackers k f))

(defn nth [q i &opt opts]
  (def r (exec q))
  (def row @{})
  (each {:name k :oid oid :value v} (core/collect-row-meta r i)
    (put row k (cast oid v)))
  (each k (get opts :unpack [])
    ((unpackers k) row))
  row)

(defn iter [q &opt ns]
  (def r (exec q))
  (loop [i :range [0 (count r)]]
    (yield (nth r i ns))))

(defn generator [q &opt ns]
  (fiber/new |(iter q ns) :iy))

(defn one [q &opt ns]
  (def r (exec q))
  (when (> (count r) 0)
    (nth r 0 ns)))

(defn scalar [q &opt ns]
  (if-let [row (one q ns)] (first (values row))))

(defn col [q k &opt ns]
  (def results @[])
  (loop [row :generate (generator q ns)]
    (array/push results (row k)))
  results)

(defn all [q &opt ns]
  (def results @[])
  (loop [row :generate (generator q ns)]
    (array/push results row))
  results)

