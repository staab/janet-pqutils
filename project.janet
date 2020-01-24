(declare-project
 :name "janet-pqutils"
 :description "A janet-pq/postgres companion library"
 :dependencies ["https://github.com/staab/janet-assert.git"
                "https://github.com/joy-framework/codec.git"
                "https://github.com/andrewchambers/janet-pq.git"])

(declare-source
  :source ["pqutils.janet"])
