//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#ifndef __CHORDBRACKET_H__
#define __CHORDBRACKET_H__

#include "arpeggio.h"

namespace Ms {
class Segment;

// Unlike arpeggio brackets, ChordBrackets are independent chord children.
// Their XML span counts tracks, not staves. Actually, MuseScore Studio seems
// to have updated regular Arpeggios to span voices at some point.
// …should try to backport that also.
class ChordBracket final : public Arpeggio {
      Spatium _hookLength { .7 };
      Direction _hookPos { Direction::AUTO };
      bool _rightSide { false };
      qreal _top { 0.0 };
      qreal _bottom { 0.0 };

      void layoutVertical(bool finalLayout);
      QVector<QLineF> gripAnchorLines(Grip) const override;
      QVector<QLineF> dragAnchorLines() const override;

   public:
      ChordBracket(Score*);
      ChordBracket* clone() const override { return new ChordBracket(*this); }
      ElementType type() const override { return ElementType::CHORD_BRACKET; }

      Spatium hookLength() const { return _hookLength; }
      Direction hookPos() const { return _hookPos; }
      bool rightSide() const { return _rightSide; }
      Chord* endChord() const;

      void read(XmlReader&) override;
      void write(XmlWriter&) const override;
      void layout() override;
      static void layoutSegment(Segment*);
      void layout2();
      void draw(QPainter*) const override;
      std::vector<QPointF> gripsPositions(const EditData& = EditData()) const override;
      bool edit(EditData&) override;
      void editDrag(EditData&) override;
      bool acceptDrop(EditData&) const override { return false; }
      void reset() override;

      QVariant getProperty(Pid) const override;
      bool setProperty(Pid, const QVariant&) override;
      QVariant propertyDefault(Pid) const override;
      };
}
#endif
