
; /// test menu
; note
; grace
; tpc
; tpcTranspose
; tpcTranspose2
; noteLimits
; tpcDegrees
; alteredUnison
; LongNoteAfterShort_183746

(emit '(load "./mtest.scm"))

; ///   note ///   read/write test of note
(emit '
(let ((chord (ms-make-chord))       ; Ms::Chord* chord = new Ms::Chord(score);
      (note  (ms-make-note)))       ; Note* note = new Note(score);
  (ms-chord-add-note chord note)    ; chord->add(note);
  ; pitch
  (set! (ms-note-pitch note) 33)         ; note->setPitch(33);
  (ms-note-setTpcFromPitch note)
  (check-write-read-elm note ms-note-pitch 33)

  ; tpc
  (set! (ms-note-tpc1 note) 22) ; note->setTpc1(22);
  (check-write-read-elm note ms-note-tpc1 22)

  (set! (ms-note-tpc1 note) 23) ; note->setTpc1(23);
  (set! (ms-note-tpc2 note) 23) ; note->setTpc2(23);
  (check-write-read-elm note ms-note-tpc2 23)

  ; small
  (set! (ms-note-small note) #t) ; note->setSmall(true);
  (check-write-read-elm note ms-note-small #t) ; n = static_cast<Note*>(writeReadElement(note)); QVERIFY(n->isSmall());

  (for-each (lambda (dir)
    (set! (ms-note-usermirror note) dir)
    (check-write-read-elm note ms-note-usermirror dir))
    '(DirectionH-LEFT
      DirectionH-RIGHT
      DirectionH-AUTO))


  (for-each (lambda (dir)
    (set! (ms-note-userDotPosition note) dir)
    (check-write-read-elm note ms-note-userDotPosition dir))
    '(Direction-UP Direction-DOWN Direction-AUTO))

  ; headGroup
  (let ((done #f))
    (for-each (lambda (sym)
      (cond
       ((eq? 'HEAD_GROUPS sym)
        (set! done #t))
       ((not done)
        (let ((grp (string->symbol (format #f "~a-~a" 'NoteHead_Group sym))))
          (set! (ms-note-headGroup note) grp)
          (check-write-read-elm note ms-note-headGroup grp)))))
      (assq-ref %note-head-group 'types)))

  ; headType
  (for-each (lambda (sym)
    (when (not (eq? 'HEAD_TYPES sym))
      (let ((typ (string->symbol (format #f "~a-~a" 'NoteHead_Type sym))))
        (set! (ms-note-headType note) typ)
        (check-write-read-elm note ms-note-headType typ))))
    (assq-ref %note-head-type 'types))


  ; velo offset
  (set! (ms-note-veloOffset note) 71)
  (check-write-read-elm note ms-note-veloOffset 71)

  ; tuning
  (set! (ms-note-tuning note) 1.3)
  (check-write-read-elm note ms-note-tuning 1.3)

  ; fret
  (set! (ms-note-fret note) 9)
  (check-write-read-elm note ms-note-fret 9)

  ; string
  (set! (ms-note-string note) 3)
  (check-write-read-elm note ms-note-string 3)


  ; ghost
  (set! (ms-note-ghost note) #t)
  (check-write-read-elm note ms-note-ghost #t)
  (set! (ms-note-ghost note) #f)
  (check-write-read-elm note ms-note-ghost #f)


  ; velo type
  (set! (ms-note-veloType note) 'note_ValueType-USER_VAL)
  (check-write-read-elm note ms-note-veloType 'note_ValueType-USER_VAL)

  (set! (ms-note-veloType note) 'note_ValueType-OFFSET_VAL)
  (check-write-read-elm note ms-note-veloType 'note_ValueType-OFFSET_VAL)


  ; ================================================
  ;   test setProperty(int, QVariant)
  ; ================================================

  (ms-note-set-property note 'PITCH 32)
  (check-write-read-elm note ms-note-pitch 32)

  (ms-note-set-property note 'TPC1 21)
  (check-write-read-elm note ms-note-tpc1 21)

  (ms-note-set-property note 'TPC1 22)
  (ms-note-set-property note 'TPC2 22)
  (check-write-read-elm note ms-note-tpc2 22)

  (ms-note-set-property note 'SMALL #f)
  (check-write-read-elm note ms-note-small #f)

  (ms-note-set-property note 'SMALL #t)
  (check-write-read-elm note ms-note-small #t)

  (for-each (lambda (dir)
    (ms-note-set-property note 'MIRROR_HEAD dir)
    (check-write-read-elm note ms-note-usermirror dir))
    '(DirectionH-LEFT
      DirectionH-RIGHT
      DirectionH-AUTO))

  (for-each (lambda (pos)
    (ms-note-set-property note 'DOT_POSITION pos)
    (check-write-read-elm note ms-note-userDotPosition pos))
    '(Direction-UP
      Direction-DOWN
      Direction-AUTO))

  ; headGroup
  (let ((done #f))
    (for-each (lambda (sym)
      (cond
       ((eq? 'HEAD_GROUPS sym)
        (set! done #t))
       ((not done)
        (let ((grp (string->symbol (format #f "~a-~a" 'NoteHead_Group sym))))
          (ms-note-set-property note 'HEAD_GROUP grp)
          (check-write-read-elm note ms-note-headGroup grp)))))
      (assq-ref %note-head-group 'types)))

  ; headType
  (for-each (lambda (sym)
    (let ((typ (string->symbol (format #f "~a-~a" 'NoteHead_Type sym))))
    (when (and (not (eq? 'NoteHead_Type-HEAD_AUTO  typ))
               (not (eq? 'NoteHead_Type-HEAD_TYPES typ)))
      (ms-note-set-property note 'HEAD_TYPE typ)
      (check-write-read-elm note ms-note-headType typ))))
    (assq-ref %note-head-type 'types))

  (ms-note-set-property note 'VELO_OFFSET 38)
  (check-write-read-elm note ms-note-veloOffset 38)

  (ms-note-set-property note 'TUNING 2.4)
  (check-write-read-elm note ms-note-tuning 2.4)

  (ms-note-set-property note 'FRET 7)
  (check-write-read-elm note ms-note-fret 7)

  (ms-note-set-property note 'STRING 4)
  (check-write-read-elm note ms-note-string 4)

  (ms-note-set-property note 'GHOST #f)
  (check-write-read-elm note ms-note-ghost #f)
  (ms-note-set-property note 'GHOST #t)
  (check-write-read-elm note ms-note-ghost #t)

  (for-each (lambda (val)
    (ms-note-set-property note 'VELO_TYPE val)
    (check-write-read-elm note ms-note-veloType val))
    '(note_ValueType-USER_VAL
      note_ValueType-OFFSET_VAL))
  ; FIX: delete chord, s7 will take care of this
  ))

(emit '
(let ((score (ms-mtest-readScore "libmscore/note/grace.mscx")))
  (ms-score-doLayout score)
  ; Ms::Chord* chord = score->firstMeasure()->findChord(Fraction(0,1), 0);
  (let* ((chord (ms-measure-findChord (ms-score-firstMeasure score) (ms-make-fraction 0 1) 0))
         (note (ms-chord-upNote chord))) ; Note* note = chord->upNote();
    ; score->setGraceNote(chord, note->pitch(), NoteType::APPOGGIATURA, MScore::division/2);
    (ms-score-setGraceNote score chord (ms-note-pitch note) 'NoteType-APPOGGIATURA (/ (ms-division) 2))
    (let* ((gracenotes (ms-chord-graceNotes chord))
           (gc (ms-chords-first gracenotes))  ;  Ms::Chord* gc = chord->graceNotes().first();
           (notes (ms-chord-notes gc))
           (gracenote (ms-notes-front notes)))
      ; this will sigsegv (c++ test did the same)
      ; FIX: could atleast assert types
      ; (check-write-read-elm gracenote ms-note-noteType 'NoteType-APPOGGIATURA)

      ; tie
      (ms-score-select score gracenote)
      (ms-score-cmdAddTie score)
      ; (check-write-read-elm gracenote ms-note-tieFor (not 0))

      ; tremolo
      (ms-score-startCmd score)
      (let ((tr (ms-make-tremolo score))) ; Tremolo* tr = new Tremolo(score);
        (set! (ms-tremolo-tremoloType tr) 'TremoloType-R16)
        (set! (ms-element-parent tr) gc)
        (set! (ms-element-track tr) (ms-element-track gc))
        (ms-score-undoAddElement score tr)
        (ms-score-endCmd score)
        (let* ((chord (ms-mtest-writeReadElement gc))
               (tr (ms-chord-tremolo chord))
               (trType (ms-tremolo-tremoloType tr)))
          ; NOTE: tst_note.cpp has this test disabled
          (ms-test-check (eq? 'TremoloType-R16 trType))))

      ; // articulation
      (ms-score-startCmd score)
      (let ((ar (ms-make-articulation score 'SymId-articAccentAbove)))
        ; Articulation* ar = new Articulation(SymId::articAccentAbove, score);
        (set! (ms-element-parent ar) gc) ; ar->setParent(gc);
        (set! (ms-element-track ar) (ms-element-track gc)) ; ar->setTrack(gc->track());
        (ms-score-undoAddElement score ar) ; score->undoAddElement(ar);
        (ms-score-endCmd score)
        ;(let* ((chord (ms-mtest-writeReadElement gc))
        ;       (ars (ms-chord-articulations chord))) ; <--- not implemented
        ;  (ms-test-check (> (length ars) 0)))
        )))
  (ms-test-check
   (ms-mtest-saveCompareScore score "grace-test.mscx", "libmscore/note/grace-ref.mscx"))))

; tpc
; test of note tpc values
(emit '
(let ((score (ms-mtest-readScore "libmscore/note/tpc.mscx")))
  (let ((is (ms-score-inputState score)))
    (set! (ms-inputstate-track is) 0)
    (set! (ms-inputstate-segment is)
          (ms-score-tick2segment score (ms-make-fraction 0 1) #f 'SegmentType-ChordRest))

    ; score->inputState().setDuration(TDuration::DurationType::V_QUARTER);
    (set! (ms-inputstate-duration is) 'DurationType-V_QUARTER)
    ; score->inputState().setNoteEntryMode(true);
    (set! (ms-inputstate-noteEntryMode is) #t)

    (let ((octave (* 5 7)))
      (ms-score-cmdAddPitch score (+ octave 1) #f #f)
      (ms-score-cmdAddPitch score (+ octave 2) #f #f)
      (ms-score-cmdAddPitch score (+ octave 3) #f #f)
      (ms-score-cmdAddPitch score (+ octave 4) #f #f)
      (ms-score-cmdAddPitch score (+ octave 5) #f #f)
      (ms-score-cmdAddPitch score (+ octave 6) #f #f)
      (ms-score-cmdAddPitch score (+ octave 7) #f #f)
      (ms-score-cmdAddPitch score (+ octave 8) #f #f)
      (ms-score-cmdConcertPitchChanged score #t #t))
    (ms-test-check
     (ms-mtest-saveCompareScore score  "tpc-test.mscx", "libmscore/note/tpc-ref.mscx")))))

; tpcTranspose
; test of note tpc values & transposition
(emit '
(let ((score (ms-mtest-readScore "libmscore/note/tpc-transpose.mscx")))
  (ms-score-startCmd score)
  (let* ((m (ms-score-firstMeasure score))) ; Measure* m = score->firstMeasure();
      ; score->select(m, SelectType::SINGLE, 0);
      (ms-score-select score m 'SelectType-SINGLE 0)
      (ms-score-changeAccidental score 'AccidentalType-FLAT)
      (ms-score-endCmd score)

      (ms-score-startCmd score)
      (set! m (ms-measure-nextMeasure m)) ; m = m->nextMeasure();
      (ms-score-select score m 'SelectType-SINGLE  0)

      (ms-score-upDown score #f 'UpDownMode-CHROMATIC)
      (ms-score-endCmd score)

      (ms-score-startCmd score)
      (ms-score-cmdConcertPitchChanged score #t #t)
      (ms-score-endCmd score))
  (ms-test-check
   (ms-mtest-saveCompareScore score  "tpc-transpose-test.mscx", "libmscore/note/tpc-transpose-ref.mscx"))))

; tpcTranspose2
; more tests of note tpc values & transposition
(emit '
(let ((score (ms-mtest-readScore "libmscore/note/tpc-transpose2.mscx")))
  (let ((is (ms-score-inputState score)))
    (set! (ms-inputstate-track is) 0) ; score->inputState().setTrack(0)
    (set! (ms-inputstate-segment is)
          (ms-score-tick2segment score (ms-make-fraction 0 1) #f 'SegmentType-ChordRest))
    (set! (ms-inputstate-duration is) 'DurationType-V_QUARTER)
    (set! (ms-inputstate-noteEntryMode is) #t)
    (let ((octave (* 5 7)))
      (ms-score-cmdAddPitch score (+ octave 3) #f #f))
    (ms-score-startCmd score)
    (ms-score-cmdConcertPitchChanged score #t #t)
    (ms-score-endCmd score))
  (ms-test-check
   (ms-mtest-saveCompareScore score  "tpc-transpose2-test.mscx", "libmscore/note/tpc-transpose2-ref.mscx"))))


;; noteLimits
(emit '
(let ((score (ms-mtest-readScore "libmscore/note/empty.mscx")))
  (let ((is (ms-score-inputState score)))
    (set! (ms-inputstate-track is) 0) ; score->inputState().setTrack(0)
    (set! (ms-inputstate-segment is)
          (ms-score-tick2segment score (ms-make-fraction 0 1) #f 'SegmentType-ChordRest))
    (set! (ms-inputstate-duration is) 'DurationType-V_QUARTER)
    (set! (ms-inputstate-noteEntryMode is) #t)

    ; over 127 shouldn't crash
    (ms-score-cmdAddPitch score 140 #f #f)
    ; below 0 shouldn't crash
    (ms-score-cmdAddPitch score -40 #f #f)

    ; stack chords
    (ms-score-cmdAddPitch score 42 #f #f)
    (do ((i 1 (+ i 1)))
        ((= i 20))
      (ms-score-cmdAddPitch score (+ 42 (* i 7)) #t #f))

    ; interval below
    (ms-score-cmdAddPitch score 42 #f #f) ; duplicate this?
    (do ((i 0 (+ i 1))) ((= i 20))
      ; std::vector<Note*> nl = score->selection().noteList();
      (let* ((sl (ms-score-selection score))
             (nl (ms-selection-noteList sl)))
        (ms-score-cmdAddInterval score -8 nl)))

      ; interval above
      (ms-score-cmdAddPitch score 42 #f #f) ; duplicate this?
      (do ((i 0 (+ i 1))) ((= i 20))
        (let* ((sl (ms-score-selection score))
               (nl (ms-selection-noteList sl)))
          (ms-score-cmdAddInterval score 8 nl)))
  (ms-test-check
   (ms-mtest-saveCompareScore score  "notelimits-test.mscx", "libmscore/note/notelimits-ref.mscx")))))

(emit '
(let ((score (ms-mtest-readScore "libmscore/note/altered-unison.mscx"))
      (check-acc (lambda (note acc-type)
                   (let ((acc (ms-note-accidental note)))
                     (ms-test-check acc)
                     (ms-test-check (eq? acc-type
                                         (ms-accidental-accidentalType acc)))))))
  (let ((chord (ms-measure-findChord (ms-score-firstMeasure score)
                                     (ms-make-fraction 0 1)
                                     0)))
    (let ((note (ms-chord-downNote chord)))
      (ms-test-check note)
      (if note (check-acc note 'AccidentalType-FLAT)))
    (let ((note (ms-chord-upNote chord)))
      (ms-test-check note)
      (if note (check-acc note 'AccidentalType-NATURAL))))
  (let ((chord (ms-measure-findChord (ms-score-firstMeasure score)
                                     (ms-make-fraction 1 4)
                                     0)))
     (let ((note (ms-chord-downNote chord)))
      (ms-test-check note)
      (if note (check-acc note 'AccidentalType-NATURAL)))
    (let ((note (ms-chord-upNote chord)))
      (ms-test-check note)
      (if note (check-acc note 'AccidentalType-SHARP))))))

(emit '
 (begin
   (ms-test-check (= (ms-tpc2degree 'Tpc-TPC_C   'Key-C) 0))
   ; (ms-test-check (= (ms-tpc2degree 'Tpc-TPC_E_S 'Key-C)  3))
   (ms-test-check (= (ms-tpc2degree 'Tpc-TPC_B    'Key-C)   6))
   (ms-test-check (= (ms-tpc2degree 'Tpc-TPC_F_S  'Key-C_S) 3))
   (ms-test-check (= (ms-tpc2degree 'Tpc-TPC_B    'Key-C_S) 6))
   (ms-test-check (= (ms-tpc2degree 'Tpc-TPC_B_B  'Key-C_S) 6))
   ; (ms-test-check (= (ms-tpc2degree 'Tpc-TPC_B_S  'Key-C_S) 7))
    ))

;;; LongNoteAfterShort_183746
;;; Put a small 128th rest
;;; Then put a long Breve note
;;; This breve will get spread out across multiple measures
;;; Verifies that the resulting notes are tied over at least 3 times (to span 3 measures) and have total duration the same as a breve,
;;; regardless of how the breve was divided up.

(emit '
(let ((score (ms-mtest-readScore "libmscore/note/empty.mscx")))
  (ms-score-doLayout score)
  (let ((is (ms-score-inputState score)))
    (set! (ms-inputstate-track is) 0)
    (set! (ms-inputstate-segment is)
          (ms-score-tick2segment score (ms-make-fraction 0 1) #f 'SegmentType-ChordRest))
    (set! (ms-inputstate-duration is) 'DurationType-V_128TH)
    (set! (ms-inputstate-noteEntryMode is) #t)
    (ms-score-cmdEnterRest score 'DurationType-V_128TH)
    (set! (ms-inputstate-duration is) 'DurationType-V_BREVE)
    (ms-score-cmdAddPitch score 47 #f #f) ; score->cmdAddPitch(47, 0, 0);

    (let* ((dur (ms-make-tduration 'DurationType-V_128TH))
           (seg (ms-score-tick2segment-1 score (ms-tduration-ticks dur))))
      (ms-test-check (and seg (eq? (ms-segment-segmentType seg) 'SegmentType-ChordRest)))
      (and (ms-segment-tick seg) (ms-make-fraction 1 128))
      (format #t "segment tick: ~s~%" (ms-fraction-toString (ms-segment-tick seg)))
      (ms-test-check (ms-fraction-equal (ms-segment-tick seg) (ms-make-fraction 1 128)))

      (let ((elm (ms-segment-firstElement seg 0)))
        (ms-test-check (and elm (ms-element-isNote elm)))

        (let ((nl (ms-note-tiedNotes elm)))
          (format #t "number of tied notes: ~s~%" (ms-notes-size nl))
          (ms-test-check (>= (ms-notes-size nl) 3))

          (let ((totalTicks (ms-make-fraction 0 1)))
            (do ((i 0 (+ i 1)))
                ((>= i (ms-notes-size nl)))
              (let* ((note (ms-notes-ref nl i))
                     (chord (ms-note-chord note)))
                (ms-fraction-add totalTicks (ms-chord-durationTypeTicks chord))))
            (let ((breveTicks (ms-tduration-ticks (ms-make-tduration 'DurationType-V_BREVE))))
              (ms-test-check (ms-fraction-equal totalTicks breveTicks))))))))))
