
(emit '(load "./mtest.scm"))

; check subtype for hairpin
(emit '
(let ((hp (ms-make-hairpin)))
  (for-each (lambda (type)
    (set! (ms-hairpin-hairpinType hp) type)
    (check-write-read-elm hp ms-hairpin-hairpinType type))
    '(hairpin-DECRESC_HAIRPIN hairpin-CRESC_HAIRPIN))))

