//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2013 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include <QtTest/QtTest>
#include "libmscore/score.h"
#include "mtest/testutils.h"

#define DIR QString("tabledit/io/")

using namespace Ms;

//---------------------------------------------------------
//   TestTablEditIO
//---------------------------------------------------------

class TestTablEditIO : public QObject, public MTest
      {
      Q_OBJECT

      void tefReadTest(const char* file);

private slots:
      void initTestCase();

      // The list of TablEdit regression tests
      // Currently failing tests are commented out and annotated with the failure reason

      void tefBass() { tefReadTest("bass"); }
      void tefChord_C_D() { tefReadTest("chord_C_D"); }
      void tefCmaj_alternatives() { tefReadTest("cmaj_alternatives"); }
      void tefGrace_1() { tefReadTest("grace_1"); }
      void tefGuitar() { tefReadTest("guitar"); }
      void tefGuitar_bass() { tefReadTest("guitar_bass"); }
      void tefGuitar_drop_D() { tefReadTest("guitar_drop_D"); }
      void tefGuitar_new_standard_tuning() { tefReadTest("guitar_new_standard_tuning"); }
      void tefKey_signatures() { tefReadTest("key_signatures"); }
      void tefKey_signatures_2() { tefReadTest("key_signatures_2"); }
      void tefMetadata() { tefReadTest("metadata"); }
      void tefMulti_track_rests() { tefReadTest("multi_track_rests"); }
      void tefNotes_dotted() { tefReadTest("notes_dotted"); }
      void tefNotes_normal() { tefReadTest("notes_normal"); }
      void tefPositions() { tefReadTest("positions"); }
      void tefReading_list_1() { tefReadTest("reading_list_1"); }
      void tefRests_dotted() { tefReadTest("rests_dotted"); }
      void tefRests_normal() { tefReadTest("rests_normal"); }
      void tefStaff_text_1() { tefReadTest("staff_text_1"); }
      void tefStaff_text_2() { tefReadTest("staff_text_2"); }
      void tefTie_1() { tefReadTest("tie_1"); }
      void tefTie_2() { tefReadTest("tie_2"); }
      void tefTime_signatures() { tefReadTest("time_signatures"); }
      void tefTime_signatures_2() { tefReadTest("time_signatures_2"); }
      void tefTriplet_eighths() { tefReadTest("triplet_eighths"); }
      void tefTriplet_quarters() { tefReadTest("triplet_quarters"); }
      void tefTriplets_mixed() { tefReadTest("triplets_mixed"); }
      void tefVoices() { tefReadTest("voices"); }
      void tefVoices_multi_part() { tefReadTest("voices_multi_part"); }
      };

//---------------------------------------------------------
//   initTestCase
//---------------------------------------------------------

void TestTablEditIO::initTestCase()
      {
      initMTest();
      }

//---------------------------------------------------------
//   tefReadTest
//   read a TablEdit file, write to a MuseScore file and verify against reference
//---------------------------------------------------------

void TestTablEditIO::tefReadTest(const char* file)
      {
      MasterScore* score = readScore(DIR + file + ".tef");
      QVERIFY(score);
      QVERIFY(saveCompareScore(score, QString("%1.tef.mscx").arg(file),
                               DIR + QString("%1.tef-ref.mscx").arg(file)));
      delete score;
      }

QTEST_MAIN(TestTablEditIO)
#if __has_include("tst_tabledit_io.moc")
#include "tst_tabledit_io.moc"
#endif
