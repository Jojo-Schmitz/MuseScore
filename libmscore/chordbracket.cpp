//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "chordbracket.h"
#include "accidental.h"
#include "chord.h"
#include "note.h"
#include "notedot.h"
#include "part.h"
#include "score.h"
#include "segment.h"
#include "staff.h"
#include "stem.h"
#include "ledgerline.h"
#include "xml.h"

namespace Ms {

static const ElementStyle chordBracketStyle {
      { Sid::chordBracketHookLen, Pid::BRACKET_HOOK_LEN },
      };

ChordBracket::ChordBracket(Score* s) : Arpeggio(s)
      {
      setArpeggioType(ArpeggioType::BRACKET);
      setPlayArpeggio(false);
      initElementStyle(&chordBracketStyle);
      }

Chord* ChordBracket::endChord() const
      {
      if (!parent() || !parent()->isChord())
            return nullptr;
      Chord* c = chord();
      if (!c->segment() || !c->part())
            return c;
      if (span() < 1 || span() > c->part()->endTrack() - track())
            return c;
      Element* e = c->segment()->element(track() + span() - 1);
      return e && e->isChord() ? toChord(e) : c;
      }

void ChordBracket::read(XmlReader& e)
      {
      while (e.readNextStartElement()) {
            const QStringRef tag(e.name());
            if (readProperty(tag, e, Pid::BRACKET_HOOK_LEN))
                  ;
            else if (readProperty(tag, e, Pid::BRACKET_HOOK_POS))
                  ;
            else if (readProperty(tag, e, Pid::BRACKET_RIGHT_SIDE))
                  ;
            else if (tag == "userLen1")
                  setUserLen1(e.readDouble() * spatium());
            else if (tag == "userLen2")
                  setUserLen2(e.readDouble() * spatium());
            else if (tag == "span")
                  setSpan(qMax(1, e.readInt())); // track span
            else if (tag == "play" || tag == "timeStretch" || tag == "subtype")
                  e.skipCurrentElement(); // this is purely visual (never a playback arpeggio)
            else if (!Element::readProperties(e))
                  e.unknown();
            }
      }

void ChordBracket::write(XmlWriter& xml) const
      {
      if (!xml.canWrite(this))
            return;
      xml.stag(this);
      Element::writeProperties(xml);
      xml.tag("bracketHookLen", _hookLength.val());
      writeProperty(xml, Pid::BRACKET_HOOK_POS, true);
      writeProperty(xml, Pid::BRACKET_RIGHT_SIDE, true);
      if (!qFuzzyIsNull(userLen1()))
            xml.tag("userLen1", userLen1() / spatium());
      if (!qFuzzyIsNull(userLen2()))
            xml.tag("userLen2", userLen2() / spatium());
      if (span() != 1)
            xml.tag("span", span());
      xml.etag();
      }

void ChordBracket::layoutVertical(bool finalLayout)
      {
      const qreal sp = spatium();
      qreal top = 0.0;
      qreal bottom = 4.0 * sp; // initialized value for a palette item
      if (parent() && parent()->isChord() && !chord()->notes().empty()) {
            Chord* c = chord();
            Chord* end = endChord();
            top = c->upNote()->pos().y() + c->upNote()->bbox().top();
            bottom = c->downNote()->pos().y() + c->downNote()->bbox().bottom();
            if (end && end != c && !end->notes().empty()) {
                  // Before a page layout, only compare pitches on the same visual staff
                  if (finalLayout || end->vStaffIdx() == c->vStaffIdx()) {
                        const qreal dy = finalLayout ? end->pagePos().y() - c->pagePos().y()
                                                     : end->pos().y() - c->pos().y();
                        top = qMin(top, dy + end->upNote()->pos().y() + end->upNote()->bbox().top());
                        bottom = qMax(bottom, dy + end->downNote()->pos().y() + end->downNote()->bbox().bottom());
                        }
                  }
            }
      const qreal halfWidth = score()->styleP(Sid::chordBracketLineWidth) * .5;
      _top = top - userLen1() - .25 * sp;
      _bottom = bottom + userLen2() + .25 * sp;
      // Following MuseScore Studio's default minimum height and hook positions:
      const qreal extra = qMax(0.0, 2.0 * sp - (_bottom - _top + 2 * halfWidth));
      if (_hookPos == Direction::DOWN)
            _top -= extra;
      else if (_hookPos == Direction::UP)
            _bottom += extra;
      else {
            _top -= extra * .5;
            _bottom += extra * .5;
            }
      setbbox(QRectF(0.0, _top - halfWidth,
                    qMax(_hookLength.val() * sp, 2 * halfWidth), _bottom - _top + 2 * halfWidth));
      }

void ChordBracket::layout()
      {
      if (staff())
            setMag(staff()->mag(tick()));
      layoutVertical(false);
      if (!parent() || !parent()->isChord()) {
            setPos(0.0, 0.0);
            return;
            }

      Chord* owner = chord();
      qreal left = 0.0;
      qreal right = owner->upNote()->headWidth();
      auto include = [&](const QRectF& r) {
            if (r.bottom() >= _top + offset().y() && r.top() <= _bottom + offset().y()) {
                  left = qMin(left, r.left());
                  right = qMax(right, r.right());
                  }
            };
      // Look through all voices without using Chord::shape(), since shape()
      // already contains this bracket and would involve its old position
      if (owner->segment() && owner->part()) {
            for (int t = owner->part()->startTrack(); t < owner->part()->endTrack(); ++t) {
                  Element* element = owner->segment()->element(t);
                  if (!element || !element->isChord())
                        continue;
                  Chord* c = toChord(element);
                  const bool sameStaff = c->vStaffIdx() == owner->vStaffIdx();
                  if (!sameStaff && (t < track() || t - track() >= span()))
                        continue;
                  const QPointF delta(c->pos().x() - owner->pos().x(), c->pos().y() - owner->pos().y());
                  for (Note* note : c->notes()) {
                        const QPointF p = note->pos() + delta;
                        QRectF r = note->bbox().translated(p);
                        // Staff distances are not yet available for cross-staff spans
                        if (!sameStaff) {
                              left = qMin(left, r.left());
                              right = qMax(right, r.right());
                              }
                        else
                              include(r);
                        if (note->accidental()) {
                              r = note->accidental()->bbox().translated(p + note->accidental()->pos());
                              if (!sameStaff)
                                    left = qMin(left, r.left());
                              else
                                    include(r);
                              }
                        for (NoteDot* dot : note->dots()) {
                              r = dot->bbox().translated(p + dot->pos());
                              if (!sameStaff)
                                    right = qMax(right, r.right());
                              else
                                    include(r);
                              }
                        }
                  if (c->stem() && sameStaff)
                        include(c->stem()->bbox().translated(c->stem()->pos() + delta));
                  for (LedgerLine* l = c->ledgerLines(); l; l = l->next()) {
                        const QRectF r = l->bbox().translated(l->pos() + delta);
                        if (!sameStaff) {
                              left = qMin(left, r.left());
                              right = qMax(right, r.right());
                              }
                        else
                              include(r);
                        }
                  if (c->arpeggio() && sameStaff)
                        include(c->arpeggio()->bbox().translated(c->arpeggio()->pos() + delta));
                  }
            }
      const qreal gap = score()->styleP(Sid::chordBracketNoteDistance);
      setPos(_rightSide ? right + gap - bbox().left() : left - gap - bbox().right(), 0.0);
      }

void ChordBracket::layoutSegment(Segment* segment)
      {
      // All voices now have notehead/accidental positions. The first chord's
      // layout must not depend on outdated geometries from another voice/staff
      for (Element* e : segment->elist()) {
            if (!e || !e->isChord())
                  continue;
            Chord* c = toChord(e);
            for (Element* child : c->el()) {
                  if (child->isChordBracket())
                        child->layout();
                  }
            }
      }

void ChordBracket::layout2()
      {
      // staff distances are now finalized. Retain horizontal placement
      layoutVertical(true);
      }

void ChordBracket::draw(QPainter* p) const
      {
      const qreal w = _hookLength.val() * spatium();
      const qreal lw = score()->styleP(Sid::chordBracketLineWidth);
      const qreal x = _rightSide ? w - lw * .5 : lw * .5;
      p->setPen(QPen(curColor(), lw, Qt::SolidLine, Qt::FlatCap));
      p->drawLine(QLineF(x, _top, x, _bottom));
      if (_hookPos != Direction::DOWN)
            p->drawLine(QLineF(0.0, _top, w, _top));
      if (_hookPos != Direction::UP)
            p->drawLine(QLineF(0.0, _bottom, w, _bottom));
      }

std::vector<QPointF> ChordBracket::gripsPositions(const EditData&) const
      {
      const qreal x = _rightSide ? _hookLength.val() * spatium() : 0.0;
      return { pagePos() + QPointF(x, _top), pagePos() + QPointF(x, _bottom) };
      }

QVector<QLineF> ChordBracket::gripAnchorLines(Grip grip) const
      {
      if (!parent() || !parent()->isChord() || chord()->notes().empty())
            return {};
      Chord* end = endChord();
      const QPointF anchor = grip == Grip::END ? end->downNote()->canvasPos() : chord()->upNote()->canvasPos();
      const qreal x = _rightSide ? _hookLength.val() * spatium() : 0.0;
      return { QLineF(anchor, canvasPos() + QPointF(x, grip == Grip::END ? _bottom : _top)) };
      }

QVector<QLineF> ChordBracket::dragAnchorLines() const
      {
      return gripAnchorLines(Grip::START);
      }

void ChordBracket::editDrag(EditData& ed)
      {
      if (ed.curGrip == Grip::START)
            setUserLen1(userLen1() - ed.delta.y());
      else if (ed.curGrip == Grip::END)
            setUserLen2(userLen2() + ed.delta.y());
      layout2();
      triggerLayout();
      }

bool ChordBracket::edit(EditData& ed)
      {
      if ((ed.curGrip != Grip::END && ed.curGrip != Grip::START) || !(ed.modifiers & Qt::ShiftModifier)
          || !parent() || !parent()->isChord() || !chord()->part())
            return false;
      const int step = ed.key == Qt::Key_Down ? 1 : ed.key == Qt::Key_Up ? -1 : 0;
      if (!step)
            return false;

      Segment* s = chord()->segment();
      if (!s)
            return true;
      const int start = track();
      const int end = start + span() - 1;
      const int partStart = chord()->part()->startTrack();
      const int partEnd = chord()->part()->endTrack();
      auto chordAt = [s](int t) -> Chord* {
            Element* e = s->element(t);
            return e && e->isChord() ? toChord(e) : nullptr;
            };
      auto nextChord = [&](int from, int direction, int limit) -> Chord* {
            for (int t = from + direction; direction > 0 ? t < limit : t >= limit; t += direction) {
                  if (Chord* c = chordAt(t))
                        return c;
                  }
            return nullptr;
            };
      auto moveAnchor = [&](Chord* c) {
            ChordBracket* replacement = clone();
            replacement->setParent(c);
            replacement->setTrack(c->track());
            replacement->setSpan(end - c->track() + 1);
            score()->undoRemoveElement(this);
            score()->undoAddElement(replacement);
            // Keyboard editing keeps its own element pointer between presses
            ed.element = replacement;
            ed.clearData();
            replacement->startEdit(ed);
            score()->select(replacement, SelectType::ADD);
            };

      if (ed.curGrip == Grip::END) {
            if (step > 0) {
                  if (Chord* c = nextChord(end, 1, partEnd))
                        undoChangeProperty(Pid::BRACKET_SPAN, c->track() - start + 1);
                  }
            else if (end > start) {
                  if (Chord* c = nextChord(end, -1, start))
                        undoChangeProperty(Pid::BRACKET_SPAN, c->track() - start + 1);
                  else
                        undoChangeProperty(Pid::BRACKET_SPAN, 1);
                  }
            else if (Chord* c = nextChord(start, -1, partStart)) {
                  // An upward extension must be anchored to the upper chord:
                  // MuseScore Studio's spanning is always positive
                  moveAnchor(c);
                  }
            }
      else if (step < 0) {
            if (Chord* c = nextChord(start, -1, partStart)) {
                  moveAnchor(c);
                  }
            }
      else if (start < end) {
            if (Chord* c = nextChord(start, 1, end + 1)) {
                  moveAnchor(c);
                  }
            }
      return true;
      }

void ChordBracket::reset()
      {
      for (Pid p : { Pid::BRACKET_HOOK_LEN, Pid::BRACKET_HOOK_POS, Pid::BRACKET_RIGHT_SIDE, Pid::BRACKET_SPAN })
            undoResetProperty(p);
      Arpeggio::reset();
      }

QVariant ChordBracket::getProperty(Pid p) const
      {
      switch (p) {
            case Pid::BRACKET_HOOK_LEN: return _hookLength;
            case Pid::BRACKET_HOOK_POS: return QVariant::fromValue(_hookPos);
            case Pid::BRACKET_RIGHT_SIDE: return _rightSide;
            case Pid::BRACKET_SPAN: return span();
            default: return Arpeggio::getProperty(p);
            }
      }

bool ChordBracket::setProperty(Pid p, const QVariant& v)
      {
      switch (p) {
            case Pid::BRACKET_HOOK_LEN: _hookLength = Spatium(qMax(0.0, v.value<Spatium>().val())); break;
            case Pid::BRACKET_HOOK_POS: _hookPos = v.value<Direction>(); break;
            case Pid::BRACKET_RIGHT_SIDE: _rightSide = v.toBool(); break;
            case Pid::BRACKET_SPAN: setSpan(qMax(1, v.toInt())); break;
            default: return Arpeggio::setProperty(p, v);
            }
      triggerLayout();
      return true;
      }

QVariant ChordBracket::propertyDefault(Pid p) const
      {
      switch (p) {
            case Pid::BRACKET_HOOK_LEN: return score()->styleS(Sid::chordBracketHookLen);
            case Pid::BRACKET_HOOK_POS: return QVariant::fromValue(Direction::AUTO);
            case Pid::BRACKET_RIGHT_SIDE: return false;
            case Pid::BRACKET_SPAN: return 1;
            case Pid::PLAY: return false;
            default: return Arpeggio::propertyDefault(p);
            }
      }
}
