//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2010-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "shadownote.h"
#include "score.h"
#include "drumset.h"
#include "sym.h"
#include "rest.h"
#include "mscore.h"
#include "accidental.h"

namespace Ms {

//---------------------------------------------------------
//   ShadowNote
//---------------------------------------------------------

ShadowNote::ShadowNote(Score* s)
   : Element(s), _notehead(SymId::noSym)
      {
      _line = 1000;
      _duration = TDuration(TDuration::DurationType::V_INVALID);
      _voice = 0;
      _rest = false;
      }

bool ShadowNote::isValid() const
      {
      return _notehead != SymId::noSym;
      }

void ShadowNote::setState(SymId noteSymbol, int voice, TDuration duration, bool rest)
      {
      // clear symbols
      _notehead = SymId::noSym;

      _notehead = noteSymbol;
      _duration = duration;
      _voice    = voice;
      _rest     = rest;
      }

SymId ShadowNote::getNoteFlag() const
      {
      SymId flag = SymId::noSym;
      if (_rest)
            return flag;
      TDuration::DurationType type = _duration.type();
      switch (type) {
            case TDuration::DurationType::V_LONG:
                  flag = SymId::lastSym;
                  break;
            case TDuration::DurationType::V_BREVE:
                  flag = SymId::noSym;
                  break;
            case TDuration::DurationType::V_WHOLE:
                  flag = SymId::noSym;
                  break;
            case TDuration::DurationType::V_HALF:
                  flag = SymId::lastSym;
                  break;
            case TDuration::DurationType::V_QUARTER:
                  flag = SymId::lastSym;
                  break;
            case TDuration::DurationType::V_EIGHTH:
                  flag = computeUp() ? SymId::flag8thUp : SymId::flag8thDown;
                  break;
            case TDuration::DurationType::V_16TH:
                  flag = computeUp() ? SymId::flag16thUp : SymId::flag16thDown;
                  break;
            case TDuration::DurationType::V_32ND:
                  flag = computeUp() ? SymId::flag32ndUp : SymId::flag32ndDown;
                  break;
            case TDuration::DurationType::V_64TH:
                  flag = computeUp() ? SymId::flag64thUp : SymId::flag64thDown;
                  break;
            case TDuration::DurationType::V_128TH:
                  flag = computeUp() ? SymId::flag128thUp : SymId::flag128thDown;
                  break;
            case TDuration::DurationType::V_256TH:
                  flag = computeUp() ? SymId::flag256thUp : SymId::flag256thDown;
                  break;
            case TDuration::DurationType::V_512TH:
                  flag = computeUp() ? SymId::flag512thUp : SymId::flag512thDown;
                  break;
            case TDuration::DurationType::V_1024TH:
                  flag = computeUp() ? SymId::flag1024thUp : SymId::flag1024thDown;
                  break;
            default:
                  flag = SymId::noSym;
            }
      return flag;
      }

bool ShadowNote::computeUp() const
      {
      if (_voice % VOICES == 0)
            return _line > 4;
      else
            return _voice % VOICES == 2;
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void ShadowNote::draw(QPainter* painter) const
      {
      if (!visible() || !isValid() || MScore::disableMouseEntry)
            return;

      QPointF ap(pagePos());
      painter->translate(ap);
      qreal lw = score()->styleP(Sid::stemWidth);
      QPen pen(MScore::selectColor[_voice].lighter(SHADOW_NOTE_LIGHT), lw, Qt::SolidLine, Qt::FlatCap);
      painter->setPen(pen);

      bool up = computeUp();

      // draw the accidental
      SymId acc = Accidental::subtype2symbol(score()->inputState().accidentalType());
      if (acc != SymId::noSym) {
            QPointF posAcc;
            posAcc.rx() -= symWidth(acc) + score()->styleP(Sid::accidentalNoteDistance) * mag();
            drawSymbol(acc, painter, posAcc);
            }

      // draw the notehead
      drawSymbol(_notehead, painter);

      // draw the dots
      qreal noteheadWidth = symWidth(_notehead);
      qreal spmag  = spatium() * mag();
      qreal spmag5 = 0.5 * spmag;
      qreal sp5    = 0.5 * spatium();
      QPointF posDot;
      if (_duration.dots() > 0) {
            qreal d  = score()->styleP(Sid::dotNoteDistance) * mag();
            qreal dd = score()->styleP(Sid::dotDotDistance) * mag();
            posDot.rx() += (noteheadWidth + d);
            if (!_rest)
                  posDot.ry() -= (_line % 2 == 0 ? sp5 : 0);
            else
                  posDot.ry() += Rest::getDotline(_duration.type()) * spmag5;
            SymId flag = getNoteFlag();
            if ((flag != SymId::lastSym) && up)
                  posDot.rx() = qMax(posDot.rx(), noteheadWidth + symBbox(flag).right());
            for (int i = 0; i < _duration.dots(); i++) {
                  posDot.rx() += dd * i;
                  drawSymbol(SymId::augmentationDot, painter, posDot, 1);
                  posDot.rx() -= dd * i;
                  }
            }

      // stem and flag
      SymId flag = getNoteFlag();
      if (flag != SymId::noSym) {
            qreal x  =  up ? (noteheadWidth - (lw / 2)) : lw / 2;
            qreal y1 = (up ? symStemUpSE(_notehead) : symStemDownNW(_notehead)).y() * mag();
            qreal y2 = (up ? -3.5 : 3.5) * spmag;
            
            if (flag != SymId::lastSym) { // If there is a flag
                  QPointF flagPoint(x - (lw / 2), y2);
                  drawSymbol(flag, painter, flagPoint, 1);
                  y2 += ( up ? symStemUpNW(flag) : symStemDownSW(flag) ).y();
                  }
            painter->drawLine(QLineF(x, y1, x, y2));
            }

      // Ledger lines
      qreal extraLen = score()->styleP(Sid::ledgerLineLength) * mag();
      qreal x1 = -extraLen;
      qreal x2 = noteheadWidth + extraLen;

      lw = score()->styleP(Sid::ledgerLineWidth);
      painter->setPen(QPen(MScore::selectColor[_voice].lighter(SHADOW_NOTE_LIGHT), lw, Qt::SolidLine, Qt::FlatCap));

      if (_line < 100 && _line > -100 && !_rest) {
            for (int i = -2; i >= _line; i -= 2) {
                  qreal y = spmag5 * (i - _line);
                  painter->drawLine(QLineF(x1, y, x2, y));
                  }
            for (int i = 10; i <= _line; i += 2) {
                  qreal y = spmag5 * (i - _line);
                  painter->drawLine(QLineF(x1, y, x2, y));
                  }
            }
      painter->translate(-ap);
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void ShadowNote::layout()
      {
      if (!isValid()) {
            setbbox(QRectF());
            return;
            }
      qreal _spatium = spatium();
      QRectF b;
      QRectF noteheadBbox = symBbox(_notehead);
      SymId flag = getNoteFlag();
      qreal height = noteheadBbox.height();
      qreal dotWidth = 0;
      if (_duration.dots() > 0) {
            qreal noteheadWidth = symWidth(_notehead);
            qreal d  = score()->styleP(Sid::dotNoteDistance) * mag();
            qreal dd = score()->styleP(Sid::dotDotDistance) * mag();
            dotWidth += (noteheadWidth + d);
            for (int i = 0; i < _duration.dots(); i++)
                  dotWidth += dd * i;
            height += (_line % 2 == 0 && !_rest ? 0.5 * spatium() : 0); // move flag up
            }
      if(flag == SymId::noSym) {
            b.setRect(noteheadBbox.x(), noteheadBbox.y(), noteheadBbox.width() + dotWidth, noteheadBbox.height());
            }
      else {
            qreal x = noteheadBbox.x();
            qreal width = noteheadBbox.width();
            qreal flagWidth = 0;
            int up = computeUp() ? 1 : 0;
            qreal y = up ? 0 : -height;
            if (flag != SymId::lastSym) {
                  QRectF flagBbox = symBbox(flag);
                  qreal lw = score()->styleP(Sid::stemWidth) * mag();
                  qreal h =  flagBbox.height() + lw / 2 + spatium() * mag();
                  y -= h * up;
                  height += h;
                  flagWidth = flagBbox.width();
                  }
            else {
                  qreal sh = 4 * spatium() * mag();
                  y -=  up * sh;
                  height += sh;
                  }
            width += qMax(flagWidth, dotWidth);

            b.setRect(x, y, width, height);
            }

      qreal lw = score()->styleP(Sid::ledgerLineWidth);

      qreal x1 = (noteheadBbox.width()) * .5 - (_spatium * mag()) - lw * .5;

      qreal x2 = x1 + 2 * _spatium * mag() + lw * .5;

      InputState ps = score()->inputState();
      QRectF r(x1, -lw * .5, x2 - x1, lw);
      if (_line < 100 && _line > -100 && !ps.rest()) {
            for (int i = -2; i >= _line; i -= 2)
                  b |= r.translated(QPointF(0, _spatium * .5 * (i - _line)));
            for (int i = 10; i <= _line; i += 2)
                  b |= r.translated(QPointF(0, _spatium * .5 * (i - _line)));
            }
      setbbox(b);
      }
}

