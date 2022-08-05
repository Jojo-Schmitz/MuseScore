(define-syntax match1
  (syntax-rules ()
    ((_ form (pattern . body))
     (apply (lambda pattern . body) form))))

(define %export-to-scheme-getset '()) ; get/setters
(define %export-to-scheme-getset2 '()) ; get/setters
(define %export-to-scheme-get '())    ; getters only
(define %export-to-scheme2 '())

(define %objects '())


(define-syntax define-object
  (syntax-rules ()
    ((_ . args)
     (register-object 'args))))

(define (register-object args)
  (let ((obj-name (car args))
        (c-type #f)
        (make-args #f)
        (methods #f))
    (set! args (cdr args))
    (for-each (lambda (arg)
           (cond
            ((eq? 'c-type (car arg)) (set! c-type (cadr arg)))
            ((eq? 'make-args (car arg)) (set! make-args (cadr arg)))
            ((eq? 'methods   (car arg)) (set! methods (cdr arg)))))
         args)
    (for-each (lambda (meth)
                (format #t "    method: ~s~%" meth)
                )
              methods)
    (format #t "registering object ~a~%" obj-name)
    (set! %objects
          (assoc-set %objects obj-name
                     `((c-type . ,c-type)
                       (make-args . ,make-args)
                       (methods . ,methods))))))

(define (emit-registered-object pair)
  (let* ((name (car pair))
         (args (cdr pair))
         (c-type (assq-ref args 'c-type))
         (make-args (assq-ref args 'make-args))
         (methods (assq-ref args 'methods)))
    (define (emit-method-nil methname methargs)
      (let ((sname (string->symbol (format #f "ms-~a-~a" name methname)))
            (gootype 0)
            (methexpr (if (null? methargs)
                        (format #f "o->~a()" methname)
                        (car methargs))))
        (emit-cfun `(,sname) 1 (list
          `(emit-pop-arg-goo ,(format #f "~a*" c-type) "o")
          (lambda (e) (format %c "
    ~a;
    return s7_t(sc);~%" methexpr))))))
    (define (emit-method-get methname methargs)
      (let* ((rets (car methargs))
             (args (cdr methargs))
             (methtype (car rets))
             (sname (string->symbol (format #f "ms-~a-~a" name methname)))
             (gootype (if (and (pair? (cdr rets)) (pair? (cddr rets)))
                        (format #f "GOO_TYPE::~a" (caddr rets))
                        0))
             (methexpr (if (null? args)
                         (format #f "o->~a()" methname)
                         (car args))))
        (emit-cfun `(,sname) 1 (list
          `(emit-pop-arg-goo ,(format #f "~a*" c-type) "o")
          (lambda (e)
            (format %c "    ~a x = ~a;" methtype methexpr))
          `(emit-return-goo "x" ,(format #f "static_cast<uint64_t>(~a)" gootype))))))
    (define (emit-method-get/set methname methargs)
      (let* ((rets (car methargs))
             (args (cdr methargs))
             (methtype (car rets))
             (cmethtype (if (eq? 'real methtype) 'double methtype))
             (methname-set (let ((s (symbol->string methname)))
                             (format #f "set~a~a"
                                     (string-upcase (substring s 0 1))
                                     (substring s 1))))
             (sname-get (string->symbol (format #f "ms-~a-~a" name methname)))
             (sname-set (string->symbol (format #f "ms-~a-~a" name methname-set)))
             (cname-get (string->symbol (format #f "ms_~a_~a" name methname)))
             (cname-set (string->symbol (format #f "ms_~a_~a" name methname-set)))
             (gootype (if (and (pair? (cdr rets)) (pair? (cddr rets)))
                        (format #f "GOO_TYPE::~a" (caddr rets))
                        0))
             (methexpr (if (null? args)
                         (format #f "o->~a()" methname)
                         (car args))))
        (set! %export-to-scheme-getset2 (cons (list sname-get cname-get sname-set cname-set)
                                              %export-to-scheme-getset2))
        ; emit getter
        (emit-cfun `(,sname-get) '(1 no-register) (list
          `(emit-pop-arg-goo ,(format #f "~a*" c-type) "o")
          `(emit-stat ,(format #f "~a x = ~a" cmethtype methexpr))
          (case (car rets)
           ((int)  '(emit-stat "return s7_make_integer(sc, x)"))
           ((real) '(emit-stat "return s7_make_real(sc, x)"))
           ((goo)  `(emit-return-goo "x" ,(format #f "static_cast<uint64_t>(~a)" gootype)))
           (else (error "unknown getter return type" (car rets))))))
        ; emit setter
        (emit-cfun `(,sname-set) '(1 no-register) (list
          `(emit-pop-arg-goo ,(format #f "~a*" c-type) "o")
          '(emit-next-arg)
          (case (car rets)
            ((int)  '(emit-pop-arg-int "x"))
            ((real) '(emit-pop-arg-real "x"))
            ((goo)  `(emit-pop-arg-goo ,(format #f "~a*" c-type) "x")) ; FIX: type not known
            (else (error "unknown getter return type" (car rets))))
          `(emit-stat ,(format #f "o->~a(x)" methname-set))
          '(emit-stat "return s7_t(sc)")))))
    (define (emit-method-apply methname methargs)
      (let* ((meth-sname (if (pair? methname)
                           (cadr methname)
                           methname))
             (methname (if (pair? methname)
                         (car methname)
                         methname))
             (sname (string->symbol (format #f "ms-~a-~a" name meth-sname)))
             (margs (car methargs))
             (mrest (cdr methargs))
             (ret-type (if (pair? mrest) (car mrest) #f))
             (gootype 0)
             (methexpr #f)
             (argsstr "")
             (callargs (let ((n 0))
                         (map (lambda (arg)
                                (set! n (+ 1 n))
                                (append (list n) arg))
                            margs))))
        (emit-cfun `(,sname) (+ 1 (length margs))
          (list `(emit-pop-arg-goo ,(format #f "~a*" c-type) "o")
                (map (lambda (callarg)
                       (let* ((n (car callarg))
                              (arg (cdr callarg))
                              (cref (and (pair? (cdr arg))
                                         (eq? 'deref (cadr arg)))))
                         (set! argsstr (format #f "~a~a~ax~a"
                                               argsstr (if (= n 1) "" ", ")
                                               (if cref "*" "") ; c-ref(&), need to ref pointer
                                               n))
                         (list
                          '(emit-next-arg)
                          (case (car arg)
                            ((int) `(emit-pop-arg-int ,(format #f "x~a" n)))
                            ((sym)
                             (let ((stype (cadr arg)))
                               (list
                                `(emit-pop-arg-sym ,(format #f "t~a" n))
                                (lambda (e) (format %c "    Ms::~a x~a = string_to_~a(t~a);~%" stype n stype n) e))))
                            (else `(emit-pop-arg-goo ,(car arg) ,(format #f "x~a" n)))))))
                     callargs)
                (if ret-type
                  (lambda (e) (format %c "    ~a r = o->~a(~a);~%" (car ret-type) methname argsstr))
                  (lambda (e) (format %c "    o->~a(~a);~%" methname argsstr)))
                ; emit return, either true/false or an object
                (if ret-type
                  `(emit-return-goo "r" ,(format #f "static_cast<uint64_t>(~a)"
                                          (if (pair? (cdr ret-type))
                                            (cadr ret-type)
                                            0)))
                  (lambda (e) (format %c "    return s7_t(sc);~%")))))))
    (for-each (lambda (meth)
                (match1 meth
                 ((typeof methname . methargs)
                  (case typeof
                   ((nil)     (emit-method-nil methname methargs))
                   ((get)     (emit-method-get methname methargs))
                   ((get/set) (emit-method-get/set methname methargs))
                   ((apply)   (emit-method-apply methname methargs))))))
              methods)))

(define (emit-registered-objects)
  (for-each emit-registered-object %objects))

; generate a .h and .c file, write to these files
(define %h (open-output-file "s7gen.h" "w"))
(define %c (open-output-file "s7gen.cpp" "w"))

(define (emit-string-to-ctype)
  (format %h "int string_to_ctype (const char *sname);~%")
  (format %c "
int string_to_ctype (const char *sname)
{
")
  (map (lambda (lst)
         (let ((typename (car lst))
               (types (cdr lst)))
           (map (lambda (lst)
                  (match lst
                    ((sname cname idx)
                     (format %c "    if (!strcmp(sname, \"~s\")) {~%" sname)
                     (format %c "        return (int)~a;~%" cname)
                     (format %c "    }~%"))))
                types)))
       %c-types)
  (format %c "    return -999999;    ~%}~%"))

(define (emit-header-getset objname memname)
  (format %h "s7_pointer ms_~a_~a (s7_scheme *sc, s7_pointer args);~%" objname memname)
  (format %h "s7_pointer ms_set_~a_~a (s7_scheme *sc, s7_pointer args);~%" objname memname))

; write the beginning of a c-function. FIX: (with-c-fun (args) body) would be better
(define (emit-func-prolog setter objname memname objtype)
  (format %c "
s7_pointer ms_~a~a_~a (s7_scheme *sc, s7_pointer args)
{
    goo_t *g = (goo_t *)s7_c_object_value(s7_car(args));
    ~a* o = (~a*) g->cd;
" (if setter "set_" "") objname memname objtype objtype))

(define (maybe-emit-args-check e)
  (unless (assis? 'argsPairKnow e)
    (format %c "    if (!s7_is_pair(args)) return s7_f(sc);~%")
    (set! e (assoc-set e 'argsPairKnow #t)))
  e)

(define (emit-pop-arg-sym args e)
  (let ((cargname (car args)))
    (format %c "~%    // emit-pop-arg-sym ~a~%" cargname)
    (set! e (maybe-emit-args-check e))
    (format %c "
    ~as = s7_car(args);
    if (!s7_is_symbol(s)) return s7_f(sc);
    const char* ~a = s7_symbol_name(s);
" (if (assis? 's e) "" "s7_pointer ") cargname)
    (set! e (assoc-set e 's #t))
    e))

(define (emit-pop-arg-int args e)
  (let ((cargname (car args)))
    (format %c "~%    // emit-pop-arg-int ~a~%" cargname)
    (set! e (maybe-emit-args-check e))
    (format %c "
    ~as = s7_car(args);
    if (!s7_is_integer(s)) return s7_f(sc);
    int ~a = s7_integer(s);
" (if (assis? 's e) "" "s7_pointer ") cargname)
    (set! e (assoc-set e 's #t))
    e))

(define (emit-pop-arg-real args e)
  (let ((cargname (car args)))
    (format %c "~%    // emit-pop-arg-real ~a~%" cargname)
    (set! e (maybe-emit-args-check e))
    (format %c "
    ~as = s7_car(args);
    if (!s7_is_real(s)) return s7_f(sc);
    double ~a = s7_real(s);
" (if (assis? 's e) "" "s7_pointer ") cargname)
    (set! e (assoc-set e 's #t))
    e))

(define (emit-pop-arg-goo args e)
  (let ((type (if (pair? args) (car args) #f))
        (name (if (and (pair? args) (pair? (cdr args))) (cadr args) #f))
        (cvar (if (and (pair? args) (pair? (cdr args)) (pair? (cddr args))) (caddr args) "g"))
        (cvarg (and (pair? args) (pair? (cdr args)) (pair? (cddr args)))))
    (format %c "~%    // emit-pop-arg-goo ~a (~a) => ~a~%" name type cvar)
    (set! e (maybe-emit-args-check e))
    (format %c "
    ~as = s7_car(args);
    if (! c_is_goo(sc, s)) return s7_f(sc);
    ~a~a = (goo_t *)s7_c_object_value(s);
" (if (assis? 's e) "" "s7_pointer ") (if (and (not cvarg) (assis? 'g e)) "" "goo_t* ") cvar)
    (set! e (assoc-set e 's #t))
    (unless cvarg
      (set! e (assoc-set e 'g #t)))
    (if (and type name)
      (format %c "    ~a ~a = (~a) ~a->cd;~%"
              type name type cvar)))
  e)

(define (emit-next-arg args e)
  (format %c "
    // emit-next-arg
    args = s7_cdr(args);
")
  (assoc-set e 'argsPairKnow #f)) ; resets checked-if-end-of-list flag

(define (emit-return-goo args e)
  (let ((varname (car args))
        (typename (cadr args)))
    (format %c "
    uint64_t ty = ~a;
    return c_make_goo(sc, ty, s7_f(sc), (void*) ~a);~%" typename varname)
  e))

(define (emit-stat args e)
  (format %c "    ~a;~%" (car args))
  e)

(define (c-ify-name sname)
  (let ((str "")
        (sname (symbol->string sname)))
    (do ((i 0 (+ 1 i)))
        ((>= i (string-length sname)))
      (set! str (format #f "~a~a" str
                        (let ((s (substring sname i (+ i 1))))
                          (if (string=? s "-") "_" s)))))
    str))

(define (emit-inst-eval-form form e)
  (cond
   ((pair? form)
    (let ((fun-name (car form)))
      ((symbol->value fun-name) (cdr form) e)
      ))
   (else (form e))))

(define (emit-inst-eval forms e)
  (when (pair? forms)
    (let ((form (car forms)))
      (emit-inst-eval (cdr forms)
                      (emit-inst-eval-form form e)))))

(define (emit-cfun-eval sname cname arity body)
  (define (flatten-inst lst)
    (let ((new '()))
      (define (iter form)
        (cond
         ((and (pair? form) (symbol? (car form)))
          (set! new (cons form new)))
         ((pair? form)
          (for-each iter form))
         (else
          (set! new (cons form new)))))
      (iter lst)
      (reverse new)))
  (let ((body (flatten-inst body)))
    (emit-inst-eval (append (list '(emit-prolog)) body)
                    `((sname . ,sname) (cname . ,cname) (arity . ,arity)
                      (s . #f) (g . #f)))))

(define (emit-cfun names args body)
  (let* ((arity (if (pair? args) (car args) args))
         (register (not (and (pair? args) (eq? 'no-register (cadr args)))))
         (sname (car names))
         (cname (if (null? (cdr names)) #f (cadr names)))
         (cname (or cname (c-ify-name sname))))
    (if register
      (set! %export-to-scheme2
            (cons (list sname cname arity)
                  %export-to-scheme2)))
    (format %h "s7_pointer ~a (s7_scheme *sc, s7_pointer args);~%" cname)
    (format %c "s7_pointer ~a (s7_scheme *sc, s7_pointer args)~%{~%" cname)
    (if (pair? body)
      (emit-cfun-eval sname cname arity body)
      (body))
    (format %c "}~%~%")))

(define (emit-setenv args e)
  (let ((key (car args))
        (val (cadr args)))
    (assoc-set e key val)))

(define (emit-getenv e key)
  (cdr (assoc key e)))

(define (emit-prolog args e)
  (unless (assis? 'c-prolog e)
    (let ((arity (assoc 'arity e)))
      (if arity (set! arity (cdr arity)))
      ;(if (and arity (> arity 0)) ; args are used, emit check that ensures they are provided
      ;  (format %c "    if (!s7_is_pair(args)) return s7_f(sc);~%"))
      (set! e (cons (cons 'c-prolog #t) e))))
  e)

(define (def-goo-setters-goo objtype objname getname setname operandtype)
  (let ((sname-getname (string->symbol (format #f "ms-~a-~a" objname getname)))
        (sname-setname (string->symbol (format #f "ms-~a-~a" objname setname))))
    (emit-cfun (list sname-getname) 1 (list
      `(emit-pop-arg-goo ,objtype "o")
      (lambda (e) (format %c "    ~a y = o->~a();~%" operandtype getname))
      `(emit-return-goo "y" 0)))
    (emit-cfun (list sname-setname) 2 (list
      `(emit-pop-arg-goo ,objtype "o" "go")
      '(emit-next-arg)
      `(emit-pop-arg-goo ,operandtype "x" "gx")
      (lambda (e) (format %c "    o->~a(x);
    return s7_t(sc);" setname))))))

(define-syntax def-goo-setters-bool
  (syntax-rules ()
    ((k objtype objname memname memnameget memnameset)
     (begin
       (set! %export-to-scheme-getset (cons (list objname memname memnameset)
                                            %export-to-scheme-getset))
       (emit-header-getset objname memname)
       (emit-func-prolog #f objname memname objtype)
       (format %c "
    return s7_make_boolean(sc, o->~a());
}
" memnameget)
       (emit-func-prolog #t objname memname objtype)
       (format %c "
  s7_pointer x = s7_cadr(args);
  o->~a(s7_boolean(sc, x));
  return x;
}
" memnameset)))))

; FIX: the transname should be a symbol that references into %c-types-info
(define-syntax def-goo-setters-sym
  (syntax-rules ()
    ((k objtype objname memname memnameset transname)
     (begin
       (set! %export-to-scheme-getset (cons (list objname memname memnameset)
                                            %export-to-scheme-getset))
       (emit-header-getset objname memname)
       (emit-func-prolog #f objname memname objtype)
       (format %c "
    return s7_make_symbol(sc, ~a_to_string(o->~a()));
}
" transname memname)
       (when memnameset
         (emit-func-prolog #t objname memname objtype)
         (format %c "
  s7_pointer x = s7_cadr(args);
  o->~a(string_to_~a(s7_symbol_name(x)));
  return x;
}
" memnameset transname))))))

(define (emit-c-type-string-maps name typelst basetypename)
  (let ()
    (format %h "const char* ~a_to_string (~a x);~%" name basetypename)
    (format %c "const char* ~a_to_string (~a x)~%" name basetypename)
    (format %c "{~%    switch (x) {~%")
    (map (lambda (lst)
                (match lst
                  ((symname c-type)
                   (format %c "        case ~a:~%" c-type)
                   (format %c "        return \"~s\";~%" symname))))
         typelst)
    (format %c "    fprintf(stderr, \"WARNING: Unknown ctype-int %i\\n\", (int) c-type);~%")
    (format %c "    }~%}~%")

    (format %h "~a string_to_~a (const char *name);~%" basetypename name)
    (format %c "~a string_to_~a (const char *name)~%" basetypename name)
    (format %c "{~%~%")
    (map (lambda (lst)
                (match lst
                  ((symname c-type)
                   (format %c "    if (!strcmp(name, \"~s\")) {~%" symname)
                   (format %c "        return ~a;~%" c-type)
                   (format %c "    }~%"))))
         typelst)
    (format %c "    fprintf(stderr, \"WARNING: Unknown ctype-str %s\\n\", name);~%")
    (format %c "    return (~a)0;~%" basetypename)
    (format %c "}~%")))

(define (emit-c-type-string-maps-simple name typelst basetypename)
  (let ()
    ; emit c-function that takes a object of a specific type and returns a string
    (format %h "const char* ~a_to_string (~a x);~%" name basetypename)
    (format %c "const char* ~a_to_string (~a x)~%" name basetypename)
    (format %c "{~%    switch (x) {~%")
    (for-each (lambda (symname)
      (format %c "        case ~a::~a:~%" basetypename symname)
      (format %c "        return \"~a\";~%" symname))
      typelst)
    (format %c "    }~%}~%")
    ; emit c-function that takes an string and returns an object of a specific type
    (format %h "~a string_to_~a (const char *name);~%" basetypename name)
    (format %c "~a string_to_~a (const char *name)~%" basetypename name)
    (format %c "{~%~%")
    (for-each (lambda (symname)
      (format %c "    if (!strcmp(name, \"~a\")) {~%" symname)
      (format %c "        return ~a::~a;~%" basetypename symname)
      (format %c "    }~%"))
      typelst)
    (format %c "    return (~a)0;~%" basetypename)
    (format %c "}~%")))

(define (emit-c-type-string-maps2 typename)
  (let* ((types (assq-ref %c-types typename))
         (typeinfo (assq-ref %c-types-info typename))
         (c-type (car typeinfo)))
    ;(set! c-type (substring c-type 0 (string-position ":" c-type)))
    ; emit c-function that takes a object of a specific type and returns a string
    (format %h "const char* ~a_to_string (Ms::~a x);~%" typename c-type)
    (format %c "const char* ~a_to_string (Ms::~a x)~%" typename c-type)
    (format %c "{~%    switch (x) {~%")
    (for-each (lambda (lst)
      (match lst
        ((sname cname idx)
          (format %c "        case ~a:~%" cname)
          (format %c "        return \"~a\";~%" sname))))
      types)
    (format %c "        default:~%")
    (format %c "        return \"\";~%")
    (format %c "    }~%}~%")
    ; emit c-function that takes an string and returns an object of a specific type
    (format %h "Ms::~a string_to_~a (const char *name);~%" c-type typename)
    (format %c "Ms::~a string_to_~a (const char *name)~%" c-type typename)
    (format %c "{~%~%")
    (for-each (lambda (lst)
      (match lst
        ((sname cname idx)
         (format %c "    if (!strcmp(name, \"~a\")) {~%" sname)
         (format %c "        return ~a;~%" cname)
         (format %c "    }~%"))))
      types)
    (format %c "    return (Ms::~a)0;~%" c-type)
    (format %c "}~%")))

(define (emit-exports-getset)
  (for-each (lambda (lst)
    (match lst
      ((sname-get cname-get sname-set cname-set)
       (format %c "s7_define_variable(sc, \"~a\", s7_dilambda(sc, \"~a\", ~a, 1, 0, ~a, 2, 0, \"\"));~%"
        sname-get sname-get cname-get cname-set))))
  %export-to-scheme-getset2))

(define (emit-exports)
  (for-each (lambda (lst)
    (match lst
      ((sname cname arity)
       (format %c "s7_define_function(sc, \"~a\", ~a, ~a, 0, false, \"(~a)\");~%" sname cname arity sname))))
    %export-to-scheme2))

(define (gen-done) 
  (format #t "closing output port~%")
  (close-output-port %h)
  (close-output-port %c))
