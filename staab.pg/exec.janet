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

(defn exec [q] (core/exec (get-connection) q))

(defn count [q] (core/collect-count (exec q)))

(defn iter [q]
  (let [result (exec q)]
    (loop [idx :range [0 (core/collect-count result)]]
      (yield (core/collect-row (get-connection) result idx)))))

(defn generator [q]
  (fiber/new |(iter q) :iy))

(defn all [q] (core/collect-all (get-connection) (exec q)))

(defn nth [q idx] (core/collect-row (get-connection) (exec q) idx))

(defn one [q] (nth q 0))

(defn scalar [q] (first (values (one q))))

(defn col [q k]
  (let [result @[]]
    (loop [row :generate (generator q)]
      (array/push result (row k)))
    (tuple ;result)))
