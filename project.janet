(declare-project
 :name "janet-pgutils"
 :description "A janet-pq/postgres companion library"
 :dependencies ["https://github.com/staab/janet-assert.git"
                "https://github.com/andrewchambers/janet-pq.git"])

(declare-source
  :source ["pgutils.janet"])
