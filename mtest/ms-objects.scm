(define-object element
  (c-type "Ms::Element")
  (methods
    (get/set track (int))
    (get isNote () (bool))
    ))

(define-object score
  (c-type "Ms::MasterScore")
  (methods
    (apply undoAddElement (("Element*" goo)))
    (apply select (("Measure*" goo) (sym SelectType opt) (int staff opt)))
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
    (set changeAccidental ((sym AccidentalType)))
    (set cmdEnterRest ((sym DurationType)))
    (set cmdAddPitch  ((int) (bool) (bool)))
    (apply cmdAddInterval ((int) ("std::vector<Note*>*" deref)))
    (set cmdConcertPitchChanged ((bool) (bool)))
    (set upDown ((bool) (sym UpDownMode)))
    (get firstMeasure () ("Ms::Measure*" goo ELEMENT_MEASURE))
    (get inputState   () ("Ms::InputState*") ref)
    (get tick2segment (("Ms::Fraction*" deref) (bool) (sym SegmentType)) ("Ms::Segment*"))
    (get (tick2segment-1 tick2segment) (("Ms::Fraction*" deref)) ("Ms::Segment*"))
    (get/set selection ("Ms::Selection*") ref)
    ))

(define-object measure
  (c-type "Ms::Measure")
  (methods
    (get findChord (("Fraction*" deref) (int track)) ("Ms::Chord*" goo ELEMENT_CHORD))
    (get nextMeasure () ("Ms::Measure*" goo ELEMENT_MEASURE))
    ))

(define-object segment
  (c-type "Ms::Segment")
  (methods
    (get segmentType () (sym SegmentType))
    (get tick () ("Ms::Fraction" stack))
    (get ticks () ("Ms::Fraction" stack))
    (get firstElement ((int)) ("Ms::Element*"))
    ))

(define-object chord
  (c-type "Ms::Chord")
  (make-args (("Ms::Score" goo)))
  (methods
    ; get -- methods that acts like getters (but has no complementary setter)
    (get upNote     () ("Ms::Note*" goo))
    (get downNote   () ("Ms::Note*" goo))
    (get graceNotes () ("QVector<Chord*>*" goo) "&(o->graceNotes())")
    (get notes      () ("std::vector<Note*>*" goo) "&(o->notes())")
    (get tremolo    () ("Tremolo*" goo))
    (get durationTypeTicks () ("Ms::Fraction" stack))
    (apply (add-note add) (("Ms::Note*" goo)))))

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
    (get/set accidental ("Accidental*"))
    (get chord () ("Ms::Chord*"))
    (get tiedNotes () ("std::vector<Note*>" stack))
    (nil setTpcFromPitch)))


(define-object accidental
  (c-type "Ms::Accidental")
  (methods
    (get/set accidentalType (sym AccidentalType))
    ))

(define-object inputstate
  (c-type "Ms::InputState")
  (methods
    (get/set noteEntryMode (bool))
    (get/set track (int))
    (get/set duration (sym DurationType))
    (get/set segment ("Ms::Segment*"))))

(define-object fraction
  (c-type "Ms::Fraction")
  (methods
    (get toString () (QString))
    (get (equal ==) (("Ms::Fraction*")) (bool) infix)
    (apply (add   +=) (("Ms::Fraction*")) #f infix) ; FIX: return void
    ))

(define-object tduration
  (c-type "Ms::TDuration")
  (methods
    (get ticks () ("Ms::Fraction" stack))
    ))

(define-object selection
  (c-type "Ms::Selection")
  (methods
    (get noteList () ("std::vector<Note*>" stack))
    ))
