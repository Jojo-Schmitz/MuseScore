//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2002-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#ifndef __MEASUREREPEAT_H__
#define __MEASUREREPEAT_H__

#include "rest.h"
#include "utils.h"

namespace Ms {
class Score;
class Segment;

//---------------------------------------------------------
//   @@ MeasureRepeat
//---------------------------------------------------------

class MeasureRepeat final : public Rest
      {
public:
      MeasureRepeat(Score*);
      MeasureRepeat& operator=(const MeasureRepeat&) = delete;

      MeasureRepeat* clone() const override { return new MeasureRepeat(*this); }
      Element* linkedClone() override { return Element::linkedClone(); }
      ElementType type() const override { return ElementType::MEASURE_REPEAT; }

      void setNumMeasures(int n) { _numMeasures = n; }
      int numMeasures() const { return _numMeasures; }
      void setSymId(SymId id) { _symId = id; }
      SymId symId() const { return _symId; }
      void setNumberSym(int n) { _numberSym = toTimeSigString(QString::number(n)); }
      std::vector<SymId> numberSym() const { return _numberSym; }
      void setNumberPos(qreal d) { _numberPos = d; }
      qreal numberPos() const { return _numberPos; }

      Measure* firstMeasureOfGroup() const { return measure()->firstOfMeasureRepeatGroup(staffIdx()); }

      void draw(QPainter*) const override;
      void layout() override;
      Fraction ticks() const override;
      Fraction actualTicks() const { return Rest::ticks(); }

      void read(XmlReader&) override;
      void write(XmlWriter& xml) const override;

      QVariant propertyDefault(Pid) const override;
      bool setProperty(Pid, const QVariant&) override;
      QVariant getProperty(Pid) const override;

      QRectF numberRect() const override;
      Shape shape() const override;

      QString accessibleInfo() const override;

      bool placeMultiple() const override { return numMeasures() == 1; }     // prevent overlapping additions with range selection

private:
      Sid getPropertyStyle(Pid) const override;
      int _numMeasures;
      std::vector<SymId> _numberSym;
      qreal _numberPos;
      SymId _symId;
      };
}     // namespace Ms
#endif
