
(define-syntax emit
  (syntax-rules ()
    ; no variables
    ((_ x)
     (emit-body x))))

(define-syntax ecall
  (syntax-rules ()
    ; no variables
    ((_ (funname . args))
     (begin
       (format %emit-port "(~a " 'funname)
       (begin . args)
       (format %emit-port ")")))))

(define-syntax elet
  (syntax-rules ()
    ; no variables
    ((_ () b1 b2 ...)
     (begin
       (format %emit-port "(begin~%")
       (begin b1 b2 ...)
       (format %emit-port "  )~%")))
    ; one variable
    ((_ ((i1 e1)) b1 b2 ...)
     (begin
       (format %emit-port "(let ((~a " 'i1) e1 (format %emit-port "))~%")
       (begin b1 b2 ...)
       (format %emit-port "  )~%")))
    ; two or more variables
    ((_ ((i1 e1) (i2 e2) ...) b1 b2 ...)
     (elet ((i1 e1))
       (elet ((i2 e2) ...) b1 b2 ...)))
    ))

(define-syntax elet*
  (syntax-rules ()
    ((_ () b1 b2 ...) (begin b1 b2 ...))
    ((_ ((i1 e1) (i2 e2) ...) b1 b2 ...)
     (elet ((i1 e1))
       (elet* ((i2 e2) ...) b1 b2 ...)))))

