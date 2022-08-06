
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

  (format #t "-- done all checking of note~%")
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
;      score->startCmd();
      (ms-score-startCmd score)
      (let ((ar (ms-make-articulation 'SymId-articAccentAbove score)))
        ; Articulation* ar = new Articulation(SymId::articAccentAbove, score);
        (set! (ms-element-parent ar) gc) ; ar->setParent(gc);
        (set! (ms-element-track ar) (ms-element-track gc)) ; ar->setTrack(gc->track());
        (ms-score-undoAddElement score ar) ; score->undoAddElement(ar);
        (ms-score-endCmd score)
        ;(let* ((chord (ms-mtest-writeReadElement gc))
        ;       (ars (ms-chord-articulations chord))) ; <--- not implemented
        ;  (ms-test-check (> (length ars) 0)))
        )


      ))
  ))
#|


      QVERIFY(saveCompareScore(score, "grace-test.mscx", DIR + "grace-ref.mscx"));
      }

//---------------------------------------------------------
///   tpc
///   test of note tpc values
//---------------------------------------------------------

void TestNote::tpc()
      {
      MasterScore* score = readScore(DIR + "tpc.mscx");

      score->inputState().setTrack(0);
      score->inputState().setSegment(score->tick2segment(Fraction(0,1), false, SegmentType::ChordRest));
      score->inputState().setDuration(TDuration::DurationType::V_QUARTER);
      score->inputState().setNoteEntryMode(true);
      int octave = 5 * 7;
      score->cmdAddPitch(octave + 1, false, false);
      score->cmdAddPitch(octave + 2, false, false);
      score->cmdAddPitch(octave + 3, false, false);
      score->cmdAddPitch(octave + 4, false, false);
      score->cmdAddPitch(octave + 5, false, false);
      score->cmdAddPitch(octave + 6, false, false);
      score->cmdAddPitch(octave + 7, false, false);
      score->cmdAddPitch(octave + 8, false, false);

      score->cmdConcertPitchChanged(true, true);

      QVERIFY(saveCompareScore(score, "tpc-test.mscx", DIR + "tpc-ref.mscx"));
      }

//---------------------------------------------------------
///   tpcTranspose
///   test of note tpc values & transposition
//---------------------------------------------------------

void TestNote::tpcTranspose()
      {
      MasterScore* score = readScore(DIR + "tpc-transpose.mscx");

      score->startCmd();
      Measure* m = score->firstMeasure();
      score->select(m, SelectType::SINGLE, 0);
      score->changeAccidental(AccidentalType::FLAT);
      score->endCmd();

      score->startCmd();
      m = m->nextMeasure();
      score->select(m, SelectType::SINGLE, 0);
      score->upDown(false, UpDownMode::CHROMATIC);
      score->endCmd();

      score->startCmd();
      score->cmdConcertPitchChanged(true, true);
      score->endCmd();

      QVERIFY(saveCompareScore(score, "tpc-transpose-test.mscx", DIR + "tpc-transpose-ref.mscx"));

      }

//---------------------------------------------------------
///   tpcTranspose2
///   more tests of note tpc values & transposition
//---------------------------------------------------------

void TestNote::tpcTranspose2()
      {
      MasterScore* score = readScore(DIR + "tpc-transpose2.mscx");

      score->inputState().setTrack(0);
      score->inputState().setSegment(score->tick2segment(Fraction(0,1), false, SegmentType::ChordRest));
      score->inputState().setDuration(TDuration::DurationType::V_QUARTER);
      score->inputState().setNoteEntryMode(true);
      int octave = 5 * 7;
      score->cmdAddPitch(octave + 3, false, false);

      score->startCmd();
      score->cmdConcertPitchChanged(true, true);
      score->endCmd();

      printf("================\n");

      QVERIFY(saveCompareScore(score, "tpc-transpose2-test.mscx", DIR + "tpc-transpose2-ref.mscx"));
      }

//---------------------------------------------------------
///   noteLimits
//---------------------------------------------------------

void TestNote::noteLimits()
      {
      MasterScore* score = readScore(DIR + "empty.mscx");

      score->inputState().setTrack(0);
      score->inputState().setSegment(score->tick2segment(Fraction(0,1), false, SegmentType::ChordRest));
      score->inputState().setDuration(TDuration::DurationType::V_QUARTER);
      score->inputState().setNoteEntryMode(true);

      // over 127 shouldn't crash
      score->cmdAddPitch(140, false, false);
      // below 0 shouldn't crash
      score->cmdAddPitch(-40, false, false);

      // stack chords
      score->cmdAddPitch(42, false, false);
      for (int i = 1; i < 20; i++)
            score->cmdAddPitch(42 + i * 7, true, false);

      // interval below
      score->cmdAddPitch(42, false, false);
      for (int i = 0; i < 20; i++) {
            std::vector<Note*> nl = score->selection().noteList();
            score->cmdAddInterval(-8, nl);
            }

      // interval above
      score->cmdAddPitch(42, false, false);
      for (int i = 0; i < 20; i++) {
            std::vector<Note*> nl = score->selection().noteList();
            score->cmdAddInterval(8, nl);
            }
      QVERIFY(saveCompareScore(score, "notelimits-test.mscx", DIR + "notelimits-ref.mscx"));
      }

void TestNote::alteredUnison()
      {
      MasterScore* score = readScore(DIR + "altered-unison.mscx");
      Measure* m = score->firstMeasure();
#ifdef __MINGW32__ // apparently defined for 64bit too. Needed to avoid a conflict with windows.h and its declaration of `Chord`
      Ms::Chord* c = m->findChord(Fraction(0, 1), 0);
#else
      Chord* c = m->findChord(Fraction(0, 1), 0);
#endif
      QVERIFY(c->downNote()->accidental() && c->downNote()->accidental()->accidentalType() == Ms::AccidentalType::FLAT);
      QVERIFY(c->upNote()->accidental() && c->upNote()->accidental()->accidentalType() == Ms::AccidentalType::NATURAL);
      c = m->findChord(Fraction(1, 4), 0);
      QVERIFY(c->downNote()->accidental() && c->downNote()->accidental()->accidentalType() == Ms::AccidentalType::NATURAL);
      QVERIFY(c->upNote()->accidental() && c->upNote()->accidental()->accidentalType() == Ms::AccidentalType::SHARP);
      }

void TestNote::tpcDegrees()
      {
      QCOMPARE(tpc2degree(Tpc::TPC_C,   Key::C),   0);
      //QCOMPARE(tpc2degree(Tpc::TPC_E_S, Key::C),   3);
      QCOMPARE(tpc2degree(Tpc::TPC_B,   Key::C),   6);
      QCOMPARE(tpc2degree(Tpc::TPC_F_S, Key::C_S), 3);
      QCOMPARE(tpc2degree(Tpc::TPC_B,   Key::C_S), 6);
      QCOMPARE(tpc2degree(Tpc::TPC_B_B, Key::C_S), 6);
      //QCOMPARE(tpc2degree(Tpc::TPC_B_S, Key::C_S), 7);
      }

//---------------------------------------------------------
///   LongNoteAfterShort_183746
///    Put a small 128th rest
///    Then put a long Breve note
///    This breve will get spread out across multiple measures
///    Verifies that the resulting notes are tied over at least 3 times (to span 3 measures) and have total duration the same as a breve,
///    regardless of how the breve was divided up.
//---------------------------------------------------------

void TestNote::LongNoteAfterShort_183746() {

      Score* score = readScore(DIR + "empty.mscx");
      score->doLayout();

      score->inputState().setTrack(0);
      score->inputState().setSegment(score->tick2segment(Fraction(0,1), false, SegmentType::ChordRest));
      score->inputState().setDuration(TDuration::DurationType::V_128TH);
      score->inputState().setNoteEntryMode(true);

      score->cmdEnterRest(TDuration::DurationType::V_128TH);

      score->inputState().setDuration(TDuration::DurationType::V_BREVE);
      score->cmdAddPitch(47, 0, 0);

      Segment* s = score->tick2segment(TDuration(TDuration::DurationType::V_128TH).ticks());
      QVERIFY(s && s->segmentType() == SegmentType::ChordRest);
      QVERIFY(s->tick() == Fraction(1,128));

      Element* e = s->firstElement(0);
      QVERIFY(e && e->isNote());

      std::vector<Note*> nl = toNote(e)->tiedNotes();
      QVERIFY(nl.size() >= 3); // the breve must be divided across at least 3 measures
      Fraction totalTicks = Fraction(0,1);
      for (Note* n : nl) {
            totalTicks += n->chord()->durationTypeTicks();
            }
      Fraction breveTicks = TDuration(TDuration::DurationType::V_BREVE).ticks();
      QVERIFY(totalTicks == breveTicks); // total duration same as a breve
      }

|#



