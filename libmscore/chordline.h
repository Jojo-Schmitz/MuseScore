//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2002-2012 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#ifndef __CHORDLINE_H__
#define __CHORDLINE_H__

#include "element.h"

namespace Ms {

class Chord;

// subtypes:
enum class ChordLineType : char {
      NOTYPE, FALL, DOIT,
      PLOP, SCOOP
      };

//---------------------------------------------------------
//   @@ ChordLine
///    bezier line attached to top note of a chord
///    implements fall, doit, plop, bend
//---------------------------------------------------------

class ChordLine final : public Element {
      bool _straight = false;
      bool _wavy = false;

      ChordLineType _chordLineType = ChordLineType::NOTYPE;
      QPainterPath path;
      bool modified = false;
      qreal _lengthX = 0.0;
      qreal _lengthY = 0.0;
      const int _initialLength = 2;
      static constexpr qreal _baseLength = 1.0;
      static constexpr qreal _waveAngle = 20;

   public:
      ChordLine(Score*);
      ChordLine(const ChordLine&);

      ChordLine* clone() const override { return new ChordLine(*this); }
      ElementType type() const override { return ElementType::CHORDLINE; }

      void setChordLineType(ChordLineType);
      ChordLineType chordLineType() const       { return _chordLineType; }
      Chord* chord() const                      { return (Chord*)(parent()); }
      bool isStraight() const           { return _straight; }
      void setStraight(bool straight)   { _straight =  straight; }
      bool isWavy() const               { return _wavy; }
      void setWavy(bool wavy)           { _wavy = wavy; }
      void setLengthX(qreal length)     { _lengthX = length; }
      void setLengthY(qreal length)     { _lengthY = length; }

      void read(XmlReader&) override;
      void write(XmlWriter& xml) const override;
      void layout() override;
      void draw(QPainter*) const override;

      void startEditDrag(EditData&) override;
      void editDrag(EditData&) override;

      QString accessibleInfo() const override;

      QVariant getProperty(Pid propertyId) const override;
      bool setProperty(Pid propertyId, const QVariant&) override;
      QVariant propertyDefault(Pid) const override;
      Pid propertyId(const QStringRef& xmlName) const override;

      Element::EditBehavior normalModeEditBehavior() const override { return Element::EditBehavior::Edit; }
      int gripsCount() const override { return _straight ? 1 : path.elementCount(); }
      Grip initialEditModeGrip() const override { return Grip(gripsCount() - 1); }
      Grip defaultGrip() const override { return initialEditModeGrip(); }
      std::vector<QPointF> gripsPositions(const EditData&) const override;

      bool isToTheLeft() const { return _chordLineType == ChordLineType::PLOP || _chordLineType == ChordLineType::SCOOP; }
      bool isBelow() const { return _chordLineType == ChordLineType::SCOOP || _chordLineType == ChordLineType::FALL; }
      };

extern const char* chordlineNames[];

}     // namespace Ms
#endif

