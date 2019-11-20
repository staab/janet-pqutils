(declare-project
 :name "janet-pg"
 :description "A libpq wrapper for Janet")

(declare-native
 :name "pg"
 :source @["staab.pg/pg.c"
           "postgresql/src/interfaces/libpq/fe-auth-scram.c"
           "postgresql/src/interfaces/libpq/fe-auth.c"
           "postgresql/src/interfaces/libpq/fe-connect.c"
           "postgresql/src/interfaces/libpq/fe-exec.c"
           "postgresql/src/interfaces/libpq/fe-gssapi-common.c"
           "postgresql/src/interfaces/libpq/fe-lobj.c"
           "postgresql/src/interfaces/libpq/fe-misc.c"
           "postgresql/src/interfaces/libpq/fe-print.c"
           "postgresql/src/interfaces/libpq/fe-protocol2.c"
           "postgresql/src/interfaces/libpq/fe-protocol3.c"
           "postgresql/src/interfaces/libpq/fe-secure-common.c"
           "postgresql/src/interfaces/libpq/fe-secure-gssapi.c"
           "postgresql/src/interfaces/libpq/fe-secure-openssl.c"
           "postgresql/src/interfaces/libpq/fe-secure.c"
           "postgresql/src/interfaces/libpq/legacy-pqsignal.c"
           "postgresql/src/interfaces/libpq/libpq-events.c"
           "postgresql/src/interfaces/libpq/pqexpbuffer.c"
           "postgresql/src/interfaces/libpq/pthread-win32.c"
           "postgresql/src/interfaces/libpq/win32.c"]
 :cflags @[
           "-Ipostgresql/build/include"
           "-Ipostgresql/build/include/internal"
           "-Ipostgresql/build/include/server"
           "-Ipostgresql/src/port"
           ])

(defn clean-pg []
  (os/shell "cd postgresql && rm -rf build && make distclean"))

(defn build-pg []
  (os/shell "cd postgresql && ./configure --prefix=$(pwd)/build")
  (os/shell "cd postgresql && make -C src/bin install")
  (os/shell "cd postgresql && make -C src/include install")
  (os/shell "cd postgresql && make -C src/interfaces install")
  (os/shell "cd postgresql && make -C doc install"))
