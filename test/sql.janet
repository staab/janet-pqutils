(use build/sql)
(use staab.assert/assert)

(assert= "<pg/identifier stuff>" (string/format "%q" (identifier "stuff")))
(assert= "<pg/literal stuff>" (string/format "%q" (literal "stuff")))
(assert= "<pg/unsafe stuff>" (string/format "%q" (unsafe "stuff")))

(pp (stringify (composite (unsafe "stuff") (literal "x"))))
