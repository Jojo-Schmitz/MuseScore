
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
#include \"libmscore/tremolo.h\"
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

(register-c-type %symid)
(emit-c-type-string-maps2 'SymId)

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

(register-c-type %note-type)
(emit-c-type-string-maps2 'NoteType)

(emit-cfun '(ms-make-chord) 0 (lambda () (format %c "
    return c_make_goo(sc,
                      static_cast<uint64_t>(GOO_TYPE::CHORD),
                      s7_nil(sc),
                      new Ms::Chord(g_mtest->score));
")))

(emit-cfun '(ms-make-note) 0 (lambda () (format %c "
    return c_make_goo(sc,
                      static_cast<uint64_t>(GOO_TYPE::NOTE),
                      s7_nil(sc),
                      new Ms::Note(g_mtest->score));
")))

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

(emit-cfun '(ms-element-type) 1 (list
  '(emit-pop-arg-goo "Element*" "e")
  (lambda (e)
    (format %c "    return s7_make_integer(sc, static_cast<uint64_t>(e->type()));~%"))))

(def-goo-setters-goo "Ms::Element*" "element" "parent" "setParent" "Element*")
(def-goo-setters "Ms::Element" "element" "track" "setTrack" integer)

(emit-cfun '(ms-score-selection-elements) 1 (list
  '(emit-pop-arg-goo "MasterScore*" "score")
  (lambda (e)
   (format %c "
    const void *elms = &(score->selection().elements());~% "))
  '(emit-return-goo "elms" 0)))

(emit-cfun '(ms-elements-getnext) 1 (list
  '(emit-pop-arg-goo "QList<Element*>*" "elms")
  (lambda (e)
   (format %c "
    int cnt = 0;
    if (g->data && s7_is_integer(g->data)) {
      cnt = s7_integer(g->data);
      cnt++;
    }
    if (cnt >= elms->size()) { // no more elements
        return s7_f(sc);
    }
    g->data = s7_make_integer(sc, cnt);
    Element* elm = elms->at(cnt);~%"))
  '(emit-return-goo "elm" 0)))
;
; breath
;
(emit-cfun '(ms-make-breath) 1 (list
  '(emit-pop-arg-goo "MasterScore*" "score")
  (lambda (e)
   (format %c "
    if (! score) return s7_f(sc);
    Breath* b = new Breath(score);
    uint64_t ty = 0;
    return c_make_goo(sc, ty, s7_f(sc), b);
"))))

(emit-cfun '(ms-make-editdata) 0 (lambda () (format %c "
    EditData* ed = new EditData(0);
    ed->view = 0;
    uint64_t ty = 0;
    return c_make_goo(sc, ty, s7_f(sc), ed);
")))

(emit-cfun '(ms-editdata-dropelement) 2 (list
  '(emit-pop-arg-goo)
  '(emit-next-arg)
  (lambda (e)
   (format %c "
    s = s7_car(args);
    if (! c_is_goo(sc, s)) return s7_f(sc);
    goo_t *g2 = (goo_t *)s7_c_object_value(s);

    EditData* ed = (EditData*) g->cd;
    ed->dropElement = (Element *) g2->cd;
    return s7_t(sc);
"))))

(emit-cfun '(ms-element-drop) 2 (list
  '(emit-pop-arg-goo)
  '(emit-next-arg)
  (lambda (e)
   (format %c "
    s = s7_car(args);
    if (! c_is_goo(sc, s)) return s7_f(sc);
    goo_t *g2 = (goo_t *)s7_c_object_value(s);

    Element *e = (Element*) g->cd;
    EditData* ed = (EditData*) g2->cd;
    if (e->acceptDrop(*ed)) {
      e->drop(*ed);
      return s7_t(sc);
    } else {
      return s7_f(sc);
    }
"))))

;
; hairpin
;

(register-c-type %hairpin-type)
(emit-c-type-string-maps2 'hairpin)

(emit-cfun '(ms-make-hairpin) 0 (list
  (lambda (e) (format %c "
    Hairpin* hp = new Hairpin(g_mtest->score);"))
  '(emit-return-goo "hp" "static_cast<uint64_t>(GOO_TYPE::ELEMENT_HAIRPIN)")))

(def-goo-setters-sym "Ms::Hairpin" "hairpin" "hairpinType" "setHairpinType" "hairpin")

; tremolo
(register-c-type %tremolo-type)
(emit-c-type-string-maps2 'TremoloType)

(emit-cfun '(ms-make-tremolo) 1 (list
  '(emit-pop-arg-goo "MasterScore*" "score")
  (lambda (e) (format %c "
    Tremolo* tr = new Tremolo(score);"))
  '(emit-return-goo "tr" "static_cast<uint64_t>(GOO_TYPE::ELEMENT_TREMOLO)")))

(def-goo-setters-sym "Ms::Tremolo" "tremolo" "tremoloType" "setTremoloType" "TremoloType")

;
; note
;

(emit-cfun '(ms-note-usermirror) 1 (list
  '(emit-pop-arg-goo "Ms::Note*" "note")
  (lambda (e)
    (format %c "    return s7_make_symbol(sc, DirectionH_to_string(note->userMirror()));~%"))))

(emit-cfun '(ms-set-note-usermirror) 2 (list
  '(emit-pop-arg-goo "Ms::Note*" "note")
  '(emit-next-arg)
  '(emit-pop-arg-sym "symname")
  (lambda (e)
    (format %c "
    note->setUserMirror(string_to_DirectionH(symname));
    return s;~%"))))

(emit-cfun '(ms-note-set-property) 3 (list
  '(emit-pop-arg-goo "Ms::Note*" "note")
  (lambda (e) (format %c "
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
"))))

;
; emit code for ms-objects set/get, and register them to be exported to scheme
;

(def-goo-setters-sym "Ms::Breath" "breath" "symId" "setSymId" "SymId")

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
(def-goo-setters-sym "Ms::Note" "note" "noteType" #f "NoteType")

(emit-cfun '(ms-note-set-tpc-from-pitch) 1 (list
  '(emit-pop-arg-goo "Ms::Note*" "note")
  (lambda (e) (format %c "
    note->setTpcFromPitch();
    return s7_t(sc);~%"))))


; emit c-functions for simple object-methods
(map (lambda (lst)
       (match lst
       ((sname cvartype cvarname meth crestype cresvarname gootype)
       (emit-cfun `(,sname) 1 (list
         `(emit-pop-arg-goo ,cvartype ,cvarname)
         (lambda (e)
           (format %c "    ~a ~a = ~a;" crestype cresvarname meth))
         `(emit-return-goo ,cresvarname ,(format #f "static_cast<uint64_t>(~a)" gootype)))))))
  '((ms-chord-upNote "Chord*" "chord" "chord->upNote()" "Ms::Note*" "note" "GOO_TYPE::ELEMENT_NOTE")
    (ms-chord-graceNotes "Chord*" "chord" "&(chord->graceNotes())" "QVector<Chord*>*" "gcs" "0 /* GOO_TYPE::CHORDLIST */")
    (ms-chords-first "QVector<Chord*>*" "chords" "chords->first()" "Ms::Chord*" "chord" "GOO_TYPE::CHORD")
    (ms-chord-notes "Chord*" "chord" "&(chord->notes())" "std::vector<Note*>*" "notes" "0 /* GOO_TYPE::NOTELIST */")
    (ms-chord-tremolo "Chord*" "chord" "chord->tremolo()" "Tremolo*" "tremolo" "0 /* GOO_TYPE::TREMOLO */")
    (ms-notes-front "std::vector<Note*>*" "notes" "notes->front()" "Note*" "note" "GOO_TYPE::NOTE")))

(emit-cfun '(ms-chord-add-note) 2 (list
  '(emit-pop-arg-goo "Ms::Chord*" "chord" "chord_g")
  '(emit-next-arg)
  '(emit-pop-arg-goo "Ms::Note*" "note" "note_g")
  (lambda (e) (format %c "
    chord->add(note);
    return s7_t(sc);~%"))))

;
; score
;
(define-syntax def-score-method
  (lambda (x)
    (define (make-sname methname)
      (string->symbol (format #f "ms-score-~a" methname)))
    (syntax-case x ()
      ((k methname)
       (let ((n1 (syntax-object->datum (syntax methname))))
         (with-syntax ((sname (datum->syntax-object (syntax k)
                                                    (make-sname n1))))
           (syntax
            (emit-cfun '(sname) 1
             (lambda ()
              (emit-pop-arg-goo '("MasterScore*" "score") '())
              (format %c "
    score->~a();
    return s7_t(sc);
" 'methname))))))))))

(def-score-method doLayout)
(def-score-method startCmd)
(def-score-method endCmd)
(def-score-method cmdSelectAll)
(def-score-method cmdAddTie)

(emit-cfun '(ms-score-firstMeasure) 1 (list
  '(emit-pop-arg-goo "MasterScore*" "score")
  (lambda (e)
  (format %c "
    Ms::Measure* mea = score->firstMeasure();
    uint64_t ty = static_cast<uint64_t>(GOO_TYPE::ELEMENT_MEASURE);
    return c_make_goo(sc, ty, s7_f(sc), mea);~%"))))


(emit-cfun '(ms-measure-findChord) 3 (list
  '(emit-pop-arg-goo "Measure*" "mea")
  '(emit-next-arg)
  '(emit-pop-arg-goo "Fraction*" "f")
  '(emit-next-arg)
  '(emit-pop-arg-int "track")
  (lambda (e)
    (format %c "
    Ms::Chord* chord = mea->findChord(*f, track);
    uint64_t ty = static_cast<uint64_t>(GOO_TYPE::ELEMENT_CHORD);
    return c_make_goo(sc, ty, s7_f(sc), chord);~%"))))

(emit-cfun '(ms-score-undoAddElement) 2 (list
  '(emit-pop-arg-goo "MasterScore*" "score")
  '(emit-next-arg)
  '(emit-pop-arg-goo "Element*" "elm")
  (lambda (e)
   (format %c "
    score->undoAddElement(elm);
    return s7_t(sc);~%"))))

(emit-cfun '(ms-score-undoStack-undo) 2 (list
  '(emit-pop-arg-goo "MasterScore*" "score")
  '(emit-next-arg)
  (lambda (e)
   (format %c "
    s = s7_car(args);
    if (c_is_goo(sc, s)) {
        g = (goo_t *)s7_c_object_value(s);
        EditData* ed = (EditData*) g->cd;
        score->undoStack()->undo(ed);
    } else {
      score->undoStack()->undo(0);
    }
    return s7_t(sc);~%"))))

(emit-cfun '(ms-score-setGraceNote) 5 (list
  '(emit-pop-arg-goo "MasterScore*" "score")
  '(emit-next-arg)
  '(emit-pop-arg-goo "Chord*" "chord")
  '(emit-next-arg)
  '(emit-pop-arg-int "pitch")
  '(emit-next-arg)
  '(emit-pop-arg-sym "notetype")
  '(emit-next-arg)
  '(emit-pop-arg-int "division")
  (lambda (e)
   (format %c "
    score->setGraceNote(chord, pitch, string_to_NoteType(notetype), division);
    return s7_t(sc);~%"))))

(emit-cfun '(ms-score-select) 2 (list
  '(emit-pop-arg-goo "MasterScore*" "score")
  '(emit-next-arg)
  '(emit-pop-arg-goo "Element*" "elm")
  ; FIX: support optional args: void select(Element* obj, SelectType = SelectType::SINGLE, int staff = 0);
  (lambda (e)
   (format %c "
    score->select(elm);~%"))))


(emit-cfun '(ms-make-fraction) 2 (list
  '(emit-pop-arg-int "numerator")
  '(emit-next-arg)
  '(emit-pop-arg-int "denominator")
  (lambda (e)
   (format %c "
    Fraction* f = new Fraction(numerator, denominator);
    uint64_t ty = static_cast<uint64_t>(0); // GOO_TYPE::ELEMENT_FRACTION
    return c_make_goo(sc, ty, s7_f(sc), f);~%"))))

(emit-cfun '(ms-division) 0 (lambda ()
  (format %c "    return s7_make_integer(sc, MScore::division);~%")))

; emit the init-function that scheme-exports all glue-functions (ms-objects set/get)
;
(format %c "void init_gen_s7 (s7_scheme *sc) {~%")
(for-each (lambda (lst)
  (match lst
    ((sname cname arity desc)
     (format %c "    s7_define_function(sc, \"~a\", ~a, ~a, 0, false, \"~a\");~%" sname cname arity desc))))
  '(
    ;("ms-score-endCmd" "ms_score_endCmd" 1 "(ms-score-endCmd score)")
    ))

(for-each (lambda (lst)
  (match lst
    ((objname memname memnameset)
     (if memnameset ; both get and setter is defined
       (format %c "s7_define_variable(sc, \"~a\", s7_dilambda(sc, \"~a\", ~a, 1, 0, ~a, 2, 0, \"~a\"));~%"
        (format #f "ms-~a-~a" objname memname)
        (format #f "ms-~a-~a" objname memname)
        (format #f "ms_~a_~a" objname memname)
        (format #f "ms_set_~a_~a" objname memname)
        (format #f "~a ~a field" objname memname))
       ; only getter is defined
       (format %c "s7_define_function(sc, \"~a\", ~a, 1, 0, false, \"~a\");~%"
        (format #f "ms-~a-~a" objname memname)
        (format #f "ms_~a_~a" objname memname)
        (format #f "~a ~a field" objname memname))))))
  %export-to-scheme-getset)

(emit-exports)

(for-each (lambda (lst)
  (match lst
    ((scm-name c-name-get c-name-set desc)
     (format %h "s7_pointer ~a (s7_scheme *sc, s7_pointer args);~%" c-name-get)
     (format %h "s7_pointer ~a (s7_scheme *sc, s7_pointer args);~%" c-name-set)
     (format %c "s7_define_variable(sc, \"~a\", s7_dilambda(sc, \"~a\", ~a, 1, 0, ~a, 2, 0, \"~a\"));
    " scm-name scm-name c-name-get c-name-set desc))))
  '(("ms-note-usermirror" "ms_note_usermirror" "ms_set_note_usermirror" "note usermirror field")
    ("ms-tremolo-tremoloType" "ms_tremolo_tremoloType" "ms_set_tremolo_tremoloType" "tremolo type file")
    ("ms-element-parent" "ms_element_parent" "ms_element_setParent" "element parent field")
    ("ms-element-track" "ms_element_track" "ms_set_element_track" "element track field")))
(format %c "}~%")

(emit-string-to-ctype)

(gen-done)

