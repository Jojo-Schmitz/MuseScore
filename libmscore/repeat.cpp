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

#include "measure.h"
#include "repeat.h"
#include "score.h"
#include "staff.h"
#include "system.h"

namespace Ms {

//---------------------------------------------------------
//   RepeatMeasure
//---------------------------------------------------------

RepeatMeasure::RepeatMeasure(Score* score)
   : Rest(score)
      {
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void RepeatMeasure::draw(QPainter* painter) const
      {
      painter->setBrush(QBrush(curColor()));
      painter->setPen(Qt::NoPen);
      painter->drawPath(path);
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void RepeatMeasure::layout()
      {
      for (Element* e : el())
            e->layout();

      Staff* st = staff();
      qreal ld = st ? st->lineDistance(tick()) : 1.0;
      qreal sp  = spatium();

      qreal y   = sp * 1.0 * ld;
      qreal w   = sp * 2.4 * ld;
      qreal h   = sp * 2.0 * ld;
      qreal lw  = sp * .50 * ld;  // line width
      qreal r   = sp * .20 * ld;  // dot radius

      setPos(0.0, (st ? (st->height() - h) / 2.0 : y) - y);

      path      = QPainterPath();

      path.moveTo(w - lw, y);
      path.lineTo(w,  y);
      path.lineTo(lw,  h+y);
      path.lineTo(0.0, h+y);
      path.closeSubpath();
      path.addEllipse(QRectF(w * .25 - r, y+h * .25 - r, r * 2.0, r * 2.0 ));
      path.addEllipse(QRectF(w * .75 - r, y+h * .75 - r, r * 2.0, r * 2.0 ));

      setbbox(path.boundingRect());
//      _space.setRw(width());
      }

//---------------------------------------------------------
//   ticks
//---------------------------------------------------------

Fraction RepeatMeasure::ticks() const
      {
      if (measure())
            return measure()->stretchedLen(staff());
      return Fraction(0, 1);
      }

//---------------------------------------------------------
//   accessibleInfo
//---------------------------------------------------------

QString RepeatMeasure::accessibleInfo() const
      {
      return Element::accessibleInfo();
      }

}

