
(define %sourcedir #f)
(define %libdir #f)
(define %libdir-mtest #f)
(define %output-file #f)
(define %source-files '())

(define %directive #f)

(let ((sarg #f))
  (do ((pair %command-line (cdr pair)))
      ((eq? '() pair))
    (let ((arg (car pair)))
      (cond
       ((string=? "-l" arg)
        (set! pair (cdr pair))
        (set! %libdir (car pair)))
       ((string=? "-s" arg)
        (set! pair (cdr pair))
        (set! %sourcedir (car pair)))
       ((string=? "-o" arg)
        (set! pair (cdr pair))
        (set! %output-file (car pair)))
       ((string=? "--mtest-gen" arg)
        (set! %directive 'mtest-gen))
       ((string=? "--" arg)
        (set! sarg #t))
       (sarg
        (set! %source-files (cons arg %source-files)))))))
(set! %source-files (reverse %source-files))
(set! %libdir-mtest (format #f "~a/../mtest" %libdir))

(format #t "---- generate scheme test code ----~%")
(format #t "-- libdir: ~s~%" %libdir)
(format #t "-- sourcedir: ~s~%" %sourcedir)
(format #t "-- source-files: ~s~%" %source-files)
(format #t "-- output-file: ~s~%" %output-file)

(if (not %libdir) (error "S7 LIBDIR not found"))

(format #t "-- spicing up the scheme interpreter~%")
(load (format #f "~a/psyntax-s7.scm" %libdir))
(load (format #f "~a/psyntax-expanded.ss" %libdir))

(define (psyntax-eval form)
  (let ((ex (sc-expand form)))
    (eval ex (rootlet))))

(define (psyntax-load file)
  (with-input-from-file file
    (lambda () 
      (do ((c (read) (read))) 
          ((eof-object? c)) 
        (psyntax-eval c)))))

; scheme extensions
(psyntax-load (format #f "~a/plib.scm" %libdir))

(when (not (eq? 'mtest-gen %directive))
  ; mtest helpers
  (psyntax-load (format #f "~a/mtestgen.scm" %libdir-mtest))

  ; musescore data structures definitions
  (load "ms.scm")
  )

(define %emit-port #f)

(define (emit-body form)
  (cond
   ((proper-list? form)
    (write-string "(" %emit-port)
    (do ((pair form (cdr pair)))
        ((eq? '() pair))
      (let ((item (car pair)))
        (emit-body item)
        (write-string " " %emit-port)))
    (write-string ")" %emit-port))
   ((string? form)
     (write-string "\"" %emit-port)
     (write-string form %emit-port)
     (write-string "\"" %emit-port))
   (else
    (write form %emit-port))))

; generate output file
(when (not (eq? 'mtest-gen %directive))
  (set! %emit-port (open-output-file %output-file "w")))

(format #t "-- loading source files~%")

(do ((pair %source-files (cdr pair)))
    ((eq? '() pair))
  (if (eq? 'mtest-gen %directive)
    (let ((file (format #f "~a/~a" %sourcedir (car pair))))
      (format #t "-- loading source file ~s~%" file)
      (psyntax-load file))
    (psyntax-load (car pair))))

(when (not (eq? 'mtest-gen %directive))
  (close-output-port %emit-port))

(format #t "---- scheme generator done ----~%")
