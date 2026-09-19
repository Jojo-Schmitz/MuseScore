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

#include "pianolevels.h"

#include "pianoruler.h"
#include "pianokeyboard.h"
#include "pianoview.h"
#include "pianolevelsfilter.h"
#include "preferences.h"

#include "libmscore/chord.h"
#include "libmscore/note.h"
#include "libmscore/noteevent.h"
#include "libmscore/score.h"
#include "libmscore/segment.h"
#include "libmscore/staff.h"

namespace Ms {

static const int LEVEL_NOTE_TIME_BUCKET_TICKS = DIVISION * 4;

//---------------------------------------------------------
//   PianoLevels
//---------------------------------------------------------

PianoLevels::PianoLevels(QWidget *parent)
    : QWidget(parent)
       {
       setMouseTracking(true);
       _score     = nullptr;
       _xpos      = 0;
       _xZoom     = X_ZOOM_INITIAL;
       _locator   = nullptr;
       _staff     = nullptr;
       _tuplet    = 1;
       _subdiv    = 0;
       _levelsIndex = 2; // Velocity (relative)
       minBeatGap = 20;
       vMargin    = 10;
       levelLen   = 20;
       _scope     = PianoRollScope::PART;
       }

//---------------------------------------------------------
//   ~PianoLevels
//---------------------------------------------------------

PianoLevels::~PianoLevels()
      {
      clearNoteData();
      }

//---------------------------------------------------------
//   setUseNoteColors
//---------------------------------------------------------

void PianoLevels::setUseNoteColors(bool value)
      {
      _useNoteColors = value;
      }

//---------------------------------------------------------
//   setColoring
//---------------------------------------------------------

void PianoLevels::setColoring(Coloring c)
      {
      if (_coloring == c)
            return;

      _coloring = c;
      update();
      }

//---------------------------------------------------------
//   setPianoView
//---------------------------------------------------------

void PianoLevels::setPianoView(PianoView* v)
      {
      _pianoView = v;
      }

//---------------------------------------------------------
//   setOrientation
//---------------------------------------------------------

void PianoLevels::setOrientation(PianoRollOrientation o)
      {
      if (_orientation == o)
            return;
      _orientation = o;
      update();
      }

//---------------------------------------------------------
//   setScore
//---------------------------------------------------------

void PianoLevels::setScore(Score* s, Pos* lc)
      {
      _score = s;
      _locator = lc;
      if (_score)
            _cursor.setContext(_score->tempomap(), _score->sigmap());
      setEnabled(_score != 0);
      }

//---------------------------------------------------------
//   setXpos
//---------------------------------------------------------

void PianoLevels::setXpos(int val)
      {
      if (_xpos == val)
            return;

      _xpos = val;
      update();
      }

//---------------------------------------------------------
//   drawTimeAxisLine
//---------------------------------------------------------

void PianoLevels::drawTimeAxisLine(QPainter& p, int pos) const
      {
      if (_orientation == PianoRollOrientation::HORIZONTAL)
            p.drawLine(pos, 0, pos, height());
      else
            p.drawLine(0, pos, width(), pos);
      }

//---------------------------------------------------------
//   drawValueAxisLine
//---------------------------------------------------------

void PianoLevels::drawValueAxisLine(QPainter& p, int pos) const
      {
      if (_orientation == PianoRollOrientation::HORIZONTAL)
            p.drawLine(0, pos, width(), pos);
      else
            p.drawLine(pos, 0, pos, height());
      }

//---------------------------------------------------------
//   drawTimeGrid
//---------------------------------------------------------

void PianoLevels::drawTimeGrid(QPainter& p,
                               int timeStart,
                               int timeEnd,
                               const QPen& penLineMajor,
                               const QPen& penLineMinor,
                               const QPen& penLineSub) const
      {
      Pos pos1(_score->tempomap(),
               _score->sigmap(),
               qMax(pixelToTick(timeStart), 0),
               TType::TICKS);

      Pos pos2(_score->tempomap(),
               _score->sigmap(),
               qMax(pixelToTick(timeEnd), 0),
               TType::TICKS);

      int bar1;
      int bar2;
      int beat;
      int tick;

      pos1.mbt(&bar1, &beat, &tick);
      pos2.mbt(&bar2, &beat, &tick);

      // Estimate bar width because changing time signatures can make
      // the actual width inconsistent. Assume four beats per bar for
      // purposes of deciding how aggressively to skip grid lines
      const qreal pixPerBar = DIVISION * 4 * _xZoom;
      const qreal pixPerBeat = DIVISION * _xZoom;

      int barSkip = qCeil(minBeatGap / pixPerBar);
      barSkip = int(pow(2, ceil(log(barSkip) / log(2))));

      int beatSkip = qCeil(minBeatGap / pixPerBeat);
      beatSkip = int(pow(2, ceil(log(beatSkip) / log(2))));

      // Round down to the first multiple of barSkip
      bar1 = (bar1 / barSkip) * barSkip;

      const int subbeats = _tuplet * (1 << _subdiv);

      for (int bar = bar1;
           bar <= bar2;
           bar += barSkip) {
            Pos barPos(
                  _score->tempomap(),
                  _score->sigmap(),
                  bar,
                  0,
                  0);

            const int beatsInBar =
                  barPos.timesig().timesig().numerator();

            for (int beatIndex = 0;
                 beatIndex < beatsInBar;
                 beatIndex += beatSkip) {
                  Pos beatPos(
                        _score->tempomap(),
                        _score->sigmap(),
                        bar,
                        beatIndex,
                        0);

                  int tp = tickToPixel(
                        beatPos.time(TType::TICKS));

                  if (tp < 0)
                        continue;

                  p.setPen(
                        beatIndex == 0
                              ? penLineMajor
                              : penLineMinor);

                  drawTimeAxisLine(p, tp);

                  for (int sub = 1;
                       sub < subbeats;
                       ++sub) {
                        Pos subBeatPos(
                              _score->tempomap(),
                              _score->sigmap(),
                              bar,
                              beatIndex,
                              sub * DIVISION / subbeats);

                        tp = tickToPixel(
                              subBeatPos.time(TType::TICKS));

                        p.setPen(penLineSub);
                        drawTimeAxisLine(p, tp);
                        }
                  }
            }
      }

//---------------------------------------------------------
//   drawValueGrid
//---------------------------------------------------------

void PianoLevels::drawValueGrid(QPainter& p,
                                PianoLevelsFilter* filter,
                                const QPen& penLineMajor,
                                const QPen& penLineMinor,
                                const QColor& textColor) const
      {
      const int div = filter->divisionGap();

      const int minGuide =
            int(floor(filter->minRange() / qreal(div)));

      const int maxGuide =
            int(ceil(filter->maxRange() / qreal(div)));

      p.setFont(QFont("FreeSans", 9));
      const QFontMetrics fm(p.font());

      for (int i = minGuide; i <= maxGuide; ++i) {
            p.setPen(
                  i == 0 || i == minGuide || i == maxGuide
                        ? penLineMajor
                        : penLineMinor);

            const int value = i * div;
            const int vp = valToPixel(value);

            drawValueAxisLine(p, vp);

            p.setPen(QPen(textColor));

            if (_orientation == PianoRollOrientation::HORIZONTAL) {
                  const QRectF textRect(
                        2,
                        vp - 12,
                        width() - 2,
                        12);

                  p.drawText(
                        textRect,
                        Qt::AlignLeft | Qt::AlignBottom,
                        QString::number(value));
                  }
            else {
                  const QString text =
                        QString::number(value);

                  const int textWidth =
                        fm.width(text);

                  const int x =
                        qBound(
                              2,
                              vp - textWidth / 2,
                              width() - textWidth - 2);

                  const QRectF textRect(
                        x,
                        height() - 14,
                        textWidth,
                        12);

                  p.drawText(
                        textRect,
                        Qt::AlignHCenter | Qt::AlignBottom,
                        text);
                  }
            }
      }

//---------------------------------------------------------
//   drawNoteLevels
//---------------------------------------------------------

void PianoLevels::drawNoteLevels(QPainter& p,
                                 PianoLevelsFilter* filter,
                                 int timeStart,
                                 int timeEnd)
      {
      const QColor noteSelected =
            pianoRollThemeColor(
                  PREF_UI_PIANOROLL_DARK_NOTE_SEL_COLOR,
                  PREF_UI_PIANOROLL_LIGHT_NOTE_SEL_COLOR);

      const QColor interactionColor =
            pianoRollThemeColor(
                  PREF_UI_PIANOROLL_DARK_NOTE_DRAG_COLOR,
                  PREF_UI_PIANOROLL_LIGHT_NOTE_DRAG_COLOR);

      const bool previewMovesNotes =
            _pianoView && _pianoView->levelPreviewMovesNotes();

      const bool previewMovesEvents =
            _pianoView && _pianoView->levelPreviewMovesEvents();

      const bool previewResizesNotes =
            _pianoView && _pianoView->levelPreviewResizesNotes();

      const bool movingPreview =
            previewMovesNotes
            || previewMovesEvents
            || previewResizesNotes;

      const Fraction previewTickOffset =
            _pianoView
                  ? _pianoView->levelPreviewTickOffset()
                  : Fraction{};

      const Fraction previewEventTickDelta =
            _pianoView
                  ? _pianoView->levelPreviewEventTickDelta()
                  : Fraction{};

      const Fraction previewLengthOffset =
            _pianoView
                  ? _pianoView->levelPreviewLengthOffset()
                  : Fraction{};

      p.setBrush(Qt::NoBrush);

      const int zeroPixel = valToPixel(0);

      // Level bars extend from tp through tp + levelLen along the
      // time axis, with the circular handle extending a few pixels
      // before tp
      const int levelTimeMargin = 5;

      auto levelTimeVisible =
            [this, timeStart, timeEnd, levelTimeMargin](int tp) {
                  const int start =
                        tp - levelTimeMargin;

                  const int end =
                        tp + levelLen + levelTimeMargin;

                  return end >= timeStart
                         && start <= timeEnd;
                  };

      // A level bar can begin slightly before the dirty region and
      // still extend into it, so expand the candidate range by the
      // complete level-bar extent
      const int candidateStartTick =
            pixelToTick(
                  timeStart
                  - levelLen
                  - levelTimeMargin);

      const int candidateEndTick =
            pixelToTick(
                  timeEnd
                  + levelTimeMargin);

      QVector<Note*> noteCandidates;

      if (movingPreview) {
            noteCandidates.reserve(noteList.size());

            for (Note* note : qAsConst(noteList))
                  noteCandidates.append(note);
            }
      else {
            noteCandidates =
                  noteCandidatesForTickRange(
                        qMin(candidateStartTick,
                             candidateEndTick),
                        qMax(candidateStartTick,
                             candidateEndTick));
            }

      // Draw unselected notes first, selected notes second,
      // Overlapping notes from another staff must not obscure
      // the selection indication
      for (int pass = 0; pass < 2; ++pass) {
            for (Note* note : qAsConst(noteCandidates)) {
                  const bool selected =
                        note->selected();

                  if ((pass == 0 && selected)
                      || (pass == 1 && !selected)) {
                        continue;
                        }

                  const bool interactionHighlighted =
                        _pianoView
                        && _pianoView->levelInteractionHighlighted(
                              note);

                  const QColor noteDeselected =
                        pianoRollNoteColor(
                              note,
                              _coloring,
                              false,
                              _useNoteColors);

                  if (filter->isPerEvent()) {
                        for (NoteEvent& event : note->playEvents()) {
                              Fraction previewNoteTick =
                                    note->chord()->tick();

                              Fraction previewNoteLen =
                                    note->chord()->ticks();

                              if (selected && previewResizesNotes) {
                                    previewNoteTick += previewTickOffset;
                                    previewNoteLen += previewLengthOffset;
                                    }

                              int previewTick;

                              if (selected && previewResizesNotes) {
                                    const Fraction eventTick =
                                          previewNoteTick
                                          + previewNoteLen
                                                * event.ontime()
                                                / 1000;

                                    previewTick =
                                          eventTick.ticks();
                                    }
                              else {
                                    previewTick =
                                          noteStartTick(
                                                note,
                                                &event);

                                    if (selected) {
                                          if (previewMovesNotes)
                                                previewTick += previewTickOffset.ticks();
                                          else if (previewMovesEvents)
                                                previewTick += previewEventTickDelta.ticks();
                                          }
                                    }

                              const int timePixel =
                                    tickToPixel(previewTick);

                              if (!levelTimeVisible(timePixel))
                                    continue;

                              int value =
                                    filter->value(
                                          note,
                                          &event);

                              int previewOntime =
                                    event.ontime();

                              int previewLen =
                                    event.len();

                              const bool eventPreviewAvailable =
                                    _pianoView
                                    && _pianoView->levelEventPreview(
                                          &event,
                                          previewOntime,
                                          previewLen);

                              const bool previewValueAvailable =
                                    eventPreviewAvailable
                                    || (selected && previewResizesNotes);

                              if (previewValueAvailable) {
                                    int previewValue;

                                    if (filter->previewValue(
                                                note,
                                                previewOntime,
                                                previewLen,
                                                previewNoteLen,
                                                previewValue)) {
                                          value = previewValue;
                                          }
                                    }

                              const int valuePixel =
                                    valToPixel(value);

                              const QColor levelColor =
                                    interactionHighlighted
                                          ? interactionColor
                                          : (selected
                                                ? noteSelected
                                                : noteDeselected);

                              drawLevelBar(
                                    p,
                                    timePixel,
                                    valuePixel,
                                    zeroPixel,
                                    levelColor);
                              }
                        }
                  else {
                        int previewTick =
                              noteStartTick(
                                    note,
                                    nullptr);

                        if (selected && previewMovesNotes)
                              previewTick += previewTickOffset.ticks();

                        const int timePixel =
                              tickToPixel(previewTick);

                        if (!levelTimeVisible(timePixel))
                              continue;

                        const int value =
                              filter->value(
                                    note,
                                    nullptr);

                        const int valuePixel =
                              valToPixel(value);

                        const QColor levelColor =
                              interactionHighlighted
                                    ? interactionColor
                                    : (selected
                                          ? noteSelected
                                          : noteDeselected);

                        drawLevelBar(
                              p,
                              timePixel,
                              valuePixel,
                              zeroPixel,
                              levelColor);
                        }
                  }
            }
      }

//---------------------------------------------------------
//   paintEvent
//---------------------------------------------------------

void PianoLevels::paintEvent(QPaintEvent* e)
      {
      QPainter p(this);

      const QColor colPianoBg =
            pianoRollThemeColor(
                  PREF_UI_PIANOROLL_DARK_BG_BASE_COLOR,
                  PREF_UI_PIANOROLL_LIGHT_BG_BASE_COLOR);

      const QColor colGridLine =
            pianoRollThemeColor(
                  PREF_UI_PIANOROLL_DARK_BG_GRIDLINE_COLOR,
                  PREF_UI_PIANOROLL_LIGHT_BG_GRIDLINE_COLOR);

      const QColor colText =
            pianoRollThemeColor(
                  PREF_UI_PIANOROLL_DARK_BG_TEXT_COLOR,
                  PREF_UI_PIANOROLL_LIGHT_BG_TEXT_COLOR);

      const QPen penLineMajor = QPen(colGridLine, 2.0, Qt::SolidLine);
      const QPen penLineMinor = QPen(colGridLine, 1.0, Qt::SolidLine);
      const QPen penLineSub = QPen(colGridLine, 1.0, Qt::DotLine);

      const QRect& r = e->rect();

      p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);

      p.fillRect(r, colPianoBg);

      if (!_score)
            return;

      const int timeStart = _orientation == PianoRollOrientation::HORIZONTAL
            ? r.x()
            : r.y();

      const int timeEnd = _orientation == PianoRollOrientation::HORIZONTAL
            ? r.x() + r.width()
            : r.y() + r.height();

      drawTimeGrid(p, timeStart, timeEnd, penLineMajor, penLineMinor, penLineSub);

      PianoLevelsFilter* filter =
            PianoLevelsFilter::FILTER_LIST[_levelsIndex];

      drawValueGrid(p, filter, penLineMajor, penLineMinor, colText);

      drawNoteLevels(p, filter, timeStart, timeEnd);

      if (_playbackLocatorValid) {
            const int tp = tickToPixel(qRound(_playbackLocatorTick));

            p.setPen(QPen(Qt::red, 1));
            drawTimeAxisLine(p, tp);
            }
      }

//---------------------------------------------------------
//   noteStartTick
//---------------------------------------------------------

int PianoLevels::noteStartTick(Note* note, NoteEvent* evt)
      {
      Chord* chord = note->chord();
      int ticks = chord->ticks().ticks();

      return note->chord()->tick().ticks() + (evt ? evt->ontime() * ticks / 1000 : 0);
      }

//---------------------------------------------------------
//   pickNoteEvent
//---------------------------------------------------------

//---------------------------------------------------------
//   pickNoteEvent
//---------------------------------------------------------

bool PianoLevels::pickNoteEvent(int x,
                                int y,
                                bool selectedOnly,
                                Note*& pickedNote,
                                NoteEvent*& pickedNoteEvent)
      {
      pickedNote = nullptr;
      pickedNoteEvent = nullptr;

      PianoLevelsFilter* filter =
            PianoLevelsFilter::FILTER_LIST[_levelsIndex];

      const int zeroPixel =
            valToPixel(0);

      const QPoint point(x, y);

      auto hitLevel =
            [this, filter, zeroPixel, &point](
                  Note* note,
                  NoteEvent* event) {
                  const int timePixel =
                        tickToPixel(
                              noteStartTick(
                                    note,
                                    event));

                  const int valuePixel =
                        valToPixel(
                              filter->value(
                                    note,
                                    event));

                  return levelHitRect(
                        timePixel,
                        valuePixel,
                        zeroPixel).contains(point);
                  };

      for (Note* note : qAsConst(noteList)) {
            if (selectedOnly && !note->selected())
                  continue;

            if (filter->isPerEvent()) {
                  for (NoteEvent& event : note->playEvents()) {
                        if (!hitLevel(note, &event))
                              continue;

                        pickedNote = note;
                        pickedNoteEvent = &event;
                        return true;
                        }
                  }
            else if (hitLevel(note, nullptr)) {
                  pickedNote = note;
                  return true;
                  }
            }

      return false;
      }

//---------------------------------------------------------
//   pickNearestLevelInTimeBand
//---------------------------------------------------------

bool PianoLevels::pickNearestLevelInTimeBand(int timePixel,
                                             int valuePixel,
                                             int timeRadius,
                                             bool selectedOnly,
                                             Note*& pickedNote,
                                             NoteEvent*& pickedEvent)
      {
      pickedNote = nullptr;
      pickedEvent = nullptr;

      PianoLevelsFilter* filter =
            PianoLevelsFilter::FILTER_LIST[_levelsIndex];

      int bestDistance {
            std::numeric_limits<int>::max()
            };

      auto considerLevel =
            [this,
             filter,
             timePixel,
             valuePixel,
             timeRadius,
             &bestDistance,
             &pickedNote,
             &pickedEvent](Note* note, NoteEvent* event) {
                  const int tp =
                        tickToPixel(
                              noteStartTick(note, event));

                  const int dt =
                        tp - timePixel;

                  if (std::abs(dt) > timeRadius)
                        return;

                  const int value =
                        filter->value(note, event);

                  const int vp =
                        valToPixel(value);

                  const int dv =
                        vp - valuePixel;

                  const int distance =
                        dt * dt + dv * dv;

                  if (distance >= bestDistance)
                        return;

                  bestDistance = distance;
                  pickedNote = note;
                  pickedEvent = event;
                  };

      for (Note* note : qAsConst(noteList)) {
            if (selectedOnly && !note->selected())
                  continue;

            if (filter->isPerEvent()) {
                  for (NoteEvent& event : note->playEvents())
                        considerLevel(note, &event);
                  }
            else {
                  considerLevel(note, nullptr);
                  }
            }

      return pickedNote != nullptr;
      }

//---------------------------------------------------------
//   clearLevelInteraction
//---------------------------------------------------------

void PianoLevels::clearLevelInteraction()
      {
      _edit.interactionNotes.clear();
      _edit.dragTargets.clear();

      if (_pianoView)
            _pianoView->clearLevelInteractionNotes();
      }

//---------------------------------------------------------
//   captureLevelDragTargets
//---------------------------------------------------------

void PianoLevels::captureLevelDragTargets(Note* anchorNote,
                                          NoteEvent* anchorEvent,
                                          bool selectedOnly)
      {
      _edit.dragTargets.clear();

      if (!anchorNote)
            return;

      PianoLevelsFilter* filter =
            PianoLevelsFilter::FILTER_LIST[_levelsIndex];

      const bool perEvent =
            filter->isPerEvent();

      NoteEvent* effectiveAnchorEvent =
            perEvent ? anchorEvent : nullptr;

      const int anchorTick =
            noteStartTick(
                  anchorNote,
                  effectiveAnchorEvent);

      _edit.dragAnchorValue =
            filter->value(
                  anchorNote,
                  effectiveAnchorEvent);

      // With no score selection, capture only the explicitly
      // chosen node. A selection is what turns simultaneous
      // nodes into a group
      if (!selectedOnly) {
            _edit.dragTargets.append({
                  anchorNote,
                  effectiveAnchorEvent,
                  _edit.dragAnchorValue
                  });
            return;
            }

      const QVector<Note*> candidates =
            noteCandidatesForTickRange(
                  anchorTick,
                  anchorTick);

      auto appendTarget =
            [this, filter, anchorTick](
                  Note* note,
                  NoteEvent* event) {
                  if (noteStartTick(note, event) != anchorTick)
                        return;

                  _edit.dragTargets.append({
                        note,
                        event,
                        filter->value(note, event)
                        });
                  };

      // Capture every selected level at the anchor's effective
      // time, preserving each one's original value:
      for (Note* note : qAsConst(candidates)) {
            if (!note->selected())
                  continue;

            if (perEvent) {
                  for (NoteEvent& event : note->playEvents())
                        appendTarget(note, &event);
                  }
            else {
                  appendTarget(note, nullptr);
                  }
            }
      }

//---------------------------------------------------------
//   adjustCapturedLevels
//---------------------------------------------------------

void PianoLevels::adjustCapturedLevels(int value)
      {
      if (_edit.dragTargets.isEmpty())
            return;

      PianoLevelsFilter* filter =
            PianoLevelsFilter::FILTER_LIST[_levelsIndex];

      const int delta =
            value - _edit.dragAnchorValue;

      for (const LevelDragTarget& target : qAsConst(_edit.dragTargets)) {
            if (!target.note)
                  continue;

            filter->setValue(
                  target.note,
                  target.event,
                  target.startValue + delta);

            _edit.interactionNotes.insert(target.note);
            }

      if (_pianoView)
            _pianoView->setLevelInteractionNotes(
                  _edit.interactionNotes);

      update();
      emit noteLevelsChanged();
      }

//---------------------------------------------------------
//   adjustLevelLerp
//---------------------------------------------------------

void PianoLevels::adjustLevel(Note* note, NoteEvent* noteEvt, int value)
      {
      PianoLevelsFilter* filter = PianoLevelsFilter::FILTER_LIST[_levelsIndex];

      filter->setValue(note, noteEvt, value);

      _edit.interactionNotes.insert(note);

      if (_pianoView)
            _pianoView->setLevelInteractionNotes(_edit.interactionNotes);

      update();
      emit noteLevelsChanged();
      }

//---------------------------------------------------------
//   drawLevelBar
//---------------------------------------------------------

void PianoLevels::drawLevelBar(QPainter& p,
                               int tp,
                               int vp,
                               int pix0,
                               const QColor& color)
      {
      QColor fillColor = color;
      fillColor.setAlphaF(0.35);

      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            const int top = qMin(pix0, vp);
            const int bottom = qMax(pix0, vp);

            QRect barRect(
                  tp,
                  top,
                  levelLen,
                  qMax(1, bottom - top));

            p.setBrush(fillColor);
            p.setPen(QPen(color, 1));
            p.drawRect(barRect);

            p.setBrush(color);
            p.setPen(QPen(color, 2));
            p.drawEllipse(tp - 4, vp - 4, 9, 9);
            }
      else if (_orientation == PianoRollOrientation::VERTICAL) {
            const int left = qMin(pix0, vp);
            const int right = qMax(pix0, vp);

            QRect barRect(
                  left,
                  tp,
                  qMax(1, right - left),
                  levelLen);

            p.setBrush(fillColor);
            p.setPen(QPen(color, 1));
            p.drawRect(barRect);

            p.setBrush(color);
            p.setPen(QPen(color, 2));
            p.drawEllipse(vp - 4, tp - 4, 9, 9);
            }
      }

//---------------------------------------------------------
//   levelHitRect
//---------------------------------------------------------

QRect PianoLevels::levelHitRect(int timePixel,
                                int valuePixel,
                                int zeroPixel) const
      {
      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            const int left =
                  timePixel - 2;

            const int right =
                  timePixel + levelLen + 2;

            const int top =
                  qMin(zeroPixel, valuePixel) - 2;

            const int bottom =
                  qMax(zeroPixel, valuePixel) + 2;

            return QRect(
                  QPoint(left, top),
                  QPoint(right, bottom));
            }

      const int left =
            qMin(zeroPixel, valuePixel) - 2;

      const int right =
            qMax(zeroPixel, valuePixel) + 2;

      const int top =
            timePixel - 2;

      const int bottom =
            timePixel + levelLen + 2;

      return QRect(QPoint(left, top),
                   QPoint(right, bottom));
      }

//---------------------------------------------------------
//   pixelToTick
//---------------------------------------------------------

int PianoLevels::pixelToTick(int pixel) const
      {
      if (_orientation == PianoRollOrientation::HORIZONTAL)
            return static_cast<int>((pixel + _xpos) / _xZoom) - MAP_OFFSET;

      if (_pianoView && height() > 0 && _pianoView->viewport()->height() > 0) {
            const qreal pianoY =
                  qreal(pixel)
                  * _pianoView->viewport()->height()
                  / height();

            const QPointF scenePos =
                  _pianoView->mapToScene(
                        QPoint(0, qRound(pianoY)));

            return _pianoView->pixelYToTick(
                  qRound(scenePos.y()));
            }

      return 0;
      }

//---------------------------------------------------------
//   tickToPixel
//---------------------------------------------------------

int PianoLevels::tickToPixel(int tick) const
      {
      if (_orientation == PianoRollOrientation::HORIZONTAL)
            return static_cast<int>((tick + MAP_OFFSET) * _xZoom - _xpos);

      if (_pianoView && height() > 0 && _pianoView->viewport()->height() > 0) {
            const int sceneY =
                  _pianoView->tickToPixelY(tick);

            const int pianoY =
                  _pianoView->mapFromScene(
                        QPointF(0.0, sceneY)).y();

            return qRound(
                  qreal(pianoY)
                  * height()
                  / _pianoView->viewport()->height());
            }

      return 0;
      }

//---------------------------------------------------------
//   valToPixel
//---------------------------------------------------------

int PianoLevels::valToPixel(int value) const
      {
      PianoLevelsFilter* filter =
            PianoLevelsFilter::FILTER_LIST[_levelsIndex];

      const int range =
            filter->maxRange() - filter->minRange();

      const qreal frac =
            (value - filter->minRange()) / qreal(range);

      if (_orientation == PianoRollOrientation::HORIZONTAL)
            return static_cast<int>(height() - vMargin * 2)
                  * (1.0 - frac) + vMargin;

      return static_cast<int>(width() - vMargin * 2)
            * frac + vMargin;
      }

//---------------------------------------------------------
//   pixelToVal
//---------------------------------------------------------

int PianoLevels::pixelToVal(int pixel) const
      {
      PianoLevelsFilter* filter =
            PianoLevelsFilter::FILTER_LIST[_levelsIndex];

      const int range =
            filter->maxRange() - filter->minRange();

      qreal frac;

      if (_orientation == PianoRollOrientation::HORIZONTAL)
            frac = 1.0 -
                  (pixel - vMargin)
                  / qreal(height() - vMargin * 2);
      else
            frac =
                  (pixel - vMargin)
                  / qreal(width() - vMargin * 2);

      return static_cast<int>(
            frac * range + filter->minRange());
      }

//---------------------------------------------------------
//   hasSelectedNotes
//---------------------------------------------------------

bool PianoLevels::hasSelectedNotes() const
      {
      for (Note* note : noteList) {
            if (note->selected())
                  return true;
            }

      return false;
      }

//---------------------------------------------------------
//   adjustLevelLerp
//       For all points between tick0 and tick1, linearly interploate between value0 and value1 and
//       use it to set the value of the level.
//---------------------------------------------------------

void PianoLevels::adjustLevelLerp(int tick0, int value0, int tick1, int value1, bool selectedOnly)
      {
      if (tick1 < tick0) {
            std::swap(tick0, tick1);
            std::swap(value0, value1);
            }

      PianoLevelsFilter* filter = PianoLevelsFilter::FILTER_LIST[_levelsIndex];
      bool hitNote = false;

      const QVector<Note*> candidates =
            noteCandidatesForTickRange(tick0, tick1);

      for (Note* note : candidates) {
            if (selectedOnly && !note->selected())
                  continue;

            if (filter->isPerEvent()) {
                  for (NoteEvent& e : note->playEvents()) {
                        int tick = noteStartTick(note, &e);
                        if (tick0 <= tick && tick <= tick1) {
                              int value = tick0 == tick1 ? value0
                                    : (value1 - value0) * (tick - tick0) / (tick1 - tick0) + value0;

                              filter->setValue(note, &e, value);

                              _edit.interactionNotes.insert(note);
                              hitNote = true;
                              }
                        }
                  }
            else {
                  int tick = noteStartTick(note, 0);
                  if (tick0 <= tick && tick <= tick1) {
                        int value = tick0 == tick1 ? value0
                              : (value1 - value0) * (tick - tick0) / (tick1 - tick0) + value0;
                        filter->setValue(note, nullptr, value);

                        _edit.interactionNotes.insert(note);
                        hitNote = true;
                        }
                  }
            }

      if (hitNote) {
            if (_pianoView)
                  _pianoView->setLevelInteractionNotes(_edit.interactionNotes);

            update();
            emit noteLevelsChanged();
            }
      }



//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void PianoLevels::mousePressEvent(QMouseEvent* e)
      {
      if (e->button() == Qt::LeftButton) {
            clearLevelInteraction();

            _edit.mouseDown = true;
            _edit.mouseDownPos = e->pos();
            _edit.lastMousePos = _edit.mouseDownPos;

            const Qt::KeyboardModifiers modifiers =
                  QGuiApplication::keyboardModifiers();
            const bool forceLerp =
                  modifiers & Qt::ControlModifier;

            const bool selectedOnly = hasSelectedNotes();

            const int timePixel =
                  mouseTimePixel(_edit.mouseDownPos);

            const int valuePixel =
                  mouseValuePixel(_edit.mouseDownPos);

            const int val =
                  pixelToVal(valuePixel);

            Note* anchorNote = nullptr;
            NoteEvent* anchorEvent = nullptr;

            // First try the actual visible bar/node hit area
            const int lerpPickRadius =
                  qMax(4, levelLen / 2);

            if (forceLerp) {
                  _edit.dragStyle = DragStyle::LERP;
                  }
            else {
                  const bool barHit =
                        pickNoteEvent(_edit.mouseDownPos.x(),
                                      _edit.mouseDownPos.y(),
                                      selectedOnly,
                                      anchorNote,
                                      anchorEvent);

                  if (barHit) {
                        // More than one filled bar can overlap.
                        // Use proximity to choose the actual anchor:
                        pickNearestLevelInTimeBand(
                              timePixel,
                              valuePixel,
                              qMax(lerpPickRadius, levelLen + 2),
                              selectedOnly,
                              anchorNote,
                              anchorEvent);

                        _edit.dragStyle = DragStyle::OFFSET;
                        }
                  else if (pickNearestLevelInTimeBand(
                        timePixel,
                        valuePixel,
                        lerpPickRadius,
                        selectedOnly,
                        anchorNote,
                        anchorEvent)) {
                        _edit.dragStyle = DragStyle::OFFSET;
                        }
                  else {
                        _edit.dragStyle = DragStyle::LERP;
                        }
                  }

            if (_score && !_edit.commandActive) {
                  _score->startCmd();
                  _edit.commandActive = true;
                  }

            if (_edit.dragStyle == DragStyle::OFFSET) {
                  captureLevelDragTargets(
                        anchorNote,
                        anchorEvent,
                        selectedOnly);

                  // Immediately jump the anchor to the pressed value
                  // All captured simultaneous selected levels follow
                  // by the same delta
                  adjustCapturedLevels(val);
                  }
            else {
                  const int tick0 =
                        pixelToTick(timePixel - lerpPickRadius);

                  const int tick1 =
                        pixelToTick(timePixel + lerpPickRadius);

                  adjustLevelLerp(
                        tick0,
                        val,
                        tick1,
                        val,
                        selectedOnly);
                  }

            update();
            }
      }


//---------------------------------------------------------
//   mouseReleaseEvent
//---------------------------------------------------------

void PianoLevels::mouseReleaseEvent(QMouseEvent* e)
      {
      if (e->button() == Qt::LeftButton) {
            if (_edit.commandActive && _score) {
                  _score->endCmd();
                  _edit.commandActive = false;
                  }

            clearLevelInteraction();

            _edit.mouseDown= false;
            _edit.dragging = false;

            update();
            }
      }

//---------------------------------------------------------
//   mouseMoveEvent
//---------------------------------------------------------

void PianoLevels::mouseMoveEvent(QMouseEvent* e)
      {
      const int modifiers = QGuiApplication::keyboardModifiers();
      const bool bnShift = modifiers & Qt::ShiftModifier;

      if (_edit.mouseDown) {
            if (!_edit.dragging) {
                  int dx = e->x() - _edit.mouseDownPos.x();
                  int dy = e->y() - _edit.mouseDownPos.y();
                  if ((dx * dx + dy * dy) > (pickRadius * pickRadius)) {
                        _edit.dragging = true;
                        }
                  }

            if (_edit.dragging) {
                  if (_edit.dragStyle == DragStyle::OFFSET) {
                        const int val = pixelToVal(mouseValuePixel(e->pos()));
                        adjustCapturedLevels(val);
                        }
                  else {
                        int tick0 =
                              pixelToTick(mouseTimePixel(_edit.lastMousePos));
                        int tick1 =
                              pixelToTick(mouseTimePixel(e->pos()));

                        int val0;
                        int val1;

                        if (bnShift) {
                              val0 = pixelToVal(mouseValuePixel(_edit.mouseDownPos));
                              val1 = val0;
                              }
                        else {
                              val0 = pixelToVal(mouseValuePixel(_edit.lastMousePos));
                              val1 = pixelToVal(mouseValuePixel(e->pos()));
                              }

                        adjustLevelLerp(tick0, val0, tick1, val1, hasSelectedNotes());
                        }

                  _edit.lastMousePos = e->pos();
                  update();
                  }
            }
      }

//---------------------------------------------------------
//   moveLocator
//---------------------------------------------------------

void PianoLevels::moveLocator(QMouseEvent* e)
      {
      Pos pos(
            _score->tempomap(),
            _score->sigmap(),
            qMax(pixelToTick(mouseTimePixel(e->pos())), 0),
            TType::TICKS);

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

void PianoLevels::leaveEvent(QEvent*)
      {
      _cursor.setInvalid();
      emit posChanged(_cursor);
      update();
      }

//---------------------------------------------------------
//   setPos
//---------------------------------------------------------

void PianoLevels::setPos(const Pos& pos)
      {
      if (_cursor != pos) {
            _cursor = pos;
            update();
            }
      }

//---------------------------------------------------------
//   setXZoom
//---------------------------------------------------------

void PianoLevels::setXZoom(qreal xZoom)
      {
      if (_xZoom == xZoom)
            return;

      _xZoom = xZoom;
      update();
      }

//---------------------------------------------------------
//   setEditableStaff
//---------------------------------------------------------
void PianoLevels::setEditableStaff(Staff* st)
      {
      _staff = st;
      }

//---------------------------------------------------------
//   setStaff
//---------------------------------------------------------

void PianoLevels::setStaff(Staff* s, Pos* l)
      {
      _locator = l;

      if (_staff == s)
            return;

      _staff    = s;
      updateNotes();
      }

//---------------------------------------------------------
//   indexNote
//---------------------------------------------------------

void PianoLevels::indexNote(Note* note)
      {
      if (!note || !note->chord())
            return;

      Chord* chord = note->chord();

      const int chordTick = chord->tick().ticks();
      const int noteTicks = chord->ticks().ticks();

      // Piano Levels allows event ontime from -1000 to +1000,
      // i.e. up to one full note duration before or after the
      // normal note onset. Index that whole possible range so
      // editing Position does not make the index stale
      int firstTick = chordTick - noteTicks - DIVISION;
      int lastTick  = chordTick + noteTicks * 2 + DIVISION;

      // Also include any existing event positions outside that
      // nominal range
      for (NoteEvent& event : note->playEvents()) {
            const int eventTick = noteStartTick(note, &event);
            firstTick = qMin(firstTick, eventTick - DIVISION);
            lastTick  = qMax(lastTick, eventTick + DIVISION);
            }

      pianoRollAddToTimeBuckets(
            _noteTimeBuckets,
            note,
            firstTick,
            lastTick,
            LEVEL_NOTE_TIME_BUCKET_TICKS);
      }

//---------------------------------------------------------
//   noteCandidatesForTickRange
//---------------------------------------------------------

QVector<Note*> PianoLevels::noteCandidatesForTickRange(
      int startTick,
      int endTick) const
      {
      return pianoRollTimeBucketCandidates(
            _noteTimeBuckets,
            startTick,
            endTick,
            LEVEL_NOTE_TIME_BUCKET_TICKS);
      }

//---------------------------------------------------------
//   addChord
//---------------------------------------------------------

void PianoLevels::addChord(Chord* chord)
      {
      for (Chord*& c : chord->graceNotes())
            addChord(c);

      for (Note* note : chord->notes()) {
            if (note->tieBack())
                  continue;

            noteList.append(note);
            indexNote(note);
            }
      }

//---------------------------------------------------------
//   updateNotes
//---------------------------------------------------------

void PianoLevels::updateNotes()
      {
      clearNoteData();

      if (!_staff)
            return;

      const Score* const score = _staff->score();
      const QVector<int> tracks =
            pianoRollScopeTracks(_staff, _scope);

      const SegmentType st = SegmentType::ChordRest;
      for (Segment* s = score->firstSegment(st); s; s = s->next1(st)) {
            for (int track : tracks) {
                  Element* e = s->element(track);
                  if (e && e->isChord())
                        addChord(toChord(e));
                  }
            }

      update();
      }

//---------------------------------------------------------
//   setPlaybackLocatorTick
//---------------------------------------------------------

void PianoLevels::setPlaybackLocatorTick(qreal tick)
      {
      const bool hadOldPos = _playbackLocatorValid;
      const int oldPos = hadOldPos
            ? tickToPixel(qRound(_playbackLocatorTick))
            : -1;

      _playbackLocatorTick = tick;
      _playbackLocatorValid = true;

      const int newPos = tickToPixel(qRound(tick));

      if (hadOldPos && oldPos == newPos)
            return;

      const int margin = 2;

      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            if (oldPos >= 0)
                  update(QRect(oldPos - margin, 0,
                               margin * 2 + 1, height()));

            update(QRect(newPos - margin, 0,
                         margin * 2 + 1, height()));
            }
      else {
            if (oldPos >= 0)
                  update(QRect(0, oldPos - margin,
                               width(), margin * 2 + 1));

            update(QRect(0, newPos - margin,
                         width(), margin * 2 + 1));
            }
      }

//---------------------------------------------------------
//   clearPlaybackLocatorTick
//---------------------------------------------------------

void PianoLevels::clearPlaybackLocatorTick()
      {
      if (!_playbackLocatorValid)
            return;

      const int oldPos =
            tickToPixel(qRound(_playbackLocatorTick));

      _playbackLocatorValid = false;

      const int margin = 2;

      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            update(QRect(oldPos - margin,
                         0,
                         margin * 2 + 1,
                         height()));
            }
      else {
            update(QRect(0,
                         oldPos - margin,
                         width(),
                         margin * 2 + 1));
            }
      }

//---------------------------------------------------------
//   setScope
//---------------------------------------------------------

void PianoLevels::setScope(PianoRollScope scope)
      {
      if (_scope == scope)
            return;

      _scope = scope;
      updateNotes();
      }

//---------------------------------------------------------
//   mouseTimePixel
//---------------------------------------------------------

int PianoLevels::mouseTimePixel(const QPointF& pos) const
      {
      return _orientation == PianoRollOrientation::HORIZONTAL
            ? qRound(pos.x())
            : qRound(pos.y());
      }

//---------------------------------------------------------
//   mouseValuePixel
//---------------------------------------------------------

int PianoLevels::mouseValuePixel(const QPointF& pos) const
      {
      return _orientation == PianoRollOrientation::HORIZONTAL
            ? qRound(pos.y())
            : qRound(pos.x());
      }

//---------------------------------------------------------
//   clearNoteData
//---------------------------------------------------------

void PianoLevels::clearNoteData()
      {
      _noteTimeBuckets.clear();
      noteList.clear();
      }

//---------------------------------------------------------
//   setTuplet
//---------------------------------------------------------

void PianoLevels::setTuplet(int value)
      {
      if (_tuplet != value) {
            _tuplet = value;
            update();
            emit tupletChanged(_tuplet);
            }
      }

//---------------------------------------------------------
//   setSubdiv
//---------------------------------------------------------

void PianoLevels::setSubdiv(int value)
      {
      if (_subdiv != value) {
            _subdiv = value;
            update();
            emit subdivChanged(_subdiv);
            }
      }

//---------------------------------------------------------
//   setLevelsIndex
//---------------------------------------------------------

void PianoLevels::setLevelsIndex(int index)
      {
      if (_levelsIndex != index) {
            _levelsIndex = index;
            update();
            emit levelsIndexChanged(_levelsIndex);
            }
      }

}
