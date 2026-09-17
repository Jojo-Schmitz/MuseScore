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

#include "pianoruler.h"
#include "pianokeyboard.h"
#include "libmscore/score.h"

namespace Ms {

#if 0 // yet(?) unused
static const int MAP_OFFSET = 480;
#endif

QPixmap* PianoRuler::markIcon[3];

static const char* rmark_xpm[]={
      "18 18 2 1",
      "# c #0000ff",
      ". c None",
      "..................",
      "..................",
      "..................",
      "..................",
      "..................",
      "..................",
      "..................",
      "........##########",
      "........#########.",
      "........########..",
      "........#######...",
      "........######....",
      "........#####.....",
      "........####......",
      "........###.......",
      "........##........",
      "........##........",
      "........##........"};
static const char* lmark_xpm[]={
      "18 18 2 1",
      "# c #0000ff",
      ". c None",
      "..................",
      "..................",
      "..................",
      "..................",
      "..................",
      "..................",
      "..................",
      "##########........",
      ".#########........",
      "..########........",
      "...#######........",
      "....######........",
      ".....#####........",
      "......####........",
      ".......###........",
      "........##........",
      "........##........",
      "........##........"};
static const char* cmark_xpm[]={
      "18 18 2 1",
      "# c #ff0000",
      ". c None",
      "..................",
      "..................",
      "..................",
      "..................",
      "..................",
      "..................",
      "..................",
      "##################",
      ".################.",
      "..##############..",
      "...############...",
      "....##########....",
      ".....########.....",
      "......######......",
      ".......####.......",
      "........##........",
      "........##........",
      "........##........"};


//---------------------------------------------------------
//   Ruler
//---------------------------------------------------------

PianoRuler::PianoRuler(QWidget* parent)
   : QWidget(parent)
      {
      if (markIcon[0] == 0) {
            markIcon[0] = new QPixmap(cmark_xpm);
            markIcon[1] = new QPixmap(rmark_xpm);
            markIcon[2] = new QPixmap(lmark_xpm);
            }
      setMouseTracking(true);
      _xpos       = 0;
      _score      = nullptr;
      _locator    = nullptr;
      _xZoom      = X_ZOOM_INITIAL;
      _timeType = TType::TICKS;
      _font2.setPixelSize(14);
      _font2.setBold(true);
      _font1.setPixelSize(10);
      }

//---------------------------------------------------------
//   setScore
//---------------------------------------------------------

void PianoRuler::setScore(Score* s, Pos* lc)
      {
      _score = s;
      _locator = lc;
      if (_score)
            _cursor.setContext(_score->tempomap(), _score->sigmap());
      setEnabled(_score != 0);
      }

//---------------------------------------------------------
//   setOrientation
//---------------------------------------------------------

void PianoRuler::setOrientation(PianoRollOrientation orientation)
      {
      if (_orientation == orientation)
            return;

      _playbackLocatorTickValid = false;
      _orientation = orientation;
      update();
      }

//---------------------------------------------------------
//   setPianoView
//---------------------------------------------------------

void PianoRuler::setPianoView(PianoView* view)
      {
      _pianoView = view;
      }

//---------------------------------------------------------
//   playbackLocatorActive
//---------------------------------------------------------

bool PianoRuler::playbackLocatorActive() const
      {
      return _orientation == PianoRollOrientation::HORIZONTAL
             && _playbackLocatorTickValid;
      }

//---------------------------------------------------------
//   setLoopEnabled
//---------------------------------------------------------

void PianoRuler::setLoopEnabled(bool enabled)
      {
      _loopEnabled = enabled;
      }

//---------------------------------------------------------
//   setXpos
//---------------------------------------------------------

void PianoRuler::setXpos(int val)
      {
      if (_xpos == val)
            return;

      const int dx = _xpos - val;
      _xpos = val;

      if (_orientation == PianoRollOrientation::HORIZONTAL
          && qAbs(dx) < width()) {
            scroll(dx, 0);
            }
      else {
            update();
            }
      }

//---------------------------------------------------------
//   pos2pix
//---------------------------------------------------------

int PianoRuler::pos2pix(const Pos& pos) const
      {
      const int tick = pos.time(TType::TICKS);

      if (_orientation == PianoRollOrientation::HORIZONTAL)
            return (tick + MAP_OFFSET) * _xZoom - _xpos;

      if (_pianoView) {
            const int sceneY = _pianoView->tickToPixelY(tick);

            return _pianoView->mapFromScene(
                  QPointF(0.0, sceneY)).y();
            }

      return 0;
      }

//---------------------------------------------------------
//    pix2pos
//---------------------------------------------------------

Pos PianoRuler::pix2pos(int pixel) const
      {
      int tick = 0;

      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            tick = (pixel + _xpos) / _xZoom - MAP_OFFSET;
            }
      else if (_pianoView) {
            // Ruler Y corresponds directly to PianoView viewport Y.
            // Convert that viewport position into scene coordinates,
            // then let PianoView perform its authoritative
            // scene-Y -> tick conversion
            const QPointF scenePos =
                  _pianoView->mapToScene(QPoint(0, pixel));

            tick = _pianoView->pixelYToTick(qRound(scenePos.y()));
            }

      if (tick < 0)
            tick = 0;

      return Pos(
            _score->tempomap(),
            _score->sigmap(),
            tick,
            _timeType);
      }

//---------------------------------------------------------
//   paintEvent
//---------------------------------------------------------

void PianoRuler::paintEvent(QPaintEvent* e)
      {
      if (_orientation == PianoRollOrientation::HORIZONTAL)
            paintHorizontal(e);
      else
            paintVertical(e);
      }

//---------------------------------------------------------
//   paintHorizontal
//---------------------------------------------------------

void PianoRuler::paintHorizontal(QPaintEvent* e)
      {
      QPainter p(this);
      const QRect& r = e->rect();

      int x  = r.x();
      int w  = r.width();
      int y  = pianoRulerHeight - 16;
      int h  = 16; // 14;
      int y1 = r.y();
      int rh = r.height();
      if (y1 < pianoRulerHeight) {
            rh -= pianoRulerHeight - y1;
            y1 = pianoRulerHeight;
            }
      int y2 = y1 + rh;

      if (x < (-_xpos))
            x = -_xpos;

      if (!_score)
            return;

      Pos pos1 = pix2pos(x);
      Pos pos2 = pix2pos(x+w);

      //---------------------------------------------------
      //    draw lines
      //---------------------------------------------------

      int bar1, bar2, beat, tick;

      pos1.mbt(&bar1, &beat, &tick);
      pos2.mbt(&bar2, &beat, &tick);

      const int minBarGapSize = 48;
      const int minBeatGapSize = 30;

      //Estimate bar width since changing time signatures can make this inconsistent.
      // Assuming 480 ticks per beat, 4 beats per bar
      qreal pixPerBar = DIVISION * 4 * _xZoom;
      qreal pixPerBeat = DIVISION * _xZoom;

      int barSkip = ceil(minBarGapSize / pixPerBar);
      barSkip = (int)pow(2, ceil(log(barSkip)/log(2)));

      int beatSkip = ceil(minBeatGapSize / pixPerBeat);
      beatSkip = (int)pow(2, ceil(log(beatSkip)/log(2)));

      //Round down to first bar to be a multiple of barSkip
      bar1 = (bar1 / barSkip) * barSkip;

      for (int bar = bar1; bar <= bar2; bar += barSkip) {
            Pos stick(_score->tempomap(), _score->sigmap(), bar, 0, 0);

            SigEvent sig = stick.timesig();
            int z = sig.timesig().numerator();
            for (int beat1 = 0; beat1 < z; beat1 += beatSkip) {
                  Pos xx(_score->tempomap(), _score->sigmap(), bar, beat1, 0);
                  int xp = pos2pix(xx);
                  if (xp < 0)
                        continue;
                  QString s;
                  QRect r1(xp+2, y + 1, 1000, h);
                  int y3;
                  int num;
                  if (beat1 == 0) {
                        num = bar + 1;
                        y3  = y + 2;
                        p.setFont(_font2);
                        }
                  else {
                        num = beat1 + 1;
                        y3  = y + 8;
                        p.setFont(_font1);
                        r1.moveTop(r1.top() + 1);
                        }
                  s.setNum(num);
                  p.setPen(Qt::black);
                  p.drawLine(xp, y3, xp, y+h);
                  p.drawText(r1, Qt::AlignLeft | Qt::AlignVCenter, s);
                  p.setPen(beat1 == 0 ? Qt::lightGray : Qt::gray);
                  if (xp > 0)
                        p.drawLine(xp, y1, xp, y2);
                  }

            }
      //
      //  draw mouse cursor marker
      //
      p.setPen(Qt::black);
      if (_cursor.valid()) {
            int xp = pos2pix(_cursor);
            if (xp >= x && xp < x+w)
                  p.drawLine(xp, 0, xp, pianoRulerHeight);
            }

      static const QColor lcColors[3] = { Qt::red, Qt::blue, Qt::blue };

      for (int i = 0; i < 3; ++i) {
            if (!_locator[i].valid())
                  continue;

            p.setPen(lcColors[i]);

            qreal xp;

            if (i == 0 && _playbackLocatorTickValid)
                  xp = tickToPixelF(_playbackLocatorTick);
            else
                  xp = pos2pix(_locator[i]);

            QPixmap* pm = markIcon[i];
            int pw = pm->width() / 2;

            qreal x1 = x - pw;
            qreal x2 = x + w + pw;

            if (xp >= x1 && xp < x2)
                  p.drawPixmap(qRound(xp) - pw, y - 2, *pm);
            }
      }

//---------------------------------------------------------
//   paintVertical
//---------------------------------------------------------

void PianoRuler::paintVertical(QPaintEvent* e)
      {
      if (!_score || !_pianoView)
            return;

      QPainter p(this);

      const QRect r = e->rect();

      const int y1 = r.top();
      const int y2 = r.bottom();

      // Time increases upward in vertical PRE mode, so the
      // top and bottom positions may arrive in reverse musical
      // order. Normalize the bar range below

      Pos posTop    = pix2pos(y1);
      Pos posBottom = pix2pos(y2);

      int barTop, barBottom;
      int beat, tick;

      posTop.mbt(&barTop, &beat, &tick);
      posBottom.mbt(&barBottom, &beat, &tick);

      int bar1 = qMin(barTop, barBottom);
      int bar2 = qMax(barTop, barBottom);

      const int minBarGapSize  = 48;
      const int minBeatGapSize = 30;

      // Vertical time will use the same time zoom value

      qreal pixPerBar  = DIVISION * 4 * _xZoom;
      qreal pixPerBeat = DIVISION * _xZoom;

      int barSkip = ceil(minBarGapSize / pixPerBar);
      barSkip = (int)pow(2, ceil(log(barSkip) / log(2)));

      int beatSkip = ceil(minBeatGapSize / pixPerBeat);
      beatSkip = (int)pow(2, ceil(log(beatSkip) / log(2)));

      bar1 = (bar1 / barSkip) * barSkip;

      const int w = width();

      for (int bar = bar1; bar <= bar2; bar += barSkip) {
            Pos stick(
                  _score->tempomap(),
                  _score->sigmap(),
                  bar, 0, 0);

            SigEvent sig = stick.timesig();
            const int beatsInBar = sig.timesig().numerator();

            for (int beat1 = 0;
                 beat1 < beatsInBar;
                 beat1 += beatSkip) {

                  Pos beatPos(
                        _score->tempomap(),
                        _score->sigmap(),
                        bar, beat1, 0);

                  const int yp = pos2pix(beatPos);

                  if (yp < r.top() || yp > r.bottom())
                        continue;

                  const bool barLine = beat1 == 0;

                  p.setFont(barLine ? _font2 : _font1);
                  p.setPen(Qt::black);

                  // Tick mark against the PianoView edge
                  const int tickLength = barLine ? 14 : 8;

                  p.drawLine(
                        w - tickLength,
                        yp,
                        w,
                        yp);

                  QString text;
                  text.setNum(barLine ? bar + 1
                                      : beat1 + 1);

                  QRect textRect(
                        2,
                        yp - 8,
                        w - tickLength - 4,
                        16);

                  p.drawText(
                        textRect,
                        Qt::AlignRight | Qt::AlignVCenter,
                        text);
                  }
            }

      // Mouse cursor marker

      p.setPen(Qt::black);

      if (_cursor.valid()) {
            const int yp = pos2pix(_cursor);

            if (yp >= r.top() && yp <= r.bottom())
                  p.drawLine(0, yp, width(), yp);
            }

      // Playback / locator markers

      static const QColor lcColors[3] = {
            Qt::red,
            Qt::blue,
            Qt::blue
            };

      for (int i = 0; i < 3; ++i) {
            if (!_locator[i].valid())
                  continue;

            const int yp = pos2pix(_locator[i]);

            if (yp < r.top() || yp > r.bottom())
                  continue;

            p.setPen(lcColors[i]);

            // The horizontal markIcon pixmaps are oriented for
            // a horizontal ruler, so just use a line:
            p.drawLine(0, yp, width(), yp);
            }
      }

//---------------------------------------------------------
//   setPlaybackLocatorTick
//---------------------------------------------------------

void PianoRuler::setPlaybackLocatorTick(qreal tick)
      {
      if (_orientation != PianoRollOrientation::HORIZONTAL)
            return;

      const qreal oldX = _playbackLocatorTickValid
            ? tickToPixelF(_playbackLocatorTick)
            : -1.0;

      _playbackLocatorTick = tick;
      _playbackLocatorTickValid = true;

      const qreal newX = tickToPixelF(tick);

      if (oldX >= 0.0 && qRound(oldX) == qRound(newX))
            return;

      const int pw = markIcon[0]->width() / 2;
      const int margin = 2;

      if (oldX >= 0.0) {
            update(QRect(
                  qRound(oldX) - pw - margin,
                  0,
                  markIcon[0]->width() + margin * 2,
                  height()));
            }

      update(QRect(
            qRound(newX) - pw - margin,
            0,
            markIcon[0]->width() + margin * 2,
            height()));
      }

//---------------------------------------------------------
//   clearPlaybackLocatorTick
//---------------------------------------------------------

void PianoRuler::clearPlaybackLocatorTick()
      {
      if (!_playbackLocatorTickValid)
            return;

      _playbackLocatorTickValid = false;
      update();
      }

//---------------------------------------------------------
//   tickToPixelF
//---------------------------------------------------------

qreal PianoRuler::tickToPixelF(qreal tick) const
      {
      if (_orientation == PianoRollOrientation::HORIZONTAL)
            return (tick + MAP_OFFSET) * _xZoom - _xpos;

      if (_pianoView) {
            const qreal sceneY = _pianoView->tickToPixelYF(tick);
            return _pianoView->mapFromScene(
                  QPointF(0.0, sceneY)).y();
            }

      return 0.0;
      }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void PianoRuler::mousePressEvent(QMouseEvent* e)
      {
      if (_loopEnabled && e->button() == Qt::LeftButton) {
            const int pixel =
                  _orientation == PianoRollOrientation::HORIZONTAL
                        ? e->pos().x()
                        : e->pos().y();

            Pos pos(pix2pos(pixel));

            // Ctrl+click sets [loop-in] Shift+click sets [loop-out]
            // Ctrl wins if both modifiers are present
            if (e->modifiers() & Qt::ControlModifier) {
                  if (_pianoView) {
                        const int tick = pos.time(TType::TICKS);
                        const Fraction snapped =
                              _pianoView->snapTickToGrid(tick, Direction::DOWN);

                        pos = Pos(
                              _score->tempomap(),
                              _score->sigmap(),
                              snapped.ticks(),
                              _timeType);
                        }

                  emit locatorMoved(1, pos);
                  return;
                  }

            if (e->modifiers() & Qt::ShiftModifier) {
                  if (_pianoView) {
                        const int tick = pos.time(TType::TICKS);
                        const Fraction snapped =
                              _pianoView->snapTickToGrid(tick, Direction::UP);

                        pos = Pos(
                              _score->tempomap(),
                              _score->sigmap(),
                              snapped.ticks(),
                              _timeType);
                        }

                  emit locatorMoved(2, pos);
                  return;
                  }
            }

      moveLocator(e);
      }

//---------------------------------------------------------
//   mouseMoveEvent
//---------------------------------------------------------

void PianoRuler::mouseMoveEvent(QMouseEvent* e)
      {
      if (_loopEnabled
          && (e->buttons() & Qt::LeftButton)
          && (e->modifiers()
              & (Qt::ControlModifier | Qt::ShiftModifier))) {
            return;
            }

      moveLocator(e);
      }

//---------------------------------------------------------
//   moveLocator
//---------------------------------------------------------

void PianoRuler::moveLocator(QMouseEvent* e)
      {
      const int pixel =
            _orientation == PianoRollOrientation::HORIZONTAL
                  ? e->pos().x()
                  : e->pos().y();

      Pos pos(pix2pos(pixel));

      if (e->buttons() & Qt::LeftButton)
            emit locatorMoved(0, pos);
      else if (e->buttons() & Qt::MiddleButton)
            emit locatorMoved(1, pos);
      else if (e->buttons() & Qt::RightButton)
            emit locatorMoved(2, pos);
      }

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void PianoRuler::leaveEvent(QEvent*)
      {
      _cursor.setInvalid();
      emit posChanged(_cursor);
      update();
      }

//---------------------------------------------------------
//   setPos
//---------------------------------------------------------

void PianoRuler::setPos(const Pos& pos)
      {
      if (_cursor != pos) {
            int p1 = pos2pix(_cursor);
            int p2 = pos2pix(pos);

            if (p1 > p2)
                  qSwap(p1, p2);

            if (_orientation == PianoRollOrientation::HORIZONTAL)
                  update(QRect(p1 - 1, 0, p2 - p1 + 2, height()));
            else
                  update(QRect(0, p1 - 1, width(), p2 - p1 + 2));

            _cursor = pos;
            }
      }

//---------------------------------------------------------
//   setXZoom
//---------------------------------------------------------

void PianoRuler::setXZoom(qreal xZoom)
      {
      _xZoom = xZoom;
      update();
      }

}
