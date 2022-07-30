
;
; initiate c and h file
;

(format %h "// WARNING: this file is generated~%")
(format %c "// WARNING: this file is generated
#include <cstdlib>
#include <cstdio>
#include \"s7/s7.h\"
#include \"s7/utils.h\"

#include <QtTest/QtTest>
#include \"libmscore/score.h\"
#include \"libmscore/undo.h\"
#include \"libmscore/measure.h\"
#include \"libmscore/element.h\"
#include \"libmscore/sym.h\"
#include \"libmscore/hairpin.h\"
#include \"libmscore/breath.h\"
#include \"mtest/testutils.h\"
#include \"s7gen.h\"

const char* my_s7_get_car_as_string (s7_pointer lst);

using namespace Ms;

extern Ms::MTest* g_mtest;
")

(format %h "void init_gen_s7 (s7_scheme *sc);~%~%")

;
;
;

(format %h "enum class GOO_TYPE : uint64_t {~%")
(format %h "  NIL = 0,~%")
(for-each (lambda (name)
  (format %h "  ~a,~%" name))
          %goo-types)

(for-each (lambda (name)
  (format %h "  ELEMENT_~a,~%" name))
          %element-types)
(format %h "  END~%};~%")


;
;
;
(newline %h)
(register-c-type %directionh)
(register-c-type %direction)
(register-c-type %note-value-type)
(register-c-type %note-head-type)
(register-c-type %note-head-group)
(emit-c-type-string-maps2 'DirectionH)
(emit-c-type-string-maps2 'Direction)
(emit-c-type-string-maps2 'note_ValueType)
(emit-c-type-string-maps2 'NoteHead_Type)
(emit-c-type-string-maps2 'NoteHead_Group)
(emit-c-type-string-maps-simple "element_pid" %element-pids "Ms::Pid")

;
; (ms-make-element <element-name>) => element-object
;
(emit-cfun '(ms-make-element) 1 (lambda () (format %c "
    if (! s7_is_symbol(s7_car(args))) {
        return (s7_wrong_type_arg_error(sc, \"ms_make_element\", 1, s7_car(args), \"an symbol\"));
    }
    const char *symname = s7_symbol_name(s7_car(args));
    uint64_t ty;
    Element* e = NULL;
    if (!strcmp(symname, \"dummy\")) {
")

(for-each (lambda (name)
  (format %c "
    } else if (!strcasecmp(symname, \"~a\")) {
        e = Element::create(ElementType::~a, g_mtest->score);
        ty = static_cast<uint64_t>(GOO_TYPE::ELEMENT_~a);
" name name name))
          %element-types)

(format %c "
    } else {
        fprintf(stderr, \"ERROR: UNRECOGNIZED ELEMENT TYPE: %s\\n\", symname);
        return s7_nil(sc);
    }
    return c_make_goo(sc, ty, s7_f(sc), e);
")))

(emit-cfun '(ms-score-selection-elements) 1 (lambda () (format %c "
    if (!s7_is_pair(args)) return s7_f(sc);
    s7_pointer s = s7_car(args);
    if (! c_is_goo(sc, s)) return s7_f(sc);
    goo_t *g = (goo_t *)s7_c_object_value(s);
    MasterScore* score = (MasterScore *) g->cd;
    const void *elms = &(score->selection().elements());
    uint64_t ty = 0;
    return c_make_goo(sc, ty, s7_f(sc), (void*) elms);
")))

(emit-cfun '(ms-elements-getnext) 1 (lambda () (format %c "
    if (!s7_is_pair(args)) return s7_f(sc);
    s7_pointer s = s7_car(args);
    if (! c_is_goo(sc, s)) return s7_f(sc);
    goo_t *g = (goo_t *)s7_c_object_value(s);
    QList<Element*> *elms = (QList<Ms::Element*>*) g->cd;
    int cnt = 0;
    if (g->data && s7_is_integer(g->data)) {
      cnt = s7_integer(g->data);
      cnt++;
    }
    if (cnt >= elms->size()) { // no more elements
        return s7_f(sc);
    }
    g->data = s7_make_integer(sc, cnt);
    Element* elm = elms->at(cnt);
    uint64_t ty = 0;
    return c_make_goo(sc, ty, s7_f(sc), (void*) elm);
")))
;
; breath
;
(emit-cfun '(ms-make-breath) 1 (lambda () (format %c "
    if (!s7_is_pair(args)) return s7_f(sc);
    s7_pointer s = s7_car(args);
    if (! c_is_goo(sc, s)) return s7_f(sc);
    goo_t *g = (goo_t *)s7_c_object_value(s);
    MasterScore* score = (MasterScore *) g->cd;
    if (! score) return s7_f(sc);
    Breath* b = new Breath(score);
    uint64_t ty = 0;
    return c_make_goo(sc, ty, s7_f(sc), b);
")))

;
; hairpin
;

(register-c-type %hairpin-type)
(emit-c-type-string-maps2 'hairpin)

(emit-cfun '(ms-make-hairpin) 0 (lambda () (format %c "
    Hairpin* hp = new Hairpin(g_mtest->score);
    uint64_t ty = static_cast<uint64_t>(GOO_TYPE::ELEMENT_HAIRPIN);
    return c_make_goo(sc, ty, s7_f(sc), hp);
")))

(def-goo-setters-sym "Ms::Hairpin" "hairpin" "hairpinType" "setHairpinType" "hairpin")

;
; note
;

(emit-cfun '(ms-note-set-property) 3 (lambda () (format %c "
    goo_t *g = (goo_t *)s7_c_object_value(s7_car(args));
    Ms::Note* note = (Ms::Note*) g->cd;
    s7_pointer sym = s7_car(s7_cdr(args));
    s7_pointer val = s7_car(s7_cddr(args));
    if (s7_is_boolean(val)) {
        note->setProperty(string_to_element_pid(s7_symbol_name(sym)), QVariant::fromValue(s7_boolean(sc, val)));
    } else if (s7_is_real(val)) {
        note->setProperty(string_to_element_pid(s7_symbol_name(sym)), QVariant::fromValue(s7_real(val)));
    } else if (s7_is_symbol(val)) {
       note->setProperty(string_to_element_pid(s7_symbol_name(sym)), QVariant::fromValue(string_to_ctype(s7_symbol_name(val))));
    } else {
        note->setProperty(string_to_element_pid(s7_symbol_name(sym)), QVariant::fromValue(s7_integer(val)));
    }
")))

;
; emit code for ms-objects set/get, and register them to be exported to scheme
;

(def-goo-setters "Ms::Note" "note" "tpc1"       #f integer)
(def-goo-setters "Ms::Note" "note" "tpc2"       #f integer)
(def-goo-setters "Ms::Note" "note" "fret"       #f integer)
(def-goo-setters "Ms::Note" "note" "pitch"      #f integer)
(def-goo-setters "Ms::Note" "note" "string"     #f integer)
(def-goo-setters "Ms::Note" "note" "tuning"     #f real)
(def-goo-setters "Ms::Note" "note" "veloOffset" #f integer)
(def-goo-setters-bool "Ms::Note" "note" "small" "isSmall" "setSmall")
(def-goo-setters-bool "Ms::Note" "note" "ghost" "ghost" "setGhost")
(def-goo-setters-sym "Ms::Note" "note" "userDotPosition" "setUserDotPosition" "Direction")
(def-goo-setters-sym "Ms::Note" "note" "headGroup" "setHeadGroup" "NoteHead_Group")
(def-goo-setters-sym "Ms::Note" "note" "headType" "setHeadType" "NoteHead_Type")
(def-goo-setters-sym "Ms::Note" "note" "veloType" "setVeloType" "note_ValueType") ; velotype uses Note::Valuetype

;
; score
;
(define-syntax def-score-method
  (lambda (x)
    (define (make-funname methname)
      (format #f "ms_score_~a" methname))
    (syntax-case x ()
      ((k methname)
       (let ((n1 (syntax-object->datum (syntax methname))))
         (with-syntax ((cname (datum->syntax-object (syntax k)
                                                    (make-funname n1))))
           (syntax
            (format %c "
s7_pointer ~a (s7_scheme *sc, s7_pointer args)
{
    if (!s7_is_pair(args)) return s7_f(sc);
    s7_pointer s = s7_car(args);
    if (! c_is_goo(sc, s)) return s7_f(sc);
    goo_t *g = (goo_t *)s7_c_object_value(s);
    MasterScore* score = (MasterScore*) g->cd;
    score->~a();
    return s7_t(sc);
}
" 'cname 'methname))))))))

(def-score-method doLayout)
(def-score-method startCmd)
(def-score-method endCmd)
(def-score-method cmdSelectAll)

; emit the init-function that scheme-exports all glue-functions (ms-objects set/get)
;
(format %c "void init_gen_s7 (s7_scheme *sc) {~%")
(for-each (lambda (lst)
  (match lst
    ((sname cname arity desc)
     (format %c "    s7_define_function(sc, \"~a\", ~a, ~a, 0, false, \"~a\");~%" sname cname arity desc))))
  '(("ms-score-doLayout" "ms_score_doLayout" 1 "(ms-score-doLayout score)")
    ("ms-score-startCmd" "ms_score_startCmd" 1 "(ms-score-startCmd score)")
    ("ms-score-endCmd" "ms_score_endCmd" 1 "(ms-score-endCmd score)")
    ("ms-score-cmdSelectAll" "ms_score_cmdSelectAll" 1 "(ms-score-cmdSelectAll score)")
    ("ms-score-cmdSelectAll" "ms_score_cmdSelectAll" 1 "(ms-score-cmdSelectAll score)")
    ))

(for-each (lambda (lst)
  (match lst
    ((objname memname)
     (format %c "s7_define_variable(sc, \"~a\", s7_dilambda(sc, \"~a\", ~a, 1, 0, ~a, 2, 0, \"~a\"));~%"
      (format #f "ms-~a-~a" objname memname)
      (format #f "ms-~a-~a" objname memname)
      (format #f "ms_~a_~a" objname memname)
      (format #f "ms_set_~a_~a" objname memname)
      (format #f "~a ~a field" objname memname)))))
  %export-to-scheme)

(emit-exports)

(for-each (lambda (lst)
  (match lst
    ((scm-name c-name-get c-name-set desc)
     (format %h "s7_pointer ~a (s7_scheme *sc, s7_pointer args);~%" c-name-get)
     (format %h "s7_pointer ~a (s7_scheme *sc, s7_pointer args);~%" c-name-set)
     (format %c "s7_define_variable(sc, \"~a\", s7_dilambda(sc, \"~a\", ~a, 1, 0, ~a, 2, 0, \"~a\"));
    " scm-name scm-name c-name-get c-name-set desc))))
  '(("ms-note-usermirror" "ms_note_usermirror" "ms_set_note_usermirror" "note usermirror field")))
(format %c "}~%")

(emit-string-to-ctype)

(gen-done)

