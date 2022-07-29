(begin

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
#include \"libmscore/element.h\"
#include \"libmscore/hairpin.h\"
#include \"mtest/testutils.h\"
#include \"s7gen.h\"

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

(format %h "s7_pointer ms_note_usermirror (s7_scheme *sc, s7_pointer args);~%")
(format %h "s7_pointer ms_set_note_usermirror (s7_scheme *sc, s7_pointer args);~%")

;
;
;
(newline %h)
(emit-c-type-string-maps "usermirror" %note-usermirror "Ms::MScore::DirectionH")
(emit-c-type-string-maps "note_direction" %note-direction "Ms::Direction")
(emit-c-type-string-maps-simple "note_headGroup" %note-head-group "Ms::NoteHead::Group")
(emit-c-type-string-maps-simple "note_headType" %note-head-type "Ms::NoteHead::Type")
(emit-c-type-string-maps-simple "note_valueType" %note-value-type "Ms::Note::ValueType")
(emit-c-type-string-maps-simple "element_pid" %element-pids "Ms::Pid")

;
; (ms-make-element <element-name>) => element-object
;
(format %h "s7_pointer ms_make_element (s7_scheme *sc, s7_pointer args);~%")
(format %c "
s7_pointer ms_make_element(s7_scheme *sc, s7_pointer args)
{
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
    return c_make_goo(sc, ty, s7_nil(sc), e);
}

")

;
; hairpin
;

(register-c-type %hairpin-type)
(emit-c-type-string-maps2 'hairpin)

(format %h "s7_pointer ms_make_hairpin (s7_scheme *sc, s7_pointer args);~%")
(format %c "
s7_pointer ms_make_hairpin (s7_scheme *sc, s7_pointer args)
{
    Hairpin* hp = new Hairpin(g_mtest->score);
    uint64_t ty = static_cast<uint64_t>(GOO_TYPE::ELEMENT_HAIRPIN);
    return c_make_goo(sc, ty, s7_nil(sc), hp);
}
")

(def-goo-setters-sym "Ms::Hairpin" "hairpin" "hairpinType" "setHairpinType" "hairpin")

;
; note
;

(format %h "s7_pointer ms_note_set_property (s7_scheme *sc, s7_pointer args);~%")
(format %c "
s7_pointer ms_note_set_property (s7_scheme *sc, s7_pointer args)
{
    goo_t *g = (goo_t *)s7_c_object_value(s7_car(args));
    Ms::Note* note = (Ms::Note*) g->cd;
    s7_pointer sym = s7_car(s7_cdr(args));
    s7_pointer val = s7_car(s7_cddr(args));
    if (s7_is_boolean(val)) {
        note->setProperty(string_to_element_pid(s7_symbol_name(sym)), QVariant::fromValue(s7_boolean(sc, val)));
    } else if (s7_is_real(val)) {
        note->setProperty(string_to_element_pid(s7_symbol_name(sym)), QVariant::fromValue(s7_real(val)));
    } else if (s7_is_symbol(val)) {
       note->setProperty(string_to_element_pid(s7_symbol_name(sym)), QVariant::fromValue(string_to_note_valueType(s7_symbol_name(val))));
    } else {
        note->setProperty(string_to_element_pid(s7_symbol_name(sym)), QVariant::fromValue(s7_integer(val)));
    }
}
")

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
(def-goo-setters-sym "Ms::Note" "note" "userDotPosition" "setUserDotPosition" "note_direction")
(def-goo-setters-sym "Ms::Note" "note" "headGroup" "setHeadGroup" "note_headGroup")
(def-goo-setters-sym "Ms::Note" "note" "headType" "setHeadType" "note_headType")
(def-goo-setters-sym "Ms::Note" "note" "veloType" "setVeloType" "note_valueType") ; velotype uses Note::Valuetype

; emit the init-function that scheme-exports all glue-functions (ms-objects set/get)
;
(format %c "void init_gen_s7 (s7_scheme *sc) {~%")
(format %c "    s7_define_function(sc, \"ms-note-set-property\", ms_note_set_property, 3, 0, false, \"(ms-note-set-property PropertyID value)\");~%")

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
(for-each (lambda (lst)
  (match lst
    ((scm-name c-name-get c-name-set desc)
     (format %c "s7_define_variable(sc, \"~a\", s7_dilambda(sc, \"~a\", ~a, 1, 0, ~a, 2, 0, \"~a\"));
    " scm-name scm-name c-name-get c-name-set desc))))
  '(("ms-note-usermirror" "ms_note_usermirror" "ms_set_note_usermirror" "note usermirror field")))
(format %c "}~%")

(gen-done)

)
