;;; The following set of definitions establishes bindings for the
;;; top-level variables assigned values in the let expression below.
;;; Uncomment them here and copy them to the front of psyntax.pp if
;;; required by your system.

;;; ($sc-put-cte symbol val top-token)
;;; used to establish top-level compile-time (expand-time) bindings.
(define $sc-put-cte #f)
(define sc-expand #f)
(define $make-environment #f)
(define environment? #f)
(define interaction-environment #f)
(define identifier? #f)
(define syntax->list #f)
(define syntax->vector #f)
(define syntax-object->datum #f)
(define datum->syntax-object #f)
(define generate-temporaries #f)
(define free-identifier=? #f)
(define bound-identifier=? #f)
(define literal-identifier=? #f)
(define syntax-error #f)
(define $syntax-dispatch #f)

;;; -----------------------------------

(define void (lambda () (if #f #f)))

(define andmap
  (lambda (f first . rest)
    (or (null? first)
        (if (null? rest)
            (let andmap ((first first))
              (let ((x (car first)) (first (cdr first)))
                (if (null? first)
                    (f x)
                    (and (f x) (andmap first)))))
            (let andmap ((first first) (rest rest))
              (let ((x (car first))
                    (xr (map car rest))
                    (first (cdr first))
                    (rest (map cdr rest)))
                (if (null? first)
                    (apply f (cons x xr))
                    (and (apply f (cons x xr)) (andmap first rest)))))))))
(define ormap
  (lambda (proc list1)
    (and (not (null? list1))
         (or (proc (car list1)) (ormap proc (cdr list1))))))

;;; (putprop symbol key value)
;;; (getprop symbol key)
;;; (remprop symbol key)
;;; key is always a symbol; value may be any object.  putprop should
;;; associate the given value with the given symbol and key in some way
;;; that it can be retrieved later with getprop.  getprop should return
;;; #f if no value is associated with the given symbol and key.  remprop
;;; should remove the association between the given symbol and key.

(define %env '())

(define (putprop symbol key value)
  (let ((kc (cons symbol key)))
    (if (assoc kc %env)
      (set! (cdr (assoc kc %env)) value)
      (set! %env (cons (cons kc value) %env)))
    0))

(define (getprop symbol key)
  (if (assoc (cons symbol key) %env)
    (cdr (assoc (cons symbol key) %env))
    #f))

(define (remprop symbol key)
  (let ((new '()))
    (do ((pair %env (cdr pair)))
        ((eq? '() pair))
      (if (not (equal? (caar pair) (cons symbol key)))
          (set! new (cons (car pair) new))))
    (set! %env new)
    0))

