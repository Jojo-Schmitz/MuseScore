//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2012 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include <QtTest/QtTest>

#include "libmscore/score.h"
#include "libmscore/element.h"
#include "libmscore/chordbracket.h"
#include "libmscore/chord.h"
#include "libmscore/note.h"
#include "mtest/testutils.h"

using namespace Ms;

//---------------------------------------------------------
//   TestElement
//---------------------------------------------------------

class TestElement : public QObject, public MTest
      {
      Q_OBJECT

   private slots:
      void initTestCase() { initMTest(); }
      void testIds();
      void testChordBracketRoundTrip();
      void testChordBracketClone();
      void testChordBracketHandles();
      };

//---------------------------------------------------------
//   testIds
//---------------------------------------------------------

void TestElement::testIds()
      {
      ElementType ids[] = {
            ElementType::VOLTA,
            ElementType::OTTAVA,
            ElementType::TEXTLINE,
            ElementType::TRILL,
            ElementType::PEDAL,
            ElementType::HAIRPIN,
            ElementType::CLEF,
            ElementType::KEYSIG,
            ElementType::TIMESIG,
            ElementType::BAR_LINE,
            ElementType::ARPEGGIO,
            ElementType::CHORD_BRACKET,
            ElementType::BREATH,
            ElementType::GLISSANDO,
//            ElementType::BRACKET,
            ElementType::ARTICULATION,
            ElementType::CHORDLINE,
            ElementType::ACCIDENTAL,
            ElementType::DYNAMIC,
            ElementType::TEXT,
            ElementType::INSTRUMENT_NAME,
            ElementType::STAFF_TEXT,
            ElementType::REHEARSAL_MARK,
            ElementType::INSTRUMENT_CHANGE,
            ElementType::NOTEHEAD,
            ElementType::NOTEDOT,
            ElementType::TREMOLO,
            ElementType::LAYOUT_BREAK,
            ElementType::MARKER,
            ElementType::JUMP,
            ElementType::REPEAT_MEASURE,
            ElementType::ICON,
            ElementType::NOTE,
            ElementType::SYMBOL,
            ElementType::FSYMBOL,
            ElementType::CHORD,
            ElementType::REST,
            ElementType::SPACER,
            ElementType::STAFF_STATE,
            ElementType::TEMPO_TEXT,
            ElementType::HARMONY,
            ElementType::FRET_DIAGRAM,
            ElementType::BEND,
            ElementType::TREMOLOBAR,
            ElementType::LYRICS,
            ElementType::FIGURED_BASS,
            ElementType::STEM,
            ElementType::SLUR,
            ElementType::FINGERING,
            ElementType::HBOX,
            ElementType::VBOX,
            ElementType::TBOX,
            ElementType::FBOX,
            ElementType::MEASURE,
            ElementType::TAB_DURATION_SYMBOL,
            ElementType::OSSIA
            };

      for (ElementType t : ids) {
            Element* e = Element::create(t, score);
            Element* ee = writeReadElement(e);
            QCOMPARE(e->type(), ee->type());
            delete e;
            delete ee;
            }
      }

void TestElement::testChordBracketRoundTrip()
      {
      for (Direction hooks : { Direction::AUTO, Direction::UP, Direction::DOWN }) {
            for (bool right : { false, true }) {
                  for (int span : { 1, 2, 5 }) {
                        ChordBracket bracket(score);
                        bracket.setProperty(Pid::BRACKET_HOOK_LEN, Spatium(1.2));
                        bracket.setProperty(Pid::BRACKET_HOOK_POS, QVariant::fromValue(hooks));
                        bracket.setProperty(Pid::BRACKET_RIGHT_SIDE, right);
                        bracket.setSpan(span);
                        bracket.setUserLen1(.75 * bracket.spatium());
                        bracket.setUserLen2(-.25 * bracket.spatium());
                        Element* element = writeReadElement(&bracket);
                        QVERIFY(element && element->isChordBracket());
                        ChordBracket* copy = toChordBracket(element);
                        QCOMPARE(copy->hookLength().val(), 1.2);
                        QCOMPARE(int(copy->hookPos()), int(hooks));
                        QCOMPARE(copy->rightSide(), right);
                        QCOMPARE(copy->span(), span);
                        QCOMPARE(copy->userLen1(), bracket.userLen1());
                        QCOMPARE(copy->userLen2(), bracket.userLen2());
                        QVERIFY(!copy->playArpeggio());
                        delete copy;
                        }
                  }
            }
      }

void TestElement::testChordBracketClone()
      {
      Chord original(score);
      original.setTrack(0);
      Note* note = new Note(score);
      note->setPitch(60);
      original.add(note);
      original.add(new Arpeggio(score));
      ChordBracket* bracket = new ChordBracket(score);
      bracket->setProperty(Pid::BRACKET_HOOK_POS, QVariant::fromValue(Direction::UP));
      original.add(bracket);
      original.add(new ChordBracket(score));

      Chord* copy = original.clone();
      QVERIFY(copy->arpeggio());
      QCOMPARE(static_cast<int>(copy->el().size()), 2);
      for (Element* child : copy->el()) {
            QVERIFY(child->isChordBracket());
            QCOMPARE(child->parent(), static_cast<Element*>(copy));
            }
      QVERIFY(copy->el().front() != bracket);
      QCOMPARE(int(toChordBracket(copy->el().front())->hookPos()), int(Direction::UP));
      delete copy;
      }

void TestElement::testChordBracketHandles()
      {
      ChordBracket bracket(score);
      bracket.layout();
      auto before = bracket.gripsPositions();
      EditData drag;
      drag.curGrip = Grip::START;
      drag.delta = QPointF(0, .25 * bracket.spatium());
      bracket.editDrag(drag);
      auto after = bracket.gripsPositions();
      QCOMPARE(after[0].y(), before[0].y() + drag.delta.y());
      QCOMPARE(after[1].y(), before[1].y());
      before = after;
      drag.curGrip = Grip::END;
      bracket.editDrag(drag);
      after = bracket.gripsPositions();
      QCOMPARE(after[0].y(), before[0].y());
      QCOMPARE(after[1].y(), before[1].y() + drag.delta.y());
      }

QTEST_MAIN(TestElement)

#include "tst_element.moc"

