
(emit '(load "./ms.scm"))

(emit '
(define (tst-element elm-name)
  (let* ((e  (ms-make-element elm-name))
         (g2 (ms-mtest-writeReadElement e)))
    (if (= (ms-element-type e) (ms-element-type g2))
        (ms-test-check-pass)
        (ms-test-check-fail)))))

(loop-list (elm-name %element-types)
  (emit (format #f "~%; test element name: ~s~%" elm-name))
  (emit (list 'tst-element (list 'quote elm-name))))
