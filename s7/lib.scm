(define-macro (match expr case1)
  (let ((lst (car case1))
        (body (cdr case1)))
  `(let ((ex ,expr))
     (define (flatten lst)
       (cond ((null? lst) ())
             ((pair? lst)
              (if (pair? (car lst))
                  (append (flatten (car lst)) (flatten (cdr lst)))
                  (cons (car lst) (flatten (cdr lst)))))
             (#t lst)))

     (define (structures-equal? l1 l2)
       (if (pair? l1)
           (and (pair? l2)
                (structures-equal? (car l1) (car l2))
                (structures-equal? (cdr l1) (cdr l2)))
           (not (pair? l2))))

     (if (not (structures-equal? ',lst ex))
         (error 'test-error "~A and ~A do not match" ',lst ex))

     (let ((names (flatten ',lst))
           (vals (flatten ex)))
       (apply (eval (list 'lambda names '(begin ,@body))) vals)))))

(define (assq-ref lst key)
  (if (assoc key lst)
      (cdr (assoc key lst))
      #f))

(define (remove-if proc lst)
  (map (lambda (x) (if (proc x) (values) x)) lst))

(define (filter proc lst)
  (map (lambda (x) (if (proc x) x (values))) lst))

(define (assis? key lst)
  (let ((pair (assoc key lst)))
    (if pair
        (cdr pair)
        #f)))

(define (assoc-set lst key val)
  (let ((pair (assq key lst)))
    (cond
     (pair
      (set-cdr! pair val)
      lst)
     (else
      (cons (cons key val) lst)))))

(define (list-nth lst index)
  (if (> (length lst) index)
      (list-ref lst index)
      #f))
