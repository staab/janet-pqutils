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

(defn count [q] (core/collect-count (get-connection) (exec q)))

(defn iter [q]
  (def r (exec q))
  (loop [idx :range [0 (core/collect-count (get-connection) r)]]
    (yield (core/collect-row (get-connection) r idx))))

(defn generator [q]
  (fiber/new |(iter q) :iy))

(defn all [q] (core/collect-all (get-connection) (exec q)))

(defn nth [q idx] (core/collect-row (get-connection) (exec q) idx))

(defn one [q]
  (def r (exec q))
  (when (> (count r) 0)
    (core/collect-row (get-connection) r 0)))

(defn scalar [q]
  (if-let [row (one q)] (first (values row))))

(defn col [q k]
  (let [results @[]]
    (loop [row :generate (generator q)]
      (array/push results (row k)))
    (tuple ;results)))
