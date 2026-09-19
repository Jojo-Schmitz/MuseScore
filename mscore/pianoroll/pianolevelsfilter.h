//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//
//  Copyright (C) 2009 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __PIANOLEVELSFILTER_H__
#define __PIANOLEVELSFILTER_H__

namespace Ms {

class Note;
class NoteEvent;
class Fraction;

//---------------------------------------------------------
//   PianoLevelsFilter
//       Manage note/event data for different views when drawing in the PianoLevels window
//---------------------------------------------------------

class PianoLevelsFilter {
public:
      static PianoLevelsFilter* FILTER_LIST[];

      virtual QString name() = 0;
      virtual QString tooltip() = 0;
      virtual int maxRange() = 0;
      virtual int minRange() = 0;
      virtual int divisionGap() = 0;  //Vertical guide line separation gap
      virtual bool isPerEvent() = 0;
      virtual int value(Note* note, NoteEvent* evt) = 0;
      virtual void setValue(Note* note, NoteEvent* evt, int value) = 0;
      virtual bool previewValue(Note* note, int previewOntime, int previewLen, const Fraction& previewNoteLen, int& value) const
            {
            Q_UNUSED(note);
            Q_UNUSED(previewOntime);
            Q_UNUSED(previewLen);
            Q_UNUSED(previewNoteLen);
            Q_UNUSED(value);
            return false;
            }
      };


//---------------------------------------------------------
//   PianoLevelFilterOnTime
//---------------------------------------------------------

class PianoLevelFilterOnTime : public PianoLevelsFilter {
      Q_DECLARE_TR_FUNCTIONS(PianoLevelFilterOnTime)

public:
      QString name() override;
      QString tooltip() override;
      int maxRange() override { return 1000; }
      int minRange() override { return -1000; }
      int divisionGap() override { return 250; }
      bool isPerEvent() override { return true; }
      int value(Note* note, NoteEvent* evt) override;
      void setValue(Note* note, NoteEvent* evt, int value) override;
      bool previewValue(Note* /*note*/, int previewOntime, int /*previewLen*/, const Fraction& /*previewNoteLen*/, int& value) const override
            {
            value = previewOntime;
            return true;
            }

      };


//---------------------------------------------------------
//   PianoLevelFilterLen
//---------------------------------------------------------


class PianoLevelFilterLenMultiplier : public PianoLevelsFilter {
      Q_DECLARE_TR_FUNCTIONS(PianoLevelFilterLen)

public:
      QString name() override;
      QString tooltip() override;
      int maxRange() override { return 1000; }
      int minRange() override { return 0; }
      int divisionGap() override { return 250; }
      bool isPerEvent() override { return true; }
      int value(Note* note, NoteEvent* evt) override;
      void setValue(Note* note, NoteEvent* evt, int value) override;
      bool previewValue(Note* /*note*/, int /*previewOntime*/, int previewLen, const Fraction& /*previewNoteLen*/, int& value) const override
            {
            value = previewLen;
            return true;
            }
      };


//---------------------------------------------------------
//   PianoLevelFilterLenOff
//---------------------------------------------------------


class PianoLevelFilterLenWholenote : public PianoLevelsFilter {
      Q_DECLARE_TR_FUNCTIONS(PianoLevelFilterLenOfftime)

public:
      QString name() override;
      QString tooltip() override;
      int maxRange() override { return 1000; }
      int minRange() override { return -1000; }
      int divisionGap() override { return 1000 / 4; }
      bool isPerEvent() override { return true; }
      int value(Note* note, NoteEvent* evt) override;
      void setValue(Note* note, NoteEvent* evt, int value) override;
      bool previewValue(Note* note, int previewOntime, int previewLen, const Fraction& previewNoteLen, int& value) const override;
      };


//---------------------------------------------------------
//   PianoLevelFilterVeloOffset
//---------------------------------------------------------


class PianoLevelFilterVeloOffset : public PianoLevelsFilter {
      Q_DECLARE_TR_FUNCTIONS(PianoLevelFilterVeloOffset)

public:
      QString name() override;
      QString tooltip() override;
      int maxRange() override { return +128; }
      int minRange() override { return -128; }
      int divisionGap() override { return 32; }
      bool isPerEvent() override { return false; }
      int value(Note* note, NoteEvent* evt) override;
      void setValue(Note* note, NoteEvent* evt, int value) override;
      };


//---------------------------------------------------------
//   PianoLevelFilterVeloUser
//---------------------------------------------------------


class PianoLevelFilterVeloUser : public PianoLevelsFilter {
      Q_DECLARE_TR_FUNCTIONS(PianoLevelFilterVeloUser)

public:
      QString name() override;
      QString tooltip() override;
      int maxRange() override { return 128; }
      int minRange() override { return 0; }
      int divisionGap() override { return 32; }
      bool isPerEvent() override { return false; }
      int value(Note* note, NoteEvent* evt) override;
      void setValue(Note* note, NoteEvent* evt, int value) override;
      };

}

#endif // __PIANOLEVELSFILTER_H__
