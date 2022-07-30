(load "./lib.scm")
(load "./ms.scm")
(load "./types.scm")

(define (check-write-read-elm elm fun val)
  (let ((e (ms-mtest-writeReadElement elm))) ; Elm e = static_cast<Elm*>(writeReadElement(elm))
    (if (number? val)
      ; QCOMPARE(e->pitch(), val) where fun: getter for note->pitch
      (ms-test-check (= val (fun e)))
      (ms-test-check (eq? val (fun e)))))) ; QCOMPARE(e->pitch(), val) where fun: getter for note->pitch

(register-c-type %hairpin-type)
