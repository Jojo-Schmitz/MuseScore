
(define %c-types '())

(define (register-c-type lst)
  (let* ((start-index (or (assq-ref lst 'start-index) 0))
         (name        (assq-ref lst 'name))
         (c-type      (assq-ref lst 'c-type))
         (types       (assq-ref lst 'types)))
    (let ((typelst (or (assq-ref %c-types name) '()))
          (idx start-index))
      (do ((pair types (cdr pair)))
          ((eq? '() pair))
        (let* ((type (car pair))
               (typename (string->symbol (format #f "~a-~a" name type)))
               (c-typename (format #f "~a::~a" c-type type)))
          (format #t "register c-type: ~s ~s ~s~%"  typename c-typename idx)
          (set! typelst (cons (list typename c-typename idx) typelst))
          (set! idx (+ idx 1))))
      (if (assq-ref %c-types name)
        (let ((pair (assoc name %c-types)))
          (set-cdr! pair typelst))
        (set! %c-types (cons (cons name typelst) %c-types))))))

(define (typeinfo-get name type what)
  (let ((types (assq-ref %c-types name)))
    (if types
      (let ((typelst (assq-ref types type)))
        (if typelst
          (cond
           ((eq? 'c-typename what) (car typelst))
           ((eq? 'index what) (cadr typelst)))
          f))
      #f)))
