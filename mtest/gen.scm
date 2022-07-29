
(define %export-to-scheme '())
(define %export-to-scheme2 '())

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
(define (emit-func-epilog setter objname memname objtype)
  (format %c "
s7_pointer ms_~a~a_~a (s7_scheme *sc, s7_pointer args)
{
    goo_t *g = (goo_t *)s7_c_object_value(s7_car(args));
    ~a* o = (~a*) g->cd;
" (if setter "set_" "") objname memname objtype objtype))

(define (c-ify-name sname)
  (let ((str "")
        (sname (symbol->string sname)))
    (do ((i 0 (+ 1 i)))
        ((>= i (string-length sname)))
      (set! str (format #f "~a~a" str
                        (let ((s (substring sname i (+ i 1))))
                          (if (string=? s "-") "_" s)))))
    str))

(define (emit-cfun names arity body)
  (let* ((sname (car names))
         (cname (if (null? (cdr names)) #f (cadr names)))
         (cname (or cname (c-ify-name sname))))
    (set! %export-to-scheme2
          (cons (list sname cname arity)
                %export-to-scheme2))
    (format %h "s7_pointer ~a (s7_scheme *sc, s7_pointer args);~%" cname)
    (format %c "s7_pointer ~a (s7_scheme *sc, s7_pointer args) {" cname)
    (newline %c)
    (body)
    (format %c "}~%~%")))

(define (emit-goo-setters objtype objname memname memnameset type)
  (let ()
     (set! %export-to-scheme (cons (list objname memname)
                                   %export-to-scheme))
     (emit-header-getset objname memname)
     (emit-func-epilog #f objname memname objtype)
     (format %c "
  return s7_make_~a(sc, o->~a());
}
" type memname)
     (emit-func-epilog #t objname memname objtype)
     (format %c "
    s7_pointer x = s7_cadr(args);
    o->~a(s7_~a(x));
    return x;
}
" memnameset type)))

(define-syntax def-goo-setters
  (lambda (x)
    (define (make-setter name memname)
      (or name
          (format #f "set~a~a"
                  (string-upcase (substring memname 0 1))
                  (substring memname 1))))
    (define (select-emitter objtype objname memname memnameset type)
      (case type
        ((integer real)
         `(emit-goo-setters ,objtype ,objname ,memname ,memnameset ',type))
        (else
         (error "UNKNOWN goo-get/set type: " 'type))))
    (syntax-case x ()
      ((k objtype objname memname memnameset type)
       (let ((n1 (syntax-object->datum (syntax objtype)))
             (n2 (syntax-object->datum (syntax objname)))
             (n3 (syntax-object->datum (syntax memname)))
             (n4 (syntax-object->datum (syntax memnameset)))
             (n5 (syntax-object->datum (syntax type))))
         (with-syntax ((memnameset2 (datum->syntax-object (syntax k)
                                       (make-setter n4 n3))))
         (let ((nm (syntax-object->datum (syntax memnameset2))))
         (with-syntax ((fun (datum->syntax-object (syntax k)
                                       (select-emitter n1 n2 n3 nm n5))))
           (syntax fun)))))))))

(define-syntax def-goo-setters-bool
  (syntax-rules ()
    ((k objtype objname memname memnameget memnameset)
     (begin
       (set! %export-to-scheme (cons (list objname memname)
                                     %export-to-scheme))
       (emit-header-getset objname memname)
       (emit-func-epilog #f objname memname objtype)
       (format %c "
    return s7_make_boolean(sc, o->~a());
}
" memnameget)
       (emit-func-epilog #t objname memname objtype)
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
       (set! %export-to-scheme (cons (list objname memname)
                                     %export-to-scheme))
       (emit-header-getset objname memname)
       (emit-func-epilog #f objname memname objtype)
       (format %c "
    return s7_make_symbol(sc, ~a_to_string(o->~a()));
}
" transname memname)
       (emit-func-epilog #t objname memname objtype)
       (format %c "
  s7_pointer x = s7_cadr(args);
  o->~a(string_to_~a(s7_symbol_name(x)));
  return x;
}
" memnameset transname)))))

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
