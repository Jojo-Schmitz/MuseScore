(define-object element
  (c-type "Ms::Element")
  (methods
    (get/set track (int))))

(define-object score
  (c-type "Ms::MasterScore")
  (methods
    (apply undoAddElement (("Element*" goo)))
    ; FIX: support optional args: void select(Element* obj, SelectType = SelectType::SINGLE, int staff = 0);
    (apply select (("Element*" goo)))
    (apply setGraceNote (("Chord*" goo)
                         (int pitch)
                         (sym NoteType)
                         (int division)))
    ; nil -- as in zero arity, so is doing side-effects
    (nil doLayout)
    (nil startCmd)
    (nil endCmd)
    (nil cmdSelectAll)
    (nil cmdAddTie)
    (get firstMeasure ("Ms::Measure*" goo ELEMENT_MEASURE))))

(define-object measure
  (c-type "Ms::Measure")
  (methods
    (apply findChord (("Fraction*" deref) (int track)) ("Ms::Chord*" GOO_TYPE::ELEMENT_CHORD))
    ))

(define-object chord
  (c-type "Ms::Chord")
  (make-args (("Ms::Score" goo)))
  (methods
    ; get -- methods that acts like getters (but has no complementary setter)
    (get upNote     ("Ms::Note*" goo))
    (get graceNotes ("QVector<Chord*>*" goo) "&(o->graceNotes())")
    (get notes      ("std::vector<Note*>*" goo) "&(o->notes())")
    (get tremolo    ("Tremolo*" goo))
    (apply (add add-note) (("Ms::Note*" goo)))))

(define-object note
  (c-type "Ms::Note")
  (make-args (("Ms::Score" goo)))
  (methods
    (get/set tpc1   (int))
    (get/set tpc2   (int))
    (get/set fret   (int))
    (get/set pitch  (int))
    (get/set string (int))
    (get/set tuning (real))
    (get/set veloOffset (int))

    (nil setTpcFromPitch)))
