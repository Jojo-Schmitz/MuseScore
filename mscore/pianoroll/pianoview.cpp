//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2009-2013 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "musescore.h"
#include "pianokeyboard.h"
#include "pianoruler.h"
#include "pianoview.h"
#include "preferences.h"
#include "scoreview.h"
#include "shortcut.h"

#include "libmscore/chord.h"
#include "libmscore/drumset.h"
#include "libmscore/measure.h"
#include "libmscore/note.h"
#include "libmscore/noteevent.h"
#include "libmscore/part.h"
#include "libmscore/rest.h"
#include "libmscore/score.h"
#include "libmscore/segment.h"
#include "libmscore/staff.h"
#include "libmscore/tie.h"
#include "libmscore/tuplet.h"
#include "libmscore/undo.h"
#include "libmscore/utils.h"

#include <QColorDialog>


namespace Ms {

extern MuseScore* mscore;

static const QString PIANO_NOTE_MIME_TYPE = "application/musescore/pianorollnotes";

static const qreal MIN_DRAG_DIST_SQ = 9;
static const int MIN_EVENT_NOTE_PIXELS = 2;
static const int NOTE_TIME_BUCKET_TICKS = DIVISION * 4;

const BarPattern PianoView::barPatterns[] = {
      {QT_TRANSLATE_NOOP("BarPattern", "C major / A minor"),   {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1}},
      {QT_TRANSLATE_NOOP("BarPattern", "D♭ major / B♭ minor"), {1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0}},
      {QT_TRANSLATE_NOOP("BarPattern", "D major / B minor"),   {0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1}},
      {QT_TRANSLATE_NOOP("BarPattern", "E♭ major / C minor"),  {1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0}},
      {QT_TRANSLATE_NOOP("BarPattern", "E major / C♯ minor"),  {0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1}},
      {QT_TRANSLATE_NOOP("BarPattern", "F major / D minor"),   {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0}},
      {QT_TRANSLATE_NOOP("BarPattern", "G♭ major / E♭ minor"), {0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1}},
      {QT_TRANSLATE_NOOP("BarPattern", "G major / E minor"),   {1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1}},
      {QT_TRANSLATE_NOOP("BarPattern", "A♭ major / F minor"),  {1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0}},
      {QT_TRANSLATE_NOOP("BarPattern", "A major / F♯ minor"),  {0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1}},
      {QT_TRANSLATE_NOOP("BarPattern", "B♭ major / G minor"),  {1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0}},
      {QT_TRANSLATE_NOOP("BarPattern", "B major / G♯ minor"),  {0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1}},
      {QT_TRANSLATE_NOOP("BarPattern", "C Diminished"),  {1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0}},
      {QT_TRANSLATE_NOOP("BarPattern", "D♭ Diminished"), {0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0}},
      {QT_TRANSLATE_NOOP("BarPattern", "D Diminished"),  {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1}},
      {QT_TRANSLATE_NOOP("BarPattern", "C Half/Whole"),  {1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0}},
      {QT_TRANSLATE_NOOP("BarPattern", "D♭ Half/Whole"), {0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1}},
      {QT_TRANSLATE_NOOP("BarPattern", "D Half/Whole"),  {1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1}},
      {QT_TRANSLATE_NOOP("BarPattern", "C Whole tone"),  {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0}},
      {QT_TRANSLATE_NOOP("BarPattern", "D♭ Whole tone"), {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}},
      {QT_TRANSLATE_NOOP("BarPattern", "C Augmented"),   {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}},
      {QT_TRANSLATE_NOOP("BarPattern", "D♭ Augmented"),  {0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0}},
      {QT_TRANSLATE_NOOP("BarPattern", "D Augmented"),   {0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0}},
      {QT_TRANSLATE_NOOP("BarPattern", "E♭ Augmented"),  {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1}},
      {"",              {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}
};

//---------------------------------------------------------
//   pianoRollScopeTracks
//---------------------------------------------------------

QVector<int> pianoRollScopeTracks(Staff* staff, PianoRollScope scope)
      {
      QVector<int> tracks;

      if (!staff)
            return tracks;

      auto appendStaffTracks = [&tracks](Staff* scopeStaff) {
            if (!scopeStaff)
                  return;

            const int staffIdx = scopeStaff->idx();
            if (staffIdx < 0)
                  return;

            const int firstTrack = staff2track(staffIdx);
            for (int voice = 0; voice < VOICES; ++voice)
                  tracks.append(firstTrack + voice);
            };

      switch (scope) {
            case PianoRollScope::STAFF:
                  appendStaffTracks(staff);
                  break;

            case PianoRollScope::PART: {
                  Part* part = staff->part();
                  if (!part || !part->staves())
                        break;

                  for (Staff* partStaff : *part->staves())
                        appendStaffTracks(partStaff);
                  break;
                  }

            case PianoRollScope::SCORE: {
                  Score* score = staff->score();
                  if (!score)
                        break;

                  for (Staff* scoreStaff : score->staves())
                        appendStaffTracks(scoreStaff);
                  break;
                  }
            }

      return tracks;
      }

//---------------------------------------------------------
//   pianoRollThemeColor
//---------------------------------------------------------

QColor pianoRollThemeColor(const QString& darkKey,
                           const QString& lightKey)
      {
      return preferences.getColor(darkTheme() ? darkKey : lightKey);
      }

//---------------------------------------------------------
//   pianoRollStaffColor
//---------------------------------------------------------

static QColor pianoRollStaffColor(const Staff* staff)
      {
      static const QString colorKeys[] = {
            PREF_UI_PIANOROLL_NOTE_COLOR_STAFF1,
            PREF_UI_PIANOROLL_NOTE_COLOR_STAFF2,
            PREF_UI_PIANOROLL_NOTE_COLOR_STAFF3,
            PREF_UI_PIANOROLL_NOTE_COLOR_STAFF4
            };

      const int staffIndex = staff ? qMax(staff->rstaff(), 0) : 0;

      return preferences.getColor(colorKeys[staffIndex % 4]);
      }

//---------------------------------------------------------
//   pianoRollNoteColor
//---------------------------------------------------------

QColor pianoRollNoteColor(const Note* note,
                          Coloring coloring,
                          bool honorSelection,
                          bool honorCustomColor)
      {
      if (!note)
            return QColor();

      if (honorSelection && note->selected()) {
            return pianoRollThemeColor(
                  PREF_UI_PIANOROLL_DARK_NOTE_SEL_COLOR,
                  PREF_UI_PIANOROLL_LIGHT_NOTE_SEL_COLOR);
            }

      if (honorCustomColor) {
            const QColor color = note->color();

            if (color != MScore::defaultColor)
                  return color;
            }

      if (coloring == Coloring::VOICING) {
            const int voice = note->voice();

            if (voice >= 0 && voice < VOICES)
                  return MScore::selectColor[voice];
            }

      else if (coloring == Coloring::STAFF) {
            return pianoRollStaffColor(note->staff());
            }

      else if (coloring == Coloring::INSTRUMENT) {
            Staff* staff = note->staff();
            Part* part = staff ? staff->part() : nullptr;

            if (part)
                  return QColor::fromRgb(part->masterPart()->color());
            }

      return MScore::defaultColor;
      }

//---------------------------------------------------------
//   darkTheme
//---------------------------------------------------------

bool darkTheme()
      {
      return preferences.effectiveGlobalStyle() == MuseScoreEffectiveStyleType::DARK_FUSION;
      }

//---------------------------------------------------------
//   PianoItem
//---------------------------------------------------------

PianoItem::PianoItem(Note* n, PianoView* pianoView)
      : _note(n), _pianoView(pianoView)
      {
      }

//---------------------------------------------------------
//   boundingRectTicks
//---------------------------------------------------------

QRect PianoItem::boundingRectTicks(NoteEvent* evt)
      {
      Chord* chord = _note->chord();
      int pitch = _note->pitch() + (evt ? evt->pitch() : 0);

      int ticks = _note->playTicks();

      Tuplet* tup = chord->tuplet();
      if (tup) {
            Fraction frac = tup->ratio();
            ticks = ticks * frac.denominator() / frac.numerator();
            }
      int tieLen = _note->playTicks() - ticks;

      int len = (evt ? ticks * evt->len() / 1000 : ticks) + tieLen;

      int x1 = _note->chord()->tick().ticks()
                  + (evt ? evt->ontime() * ticks / 1000 : 0);
      qreal y1 = pitch;

      QRect rect;
      rect.setRect(x1, y1, len, 1);
      return rect;
      }

//---------------------------------------------------------
//   intersectsBlock
//---------------------------------------------------------

bool PianoItem::intersectsBlock(int startTick, int endTick, int highPitch, int lowPitch, NoteEvent* evt)
      {
      QRect r = boundingRectTicks(evt);
      int pitch = r.y();

      return r.right() >= startTick && r.left() <= endTick
                  && pitch >= lowPitch && pitch <= highPitch;
      }

//---------------------------------------------------------
//   intersects
//---------------------------------------------------------

bool PianoItem::intersects(int startTick, int endTick, int highPitch, int lowPitch)
      {
      if (_pianoView->eventsAdjustTool()) {
            for (NoteEvent& e : _note->playEvents())
                  if (intersectsBlock(startTick, endTick, highPitch, lowPitch, &e))
                        return true;
            return false;
            }
      else
            return intersectsBlock(startTick, endTick, highPitch, lowPitch, 0);

      }

//---------------------------------------------------------
//    selectionRectAllowed
//---------------------------------------------------------

bool PianoView::selectionRectAllowed() const
      {
      return _editNoteTool == PianoRollEditTool::SELECT
            || _editNoteTool == PianoRollEditTool::EVENT_ADJUST;
      }

//---------------------------------------------------------
//    levelPreviewTickOffset
//---------------------------------------------------------

Fraction PianoView::levelPreviewTickOffset() const
      {
      return _levelPreviewTickOffset;
      }

//---------------------------------------------------------
//    levelPreviewEventTickDelta
//---------------------------------------------------------

Fraction PianoView::levelPreviewEventTickDelta() const
      {
      return _levelPreviewEventTickDelta;
      }

//---------------------------------------------------------
//    levelPreviewMovesNotes
//---------------------------------------------------------

bool PianoView::levelPreviewMovesNotes() const
      {
      return _levelPreviewActive && _dragStyle == DragStyle::NOTE_POSITION;
      }

//---------------------------------------------------------
//    levelPreviewMovesEvents
//---------------------------------------------------------

bool PianoView::levelPreviewMovesEvents() const
      { return _levelPreviewActive && (_dragStyle == DragStyle::EVENT_ONTIME || _dragStyle == DragStyle::EVENT_MOVE); }

//---------------------------------------------------------
//   levelEventPreview
//---------------------------------------------------------

bool PianoView::levelEventPreview(const NoteEvent* event, int& ontime, int& len) const
      {
      auto it = _levelEventPreviews.constFind(event);
      if (it == _levelEventPreviews.constEnd())
            return false;

      ontime = it.value().ontime;
      len = it.value().len;
      return true;
      }

//---------------------------------------------------------
//   levelPreviewLengthOffset
//---------------------------------------------------------

Fraction PianoView::levelPreviewLengthOffset() const
      {
      return _levelPreviewLengthOffset;
      }

//---------------------------------------------------------
//   levelPreviewResizesNotes
//---------------------------------------------------------

bool PianoView::levelPreviewResizesNotes() const
      {
      return _levelPreviewActive
            && (_dragStyle == DragStyle::NOTE_LENGTH_START
                || _dragStyle == DragStyle::NOTE_LENGTH_END);
      }

//---------------------------------------------------------
//   setLevelInteractionNotes
//---------------------------------------------------------

void PianoView::setLevelInteractionNotes(const QSet<const Note*>& notes)
      {
      _levelInteractionNotes = notes;
      viewport()->update();
      }

//---------------------------------------------------------
//   clearLevelInteractionNotes
//---------------------------------------------------------

void PianoView::clearLevelInteractionNotes()
      {
      if (_levelInteractionNotes.isEmpty())
            return;

      _levelInteractionNotes.clear();
      viewport()->update();
      }

//---------------------------------------------------------
//   levelInteractionHighlighted
//---------------------------------------------------------

bool PianoView::levelInteractionHighlighted(const Note* note) const
      {
      return _levelInteractionNotes.contains(note);
      }

//---------------------------------------------------------
//   setScope
//---------------------------------------------------------

void PianoView::setScope(PianoRollScope scope)
      {
      if (_scope == scope)
            return;

      _scope = scope;
      updateNotes();
      }

//---------------------------------------------------------
//   setColoring
//---------------------------------------------------------

void PianoView::setColoring(Coloring c)
      {
      if (_coloring == c)
            return;

      _coloring = c;
      scene()->update();
      }

//---------------------------------------------------------
//   setUseNoteColors
//---------------------------------------------------------

void PianoView::setUseNoteColors(bool value)
      {
      _useNoteColors = value;
      }

//---------------------------------------------------------
//   getTweakNoteEvent
//---------------------------------------------------------

NoteEvent* PianoItem::getTweakNoteEvent()
      {
      //Get topmost play event for note
      if (_note->playEvents().size() > 0)
            return &(_note->playEvents()[_note->playEvents().size() - 1]);

      return 0;
      }

//---------------------------------------------------------
//   PianoView
//---------------------------------------------------------

PianoView::PianoView()
      : QGraphicsView()
      {
      setFrameStyle(QFrame::NoFrame);
      setLineWidth(0);
      setMidLineWidth(0);
      setScene(new QGraphicsScene);
      setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
      setResizeAnchor(QGraphicsView::AnchorUnderMouse);
      setMouseTracking(true);
      _timeType   = TType::TICKS;
      _staff      = nullptr;
      _chord      = nullptr;
      _locator    = nullptr;
      _ticks      = 0;
      _barPattern = 0;
      _tuplet     = 1;
      _subdiv     = 0;
      _noteHeight = DEFAULT_KEY_HEIGHT;
      _xZoom      = X_ZOOM_INITIAL;
      _dragStarted = false;
      _dragStartPitch = 0;
      _mouseDown   = false;
      _dragStyle   = DragStyle::NONE;
      _inProgressUndoEvent = false;
      _scope = PianoRollScope::PART;
      _orientation = PianoRollOrientation::UNDEFINED;
      setOrientation(_orientation);
      _verticalPitchLayout = preferences.getBool(PREF_UI_PIANOROLL_VERTICAL_KEYBOARD_ALIGNED_GRID)
                  ? VerticalPitchLayout::KEYBOARD_ALIGNED
                  : VerticalPitchLayout::CHROMATIC;

      _cursorModifiers = QGuiApplication::keyboardModifiers();

      QPixmap addCursorPixmap =
            QIcon(":/data/icons/pianoroll-add.svg").pixmap(24, 24);
      QPixmap eraseCursorPixmap =
            QIcon(":/data/icons/pianoroll-erase.svg").pixmap(24, 24);
      QPixmap scissorsCursorPixmap =
            QIcon(":/data/icons/pianoroll-scissors.svg").pixmap(24, 24);
      QPixmap tieCursorPixmap =
            QIcon(":/data/icons/pianoroll-tie.svg").pixmap(24, 24);
      QPixmap tieConsolidateCursorPixmap =
                  QIcon(":/data/icons/pianoroll-consolidate-tie.svg").pixmap(24, 24);

      const QPoint addNoteHotSpot(3, 3);
      const QPoint eraseNoteHotSpot(3, 3);
      const QPoint scissorsNoteHotSpot(12, 12);
      const QPoint tieNoteHotSpot(12, 12);
      const QPoint tieConsolidateNoteHotSpot(12, 12);

      _addNoteCursor = QCursor(
            addCursorPixmap,
            addNoteHotSpot.x(),
            addNoteHotSpot.y());
      _eraseNoteCursor = QCursor(
            eraseCursorPixmap,
            eraseNoteHotSpot.x(),
            eraseNoteHotSpot.y());
      _scissorsNoteCursor = QCursor(
            scissorsCursorPixmap,
            scissorsNoteHotSpot.x(),
            scissorsNoteHotSpot.y());
      _tieNoteCursor = QCursor(
            tieCursorPixmap,
            tieNoteHotSpot.x(),
            tieNoteHotSpot.y());
      _tieConsolidateNoteCursor = QCursor(
            tieConsolidateCursorPixmap,
            tieConsolidateNoteHotSpot.x(),
            tieConsolidateNoteHotSpot.y());

      qApp->installEventFilter(this);

      memset(_pitchHighlight, 0, 128);
      }

//---------------------------------------------------------
//   ~PianoView
//---------------------------------------------------------

PianoView::~PianoView()
      {
      qApp->removeEventFilter(this);
      clearNoteData();
      }

//---------------------------------------------------------
//   drawTimeGrid
//---------------------------------------------------------

void PianoView::drawTimeGrid(QPainter* p,
                             int tick1,
                             int tick2,
                             qreal lineStart,
                             qreal lineEnd,
                             const QPen& penLineMajor,
                             const QPen& penLineMinor,
                             const QPen& penLineSub)
      {
      Score* score = currentScore();

      Pos pos1(score->tempomap(),
               score->sigmap(),
               tick1,
               TType::TICKS);

      Pos pos2(score->tempomap(),
               score->sigmap(),
               tick2,
               TType::TICKS);

      int bar1;
      int bar2;
      int beat;
      int tick;

      pos1.mbt(&bar1, &beat, &tick);
      pos2.mbt(&bar2, &beat, &tick);

      const int minBeatGap = 20;

      for (int bar = bar1; bar <= bar2; ++bar) {
            Pos barPos(
                  score->tempomap(),
                  score->sigmap(),
                  bar,
                  0,
                  0);

            const int beatsInBar =
                  barPos.timesig().timesig().numerator();

            const int ticksPerBeat =
                  barPos.timesig().timesig().beatTicks();

            const double pixPerBeat =
                  ticksPerBeat * _xZoom;

            int beatSkip =
                  ceil(minBeatGap / pixPerBeat);

            // Round up to next power of 2
            beatSkip =
                  int(pow(2, ceil(log(beatSkip) / log(2))));

            for (int beatIndex = 0;
                 beatIndex < beatsInBar;
                 beatIndex += beatSkip) {
                  Pos beatPos(
                        score->tempomap(),
                        score->sigmap(),
                        bar,
                        beatIndex,
                        0);

                  const int beatTick =
                        beatPos.time(TType::TICKS);

                  const qreal beatPixel =
                        _orientation == PianoRollOrientation::HORIZONTAL
                              ? tickToPixelX(beatTick)
                              : tickToPixelY(beatTick);

                  p->setPen(penLineMinor);

                  if (_orientation == PianoRollOrientation::HORIZONTAL)
                        p->drawLine(
                              beatPixel,
                              lineStart,
                              beatPixel,
                              lineEnd);
                  else
                        p->drawLine(
                              lineStart,
                              beatPixel,
                              lineEnd,
                              beatPixel);

                  const int subbeats =
                        _tuplet * (1 << _subdiv);

                  for (int sub = 1;
                       sub < subbeats;
                       ++sub) {
                        Pos subBeatPos(
                              score->tempomap(),
                              score->sigmap(),
                              bar,
                              beatIndex,
                              sub * DIVISION / subbeats);

                        const int subBeatTick =
                              subBeatPos.time(TType::TICKS);

                        const qreal subBeatPixel =
                              _orientation == PianoRollOrientation::HORIZONTAL
                                    ? tickToPixelX(subBeatTick)
                                    : tickToPixelY(subBeatTick);

                        p->setPen(penLineSub);

                        if (_orientation == PianoRollOrientation::HORIZONTAL)
                              p->drawLine(
                                    subBeatPixel,
                                    lineStart,
                                    subBeatPixel,
                                    lineEnd);
                        else
                              p->drawLine(
                                    lineStart,
                                    subBeatPixel,
                                    lineEnd,
                                    subBeatPixel);
                        }
                  }

            const int barTick =
                  barPos.time(TType::TICKS);

            const qreal barPixel =
                  _orientation == PianoRollOrientation::HORIZONTAL
                        ? tickToPixelX(barTick)
                        : tickToPixelY(barTick);

            // Preserve the existing orientation-specific
            // zero-tick behavior
            if (_orientation == PianoRollOrientation::HORIZONTAL)
                  p->setPen(
                        barPixel > 0.0
                              ? penLineMajor
                              : QPen(Qt::black, 2.0));
            else
                  p->setPen(
                        barTick > 0
                              ? penLineMajor
                              : QPen(Qt::black, 2.0));

            if (_orientation == PianoRollOrientation::HORIZONTAL)
                  p->drawLine(
                        barPixel,
                        lineStart,
                        barPixel,
                        lineEnd);
            else
                  p->drawLine(
                        lineStart,
                        barPixel,
                        lineEnd,
                        barPixel);
            }
      }

//---------------------------------------------------------
//   drawVisibleNotes
//---------------------------------------------------------

void PianoView::drawVisibleNotes(QPainter* p, const QRectF& exposedRect)
      {
      // drawBackground() may be called for only a small exposed portion
      // of the scene, so avoid running the complete note-painting path for
      // notes that can't affect that region
      const QRectF noteCullRect =
            exposedRect.adjusted(-3.0, -3.0, 3.0, 3.0);

      const bool applyEvents = eventsAdjustTool();

      auto noteBlockVisible =
            [this, &noteCullRect, applyEvents](PianoItem* block) {
                  if (!block)
                        return false;

                  Note* note = block->note();
                  if (!note || note->tieBack())
                        return false;

                  const NoteEventList& playEvents = note->playEvents();

                  if (playEvents.isEmpty()) {
                        return noteCullRect.intersects(
                              QRectF(boundingRect(note, nullptr, false)));
                        }

                  for (const NoteEvent& event : playEvents) {
                        const QRectF bounds =
                              boundingRect(note, &event, applyEvents);

                        if (noteCullRect.intersects(bounds))
                              return true;
                        }

                  return false;
                  };

      int tick1;
      int tick2;

      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            tick1 = scenePosToTick(
                  QPointF(noteCullRect.left(), 0.0));
            tick2 = scenePosToTick(
                  QPointF(noteCullRect.right(), 0.0));
            }
      else {
            tick1 = scenePosToTick(
                  QPointF(0.0, noteCullRect.top()));
            tick2 = scenePosToTick(
                  QPointF(0.0, noteCullRect.bottom()));
            }

      const QVector<PianoItem*> noteCandidates =
            noteCandidatesForTickRange(
                  qMin(tick1, tick2),
                  qMax(tick1, tick2));

      p->setRenderHints(
            QPainter::Antialiasing
            | QPainter::SmoothPixmapTransform
            | QPainter::TextAntialiasing);

      for (PianoItem* block : noteCandidates) {
            if (!block->note()->selected()
                && noteBlockVisible(block)) {
                  drawNoteBlock(p, block);
                  }
            }

      // Selected notes must have higher Z-order precedence
      for (PianoItem* block : noteCandidates) {
            if (block->note()->selected()
                && noteBlockVisible(block)) {
                  drawNoteBlock(p, block);
                  }
            }

      if (_dragStyle == DragStyle::NOTE_POSITION
          || _dragStyle == DragStyle::NOTE_LENGTH_END
          || _dragStyle == DragStyle::NOTE_LENGTH_START
          || _dragStyle == DragStyle::DRAW_NOTE
          || _dragStyle == DragStyle::EVENT_LENGTH
          || _dragStyle == DragStyle::EVENT_MOVE
          || _dragStyle == DragStyle::EVENT_ONTIME) {
            drawDraggedNotes(p);
            }
      }

//---------------------------------------------------------
//   drawBackground
//---------------------------------------------------------

void PianoView::drawBackground(QPainter* p, const QRectF& r)
      {
      if (_staff == 0)
            return;

      const QColor colSelectionBox =
            pianoRollThemeColor(
                  PREF_UI_PIANOROLL_DARK_SELECTION_BOX_COLOR,
                  PREF_UI_PIANOROLL_LIGHT_SELECTION_BOX_COLOR);

      const QColor colWhiteKeyBg =
            pianoRollThemeColor(
                  PREF_UI_PIANOROLL_DARK_BG_KEY_WHITE_COLOR,
                  PREF_UI_PIANOROLL_LIGHT_BG_KEY_WHITE_COLOR);

      const QColor colGutter =
            pianoRollThemeColor(
                  PREF_UI_PIANOROLL_DARK_BG_BASE_COLOR,
                  PREF_UI_PIANOROLL_LIGHT_BG_BASE_COLOR);

      const QColor colBlackKeyBg =
            pianoRollThemeColor(
                  PREF_UI_PIANOROLL_DARK_BG_KEY_BLACK_COLOR,
                  PREF_UI_PIANOROLL_LIGHT_BG_KEY_BLACK_COLOR);

      const QColor colHilightKeyBg =
            pianoRollThemeColor(
                  PREF_UI_PIANOROLL_DARK_BG_KEY_HIGHLIGHT_COLOR,
                  PREF_UI_PIANOROLL_LIGHT_BG_KEY_HIGHLIGHT_COLOR);

      const QColor colGridLine =
            pianoRollThemeColor(
                  PREF_UI_PIANOROLL_DARK_BG_GRIDLINE_COLOR,
                  PREF_UI_PIANOROLL_LIGHT_BG_GRIDLINE_COLOR);

      const QColor colSelectionBoxFill =
            QColor(colSelectionBox.red(),
                   colSelectionBox.green(),
                   colSelectionBox.blue(),
                   128);

      const QPen penLineMajor = QPen(colGridLine, 2.0, Qt::SolidLine);
      const QPen penLineMinor = QPen(colGridLine, 1.0, Qt::SolidLine);
      const QPen penLineSub   = QPen(colGridLine, 1.0, Qt::DotLine);

      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            QRectF r1;
            r1.setCoords(-DBL_MAX, 0.0, tickToPixelX(0), DBL_MAX);
            QRectF r2;
            r2.setCoords(tickToPixelX(_ticks), 0.0, DBL_MAX, DBL_MAX);

            p->fillRect(r, colWhiteKeyBg);
            if (r.intersects(r1))
                  p->fillRect(r.intersected(r1), colGutter);
            if (r.intersects(r2))
                  p->fillRect(r.intersected(r2), colGutter);

            //
            // Draw horizontal grid lines
            //
            qreal y1 = r.y();
            qreal y2 = y1 + r.height();
            qreal x1 = qMax(r.x(), (qreal)tickToPixelX(0));
            qreal x2 = qMin(x1 + r.width(), (qreal)tickToPixelX(_ticks));

            int topPitch = ceil((_noteHeight * 128 - y1) / _noteHeight);
            int bmPitch = floor((_noteHeight * 128 - y2) / _noteHeight);

            Part* part = _staff->part();
            Interval transp = part->instrument()->transpose();

            // MIDI notes span [0, 127] and map to pitches starting at C-1
            for (int pitch = bmPitch; pitch <= topPitch; ++pitch) {
                  int y = (127 - pitch) * _noteHeight;

                  int degree = (pitch - transp.chromatic + 60) % 12;
                  const BarPattern& pat = barPatterns[_barPattern];

                  if (!pat.isWhiteKey[degree] || _pitchHighlight[pitch]) {
                        qreal px0 = qMax(r.x(), (qreal)tickToPixelX(0));
                        qreal px1 = qMin(r.x() + r.width(), (qreal)tickToPixelX(_ticks));
                        QRectF hbar;

                        hbar.setCoords(px0, y, px1, y + _noteHeight);
                        p->fillRect(hbar,
                                    _pitchHighlight[pitch] ? colHilightKeyBg : colBlackKeyBg);
                        }

                  // Lines between rows
                  p->setPen(degree == 0 ? penLineMajor : penLineMinor);
                  p->drawLine(QLineF(x1, y + _noteHeight, x2, y + _noteHeight));
                  }

            //
            // Draw vertical grid lines
            //
            const int tick1 = qMax(pixelXToTick(int(x1)), 0);
            const int tick2 = qMax(pixelXToTick(int(x2)), 0);

            drawTimeGrid(
                  p,
                  tick1,
                  tick2,
                  y1,
                  y2,
                  penLineMajor,
                  penLineMinor,
                  penLineSub);

            drawVisibleNotes(p, r);

            //
            // Draw locators
            //
            for (int i = 0; i < 3; ++i) {
                  if (!_locator[i].valid())
                        continue;

                  if (i == 0 && !preferences.getBool(PREF_UI_PIANOROLL_PLAYBACK_SHOW_CURSOR))
                        continue;

                  p->setPen(QPen(i == 0 ? Qt::red : Qt::blue, 2));

                  qreal x;

                  if (i == 0 && _playbackLocatorTickValid)
                        x = tickToPixelXF(_playbackLocatorTick);
                  else
                        x = tickToPixelX(_locator[i].time(TType::TICKS));

                  p->drawLine(x, y1, x, y2);
                  }
            }
      else {
            // In vertical mode:
            //
            //    X = pitch, low -> high
            //    Y = time, future -> past
            //
            // tick 0 is therefore at the bottom of the score
            // and the final tick is at the top

            const qreal scoreTop    = tickToPixelY(_ticks);
            const qreal scoreBottom = tickToPixelY(0);

            //
            // Background and gutters outside the score time range
            //

            p->fillRect(r, colWhiteKeyBg);

            QRectF topGutter;
            topGutter.setCoords(-DBL_MAX, -DBL_MAX, DBL_MAX, scoreTop);

            QRectF bottomGutter;
            bottomGutter.setCoords(-DBL_MAX, scoreBottom, DBL_MAX, DBL_MAX);

            if (r.intersects(topGutter))
                  p->fillRect(r.intersected(topGutter), colGutter);

            if (r.intersects(bottomGutter))
                  p->fillRect(r.intersected(bottomGutter), colGutter);

            //
            // Visible bounds within the actual score/pitch area
            //

            qreal x1 = qMax(r.left(), 0.0);
            qreal x2 = qMin(r.right(), qreal(_noteHeight * 128));

            qreal y1 = qMax(r.top(), scoreTop);
            qreal y2 = qMin(r.bottom(), scoreBottom);

            //
            // Draw vertical pitch columns
            //

            // Horizontal view draws each pitch as a row.
            // Here, each pitch becomes a column

            Part* part = _staff->part();
            Interval transp = part->instrument()->transpose();
            const BarPattern& pat = barPatterns[_barPattern];

            //
            // Draw vertical pitch grid
            //

            if (_verticalPitchLayout == VerticalPitchLayout::KEYBOARD_ALIGNED) {
                  // Black-pitch lanes match the physical black keys exactly
                  // White-pitch lanes fill the remaining space
                  for (int pitch = 0; pitch < 128; ++pitch) {
                        QRectF lane = keyboardAlignedPitchLane(pitch);

                        if (lane.width() <= 0.0)
                              continue;

                        qreal laneLeft  = lane.left();
                        qreal laneRight = lane.right();

                        if (laneRight < r.left() || laneLeft > r.right())
                              continue;

                        int degree = (pitch - transp.chromatic + 60) % 12;
                        if (degree < 0)
                              degree += 12;

                        // Preserve the normal black-key shading and
                        // pitch-highlight behavior

                        if (!pat.isWhiteKey[degree] || _pitchHighlight[pitch]) {
                              QRectF vbar(
                                    laneLeft,
                                    y1,
                                    lane.width(),
                                    y2 - y1
                                    );

                              p->fillRect(
                                    vbar,
                                    _pitchHighlight[pitch]
                                          ? colHilightKeyBg
                                          : colBlackKeyBg
                                    );
                              }

                        // Draw the boundary at the left side of each lane
                        // C gets the stronger octave boundary

                        p->setPen(degree == 0 ? penLineMajor : penLineMinor);
                        p->drawLine(
                              QLineF(
                                    laneLeft,
                                    y1,
                                    laneLeft,
                                    y2
                                    )
                              );
                        }

                  //
                  // Finish the right edge of the MIDI range
                  //

                  QRectF lastLane = keyboardAlignedPitchLane(127);

                  if (lastLane.width() > 0.0) {
                        qreal x = lastLane.right();

                        if (x >= r.left() && x <= r.right()) {
                              p->setPen(penLineMinor);
                              p->drawLine(QLineF(x, y1, x, y2));
                              }
                        }


                  }
            else {
                  //
                  // Normal chromatic layout:
                  // every MIDI semitone occupies exactly one equal-width lane
                  //

                  for (int pitch = 0; pitch < 128; ++pitch) {
                        qreal x = pitch * _noteHeight;

                        int degree = (pitch - transp.chromatic + 60) % 12;
                        if (degree < 0)
                              degree += 12;

                        if (!pat.isWhiteKey[degree] || _pitchHighlight[pitch]) {
                              QRectF vbar(
                                    x,
                                    y1,
                                    _noteHeight,
                                    y2 - y1
                                    );

                              p->fillRect(
                                    vbar,
                                    _pitchHighlight[pitch]
                                          ? colHilightKeyBg
                                          : colBlackKeyBg
                                    );
                              }

                        p->setPen(degree == 0 ? penLineMajor : penLineMinor);
                        p->drawLine(
                              QLineF(
                                    x + _noteHeight,
                                    y1,
                                    x + _noteHeight,
                                    y2
                                    )
                              );
                        }
                  }

            //
            // Horizontal time grid
            //

            int tick1 = qBound(0, pixelYToTick(int(y2)), _ticks);
            int tick2 = qBound(0, pixelYToTick(int(y1)), _ticks);

            if (tick2 < tick1)
                  qSwap(tick1, tick2);

            drawTimeGrid(
                  p,
                  tick1,
                  tick2,
                  x1,
                  x2,
                  penLineMajor,
                  penLineMinor,
                  penLineSub);

            drawVisibleNotes(p, r);


            // Vertical mode: deliberately have no locator lines -
            // The bottom edge will eventually function as the fixed
            // playback / activation position
            }

      //
      // Draw drag selection box
      //
      if (_dragStarted && _dragStyle == DragStyle::SELECTION_RECT) {
            const int minX =
                  qMin(_mouseDownPos.x(), _lastMousePos.x());
            const int minY =
                  qMin(_mouseDownPos.y(), _lastMousePos.y());
            const int maxX =
                  qMax(_mouseDownPos.x(), _lastMousePos.x());
            const int maxY =
                  qMax(_mouseDownPos.y(), _lastMousePos.y());

            const QRectF rect(minX, minY,
                              maxX - minX + 1, maxY - minY + 1);

            p->setPen(QPen(colSelectionBox, 2));
            p->setBrush(QBrush(colSelectionBoxFill, Qt::SolidPattern));
            p->drawRect(rect);
            }

      }

//---------------------------------------------------------
//   useOnsetDiamond
//---------------------------------------------------------

bool PianoView::useOnsetDiamond(const Staff* staff,
                                const Fraction& tick) const
      {
      if (!staff || !staff->part())
            return false;

      const Instrument* instrument =
            staff->part()->instrument(tick);

      if (!instrument)
            return false;

      switch (instrument->pianoRollNoteShape()) {
            case PianoRollNoteShape::RECTANGLE:
                  return false;

            case PianoRollNoteShape::DIAMOND:
                  return true;

            case PianoRollNoteShape::AUTO:
            default:
                  return staff->isDrumStaff(tick);
            }
      }

//---------------------------------------------------------
//   useOnsetDiamond
//---------------------------------------------------------

bool PianoView::useOnsetDiamond(const Note* note) const
      {
      if (!note)
            return false;

      return useOnsetDiamond(
            note->staff(),
            note->tick());
      }

//---------------------------------------------------------
//   onsetDiamondRect
//---------------------------------------------------------

QRect PianoView::onsetDiamondRect(const Note* note,
                                  const NoteEvent* event,
                                  bool applyEvents) const
      {
      if (!note)
            return QRect();

      Chord* chord = note->chord();
      if (!chord)
            return QRect();

      Fraction ticks = chord->ticks();

      if (Tuplet* tuplet = chord->tuplet())
            ticks *= tuplet->ratio().inverse();

      Fraction centerTick = chord->tick();

      // In [playback-event adjustment mode] the event on-time moves
      // the diamond itself.  Event length deliberately has no visual
      // effect on a drum diamond
      if (event && applyEvents)
            centerTick += ticks * event->ontime() / 1000;

      const int pitch =
            note->pitch() + (event && applyEvents ? event->pitch() : 0);

      // Diamond diameter is tied to pitch-lane thickness rather than
      // note duration. Keep it slightly smaller than the lane
      const int subbeats = _tuplet * (1 << _subdiv);
      const Fraction gridLength(1, 4 * subbeats);

      const qreal gridPixels = qAbs(
            tickToPixelXF((centerTick + gridLength).ticks())
            - tickToPixelXF(centerTick.ticks()));

      const qreal size =
            qMax<qreal>(6.0,
                        qMin<qreal>(qreal(_noteHeight),
                                    gridPixels));

      const qreal half = size / 2.0;

      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            const qreal cx = tickToPixelXF(centerTick.ticks());

            const qreal cy =
                  (pitchToPixelY(pitch)
                   + pitchToPixelY(pitch + 1)) / 2.0;

            return QRect(
                  qRound(cx - half),
                  qRound(cy - half),
                  qRound(size),
                  qRound(size));
            }
      else {
            qreal cx;

            if (_verticalPitchLayout == VerticalPitchLayout::KEYBOARD_ALIGNED)
                  cx = keyboardAlignedPitchLane(pitch).center().x();
            else
                  cx = (pitch + 0.5) * _noteHeight;

            const qreal cy = tickToPixelYF(centerTick.ticks());

            return QRect(
                  qRound(cx - half),
                  qRound(cy - half),
                  qRound(size),
                  qRound(size));
            }
      }

//---------------------------------------------------------
//   drawNoteBlock
//---------------------------------------------------------

void PianoView::drawNoteBlock(QPainter* p, PianoItem* block)
      {
      Note* note = block->note();
      if (note->tieBack()) {
            return;
            }

      const qreal outlineSize = 2.5;

      NoteEventList& playEvents = note->playEvents();

      for (int index = 0; index < playEvents.size(); ++index) {
            NoteEvent& e = playEvents[index];

            const bool playing =
                  preferences.getBool(PREF_UI_PIANOROLL_PLAYBACK_HIGHLIGHT_NOTES)
                  && _playbackNoteEvents.value(note).contains(index);

            QColor noteColor;

            if (playing) {
                  noteColor = pianoRollThemeColor(
                                    PREF_UI_PIANOROLL_DARK_NOTE_SEL_COLOR,
                                    PREF_UI_PIANOROLL_LIGHT_NOTE_SEL_COLOR);
                  }
            else {
                  if (levelInteractionHighlighted(note)) {
                        noteColor = pianoRollThemeColor(
                                          PREF_UI_PIANOROLL_DARK_NOTE_DRAG_COLOR,
                                          PREF_UI_PIANOROLL_LIGHT_NOTE_DRAG_COLOR);
                        }
                  else {
                        noteColor = pianoRollNoteColor(note, _coloring, true, _useNoteColors);
                        }

                  const bool ghostOriginal =
                        _dragStarted
                        && note->selected()
                        && (_dragStyle == DragStyle::NOTE_POSITION
                            || _dragStyle == DragStyle::NOTE_LENGTH_START
                            || _dragStyle == DragStyle::NOTE_LENGTH_END
                            || _dragStyle == DragStyle::EVENT_ONTIME
                            || _dragStyle == DragStyle::EVENT_MOVE
                            || _dragStyle == DragStyle::EVENT_LENGTH);

                  if (ghostOriginal)
                        noteColor.setAlphaF(0.25);
                  }

            const QColor borderColor =
                  preferences.getBool(PREF_UI_PIANOROLL_NOTE_BORDER_COLOR_LIGHTER)
                        ? noteColor.lighter(125)
                        : noteColor.darker(175);

            p->setBrush(noteColor);
            p->setPen(QPen(borderColor, outlineSize));

            const bool onsetDiamond = useOnsetDiamond(note);

            QRect bounds =
                  onsetDiamond
                  ? onsetDiamondRect(
                        note,
                        &e,
                        _editNoteTool == PianoRollEditTool::EVENT_ADJUST)
                  : boundingRect(
                        note,
                        &e,
                        _editNoteTool == PianoRollEditTool::EVENT_ADJUST);

            if (onsetDiamond) {
                  const QPointF c = bounds.center();

                  QPolygonF diamond;
                  diamond
                        << QPointF(c.x(), bounds.top())
                        << QPointF(bounds.right(), c.y())
                        << QPointF(c.x(), bounds.bottom())
                        << QPointF(bounds.left(), c.y());

                  p->drawPolygon(diamond);
                  }
            else {
                  p->drawRoundedRect(
                        bounds,
                        _noteRectRoundedRadius,
                        _noteRectRoundedRadius);

                  drawPitchText(
                        p,
                        bounds,
                        note->tpcUserName(),
                        noteColor);
                  }
            }

      if (!useOnsetDiamond(note)
          && _editNoteTool != PianoRollEditTool::EVENT_ADJUST) {

            const QColor colorTie =
                  pianoRollThemeColor(
                        PREF_UI_PIANOROLL_DARK_BG_TIE_COLOR,
                        PREF_UI_PIANOROLL_LIGHT_BG_TIE_COLOR);

            const qreal outlineTieSize = outlineSize * 1.25;
            p->setPen(QPen(colorTie, outlineTieSize));
            for (Tie* note_tie = note->tieFor(); note_tie != nullptr; note_tie = note_tie->endNote()->tieFor()) {
                  Fraction tieTime = note_tie->endNote()->tick();
                  float xpos = tickToPixelX(tieTime.ticks());
                  int pitch = note_tie->endNote()->pitch();
                  int y0 = pitchToPixelY(pitch + 1);
                  int y1 = pitchToPixelY(pitch);

                  p->drawLine(xpos, y0, xpos, y1);
                  }
            }
      }

QRect PianoView::boundingRect(const Note* note, bool applyEvents)
      {
      if (note->playEvents().size())
            return boundingRect(note, &note->playEvents().first(), applyEvents);
      return QRect();
      }


QRect PianoView::boundingRect(const Note* note, const NoteEvent* evt, bool applyEvents)
      {
      if (useOnsetDiamond(note))
            return onsetDiamondRect(note, evt, applyEvents);

      Chord* chord = note->chord();
      int pitch = note->pitch() + (evt ? evt->pitch() : 0);

      Fraction ticks = chord->ticks();
      Tuplet* tup = chord->tuplet();
      if (tup) {
            Fraction frac = tup->ratio();
            ticks = ticks * frac.inverse();
            }
      Fraction tieLen = note->playTicksFraction() - ticks;

      Fraction start;
      Fraction len;
      if (evt && applyEvents) {
            start = note->chord()->tick() + ticks * evt->ontime() / 1000;
            len = ticks * evt->len() / 1000 + tieLen;
            }
      else {
            start = note->chord()->tick();
            len = ticks + tieLen;
            }

      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            int x0 = tickToPixelX(start.ticks());
            int y0 = pitchToPixelY(pitch + 1);
            int x1 = tickToPixelX((start + len).ticks());
            int y1 = pitchToPixelY(pitch);

            int width = x1 - x0;

            if (evt && applyEvents && width >= 0)
                  width = qMax(width, MIN_EVENT_NOTE_PIXELS);

            QRect rect;
            rect.setRect(x0, y0, width, y1 - y0);
            return rect;
            }
      else { // VERTICAL
            qreal center;
            qreal width = _noteHeight;

            if (_verticalPitchLayout == VerticalPitchLayout::KEYBOARD_ALIGNED) {
                  QRectF lane = keyboardAlignedPitchLane(pitch);

                  center = lane.center().x();

                  // All note items remain uniform width,
                  // equal to the black-key width
                  width = _noteHeight;
                  }
            else {
                  center = (pitch + 0.5) * _noteHeight;
                  width = _noteHeight;
                  }

            int x0 = qRound(center - width / 2.0);

            int y0 = tickToPixelY((start + len).ticks());
            int y1 = tickToPixelY(start.ticks());

            int height = y1 - y0;

            if (evt && applyEvents && height >= 0)
                  height = qMax(height, MIN_EVENT_NOTE_PIXELS);

            QRect rect;
            rect.setRect(x0, y0, qRound(width), height);
            return rect;
            }

      }

//---------------------------------------------------------
//   moveLocator
//---------------------------------------------------------

void PianoView::moveLocator(int /*i*/)
      {
      scene()->update();
      }

//---------------------------------------------------------
//   setPlaybackNoteEvents
//---------------------------------------------------------

void PianoView::setPlaybackNoteEvents(const QHash<const Note*, QSet<int>>& events)
      {
      if (_playbackNoteEvents == events)
            return;

      QRectF dirtyRect;

      // Repaint both the previously-active and newly-active
      // NoteEvents.  This removes old highlights and paints
      // the new ones without invalidating the whole scene

      QSet<const Note*> notes;

      for (auto it = _playbackNoteEvents.constBegin();
           it != _playbackNoteEvents.constEnd(); ++it)
            notes.insert(it.key());

      for (auto it = events.constBegin();
           it != events.constEnd(); ++it)
            notes.insert(it.key());

      for (const Note* note : notes) {
            const NoteEventList& playEvents = note->playEvents();

            QSet<int> indices = _playbackNoteEvents.value(note);
            indices.unite(events.value(note));

            for (int index : indices) {
                  if (index < 0 || index >= playEvents.size())
                        continue;

                  const NoteEvent* event = &playEvents[index];

                  dirtyRect |= boundingRect(
                        note,
                        event,
                        _editNoteTool == PianoRollEditTool::EVENT_ADJUST);
                  }
            }

      _playbackNoteEvents = events;

      if (!dirtyRect.isNull()) {
            dirtyRect.adjust(-3.0, -3.0, 3.0, 3.0);
            scene()->update(dirtyRect);
            }
      }

//---------------------------------------------------------
//   clearPlaybackNoteEvents
//---------------------------------------------------------

void PianoView::clearPlaybackNoteEvents()
      {
      setPlaybackNoteEvents(QHash<const Note*, QSet<int>>());
      }

//---------------------------------------------------------
//   setPlaybackLocatorTick
//---------------------------------------------------------

void PianoView::setPlaybackLocatorTick(qreal tick)
      {
      if (_orientation != PianoRollOrientation::HORIZONTAL)
            return;

      const qreal oldX = _playbackLocatorTickValid
            ? tickToPixelXF(_playbackLocatorTick)
            : -1.0;

      const qreal newX = tickToPixelXF(tick);

      _playbackLocatorTick = tick;
      _playbackLocatorTickValid = true;

      const QRectF sr = sceneRect();
      const qreal margin = 3.0;

      if (oldX >= 0.0) {
            scene()->update(
                  QRectF(oldX - margin,
                         sr.top(),
                         margin * 2.0 + 1.0,
                         sr.height()));
            }

      scene()->update(
            QRectF(newX - margin,
                   sr.top(),
                   margin * 2.0 + 1.0,
                   sr.height()));
      }

//---------------------------------------------------------
//   clearPlaybackLocatorTick
//---------------------------------------------------------

void PianoView::clearPlaybackLocatorTick()
      {
      if (!_playbackLocatorTickValid)
            return;

      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            const qreal oldX =
                  tickToPixelXF(_playbackLocatorTick);

            const QRectF sr = sceneRect();
            const qreal margin = 3.0;

            scene()->update(
                  QRectF(oldX - margin,
                         sr.top(),
                         margin * 2.0 + 1.0,
                         sr.height()));
            }

      _playbackLocatorTickValid = false;
      }

//---------------------------------------------------------
//   snapTickToGrid
//---------------------------------------------------------

Fraction PianoView::snapTickToGrid(int tick, Direction direction) const
      {
      return roundToNearestBeat(tick, direction == Direction::DOWN);
      }

//---------------------------------------------------------
//   pixelXToTick
//---------------------------------------------------------

int PianoView::pixelXToTick(int pixX) const
      {
      return static_cast<int>(pixX / _xZoom) - MAP_OFFSET;
      }

//---------------------------------------------------------
//   tickToPixelX
//---------------------------------------------------------

int PianoView::tickToPixelX(int tick) const
      {
      return static_cast<int>(tick + MAP_OFFSET) * _xZoom;
      }

//---------------------------------------------------------
//   tickToPixelXF
//---------------------------------------------------------

qreal PianoView::tickToPixelXF(qreal tick) const
      {
      return tick * _xZoom + MAP_OFFSET * _xZoom;
      }

//---------------------------------------------------------
//   pixelYToTick
//---------------------------------------------------------

int PianoView::pixelYToTick(int y) const
      {
      return _ticks - pixelXToTick(y);
      }

//---------------------------------------------------------
//   tickToPixelY
//---------------------------------------------------------

int PianoView::tickToPixelY(int tick) const
      {
      return tickToPixelX(_ticks - tick);
      }

//---------------------------------------------------------
//   tickToPixelYF
//---------------------------------------------------------

qreal PianoView::tickToPixelYF(qreal tick) const
      {
      return tickToPixelXF(_ticks - tick);
      }

//---------------------------------------------------------
//   pixelXtoPitch
//---------------------------------------------------------

int PianoView::pixelXToPitch(int pixX) const
      {
      if (_verticalPitchLayout == VerticalPitchLayout::KEYBOARD_ALIGNED) {
            for (int pitch = 0; pitch < 128; ++pitch) {
                  const QRectF lane = keyboardAlignedPitchLane(pitch);

                  if (lane.width() <= 0.0)
                        continue;

                  if (pixX >= lane.left() && pixX < lane.right())
                        return pitch;
                  }

            if (pixX < keyboardAlignedPitchLane(0).left())
                  return 0;

            return 127;
            }

      return qBound(
            0,
            int(floor(pixX / qreal(_noteHeight))),
            127);
      }

//---------------------------------------------------------
//   pixelYtoPitch
//---------------------------------------------------------

int PianoView::pixelYToPitch(int pixY) const
      {
      return (int)floor(128 - pixY / (qreal)_noteHeight);
      }

//---------------------------------------------------------
//   pitchToPixelX
//---------------------------------------------------------

int PianoView::pitchToPixelX(int pitch) const
      {
      pitch = qBound(0, pitch, 127);

      if (_verticalPitchLayout == VerticalPitchLayout::KEYBOARD_ALIGNED)
            return qRound(keyboardAlignedPitchLane(pitch).left());

      return pitch * _noteHeight;
      }

//---------------------------------------------------------
//   pixelToPixelY
//---------------------------------------------------------

int PianoView::pitchToPixelY(int pitch) const
      {
      return (128 - pitch) * _noteHeight;
      }

//---------------------------------------------------------
//   scenePosToTick
//---------------------------------------------------------

int PianoView::scenePosToTick(const QPointF& pos) const
      {
      if (_orientation == PianoRollOrientation::VERTICAL)
            return pixelYToTick(int(pos.y()));

      return pixelXToTick(int(pos.x()));
      }

//---------------------------------------------------------
//   scenePosToPitch
//---------------------------------------------------------

int PianoView::scenePosToPitch(const QPointF& pos) const
      {
      if (_orientation == PianoRollOrientation::HORIZONTAL)
            return pixelYToPitch(pos.y());

      // Vertical / chromatic mode:
      // one equal-width lane per MIDI semitone
      if (_verticalPitchLayout == VerticalPitchLayout::CHROMATIC) {
            int pitch = int(floor(pos.x() / _noteHeight));
            return qBound(0, pitch, 127);
            }

      // Vertical / keyboard-aligned mode:
      // determine which keyboard-aligned lane contains X
      for (int pitch = 0; pitch < 128; ++pitch) {
            QRectF lane = keyboardAlignedPitchLane(pitch);

            if (lane.width() <= 0.0)
                  continue;

            if (pos.x() >= lane.left() && pos.x() < lane.right())
                  return pitch;
            }

      // Outside the represented pitch range:
      return -1;
      }

//---------------------------------------------------------
//   toolCanDragNotes
//---------------------------------------------------------

bool PianoView::toolCanDragNotes() const
      {
      return _editNoteTool == PianoRollEditTool::SELECT
            || _editNoteTool == PianoRollEditTool::ADD
            || _editNoteTool == PianoRollEditTool::CUT
            || _editNoteTool == PianoRollEditTool::TIE;
      }

//---------------------------------------------------------
//   calculateNoteDragOffsets
//---------------------------------------------------------

bool PianoView::calculateNoteDragOffsets(Fraction& pasteTickOffset,
                                         Fraction& pasteLengthOffset,
                                         int& pitchOffset) const
      {
      pasteTickOffset = Fraction(0, 1);
      pasteLengthOffset = Fraction(0, 1);
      pitchOffset = 0;

      if (!_staff)
            return false;

      Score* score = currentScore();
      if (!score)
            return false;

      int currentTick = qBound(0,
                               scenePosToTick(_lastMousePos),
                               _ticks);

      Fraction pos = Fraction::fromTicks(currentTick);
      Measure* m = score->tick2measure(pos);

      if (!m)
            return false;

      Fraction timeSig = m->timesig();
      int noteWithBeat = timeSig.denominator();

      // Number of smaller pieces the beat is divided into
      int subbeats = _tuplet * (1 << _subdiv);
      int divisions = noteWithBeat * subbeats;

      double dragToTick = scenePosToTick(_lastMousePos);
      double startTick = scenePosToTick(_mouseDownPos);

      Fraction dragOffsetTicks =
            Fraction::fromTicks(dragToTick - startTick);

      int dragToPitch = scenePosToPitch(_lastMousePos);
      int startPitch = scenePosToPitch(_mouseDownPos);

      if (dragToPitch < 0 || startPitch < 0)
            return false;

      if (_dragStyle == DragStyle::NOTE_POSITION) {
            Fraction mouseStartGrid =
                  roundToNearestBeat(
                        scenePosToTick(_mouseDownPos),
                        true);

            Fraction mouseCurrentGrid =
                  roundToNearestBeat(
                        scenePosToTick(_lastMousePos),
                        true);

            pasteTickOffset =
                  mouseCurrentGrid - mouseStartGrid;

            pitchOffset =
                  dragToPitch - startPitch;
            }
      else if (_dragStyle == DragStyle::NOTE_LENGTH_END
               || _dragStyle == DragStyle::NOTE_LENGTH_START) {
            const qint64 scaledNumerator =
                  dragOffsetTicks.numerator() * divisions;

            const qint64 denominator =
                  dragOffsetTicks.denominator();

            qint64 alignedDivisions =
                  scaledNumerator / denominator;

            if (scaledNumerator % denominator) {
                  if (_dragStyle == DragStyle::NOTE_LENGTH_END) {
                        // The end of a note spills forward to the next
                        // grid boundary, just like drawing a new note
                        if (scaledNumerator > 0)
                              ++alignedDivisions;
                        }
                  else {
                        // The start of a note spills backward to the previous
                        // grid boundary, just like drawing a new note
                        if (scaledNumerator < 0)
                              --alignedDivisions;
                        }
                  }

            const Fraction alignedDragOffset(
                  alignedDivisions,
                  divisions);

            if (_dragStyle == DragStyle::NOTE_LENGTH_END) {
                  pasteLengthOffset =
                        alignedDragOffset;
                  }
            else {
                  pasteTickOffset =
                        alignedDragOffset;

                  pasteLengthOffset =
                        Fraction{} - alignedDragOffset;
                  }
            }

      return true;
      }

//---------------------------------------------------------
//   paintOnsetDragSegment
//---------------------------------------------------------

bool PianoView::paintOnsetDragSegment(const QPointF& from,
                                      const QPointF& to)
      {
      Q_UNUSED(from);

      if (!_staff)
            return false;

      Score* score = currentScore();

      const int pitch = scenePosToPitch(_mouseDownPos);
      if (!pitchIsValid(pitch))
            return false;

      bool changed = false;

      // Recalculate the complete set of grid boundaries which the
      // current gesture should own. This makes pulling the mouse
      // backward naturally contract the painted onset range:
      const QVector<Fraction> ticks =
            onsetPaintTicks(_mouseDownPos, to);

      QHash<int, Fraction> desiredTicks;

      for (const Fraction& tick : ticks) {
            if (tick < Fraction{}
                || tick > Fraction::fromTicks(_ticks)) {
                  continue;
                  }

            desiredTicks.insert(tick.ticks(), tick);
            }

      // Remove notes which this gesture previously created,
      // though no longer inside its current extent
      QList<int> ticksToRemove;

      for (auto it = _onsetPaint.notes.constBegin();
           it != _onsetPaint.notes.constEnd();
           ++it) {
            if (!desiredTicks.contains(it.key()))
                  ticksToRemove.append(it.key());
            }

      for (int tickValue : ticksToRemove) {
            const QVector<Note*> notes =
                  _onsetPaint.notes.value(tickValue);

            if (!notes.isEmpty()) {
                  score->startCmd();

                  for (Note* note : notes) {
                        if (note)
                              score->deleteItem(note);
                        }

                  score->endCmd();

                  changed = true;
                  }

            _onsetPaint.notes.remove(tickValue);
            }

      // Add any newly-covered grid boundaries this gesture
      // does not already own
      for (const Fraction& tick : ticks) {
            if (tick < Fraction{}
                || tick > Fraction::fromTicks(_ticks)) {
                  continue;
                  }

            const int tickValue = tick.ticks();

            if (_onsetPaint.notes.contains(tickValue))
                  continue;

            const Fraction duration = gridLengthAt(tick);
            if (duration <= Fraction(0, 1))
                  continue;

            const int voice =
                  insertionVoiceForNote(
                        tick,
                        duration,
                        pitch,
                        _staff->idx(),
                        _editNoteVoice);

            const int track = staff2track(_staff->idx()) + voice;

            Measure* measure = score->tick2measure(tick);
            if (!measure)
                  continue;

            QVector<Note*> added;

            score->startCmd();

            ChordRest* cr =
                  findOrExpandChordRest(
                        measure,
                        tick,
                        track);

            if (cr) {
                  added = addNote(
                              tick,
                              duration,
                              pitch,
                              track);
                  }

            score->endCmd();

            // Record the tick even when nothing was added. An empty
            // vector means this gesture encountered the grid position
            // but did not create anything there, so pullback must not
            // delete any pre-existing score material
            _onsetPaint.notes.insert(tickValue, added);

            if (!added.isEmpty())
                  changed = true;
            }

      return changed;
      }

//---------------------------------------------------------
//   drawPitchText
//---------------------------------------------------------

void PianoView::drawPitchText(QPainter* p,
                              const QRectF& bounds,
                              const QString& name,
                              const QColor& noteColor)
      {
      if (!preferences.getBool(PREF_UI_PIANOROLL_SHOW_PITCH_TEXT))
            return;

      const qreal pitchThickness =
            _orientation == PianoRollOrientation::HORIZONTAL
            ? bounds.height()
            : bounds.width();

      const qreal timeLength =
            _orientation == PianoRollOrientation::HORIZONTAL
            ? bounds.width()
            : bounds.height();

      if (pitchThickness < 12.0 || timeLength < 20.0)
            return;

      const int fontSize =
            qBound(8,
                   qRound(pitchThickness * 0.65),
                   28);

      QFont f("FreeSans");
      f.setPixelSize(fontSize);
      p->setFont(f);

      const qreal textInset =
            qMax<qreal>(2.0, fontSize * 0.15);

      const qreal shadowOffset =
            qMax<qreal>(1.0, fontSize * 0.08);

      QRectF textRect;
      Qt::Alignment textAlign;

      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            textRect = QRectF(
                  bounds.x() + textInset,
                  bounds.y(),
                  bounds.width() - textInset * 2.0,
                  bounds.height());

            textAlign = Qt::Alignment(
                  Qt::AlignLeft | Qt::AlignTop);
            }
      else {
            textRect = QRectF(
                  bounds.x(),
                  bounds.y() + textInset,
                  bounds.width(),
                  bounds.height() - textInset * 2.0);

            textAlign = Qt::Alignment(
                  Qt::AlignHCenter | Qt::AlignBottom);
            }

      QRectF textHiliteRect = textRect.translated(shadowOffset, shadowOffset);

      p->setPen(QPen(noteColor.lighter(130)));
      p->drawText(textHiliteRect, textAlign, name);

      p->setPen(QPen(noteColor.darker(180)));
      p->drawText(textRect, textAlign, name);
      }

//---------------------------------------------------------
//   pitchNameForMidi
//---------------------------------------------------------

QString PianoView::pitchNameForMidi(int pitch) const
      {
      static const char* names[] = {
            "C", "C#", "D", "D#", "E", "F",
            "F#", "G", "G#", "A", "A#", "B"
            };

      if (!pitchIsValid(pitch))
            return QString();

      return QString("%1%2")
            .arg(names[pitch % 12])
            .arg(pitch / 12 - 1);
      }

//---------------------------------------------------------
//   updateTrackingPos
//---------------------------------------------------------

void PianoView::updateTrackingPos(const QPoint& viewportPos)
      {
      QPointF p = mapToScene(viewportPos);

      int pitch = scenePosToPitch(p);
      if (pitch >= 0)
            emit pitchChanged(pitch);

      int tick = scenePosToTick(p);

      if (tick < 0 || tick > _ticks) {
            tick = qBound(0, tick, _ticks);
            _trackingPos.setTick(tick);
            _trackingPos.setInvalid();
            }
      else {
            _trackingPos.setTick(tick);
            }

      emit trackingPosChanged(_trackingPos);
      }

//---------------------------------------------------------
//   viewportReferenceTick
//---------------------------------------------------------

int PianoView::viewportReferenceTick() const
      {
      QRectF viewRect = mapToScene(viewport()->rect()).boundingRect();

      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            qreal x = viewRect.center().x();
            return qBound(0, pixelXToTick(int(x)), _ticks);
            }

      // In vertical mode, the meaningful reference is the
      // activation boundary at the bottom of the viewport
      qreal y = viewRect.bottom();
      return qBound(0, pixelYToTick(int(y)), _ticks);
      }

//---------------------------------------------------------
//   positionViewportAtTick
//---------------------------------------------------------

void PianoView::positionViewportAtTick(int tick)
      {
      tick = qBound(0, tick, _ticks);

      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            int x = tickToPixelX(tick);
            horizontalScrollBar()->setValue(
                  qMax(0, x - viewport()->width() / 2));
            }
      else {
            int y = tickToPixelY(tick);

            // Put the requested tick at the bottom activation edge:
            verticalScrollBar()->setValue(
                  qMax(0, y - viewport()->height()));
            }
      }

//---------------------------------------------------------
//   keyboardAlignedPitchLane
//---------------------------------------------------------

QRectF PianoView::keyboardAlignedPitchLane(int midiPitch) const
      {
      Interval transp;
      if (_staff)
            transp = _staff->part()->instrument()->transpose();

      int instrPitch = midiPitch - transp.chromatic;
      int octave = instrPitch / 12;
      int degree = instrPitch % 12;

      if (degree < 0) {
            degree += 12;
            --octave;
            }

      const qreal whiteKeyWidth =
            pianoRollWhiteKeyWidth(_noteHeight);

      // Exact keyboard alignment uses * 1.0
      // Possible experiment: attenuate to ~0.85-0.90 to give
      // neighboring white lanes more room while keeping black lanes centered:
      const qreal blackKeyWidth =
            _noteHeight * 1.00;

      const qreal octaveLeft =
            (octave * 12 + transp.chromatic)
                  * _noteHeight;

      // Black pitch lanes exactly match the black keys:
      const int blackKey = pianoRollBlackKeyIndex(degree);

      if (blackKey >= 0) {
            const qreal center =
                  octaveLeft + pianoRollBlackKeyBoundary(blackKey)
                        * whiteKeyWidth;

            return QRectF(center - blackKeyWidth / 2.0, 0.0,
                          blackKeyWidth, 0.0);
            }

      // White pitch lanes occupy the remaining regions
      // between adjacent black-key lanes / octave edges
      const int whiteKey = pianoRollWhiteKeyIndex(degree);

      if (whiteKey < 0)
            return QRectF();

      qreal left =
            octaveLeft + whiteKey * whiteKeyWidth;

      qreal right =
            left + whiteKeyWidth;

      if (pianoRollHasBlackKeyBoundary(whiteKey))
            left += blackKeyWidth / 2.0;

      if (pianoRollHasBlackKeyBoundary(whiteKey + 1))
            right -= blackKeyWidth / 2.0;

      return QRectF(left, 0.0, right - left, 0.0);
      }

//---------------------------------------------------------
//   zoomView
//---------------------------------------------------------

void PianoView::zoomView(int step, bool horizontal, int centerX, int centerY)
      {
      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            // Original PRE behavior

            if (horizontal) {
                  // Time zoom along X-axis
                  QRectF viewRect =
                        mapToScene(viewport()->geometry()).boundingRect();

                  int mouseTick =
                        pixelXToTick(centerX + int(viewRect.x()));

                  _xZoom *= pow(X_ZOOM_RATIO, step);
                  emit xZoomChanged(_xZoom);

                  updateBoundingSize();
                  int mousePixX = tickToPixelX(mouseTick);
                  horizontalScrollBar()->setValue(mousePixX - centerX);
                  }
            else {
                  // Pitch zoom along Y-axis
                  QRectF viewRect =
                        mapToScene(viewport()->geometry()).boundingRect();

                  qreal mouseYNote =
                        (centerY + int(viewRect.y())) / qreal(_noteHeight);

                  int oldNoteHeight = _noteHeight;

                  _noteHeight = qMax(
                        qMin(_noteHeight + step, MAX_KEY_HEIGHT),
                        MIN_KEY_HEIGHT);

                  if (_noteHeight == oldNoteHeight)
                        return;

                  emit noteHeightChanged(_noteHeight);

                  updateBoundingSize();

                  int mousePixY =
                        static_cast<int>(mouseYNote * _noteHeight);

                  verticalScrollBar()->setValue(mousePixY - centerY);
                  }

            scene()->update();
            return;
            }

      //
      // Vertical / falling PRE
      //
      if (horizontal) {
            // Physical X is pitch
            // Preserve the pitch under the mouse while changing
            // _noteHeight

            QPointF oldScenePos = mapToScene(QPoint(centerX, centerY));
            int pitch = scenePosToPitch(oldScenePos);

            _noteHeight = qMax(
                  qMin(_noteHeight + step, MAX_KEY_HEIGHT),
                  MIN_KEY_HEIGHT);

            emit noteHeightChanged(_noteHeight);

            updateBoundingSize();

            if (pitch >= 0) {
                  qreal newCenter;

                  if (_verticalPitchLayout
                        == VerticalPitchLayout::KEYBOARD_ALIGNED) {
                        QRectF lane = keyboardAlignedPitchLane(pitch);
                        newCenter = lane.center().x();
                        }
                  else {
                        newCenter = (pitch + 0.5) * _noteHeight;
                        }

                  horizontalScrollBar()->setValue(
                        qMax(int(newCenter - centerX), 0));
                  }
            }
      else {
            // Physical Y is time
            // Preserve the tick under the mouse while changing _xZoom

            QPointF oldScenePos = mapToScene(QPoint(centerX, centerY));
            int mouseTick = scenePosToTick(oldScenePos);

            _xZoom *= pow(X_ZOOM_RATIO, step);
            emit xZoomChanged(_xZoom);

            updateBoundingSize();

            int mousePixY = tickToPixelY(mouseTick);

            verticalScrollBar()->setValue(
                  qMax(mousePixY - centerY, 0));
            }

      scene()->update();
      }

//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------

void PianoView::wheelEvent(QWheelEvent* event)
      {
      const int step = event->angleDelta().y() / 120;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
      QPoint viewportPos = event->position().toPoint();
#else
      QPoint viewportPos = event->pos();
#endif

      if (event->modifiers() == 0) {

            // Vertical scroll
            QGraphicsView::wheelEvent(event);
            }
      else if (event->modifiers() == Qt::ShiftModifier) {
            // Horizontal scroll
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            QWheelEvent we(event->position(),
                           event->globalPosition(),
                           event->pixelDelta().transposed(),
                           event->angleDelta().transposed(),
                           event->buttons(),
                           Qt::NoModifier,
                           Qt::ScrollPhase::NoScrollPhase,
                           false);
#else
            QWheelEvent we(event->pos(),
                           event->delta(),
                           event->buttons(),
                           0,
                           Qt::Horizontal);
#endif
            QGraphicsView::wheelEvent(&we);
            }
      else if (event->modifiers() == Qt::ControlModifier) {
            // Vertical zoom
            zoomView(step, false, viewportPos.x(), viewportPos.y());
            }
      else if (event->modifiers()
               == (Qt::ShiftModifier | Qt::ControlModifier)) {
            // Horizontal zoom
            zoomView(step, true, viewportPos.x(), viewportPos.y());
            }

      updateTrackingPos(viewportPos);
      }

//---------------------------------------------------------
//   showPopupMenu
//---------------------------------------------------------

void PianoView::showPopupMenu(const QPoint& posGlobal)
      {
      QMenu popup(this);

      QAction* act;

      act = new QAction(tr("Cut notes"));
      connect(act, &QAction::triggered, this, &PianoView::cutNotes);
      popup.addAction(act);

      act = new QAction(tr("Copy notes"));
      connect(act, &QAction::triggered, this, &PianoView::copyNotes);
      popup.addAction(act);

      act = new QAction(tr("Paste notes here"));
      connect(act, &QAction::triggered, this, &PianoView::pasteNotesAtCursor);
      popup.addAction(act);

      popup.addAction(getAction("delete"));

      popup.addSeparator();

      act = new QAction(tr("Set Voice 1"));
      connect(act, &QAction::triggered, this, [this](){this->setNotesToVoice(0);});
      popup.addAction(act);

      act = new QAction(tr("Set Voice 2"));
      connect(act, &QAction::triggered, this, [this](){this->setNotesToVoice(1);});
      popup.addAction(act);

      act = new QAction(tr("Set Voice 3"));
      connect(act, &QAction::triggered, this, [this](){this->setNotesToVoice(2);});
      popup.addAction(act);

      act = new QAction(tr("Set Voice 4"));
      connect(act, &QAction::triggered, this, [this](){this->setNotesToVoice(3);});
      popup.addAction(act);

      popup.addSeparator();

      act = new QAction(tr("Color..."));
      connect(act, &QAction::triggered, this, &PianoView::setSelectedNoteColor);
      popup.addAction(act);

      act = new QAction(tr("Reset Color"));
      connect(act, &QAction::triggered, this, &PianoView::resetSelectedNoteColor);
      popup.addAction(act);

      popup.addSeparator();

      QMenu* menuTuplet = new QMenu(tr("Tuplets"));
      for (auto i : { "duplet", "triplet", "quadruplet", "quintuplet", "sextuplet",
           "septuplet", "octuplet", "nonuplet", "tuplet-dialog" })
            menuTuplet->addAction(getAction(i));
      popup.addMenu(menuTuplet);

      popup.exec(posGlobal);
      }

//---------------------------------------------------------
//   contextMenuEvent
//---------------------------------------------------------

void PianoView::contextMenuEvent(QContextMenuEvent *event)
      {
      _popupMenuPos = mapToScene(event->pos());

      showPopupMenu(event->globalPos());
      }

//---------------------------------------------------------
//   eventFilter
//---------------------------------------------------------

bool PianoView::eventFilter(QObject* watched, QEvent* event)
      {
      if (event->type() == QEvent::KeyPress
          || event->type() == QEvent::KeyRelease) {
            QKeyEvent* keyEvent =
                  static_cast<QKeyEvent*>(event);

            Qt::KeyboardModifier modifier =
                  Qt::NoModifier;

            switch (keyEvent->key()) {
                  case Qt::Key_Control:
                        modifier = Qt::ControlModifier;
                        break;

                  case Qt::Key_Shift:
                        modifier = Qt::ShiftModifier;
                        break;

                  case Qt::Key_Alt:
                        modifier = Qt::AltModifier;
                        break;

                  case Qt::Key_Meta:
                        modifier = Qt::MetaModifier;
                        break;

                  default:
                        break;
                  }

            if (modifier != Qt::NoModifier) {
                  if (event->type() == QEvent::KeyPress)
                        _cursorModifiers |= modifier;
                  else
                        _cursorModifiers &= ~modifier;

                  updateCursor();
                  }
            }

      return QGraphicsView::eventFilter(
            watched,
            event);
      }

//---------------------------------------------------------
//   keyReleaseEvent
//---------------------------------------------------------

void PianoView::keyReleaseEvent(QKeyEvent* event) {
      if (_dragStyle == DragStyle::NOTE_POSITION || _dragStyle == DragStyle::SELECTION_RECT) {
            if (event->key() == Qt::Key_Escape) {
                  //Cancel drag
                  _dragStyle = DragStyle::CANCELLED;
                  _dragNoteCache = "";
                  _dragStarted = false;
                  scene()->update();
                  }
            }
      }


//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void PianoView::mousePressEvent(QMouseEvent* event)
      {
      _cursorModifiers = event->modifiers();

      updateCursor();

      if (mscore->currentScoreView())
            mscore->currentScoreView()->setFocus();

      bool rightBn = event->button() == Qt::RightButton;

      if (!rightBn) {
            if (_playbackActive)
                  return;

            _mouseDown = true;
            _mouseDownScreenPos = event->pos();
            _mouseDownPos = mapToScene(event->pos());
            _lastMousePos = _mouseDownPos;
            _cutDrag.lastPos = _mouseDownPos;
            _tieDrag.lastPos = _mouseDownPos;

            _tieDrag.targets.clear();

            _selectionHandledOnPress = false;
            _actionHandledOnPress = false;

            const Qt::KeyboardModifiers modifiers =
                  event->modifiers();
            const bool ctrlPressed =
                  (modifiers & Qt::ControlModifier);
            const bool shiftPressed =
                  (modifiers & Qt::ShiftModifier);

            if (ctrlPressed && (cutTool() || tieTool())) {
                  regroupNoteAt(_mouseDownPos);
                  _actionHandledOnPress = true;
                  }

            if (selectTool() || eventsAdjustTool()) {
                  const bool hasSelectionModifier = ctrlPressed || shiftPressed;

                  PianoItem* pressedItem = pickNote(_mouseDownPos);

                  // A plain press on one member of an existing multi-selection
                  // must not collapse that selection before we know whether the
                  // user intends to drag the group
                  const bool deferExistingMultiSelection =
                        !hasSelectionModifier
                        && pressedItem
                        && pressedItem->note()->selected()
                        && getSelectedItems().size() > 1;

                  if (!deferExistingMultiSelection) {
                        handleSelectionClick();
                        _selectionHandledOnPress = true;
                        }
                  }

            scene()->update();
            }
      }

//---------------------------------------------------------
//   finishDragUndoGroup
//---------------------------------------------------------

void PianoView::finishDragUndoGroup(int& undoStartIdx)
      {
      if (undoStartIdx < 0)
            return;

      Score* score = currentScore();
      const int curUndoIdx =
            score->undoStack()->getCurIdx();

      if (curUndoIdx > undoStartIdx)
            score->undoStack()->mergeCommands(undoStartIdx);

      undoStartIdx = -1;
      }

//---------------------------------------------------------
//   mouseReleaseEvent
//---------------------------------------------------------

void PianoView::mouseReleaseEvent(QMouseEvent* event)
      {
      _cursorModifiers = event->modifiers();

      if (_playbackActive) {
            _mouseDown = false;
            _dragStarted = false;
            return;
            }

      if (_dragStyle == DragStyle::CANCELLED) {
            _dragStyle = DragStyle::NONE;
            _mouseDown = false;
            scene()->update();
            return;
            }

      int modifiers = QGuiApplication::keyboardModifiers();
      bool bnShift = modifiers & Qt::ShiftModifier;
      bool bnCtrl = modifiers & Qt::ControlModifier;

      bool rightBn = event->button() == Qt::RightButton;
      if (rightBn) {
            //Right clicks have been handled as popup menu
            return;
            }

      NoteSelectType selType = bnShift ? (bnCtrl ? NoteSelectType::SUBTRACT : NoteSelectType::XOR)
                                       : (bnCtrl ? NoteSelectType::ADD : NoteSelectType::REPLACE);

      if (_dragStarted) {
            if (_dragStyle == DragStyle::CUT) {
                  const QPointF releasePos = mapToScene(event->pos());

                  if (cutChordDragSegment(_cutDrag.lastPos, releasePos))
                        updateNotes();

                  finishDragUndoGroup(_cutDrag.undoStartIdx);
                  }
            else if (_dragStyle == DragStyle::TIE) {
                  const QPointF releasePos = mapToScene(event->pos());

                  toggleTieDragSegment(_tieDrag.lastPos, releasePos);

                  finishDragUndoGroup(_tieDrag.undoStartIdx);

                  updateNotes();
                  }
            else if (_dragStyle == DragStyle::SELECTION_RECT) {
                  //Update selection
                  qreal minX = qMin(_mouseDownPos.x(), _lastMousePos.x());
                  qreal minY = qMin(_mouseDownPos.y(), _lastMousePos.y());
                  qreal maxX = qMax(_mouseDownPos.x(), _lastMousePos.x());
                  qreal maxY = qMax(_mouseDownPos.y(), _lastMousePos.y());

                  int startTick;
                  int endTick;
                  int lowPitch;
                  int highPitch;

                  if (_orientation == PianoRollOrientation::HORIZONTAL) {
                        startTick = pixelXToTick(int(minX));
                        endTick   = pixelXToTick(int(maxX));

                        lowPitch  = pixelYToPitch(maxY);
                        highPitch = pixelYToPitch(minY);
                        }
                  else {
                        // Time runs vertically and is reversed:
                        // lower screen Y = earlier time
                        startTick = pixelYToTick(int(maxY));
                        endTick   = pixelYToTick(int(minY));

                        // Pitch runs left -> right
                        lowPitch  = scenePosToPitch(QPointF(minX, minY));
                        highPitch = scenePosToPitch(QPointF(maxX, maxY));

                        if (lowPitch < 0 || highPitch < 0)
                              return;
                        }

                  if (startTick > endTick)
                        qSwap(startTick, endTick);

                  if (lowPitch > highPitch)
                        qSwap(lowPitch, highPitch);

                  const NoteSelectType rectSelType =
                        _editNoteTool == PianoRollEditTool::ADD
                              ? NoteSelectType::REPLACE
                              : selType;

                  selectNotes(startTick, endTick, lowPitch, highPitch, rectSelType);
                  }
            else if (_dragStyle == DragStyle::NOTE_POSITION || _dragStyle == DragStyle::NOTE_LENGTH_START
                     || _dragStyle == DragStyle::NOTE_LENGTH_END) {
                  if (_editNoteTool == PianoRollEditTool::SELECT || _editNoteTool == PianoRollEditTool::ADD) {
                        finishNoteGroupDrag(event);

                        // Keep last note drag event, if any
                        if (_inProgressUndoEvent)
                              _inProgressUndoEvent = false;
                        }
                  }
            else if (_dragStyle == DragStyle::EVENT_ONTIME || _dragStyle == DragStyle::EVENT_MOVE
                     || _dragStyle == DragStyle::EVENT_LENGTH) {
                  finishNoteEventAdjustDrag();
                  }
            else if (_dragStyle == DragStyle::DRAW_NOTE) {
                  const int pitch = scenePosToPitch(_mouseDownPos);
                  if (!pitchIsValid(pitch))
                        return;

                  Score* score = currentScore();

                  if (_onsetPaint.undoStartIdx >= 0) {
                        const QPointF releasePos =
                              mapToScene(event->pos());

                        if (paintOnsetDragSegment(
                                    _onsetPaint.lastPos,
                                    releasePos)) {
                              updateNotes();
                              }

                        finishDragUndoGroup(_onsetPaint.undoStartIdx);
                        _onsetPaint.notes.clear();
                        }
                  else {
                        double startTick =
                              scenePosToTick(_mouseDownPos);
                        double endTick =
                              scenePosToTick(_lastMousePos);

                        if (startTick > endTick)
                              std::swap(startTick, endTick);

                        Fraction startTickFrac =
                              roundToNearestBeat(startTick);

                        Fraction endTickFrac =
                              roundToNearestBeat(endTick, false);

                        startTickFrac =
                              clampTickToScore(startTickFrac);

                        endTickFrac =
                              clampTickToScore(endTickFrac);

                        if (endTickFrac != startTickFrac) {
                              Fraction duration =
                                    endTickFrac - startTickFrac;

                              _editNoteLength = duration;
                              _editNoteDots = 0;
                              emit editNoteLengthChanged(duration);

                              const int voice =
                                    insertionVoiceForNote(
                                          startTickFrac,
                                          duration,
                                          pitch,
                                          _staff->idx(),
                                          _editNoteVoice);

                              const int track =
                                    staff2track(_staff->idx()) + voice;

                              Measure* measure =
                                    score->tick2measure(startTickFrac);

                              if (measure) {
                                    score->startCmd();

                                    ChordRest* cr =
                                          findOrExpandChordRest(
                                                measure,
                                                startTickFrac,
                                                track);

                                    if (cr) {
                                          addNote(
                                                startTickFrac,
                                                duration,
                                                pitch,
                                                track);
                                          }

                                    score->endCmd();

                                    updateNotes();
                                    }
                              }
                        }
                  }
            _dragStarted = false;
            }
      else if (!_actionHandledOnPress) {
            // This was just a click, not a drag
            switch (_editNoteTool) {
                  case SELECT:
                  case EVENT_ADJUST:
                        if (!_selectionHandledOnPress)
                              handleSelectionClick();
                        break;
                  case ERASE:
                        eraseNote(_mouseDownPos);
                        break;
                  case ADD:
                        if (bnCtrl)
                              eraseNote(_mouseDownPos);
                        else
                              insertNote(modifiers);
                        break;
                  case CUT:
                        if (bnShift)
                              toggleTie(_mouseDownPos);
                        else
                              cutChord(_mouseDownPos);
                        break;
                  case TIE:
                        toggleTie(_mouseDownPos);
                        break;
                  default:
                        break;
                  }

            }

      _selectionHandledOnPress = false;
      _actionHandledOnPress = false;
      _dragStyle = DragStyle::NONE;
      _mouseDown = false;

      _cutDrag.undoStartIdx = -1;
      _tieDrag.undoStartIdx = -1;
      _tieDrag.targets.clear();

      updateCursor();
      scene()->update();
      }


//---------------------------------------------------------
//   finishNoteEventAdjustDrag
//---------------------------------------------------------

void PianoView::finishNoteEventAdjustDrag()
      {
      Score* score = currentScore();

      Fraction tickDelta = Fraction::fromTicks(scenePosToTick(_lastMousePos) - scenePosToTick(_mouseDownPos));

      score->startCmd();

      for (int i = 0; i < _noteList.size(); ++i) {
            PianoItem* pi = _noteList[i];
            if (pi->note()->selected()) {
                  for (NoteEvent& e : pi->note()->playEvents()) {
                        Chord* chord = pi->note()->chord();
                        Fraction ticks = chord->ticks();
                        Tuplet* tup = chord->tuplet();
                        if (tup) {
                              Fraction frac = tup->ratio();
                              ticks = ticks * frac.inverse();
                              }

                        Fraction start = pi->note()->chord()->tick();
                        Fraction tieLen = pi->note()->playTicksFraction() - ticks;

                        Fraction startAdj =
                              start + ticks * e.ontime() / 1000;

                        Fraction lenAdj =
                              ticks * e.len() / 1000
                              + tieLen;

                        //Calc start, duration of where we dragged to
                        Fraction startNew;
                        Fraction lenNew;
                        switch (_dragStyle) {
                              case DragStyle::EVENT_ONTIME:
                                    startNew = startAdj + tickDelta;
                                    lenNew = lenAdj - tickDelta;
                                    break;
                              case DragStyle::EVENT_MOVE:
                                    startNew = startAdj + tickDelta;
                                    lenNew = lenAdj;
                                    break;
                              default:
                              case DragStyle::EVENT_LENGTH:
                                    startNew = startAdj;
                                    lenNew = lenAdj + tickDelta;
                                    break;
                              }

                        int evtOntimeNew = int(((startNew - start) / ticks).toDouble() * 1000);
                        int evtLenNew =
                              int(((lenNew - tieLen) / ticks).toDouble() * 1000);
                        if (evtLenNew < 1) {
                              evtLenNew = 1;
                              }

                        NoteEvent ne = e;
                        ne.setOntime(evtOntimeNew);
                        ne.setLen(evtLenNew);

                        score->undo(new ChangeNoteEvent(pi->note(), &e, ne));
                        }
                  }
            }

      score->endCmd();

      _levelEventPreviews.clear();
      _levelPreviewActive = false;

      rebuildNoteTimeBuckets();

      update();
      emit noteEventsChanged();
      }

//---------------------------------------------------------
//   effectiveCursorMode
//---------------------------------------------------------

PianoRollCursorMode PianoView::effectiveCursorMode() const
      {
      // Resize intent is established on mouse press prior to
      // promoting the gesture into an active drag via movement
      if (_mouseDown) {
            switch (_dragStyle) {
                  case DragStyle::NOTE_LENGTH_START:
                  case DragStyle::NOTE_LENGTH_END:
                  case DragStyle::EVENT_ONTIME:
                  case DragStyle::EVENT_LENGTH:
                        return PianoRollCursorMode::RESIZE;

                  default:
                        break;
                  }
            }

      // Once a drag has begun, the gesture's established drag style
      // takes precedence over modifiers subsequently being changed:
      if (_dragStarted) {
            switch (_dragStyle) {
                  case DragStyle::DRAW_NOTE:
                        return PianoRollCursorMode::ADD;

                  case DragStyle::ERASE:
                        return PianoRollCursorMode::ERASE;

                  case DragStyle::CUT:
                        return PianoRollCursorMode::CUT;

                  case DragStyle::TIE:
                        return PianoRollCursorMode::TIE;

                  case DragStyle::SELECTION_RECT:
                        return PianoRollCursorMode::SELECTION_RECT;

                  case DragStyle::NOTE_POSITION:
                        return PianoRollCursorMode::MOVE;

                  case DragStyle::EVENT_MOVE:
                  case DragStyle::NOTE_LENGTH_START:
                  case DragStyle::NOTE_LENGTH_END:
                  case DragStyle::EVENT_ONTIME:
                  case DragStyle::EVENT_LENGTH:
                        return PianoRollCursorMode::RESIZE;

                  default:
                        break;
                  }
            }

      const bool ctrlPressed = _cursorModifiers & Qt::ControlModifier;
      const bool shiftPressed = _cursorModifiers & Qt::ShiftModifier;

      switch (_editNoteTool) {
            case PianoRollEditTool::ADD:
                  // Match the precedence in mouseMoveEvent():
                  // Ctrl+Add is Erase before Shift+Add is considered
                  if (ctrlPressed)
                        return PianoRollCursorMode::ERASE;

                  if (shiftPressed)
                        return PianoRollCursorMode::SELECTION_RECT;

                  return PianoRollCursorMode::ADD;

            case PianoRollEditTool::ERASE:
                  return PianoRollCursorMode::ERASE;

            case PianoRollEditTool::CUT:
                  if (ctrlPressed)
                        return PianoRollCursorMode::CONSOLIDATE_TIES;

                  if (shiftPressed)
                        return PianoRollCursorMode::TIE;

                  return PianoRollCursorMode::CUT;

            case PianoRollEditTool::TIE:
                  if (ctrlPressed)
                        return PianoRollCursorMode::CONSOLIDATE_TIES;

                  return PianoRollCursorMode::TIE;

            case PianoRollEditTool::EVENT_ADJUST:
                  return PianoRollCursorMode::EVENT_ADJUST;

            case PianoRollEditTool::SELECT:
            default:
                  return PianoRollCursorMode::SELECT;
            }
      }

//---------------------------------------------------------
//   updateCursor
//---------------------------------------------------------

void PianoView::updateCursor()
      {
      const PianoRollCursorMode cursorMode =
            effectiveCursorMode();

      // Preserve note-edge resize cursors for modes which actually
      // support note/event resizing
      if (cursorMode == PianoRollCursorMode::SELECT
          || cursorMode == PianoRollCursorMode::ADD
          || cursorMode == PianoRollCursorMode::EVENT_ADJUST) {

            const QPointF pos = (_mouseDown && !_dragStarted)
                  ? _mouseDownPos
                  : _lastMousePos;

            const int pitch = scenePosToPitch(pos);

            if (pitch >= 0) {
                  PianoItem* pi = pickNote(pos);

                  if (pi) {
                        const bool eventAdjust =
                              cursorMode == PianoRollCursorMode::EVENT_ADJUST;

                        const QRect bounds =
                              boundingRect(pi->note(), eventAdjust);

                        if (!useOnsetDiamond(pi->note())
                            && bounds.contains(pos.x(), pos.y())) {
                              if (isHorizontal()) {
                                    if (pos.x() <= bounds.left() + _dragNoteLengthMargin
                                        || pos.x() >= bounds.right() - _dragNoteLengthMargin) {
                                          setCursor(Qt::SizeHorCursor);
                                          return;
                                          }
                                    }
                              else {
                                    if (pos.y() <= bounds.top() + _dragNoteLengthMargin
                                        || pos.y() >= bounds.bottom() - _dragNoteLengthMargin) {
                                          setCursor(Qt::SizeVerCursor);
                                          return;
                                          }
                                    }
                              }
                        }
                  }
            }

      // Placeholder cursors
      // These are deliberately temporary. Later, this switch
      // becomes the one place where customcursor pixmaps are
      // installed
      switch (cursorMode) {
            case PianoRollCursorMode::SELECT:
                  setCursor(Qt::ArrowCursor);
                  break;

            case PianoRollCursorMode::SELECTION_RECT:
                  setCursor(Qt::CrossCursor);
                  break;

            case PianoRollCursorMode::ADD:
                  setCursor(_addNoteCursor);
                  break;

            case PianoRollCursorMode::ERASE:
                  setCursor(_eraseNoteCursor);
                  break;

            case PianoRollCursorMode::CUT:
                  setCursor(_scissorsNoteCursor);
                  break;

            case PianoRollCursorMode::TIE:
                  setCursor(_tieNoteCursor);
                  break;

            case PianoRollCursorMode::CONSOLIDATE_TIES:
                  setCursor(_tieConsolidateNoteCursor);
                  break;

            case PianoRollCursorMode::EVENT_ADJUST:
                  setCursor(Qt::ArrowCursor);
                  break;

            case PianoRollCursorMode::RESIZE:
                  if (isHorizontal())
                        setCursor(Qt::SizeHorCursor);
                  else
                        setCursor(Qt::SizeVerCursor);
                  break;

            case PianoRollCursorMode::MOVE:
                  setCursor(Qt::SizeAllCursor);
                  break;
            }
      }

//---------------------------------------------------------
//   mouseMoveEvent
//---------------------------------------------------------

void PianoView::mouseMoveEvent(QMouseEvent* event)
      {
      if (_playbackActive)
            return;

      if (_dragStyle == DragStyle::CANCELLED)
            return;

      _lastMouseScreenPos = event->pos();
      _lastMousePos = mapToScene(event->pos());

      _cursorModifiers = event->modifiers();

      if (_mouseDown && !_dragStarted && !_actionHandledOnPress) {
            qreal dx = _lastMousePos.x() - _mouseDownPos.x();
            qreal dy = _lastMousePos.y() - _mouseDownPos.y();

            if (dx * dx + dy * dy >= MIN_DRAG_DIST_SQ) {
                  //Start dragging
                  _dragStarted = true;

                  if (event->buttons() & Qt::MiddleButton) {
                        _dragStyle = DragStyle::MOVE_VIEWPORT;

                        QRectF rect = mapToScene(viewport()->geometry()).boundingRect();
                        _viewportFocus = rect.center();
                        }
                  else {
                        //Check for move note
                        int tick = scenePosToTick(_mouseDownPos);
                        int mouseDownPitch = scenePosToPitch(_mouseDownPos);

                        if (mouseDownPitch < 0)
                              return;

                        if (_editNoteTool == PianoRollEditTool::ADD
                            && (event->modifiers() & Qt::ControlModifier)) {
                              // Ctrl temporarily turns ADD into the Erase tool for
                              // the duration of this drag gesture
                              _dragStyle = DragStyle::ERASE;

                              // include the point where the gesture began
                              eraseNote(_mouseDownPos);
                              scene()->update();
                              }
                        else if (_editNoteTool == PianoRollEditTool::ADD
                                 && (event->modifiers() & Qt::ShiftModifier)) {
                              // Shift temporarily turns ADD into rectangular selection
                              _dragStyle = DragStyle::SELECTION_RECT;
                              }
                        else if (_editNoteTool == PianoRollEditTool::CUT) {
                              PianoItem* pi = pickNote(_mouseDownPos);

                              if (pi && useOnsetDiamond(pi->note())) {
                                    _dragStyle = DragStyle::NONE;
                                    }
                              else if (event->modifiers() & Qt::ShiftModifier) {
                                    // Shift+Cut uses the tie drag gesture
                                    _dragStyle = DragStyle::TIE;
                                    _tieDrag.targets.clear();
                                    _tieDrag.lastPos = _mouseDownPos;
                                    _tieDrag.undoStartIdx =
                                          currentScore()->undoStack()->getCurIdx();
                                    }
                              else {
                                    _dragStyle = DragStyle::CUT;
                                    _cutDrag.lastPos = _mouseDownPos;
                                    _cutDrag.undoStartIdx =
                                          currentScore()->undoStack()->getCurIdx();
                                    }
                              }
                        else if (_editNoteTool == PianoRollEditTool::TIE) {
                              _dragStyle = DragStyle::TIE;

                              _tieDrag.targets.clear();
                              _tieDrag.lastPos = _mouseDownPos;

                              _tieDrag.undoStartIdx =
                                    currentScore()->undoStack()->getCurIdx();
                              }
                        else {
                              PianoItem* pi = pickNote(_mouseDownPos);
                              if (pi && (_editNoteTool == PianoRollEditTool::SELECT || _editNoteTool == PianoRollEditTool::ADD)) {
                                    if (!pi->note()->selected()) {
                                          selectNotes(tick, tick, mouseDownPitch, mouseDownPitch, NoteSelectType::REPLACE);

                                          // Selection updating may rebuild _noteList
                                          pi = pickNote(_mouseDownPos);
                                          if (!pi) {
                                                _dragStyle = DragStyle::NONE;
                                                return;
                                                }
                                          }

                                    QRect bounds = boundingRect(pi->note(), false);

                                    if (useOnsetDiamond(pi->note())) {
                                          // Drum diamonds represent onset only. They can move in
                                          // time/pitch, but they have no duration handles
                                          _dragStyle = DragStyle::NOTE_POSITION;
                                          }
                                    else if (_orientation == PianoRollOrientation::HORIZONTAL) {
                                          if (_mouseDownPos.x() <= bounds.left() + _dragNoteLengthMargin)
                                                _dragStyle = DragStyle::NOTE_LENGTH_START;
                                          else if (_mouseDownPos.x() >= bounds.right() - _dragNoteLengthMargin)
                                                _dragStyle = DragStyle::NOTE_LENGTH_END;
                                          else
                                                _dragStyle = DragStyle::NOTE_POSITION;
                                          }
                                    else {
                                          if (_mouseDownPos.y() >= bounds.bottom() - _dragNoteLengthMargin)
                                                _dragStyle = DragStyle::NOTE_LENGTH_START;
                                          else if (_mouseDownPos.y() <= bounds.top() + _dragNoteLengthMargin)
                                                _dragStyle = DragStyle::NOTE_LENGTH_END;
                                          else
                                                _dragStyle = DragStyle::NOTE_POSITION;
                                          }

                                    _dragStartPitch = mouseDownPitch;
                                    _dragStartTick = pi->note()->tick();
                                    _dragEndTick = _dragStartTick + pi->note()->chord()->ticks();
                                    _dragNoteCache = serializeSelectedNotes();
                                    }
                              else if (pi && _editNoteTool == PianoRollEditTool::EVENT_ADJUST) {
                                    if (!pi->note()->selected()) {
                                          selectNotes(tick, tick, mouseDownPitch, mouseDownPitch, NoteSelectType::REPLACE);

                                          // selectNotes() can cause PianoRollEditor::updateAll,
                                          // which rebuilds _noteList and invalidates the PianoItem
                                          // returned by pickNote() above
                                          pi = pickNote(_mouseDownPos);
                                          if (!pi) {
                                                _dragStyle = DragStyle::NONE;
                                                return;
                                                }
                                          }

                                    QRect bounds = boundingRect(pi->note(), true);

                                    if (useOnsetDiamond(pi->note())) {
                                          // A diamond has no displayed playback length. Dragging it in
                                          // EVENT_ADJUST moves the event on-time and therefore moves the
                                          // diamond center
                                          _dragStyle = DragStyle::EVENT_MOVE;
                                          }
                                    else if (_orientation == PianoRollOrientation::HORIZONTAL) {
                                          const bool nearOntime =
                                                _mouseDownPos.x()
                                                <= bounds.left() + _dragNoteLengthMargin;

                                          const bool nearLength =
                                                _mouseDownPos.x()
                                                >= bounds.right() - _dragNoteLengthMargin;

                                          if (nearLength
                                              && (!nearOntime
                                                  || qAbs(_mouseDownPos.x() - bounds.right())
                                                     <= qAbs(_mouseDownPos.x() - bounds.left()))) {
                                                _dragStyle = DragStyle::EVENT_LENGTH;
                                                }
                                          else if (nearOntime) {
                                                _dragStyle = DragStyle::EVENT_ONTIME;
                                                }
                                          else {
                                                _dragStyle = DragStyle::EVENT_MOVE;
                                                }
                                          }
                                    else {
                                          // Vertical:
                                          // bottom = event on-time
                                          // top    = event end / length
                                          const bool nearOntime =
                                                _mouseDownPos.y()
                                                >= bounds.bottom() - _dragNoteLengthMargin;

                                          const bool nearLength =
                                                _mouseDownPos.y()
                                                <= bounds.top() + _dragNoteLengthMargin;

                                          if (nearLength
                                              && (!nearOntime
                                                  || qAbs(_mouseDownPos.y() - bounds.top())
                                                     <= qAbs(_mouseDownPos.y() - bounds.bottom()))) {
                                                _dragStyle = DragStyle::EVENT_LENGTH;
                                                }
                                          else if (nearOntime) {
                                                _dragStyle = DragStyle::EVENT_ONTIME;
                                                }
                                          else {
                                                _dragStyle = DragStyle::EVENT_MOVE;
                                                }
                                          }
                                    }
                              else if (!pi && selectionRectAllowed()) {
                                    _dragStyle = DragStyle::SELECTION_RECT;
                                    }
                              else if (!pi && _editNoteTool == PianoRollEditTool::ADD) {
                                    _dragStyle = DragStyle::DRAW_NOTE;

                                    const Fraction tick =
                                          roundToNearestBeat(
                                                scenePosToTick(_mouseDownPos),
                                                true);

                                    if (useOnsetDiamond(_staff, tick)) {
                                          _onsetPaint.notes.clear();
                                          _onsetPaint.lastPos = _mouseDownPos;
                                          _onsetPaint.undoStartIdx =
                                                currentScore()->undoStack()->getCurIdx();
                                          }
                                    }
                              else
                                    _dragStyle = DragStyle::NONE;
                              }
                        }
                  }
            }

      updateCursor();

      if (_dragStarted) {
            if (_dragStyle == DragStyle::MOVE_VIEWPORT) {
                  qreal dx = _lastMouseScreenPos.x() - _mouseDownScreenPos.x();
                  qreal dy = _lastMouseScreenPos.y() - _mouseDownScreenPos.y();

                  QRectF rect = mapToScene(viewport()->geometry()).boundingRect();
                  qreal px = _viewportFocus.x() - dx;
                  qreal py = _viewportFocus.y() - dy;
                  horizontalScrollBar()->setValue(qMax(px - rect.width() / 2, 0.0));
                  verticalScrollBar()->setValue(qMax(py - rect.height() / 2, 0.0));
                  }
            else {
                  if (_dragStyle == DragStyle::CUT) {
                        if (cutChordDragSegment(_cutDrag.lastPos, _lastMousePos)) {
                              updateNotes();
                              }

                        _cutDrag.lastPos = _lastMousePos;
                        scene()->update();
                        }
                  else if (_dragStyle == DragStyle::TIE) {
                        toggleTieDragSegment(
                              _tieDrag.lastPos,
                              _lastMousePos);

                        _tieDrag.lastPos = _lastMousePos;
                        scene()->update();
                        }
                  else if (_dragStyle == DragStyle::ERASE) {
                        eraseNote(_lastMousePos);
                        scene()->update();
                        }
                  else if (_dragStyle == DragStyle::DRAW_NOTE
                           && _onsetPaint.undoStartIdx >= 0) {
                        if (paintOnsetDragSegment(
                                    _onsetPaint.lastPos,
                                    _lastMousePos)) {
                              updateNotes();
                              }

                        _onsetPaint.lastPos = _lastMousePos;
                        scene()->update();
                        }
                  else {
                        switch (_editNoteTool) {
                              case SELECT:
                              case ADD:
                              case EVENT_ADJUST:
                                    scene()->update();
                                    break;

                              case ERASE:
                                    eraseNote(_lastMousePos);
                                    scene()->update();
                                    break;

                              default:
                                    break;
                              }
                        }
                  }
            }

      // Update mouse tracker
      updateTrackingPos(event->pos());
      }


//---------------------------------------------------------
//   dragSelectionNoteGroup
//---------------------------------------------------------

void PianoView::dragSelectionNoteGroup() {
      int curPitch = pixelYToPitch(_lastMousePos.y());
      if (curPitch != _dragStartPitch) {
            int pitchDelta = curPitch - _dragStartPitch;

            Score* score = currentScore();
            if (_inProgressUndoEvent) {
                  _inProgressUndoEvent = false;
                  }

            score->startCmd();
            score->upDownDelta(pitchDelta);
            score->endCmd();

            _inProgressUndoEvent = true;
            _dragStartPitch = curPitch;
            }

      scene()->update();
      }

//---------------------------------------------------------
//   chordRestAt
//    returns a ChordRest that
//    must begin exactly at [tick] on [track]
//---------------------------------------------------------

ChordRest* PianoView::chordRestAt(const Fraction& tick, int track) const
      {
      Score* score = currentScore();
      if (!score)
            return nullptr;

      ChordRest* cr = score->findCR(tick, track);

      return cr && cr->tick() == tick
            ? cr
            : nullptr;
      }

//---------------------------------------------------------
//   findOrExpandChordRest
//---------------------------------------------------------

ChordRest* PianoView::findOrExpandChordRest(Measure* measure,
                                            const Fraction& tick,
                                            int track)
      {
      if (!measure)
            return nullptr;

      Score* score = currentScore();
      ChordRest* cr = score->findCR(tick, track);

      if (cr)
            return cr;

      Segment* seg = measure->undoGetSegment(SegmentType::ChordRest, tick);

      score->expandVoice(seg, track);

      return score->findCR(tick, track);
      }

//---------------------------------------------------------
//   getSegmentNotes
//---------------------------------------------------------

QVector<Note*> PianoView::getSegmentNotes(Segment* seg, int track)
      {
      QVector<Note*> notes;

      ChordRest* cr = seg->cr(track);
      if (cr && cr->isChord()) {
            Chord* chord = toChord(cr);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
            notes.append(QVector<Note*>(chord->notes().begin(), chord->notes().end()));
#else
            notes.append(QVector<Note*>::fromStdVector(chord->notes()));
#endif
            }

      return notes;
      }

//---------------------------------------------------------
//   noteRangeContainsChord
//---------------------------------------------------------

bool PianoView::noteRangeContainsChord(const Fraction& startTick,
                                       const Fraction& duration,
                                       int track) const
      {
      if (!_staff || duration <= Fraction(0, 1))
            return false;

      Score* score = currentScore();
      const Fraction endTick = startTick + duration;

      ChordRest* cr = score->findCR(startTick, track);

      while (cr && cr->tick() < endTick) {
            // findCR() may return a ChordRest beginning before startTick,
            // so make sure it actually overlaps the requested interval:
            if (cr->tick() + cr->actualTicks() > startTick
                && cr->isChord()) {
                  return true;
                  }

            Segment* seg =
                  cr->nextSegmentAfterCR(SegmentType::ChordRest);

            if (!seg)
                  break;

            cr = seg->cr(track);

            // A missing ChordRest in a secondary voice does not
            // represent existing note material, so keep scanning
            // subsequent ChordRest segments:
            while (!cr && seg) {
                  seg = seg->next1(SegmentType::ChordRest);
                  if (seg)
                        cr = seg->cr(track);
                  }
            }

      return false;
      }

//---------------------------------------------------------
//   voiceRangeIsFree
//---------------------------------------------------------

bool PianoView::voiceRangeIsFree(const Fraction& startTick,
                                 const Fraction& duration,
                                 int track) const
      {
      if (!_staff || duration <= Fraction(0, 1))
            return false;

      return !noteRangeContainsChord(
            startTick,
            duration,
            track);
      }

//---------------------------------------------------------
//   voiceHasMatchingChord
//---------------------------------------------------------

bool PianoView::voiceHasMatchingChord(const Fraction& startTick,
                                      const Fraction& duration,
                                      int track) const
      {
      if (!_staff || duration <= Fraction(0, 1))
            return false;

      ChordRest* cr = chordRestAt(startTick, track);

      return cr && cr->isChord() && (cr->actualTicks() == duration);
      }

//---------------------------------------------------------
//   automaticVoiceForNote
//---------------------------------------------------------

int PianoView::automaticVoiceForNote(const Fraction& startTick,
                                     const Fraction& duration,
                                     int pitch,
                                     int staffIdx,
                                     int preferredVoice) const
      {
      if (!_staff)
            return preferredVoice;

      Score* score = currentScore();
      const int staffTrack = staff2track(staffIdx);

      // Determine whether the new note is primarily above or below
      // existing material at the insertion position:
      bool foundExistingPitch = false;
      int lowestPitch = 127;
      int highestPitch = 0;

      for (int voice = 0; voice < VOICES; ++voice) {
            const int track = staffTrack + voice;
            ChordRest* cr = score->findCR(startTick, track);

            if (!cr
                || !cr->isChord()
                || startTick < cr->tick()
                || startTick >= cr->tick() + cr->actualTicks()) {
                  continue;
                  }

            Chord* chord = toChord(cr);

            for (Note* note : chord->notes()) {
                  if (!note)
                        continue;

                  lowestPitch = qMin(lowestPitch, note->pitch());
                  highestPitch = qMax(highestPitch, note->pitch());
                  foundExistingPitch = true;
                  }
            }

      // MuseScore's conventional voice directions are
      // Up: voices 1 & 3
      // Down: voices 2 & 4
      int candidates[VOICES];

      if (foundExistingPitch && pitch < lowestPitch) {
            // New note is below the existing material
            candidates[0] = 1; // voice 2
            candidates[1] = 3; // voice 4
            candidates[2] = 0; // voice 1
            candidates[3] = 2; // voice 3
            }
      else {
            // New note is above, within, or there was nothing useful
            // to compare against
            candidates[0] = 0; // voice 1
            candidates[1] = 2; // voice 3
            candidates[2] = 1; // voice 2
            candidates[3] = 3; // voice 4
            }

      // First preference: join an existing chord whose rhythmic span
      // exactly matches the requested PRE duration. This preserves a
      // voice already expressing the same rhythmic layer instead of
      // unnecessarily consuming another voice
      for (int i = 0; i < VOICES; ++i) {
            const int voice = candidates[i];
            const int track = staffTrack + voice;

            if (voiceHasMatchingChord(
                        startTick,
                        duration,
                        track)) {
                  return voice;
                  }
            }

      // Second preference: use a completely free voice over the
      // requested interval
      for (int i = 0; i < VOICES; ++i) {
            const int voice = candidates[i];
            const int track = staffTrack + voice;

            if (voiceRangeIsFree(
                        startTick,
                        duration,
                        track)) {
                  return voice;
                  }
            }

      // No alternate voice can accept the note intact. Let the
      // existing PRE insertion algorithm handle it in the selected
      // voice using its normal splitting/tie behavior
      return preferredVoice;
      }

//---------------------------------------------------------
//   insertionVoiceForNote
//---------------------------------------------------------

int PianoView::insertionVoiceForNote(const Fraction& startTick,
                                     const Fraction& duration,
                                     int pitch,
                                     int staffIdx,
                                     int preferredVoice) const
      {
      if (!_automaticVoiceAssignment || !_staff)
            return preferredVoice;

      Staff* staff = currentScore()->staff(staffIdx);
      if (!staff || !staff->part())
            return automaticVoiceForNote(
                  startTick,
                  duration,
                  pitch,
                  staffIdx,
                  preferredVoice);

      const Instrument* instrument =
            staff->part()->instrument(startTick);

      if (instrument
          && instrument->useDrumset()
          && instrument->drumset()
          && pitch >= 0
          && pitch < DRUM_INSTRUMENTS
          && instrument->drumset()->isValid(pitch)) {
            const int drumVoice =
                  instrument->drumset()->voice(pitch);

            if (drumVoice >= 0 && drumVoice < VOICES)
                  return drumVoice;
            }

      return automaticVoiceForNote(
            startTick,
            duration,
            pitch,
            staffIdx,
            preferredVoice);
      }

//---------------------------------------------------------
//   addNote
//---------------------------------------------------------

QVector<Note*> PianoView::addNote(Fraction startTick, Fraction duration, int pitch, int track)
      {
      QVector<Note*> addedNotes;
      if (!pitchIsValid(pitch) || duration <= Fraction{})
            return addedNotes;

      Score* score = currentScore();
      const NoteVal newPitch(pitch);

      const Fraction requestedStartTick = startTick;
      const Fraction requestedDuration = duration;

      const bool preserveExistingRhythm =
            noteRangeContainsChord(requestedStartTick,
                                   requestedDuration,
                                   track);

      ChordRest* curCr = score->findCR(startTick, track);
      if (curCr) {
            ChordRest* cr0 = nullptr;
            ChordRest* curChordRest = curCr;

            // Cut first chord/rest if the new note really starts inside it.
            if (startTick > curCr->tick()) {
                  ChordRest* splitStart = nullptr;

                  if (cutChordRest(curCr, track, startTick, cr0, splitStart, true))
                        curChordRest = splitStart;
                  else
                        curChordRest = chordRestAt(startTick, track);
                  }

            if (!curChordRest)
                  return addedNotes;

            if (!preserveExistingRhythm) {
                  // Nothing in the requested interval contains existing note
                  // material whose rhythmic boundaries need to be preserved, so
                  // let setNoteRest() realize the requested PRE duration directly,
                  // as with PRE in 3.6.2
                  Segment* newSeg =
                        score->setNoteRest(
                              curChordRest->segment(),
                              track,
                              newPitch,
                              requestedDuration);

                  if (newSeg)
                        addedNotes.append(
                              getSegmentNotes(newSeg, track));

                  return addedNotes;
                  }

            Fraction curStartTick = curChordRest->tick();
            Fraction curDur = curChordRest->ticks();
            while (startTick + duration >= curStartTick + curDur) {
                  if (curChordRest->isChord()) {
                        Chord* ch = toChord(curChordRest);
                        if (!std::any_of(ch->notes().begin(), ch->notes().end(), [pitch](Note* n) { return n->pitch() == pitch; }))
                              addedNotes.append(score->addNote(ch, newPitch));
                        }
                  else {
                        Segment* newSeg = score->setNoteRest(curChordRest->segment(), track, newPitch, curDur);
                        if (newSeg)
                              addedNotes.append(getSegmentNotes(newSeg, track));
                        }

                  startTick += curDur;
                  duration -= curDur;

                  if (duration <= Fraction(0, 1))
                        break;

                  Segment* seg = curChordRest->nextSegmentAfterCR(SegmentType::ChordRest);
                  if (!seg)
                        break;

                  curChordRest = seg->cr(track);

                  // Secondary voices are not guaranteed to have a ChordRest
                  // at every ChordRest segment. Materialize the missing voice
                  // with rests so the insertion can continue through the gap
                  if (!curChordRest) {
                        score->expandVoice(seg, track);
                        curChordRest = seg->cr(track);
                        }

                  if (!curChordRest)
                        break;

                  curStartTick = curChordRest->tick();
                  curDur = curChordRest->ticks();
                  }

            if (duration > Fraction(0, 1) && curChordRest) {
                  ChordRest* crMid = curChordRest;
                  ChordRest* crEnd = nullptr;

                  const Fraction endTick = startTick + duration;

                  // Split only when endTick actually falls inside curChordRest:
                  if (endTick > curChordRest->tick()
                      && endTick < curChordRest->tick() + curChordRest->actualTicks()) {
                        if (!cutChordRest(
                                  curChordRest,
                                  track,
                                  endTick,
                                  crMid,
                                  crEnd,
                                  true)) {
                              return addedNotes;
                              }
                        }

                  if (!crMid)
                        return addedNotes;

                  if (crMid->isChord()) {
                        Chord* ch = toChord(crMid);

                        if (!std::any_of(
                                  ch->notes().begin(),
                                  ch->notes().end(),
                                  [pitch](Note* n) {
                                        return n->pitch() == pitch;
                                        })) {
                              addedNotes.append(
                                    score->addNote(ch, newPitch));
                              }
                        }
                  else {
                        Segment* newSeg = score->setNoteRest(
                              crMid->segment(),
                              track,
                              newPitch,
                              duration);

                        if (newSeg)
                              addedNotes.append(
                                    getSegmentNotes(newSeg, track));
                        }
                  }
            }

      for (auto note : addedNotes) {
            if (note != addedNotes.last())
                  toggleTie(note);
            }

      return addedNotes;
      }


//---------------------------------------------------------
//   eraseNote
//---------------------------------------------------------

void PianoView::eraseNote(const QPointF& pos)
      {
      Score* score = currentScore();

      PianoItem* pn = pickNote(pos);

      if (!pn)
            return;

      Note* noteStart = pn->note()->firstTiedNote();

      QList<Note*> notesToDelete;
      notesToDelete.append(noteStart);

      for (Note* note = noteStart;
           note->tieFor();
           note = note->tieFor()->endNote()) {
            notesToDelete.append(note->tieFor()->endNote());
            }

      score->startCmd();

      for (Note* note : notesToDelete)
            score->deleteItem(note);

      score->endCmd();
      }

//---------------------------------------------------------
//   changeChordLength
//---------------------------------------------------------

void PianoView::changeChordLength(const QPointF& pos) {
      Score* score = currentScore();
      int pickTick = pixelXToTick((int)pos.x());
      int pickPitch = pixelYToPitch(pos.y());
      PianoItem *pn = pickNote(pickTick, pickPitch);

      if (pn) {
            Note* const note = pn->note();
            const int track = note->track();
            Fraction frac = noteEditLength();
            Chord* chord = note->chord();
            if (chord->ticks() != frac) {
                  //Copy existing cord
                  QList<NoteVal> nvList;
                  for (Note* n: chord->notes())
                        nvList.push_back(n->noteVal());

                  //Rebuild chord
                  score->startCmd();
                  score->deleteItem(chord);
                  Fraction startTick = chord->segment()->tick();

                  for (int i = 0; i < nvList.length(); ++i) {
                        if (i == 0) {
                              ChordRest* cr = chordRestAt(startTick, track);
                              score->setNoteRest(cr->segment(), track, nvList.at(i), frac);
                              chord = toChord(chordRestAt(startTick, track));
                              }
                        else
                              score->addNote(chord, nvList.at(i));
                        }
                  score->endCmd();
                  }
            }
      }

//---------------------------------------------------------
//   gridLengthAt
//---------------------------------------------------------

Fraction PianoView::gridLengthAt(const Fraction& tick) const
      {
      // Move one tick beyond an exact grid boundary so that ceil()
      // gives us the following boundary rather than the same one
      const Fraction next =
            roundToNearestBeat(tick.ticks() + 1, false);

      if (next <= tick)
            return Fraction(0, 1);

      return next - tick;
      }

//---------------------------------------------------------
//   clampTickToScore
//    negative       → 0
//    inside score   → unchanged
//    past score end → _ticks
//---------------------------------------------------------

Fraction PianoView::clampTickToScore(const Fraction& tick) const
      {
      if (tick < Fraction{})
            return Fraction{};

      const Fraction scoreEnd =
            Fraction::fromTicks(_ticks);

      if (tick > scoreEnd)
            return scoreEnd;

      return tick;
      }

//---------------------------------------------------------
//   onsetPaintTicks
//---------------------------------------------------------

QVector<Fraction> PianoView::onsetPaintTicks(const QPointF& from,
                                             const QPointF& to) const
      {
      QVector<Fraction> ticks;

      if (!_staff)
            return ticks;

      int fromTick = scenePosToTick(from);
      int toTick   = scenePosToTick(to);

      if (fromTick == toTick)
            return ticks;

      if (fromTick < toTick) {
            Fraction tick =
                  roundToNearestBeat(fromTick, false);

            while (tick.ticks() <= toTick) {
                  if (tick.ticks() > fromTick)
                        ticks.append(tick);

                  const Fraction length = gridLengthAt(tick);
                  if (length <= Fraction(0, 1))
                        break;

                  tick += length;
                  }
            }
      else {
            Fraction tick =
                  roundToNearestBeat(fromTick, true);

            while (tick.ticks() >= toTick) {
                  if (tick.ticks() < fromTick)
                        ticks.append(tick);

                  // For reverse traversal, obtain the previous
                  // grid boundary rather than advancing forward:
                  const Fraction previous =
                        roundToNearestBeat(tick.ticks() - 1, true);

                  if (previous >= tick)
                        break;

                  tick = previous;
                  }
            }

      return ticks;
      }

//---------------------------------------------------------
//   roundToStartBeat
//---------------------------------------------------------

Fraction PianoView::roundToNearestBeat(int tick, bool down)  const
      {
      Score* score = currentScore();
      Pos barPos(score->tempomap(), score->sigmap(), tick, TType::TICKS);

      int noteWithBeat = barPos.timesig().timesig().denominator();

      //Number of smaller pieces the beat is divided into
      int subbeats = _tuplet * (1 << _subdiv);
      int divisions = noteWithBeat * subbeats;

      //Round down to nearest division
      Fraction pickFrac = Fraction::fromTicks(tick);
      double frac = (pickFrac.numerator() * divisions / (double)pickFrac.denominator());
      int numDiv = (int)(down ? floor(frac) : ceil(frac));
      return Fraction(numDiv, divisions);
      }


//---------------------------------------------------------
//   noteEditLength
//---------------------------------------------------------

Fraction PianoView::noteEditLength() const
      {
      if (_editNoteDots <= 0)
            return _editNoteLength;

      const int denominator = 1 << _editNoteDots;
      const int numerator   = (1 << (_editNoteDots + 1)) - 1;

      return _editNoteLength * Fraction(numerator, denominator);
      }

//---------------------------------------------------------
//   insertNote
//---------------------------------------------------------

void PianoView::insertNote(int modifiers)
      {
      bool bnShift = modifiers & Qt::ShiftModifier;

      Score* score = currentScore();

      int pickTick = scenePosToTick(_mouseDownPos);
      int pickPitch = scenePosToPitch(_mouseDownPos);

      if (pickPitch < 0)
            return;

      if (bnShift) {
            // If shift is held, select the note instead
            PianoItem* pn = pickNote(_mouseDownPos);
            if (pn) {
                  mscore->play(pn->note());
                  score->setPlayNote(false);

                  selectItem(pn, NoteSelectType::REPLACE);
                  }
            else {
                  clearNoteSelection();
                  }
            return;
            }

      Fraction insertPosition = roundToNearestBeat(pickTick);
      Fraction noteLen = noteEditLength();

      const int voice =
            insertionVoiceForNote(
                  insertPosition,
                  noteLen,
                  pickPitch,
                  _staff->idx(),
                  _editNoteVoice);

      const int track = staff2track(_staff->idx()) + voice;

      Measure* measure = score->tick2measure(insertPosition);
      if (!measure)
            return;

      score->startCmd();

      ChordRest* cr =
            findOrExpandChordRest(
                  measure,
                  insertPosition,
                  track);

      if (cr)
            addNote(insertPosition, noteLen, pickPitch, track);

      score->endCmd();
      }


//---------------------------------------------------------
//   toggleTie
//---------------------------------------------------------

void PianoView::toggleTie(const QPointF& pos)
      {
      if (!_staff)
            return;

      Note* note = tieNoteAt(pos);
      if (!note)
            return;

      if (useOnsetDiamond(note))
            return;

      Score* score = currentScore();

      score->startCmd();
      toggleTie(note);
      score->endCmd();

      updateNotes();
      }


//---------------------------------------------------------
//   toggleTie
//---------------------------------------------------------

bool PianoView::toggleTie(Note* note)
      {
      if (!note || !_staff)
            return false;

      // Based on Score::cmdToggleTie()
      Score* score = currentScore();

      Tie* tie = note->tieFor();

      if (tie) {
            score->undoRemoveElement(tie);
            return true;
            }

      Note* note2 = score->findOrCreateTieTarget(note);
      if (!note2)
            return false;

      tie = new Tie(score);
      tie->setStartNote(note);
      tie->setEndNote(note2);
      tie->setTrack(note->track());
      tie->setTick(note->chord()->segment()->tick());
      tie->setTicks(
            note2->chord()->segment()->tick()
            - note->chord()->segment()->tick());

      score->undoAddElement(tie);
      return true;
      }

//---------------------------------------------------------
//   tieNoteAt
//---------------------------------------------------------

Note* PianoView::tieNoteAt(const QPointF& pos)
      {
      if (!_staff)
            return nullptr;

      const int pickTick = scenePosToTick(pos);
      const int pickPitch = scenePosToPitch(pos);

      if (!pitchIsValid(pickPitch))
            return nullptr;

      // Use the visible PianoItem only to determine which track/voice
      // the pointer is hovering over
      PianoItem* item = pickNote(pickTick, pickPitch);
      if (!item || !item->note())
            return nullptr;

      const int track = item->note()->track();
      Score* score = currentScore();

      // Now resolve the actual underlying ChordRest
      // This matters for tied chains because continuation notes are
      // omitted from _noteList and visually represented by the
      // preceding PianoItem
      ChordRest* cr = score->findCR(
            Fraction::fromTicks(pickTick),
            track);

      if (!cr || !cr->isChord())
            return nullptr;

      Chord* chord = toChord(cr);

      // findCR() returns the most recent CR <= pickTick, so make sure
      // the pointer is still inside this chord's duration:
      if (Fraction::fromTicks(pickTick)
            >= chord->tick() + chord->actualTicks())
            return nullptr;

      for (Note* note : chord->notes()) {
            if (note && note->pitch() == pickPitch)
                  return note;
            }

      return nullptr;
      }

//---------------------------------------------------------
//   toggleTieDragSegment
//---------------------------------------------------------

bool PianoView::toggleTieDragSegment(const QPointF& from,
                                     const QPointF& to)
      {
      if (!_staff)
            return false;

      bool changed = false;

      const qreal dx = to.x() - from.x();
      const qreal dy = to.y() - from.y();

      const int steps = qMax(
            1,
            int(ceil(qMax(qAbs(dx), qAbs(dy)))));

      for (int i = 0; i <= steps; ++i) {
            const qreal amount = qreal(i) / steps;

            const QPointF pos(
                  from.x() + dx * amount,
                  from.y() + dy * amount);

            Note* note = tieNoteAt(pos);
            if (!note)
                  continue;

            if (useOnsetDiamond(note)) {
                  continue;
                  }

            const TieDragTarget target {
                  note->chord()->tick(),
                  note->track(),
                  note->pitch()
                  };

            bool alreadyHandled = false;

            for (const TieDragTarget& handled : _tieDrag.targets) {
                  if (handled == target) {
                        alreadyHandled = true;
                        break;
                        }
                  }

            if (alreadyHandled) {
                  continue;
                  }

            // Store before modifying the score so that moving
            // backward across this note cannot toggle it again
            _tieDrag.targets.append(target);

            Score* score = currentScore();

            score->startCmd();
            const bool toggled = toggleTie(note);
            score->endCmd();

            if (toggled) {
                  changed = true;
                  updateNotes();
                  }
            }

      return changed;
      }

//---------------------------------------------------------
//   cutChord
//---------------------------------------------------------

void PianoView::cutChord(const QPointF& pos)
      {
      if (!_staff || _tuplet != 1)
            return;

      Score* score = currentScore();

      const int pickTick = scenePosToTick(pos);
      const int pickPitch = scenePosToPitch(pos);

      if (!pitchIsValid(pickPitch))
            return;

      PianoItem* pn = pickNote(pickTick, pickPitch);

      if (!pn || !pn->note())
            return;

      Note* note = pn->note();

      if (useOnsetDiamond(note))
            return;

      const int track = note->track();

      const Fraction insertPosition = roundToNearestBeat(pickTick);

      score->startCmd();
      const bool changed = cutChordAt(insertPosition, track);
      score->endCmd();

      if (changed)
            updateNotes();
      }

//---------------------------------------------------------
//   cutChordAt
//---------------------------------------------------------

bool PianoView::cutChordAt(const Fraction& insertPosition, int track)
      {
      if (!_staff || _tuplet != 1)
            return false;

      Score* score = currentScore();

      // If a ChordRest already begins exactly here, there is nothing
      // left to split. In Cut mode, treat an incoming tie at this
      // existing boundary as the thing to cut instead
      if (chordRestAt(insertPosition, track))
            return removeTiesAtBoundary(insertPosition, track);

      Segment* seg = score->tick2segment(insertPosition);
      score->expandVoice(seg, track);

      ChordRest* cr = score->findCR(insertPosition, track);

      if (!cr || cr->tuplet())
            return false;

      // expandVoice() may itself have produced an exact boundary
      if (insertPosition == cr->tick())
            return removeTiesAtBoundary(insertPosition, track);

      ChordRest* cr0 = nullptr;
      ChordRest* cr1 = nullptr;

      return cutChordRest(cr, track, insertPosition, cr0, cr1);
      }

//---------------------------------------------------------
//   regroupNoteAt
//---------------------------------------------------------

void PianoView::regroupNoteAt(const QPointF& pos)
      {
      if (!_staff)
            return;

      const int pickTick =
            scenePosToTick(pos);

      const int pickPitch =
            scenePosToPitch(pos);

      if (!pitchIsValid(pickPitch))
            return;

      PianoItem* item =
            pickNote(pickTick, pickPitch);

      if (!item || !item->note())
            return;

      Note* note = item->note();

      Note* first = note->firstTiedNote();
      Note* last = note->lastTiedNote();

      Chord* firstChord = first->chord();
      Chord* lastChord = last->chord();

      if (!firstChord || !lastChord)
            return;

      Score* score = currentScore();
      const int track = note->track();

      if (!score->selectionFilter().canSelectVoice(track))
            return;

      const Fraction startTick =
            firstChord->tick();

      const Fraction endTick =
            lastChord->tick()
            + lastChord->actualTicks();

      score->startCmd();

      score->regroupNotesAndRests(
            startTick,
            endTick,
            track);

      score->endCmd();

      updateNotes();
      }

//---------------------------------------------------------
//   removeTiesAtBoundary
//---------------------------------------------------------

bool PianoView::removeTiesAtBoundary(const Fraction& tick, int track)
      {
      if (!_staff)
            return false;

      Score* score = currentScore();
      ChordRest* cr = chordRestAt(tick, track);

      if (!cr || !cr->isChord())
            return false;

      bool changed = false;
      Chord* chord = toChord(cr);

      // Cut is voice-wide, so remove every tie entering this chord
      // on this track rather than only the pitch under the mouse
      for (Note* note : chord->notes()) {
            if (!note)
                  continue;

            Tie* tie = note->tieBack();
            if (!tie)
                  continue;

            score->undoRemoveElement(tie);
            changed = true;
            }

      return changed;
      }

//---------------------------------------------------------
//   cutChordDragSegment
//---------------------------------------------------------

bool PianoView::cutChordDragSegment(const QPointF& from,
                                    const QPointF& to)
      {
      if (!_staff || _tuplet != 1)
            return false;

      // First collect plain tick/track values.  Do not modify the
      // score while consulting PianoItems, since a cut can rebuild
      // the note representation
      QVector<QPair<Fraction, int>> targets;

      const qreal dx = to.x() - from.x();
      const qreal dy = to.y() - from.y();

      const int steps = qMax(1,
            int(ceil(qMax(qAbs(dx), qAbs(dy)))));

      for (int i = 0; i <= steps; ++i) {
            const qreal amount = qreal(i) / steps;

            const QPointF pos(
                  from.x() + dx * amount,
                  from.y() + dy * amount);

            const int pickTick = scenePosToTick(pos);
            const int pickPitch = scenePosToPitch(pos);

            if (!pitchIsValid(pickPitch))
                  continue;

            const Fraction cutTick = roundToNearestBeat(pickTick);

            PianoItem* pn = pickNote(pickTick, pickPitch);
            if (!pn || !pn->note())
                  continue;

            Note* note = pn->note();

            if (useOnsetDiamond(note))
                  continue;

            const int track = note->track();

            const QPair<Fraction, int> target(cutTick, track);

            if (!targets.isEmpty() && targets.back() == target)
                  continue;

            targets.append(target);
            }

      bool changed = false;
      Score* score = currentScore();

      for (const QPair<Fraction, int>& target : targets) {
            score->startCmd();

            const bool cut =
                  cutChordAt(target.first, target.second);

            score->endCmd();

            if (cut)
                  changed = true;
            }

      return changed;
      }

//---------------------------------------------------------
//   handleSelectionClick
//---------------------------------------------------------

void PianoView::handleSelectionClick()
      {
      int modifiers = QGuiApplication::keyboardModifiers();
      bool bnShift = modifiers & Qt::ShiftModifier;
      bool bnCtrl = modifiers & Qt::ControlModifier;
      NoteSelectType selType = bnShift ? (bnCtrl ? NoteSelectType::SUBTRACT : NoteSelectType::XOR)
                                       : (bnCtrl ? NoteSelectType::ADD : NoteSelectType::REPLACE);

      Score* score = currentScore();

      int pickTick = scenePosToTick(_mouseDownPos);
      int pickPitch = scenePosToPitch(_mouseDownPos);

      if (pickPitch < 0)
            return;

      PianoItem* pn = pickNote(_mouseDownPos);

      if (pn) {
            mscore->play(pn->note());
            score->setPlayNote(false);

            selectItem(pn, selType);
            }
      else {
            if (!bnShift && !bnCtrl) {
                  // An empty direct click clears selection.
                  // Don't route this into duration-based note intersection
                  clearNoteSelection();
                  }
            else if (!bnShift && bnCtrl) {

                  //Insert a new note at nearest subbeat
                  Fraction insertPosition = roundToNearestBeat(pickTick);

                  InputState& is = score->inputState();
                  int voice = _editNoteVoice;
                  int track = staff2track(_staff->idx()) + voice;

                  NoteVal nv(pickPitch);

                  Segment* seg = score->tick2segment(insertPosition);
                  score->expandVoice(seg, track);

                  ChordRest* e = score->findCR(insertPosition, track);
                  if (e && !e->tuplet() && _tuplet == 1) {
                        //Ignore tuplets
                        score->startCmd();

                        ChordRest* cr0;
                        ChordRest* cr1;
                        Fraction frac = is.duration().fraction();

                        //Default to quarter note if faction is invalid
                        if (!frac.isValid() || frac.isZero())
                              frac.set(1, 4);

                        if (cutChordRest(e, track, insertPosition, cr0, cr1, true)) {
                              score->setNoteRest(cr1->segment(), track, nv, frac);
                              }
                        else {
                              if (cr0->isChord() && cr0->ticks().ticks() == frac.ticks()) {
                                    Chord* ch = toChord(cr0);
                                    score->addNote(ch, nv);
                                    }
                              else {
                                    score->setNoteRest(cr0->segment(), track, nv, frac);
                                    }
                              }

                        score->endCmd();
                        }

                  }
            else if (bnShift && !bnCtrl) {
                  //Append a pitch to our current chord/rest
                  int voice = _editNoteVoice;

                  //Find best chord to add to
                  int track = staff2track(_staff->idx()) + voice;

                  Fraction pt = Fraction::fromTicks(pickTick);
                  Segment* seg = score->tick2segment(pt);
                  score->expandVoice(seg, track);

                  ChordRest* e = score->findCR(pt, track);

                  if (e && e->isChord()) {
                        Chord* ch = toChord(e);

                        if (pt >= e->tick() && pt < (ch->tick() + ch->ticks())) {
                              NoteVal nv(pickPitch);
                              score->startCmd();
                              score->addNote(ch, nv);
                              score->endCmd();
                              }

                        }
                  else if (e && e->isRest()) {
                        Rest* r = toRest(e);
                        NoteVal nv(pickPitch);
                        score->startCmd();
                        score->setNoteRest(r->segment(), track, nv, r->ticks());
                        score->endCmd();
                        }
                  }
            else if (bnShift && bnCtrl) {
                  //Cut the chord/rest at the nearest subbeat
                  int voice = _editNoteVoice;

                  //Find best chord to add to
                  int track = staff2track(_staff->idx()) + voice;

                  Fraction insertPosition = roundToNearestBeat(pickTick);

                  Segment* seg = score->tick2segment(insertPosition);
                  score->expandVoice(seg, track);

                  ChordRest* e = score->findCR(insertPosition, track);
                  if (e && !e->tuplet() && _tuplet == 1) {
                        score->startCmd();
                        Fraction startTick = e->tick();

                        if (insertPosition != startTick) {
                              ChordRest* cr0 = nullptr;
                              ChordRest* cr1 = nullptr;
                              cutChordRest(e, track, insertPosition, cr0, cr1);
                              }
                        score->endCmd();
                        }
                  }
            }
      }


//---------------------------------------------------------
//   cutChordRest
//   @cr0 Will be set to the first piece of the split chord, or targetCr if no split occurs
//   @cr1 Will be set to the second piece of the split chord, or nullptr if no split occurs
//   @return true if chord was cut
//---------------------------------------------------------

bool PianoView::cutChordRest(ChordRest* targetCr,
                             int track,
                             Fraction cutTick,
                             ChordRest*& cr0,
                             ChordRest*& cr1,
                             bool preserveOriginalDuration)
      {
      cr0 = targetCr;
      cr1 = nullptr;

      if (!targetCr)
            return false;

      Fraction startTick = targetCr->segment()->tick();
      Fraction durationTuplet = targetCr->ticks();

      Fraction measureToTuplet(1, 1);
      Fraction tupletToMeasure(1, 1);

      if (targetCr->tuplet()) {
            Fraction ratio = targetCr->tuplet()->ratio();
            measureToTuplet = ratio;
            tupletToMeasure = ratio.inverse();
            }

      Fraction durationMeasure = durationTuplet * tupletToMeasure;
      Fraction endTick = startTick + durationMeasure;

      // There is nothing to split unless cutTick is strictly
      // inside this ChordRest:
      if (cutTick <= startTick || cutTick >= endTick)
            return false;

      // Preserve whether this was originally a chord.  targetCr may
      // no longer be valid after setNoteRest() modifies the score
      const bool wasChord = targetCr->isChord();

      // Save the original pitches before modifying the ChordRest:
      QVector<NoteVal> chordNotes;

      QMap<int, QPair<Fraction, Fraction>> preservedTieRanges;
      QMap<int, Fraction> preservedIncomingTieTicks;

      if (wasChord) {
            Chord* chord = toChord(targetCr);

            for (Note* note : chord->notes()) {
                  chordNotes.append(note->noteVal());

                  if (note->tieBack()) {
                        Note* previous =
                              note->tieBack()->startNote();

                        if (previous && previous->chord()) {
                              preservedIncomingTieTicks.insert(
                                    note->pitch(),
                                    previous->chord()->tick());
                              }
                        }

                  Note* first =
                        preserveOriginalDuration
                              ? note->firstTiedNote()
                              : note;

                  Note* last = note->lastTiedNote();

                  const Fraction logicalStart =
                        first->chord()->tick();
                  const Fraction logicalEnd =
                        last->chord()->tick()
                        + last->chord()->actualTicks();

                  preservedTieRanges.insert(
                        note->pitch(),
                        qMakePair(logicalStart, logicalEnd));

                  note->setSelected(false);
                  }
            }
      else if (targetCr->isRest()) {
            toRest(targetCr)->setSelected(false);
            }

      Score* score = currentScore();

      // Subdivide at cutTick by replacing the first portion with a rest
      NoteVal restValue(-1);

      Segment* splitSegment = score->setNoteRest(
            targetCr->segment(),
            track,
            restValue,
            (cutTick - startTick) * measureToTuplet);

      if (!splitSegment)
            return false;

      ChordRest* firstCR =
            chordRestAt(startTick, track);

      ChordRest* secondCR =
            chordRestAt(cutTick, track);

      if (!firstCR || !secondCR) {
            cr0 = firstCR;
            cr1 = nullptr;
            return false;
            }

      // If the original object was a chord, restore its notes into the
      // first portion:
      if (wasChord && secondCR->isChord()) {
            Chord* firstChord = nullptr;

            for (const NoteVal& notePitch : chordNotes) {
                  if (!firstChord) {
                        Segment* segment = score->setNoteRest(
                              firstCR->segment(),
                              track,
                              notePitch,
                              firstCR->ticks());

                        if (!segment)
                              return false;

                        ChordRest* restoredCR = segment->cr(track);
                        if (!restoredCR || !restoredCR->isChord())
                              return false;

                        firstChord = toChord(restoredCR);

                        if (firstChord->notes().empty())
                              return false;
                        }
                  else {
                        score->addNote(firstChord, notePitch);
                        }
                  }

            cr0 = firstChord;
            }
      else
            cr0 = chordRestAt(startTick, track);

      cr1 = chordRestAt(cutTick, track);

      // Enforce the advertised postcondition:
      // both resulting ChordRests must exist
      if (!cr0 || !cr1) {
            cr1 = nullptr;
            return false;
            }

      // Ordinary Cut creates a new attack at cutTick, so cr0 must not
      // be tied to cr1.  However, if the original ChordRest had an
      // incoming tie, preserve that existing relationship into cr0
      if (!preserveOriginalDuration && wasChord && cr0->isChord()) {

            Chord* firstChord = toChord(cr0);

            for (Note* note : firstChord->notes()) {
                  if (!note)
                        continue;

                  auto incomingIt =
                        preservedIncomingTieTicks.constFind(note->pitch());

                  if (incomingIt == preservedIncomingTieTicks.constEnd())
                        continue;

                  ChordRest* previousCR =
                        chordRestAt(incomingIt.value(), track);

                  if (!previousCR || !previousCR->isChord())
                        continue;

                  Note* previousNote = nullptr;

                  for (Note* candidate : toChord(previousCR)->notes()) {
                        if (candidate
                            && candidate->pitch() == note->pitch()) {
                              previousNote = candidate;
                              break;
                              }
                        }

                  if (!previousNote)
                        continue;

                  if (previousNote->tieFor() || note->tieBack())
                        continue;

                  Tie* tie = new Tie(score);
                  tie->setStartNote(previousNote);
                  tie->setEndNote(note);
                  tie->setTrack(previousNote->track());
                  tie->setTick(previousNote->chord()->segment()->tick());
                  tie->setTicks(
                        note->chord()->segment()->tick()
                        - previousNote->chord()->segment()->tick());

                  score->undoAddElement(tie);
                  }
            }

      // setNoteRest() may rhythmically decompose the remainder after
      // cutTick into more than one ChordRest. Those extra fragments
      // are notation of the same untouched remainder, not additional
      // cuts, so tie the matching pitches between consecutive fragments

      // Do not tie cr0 to cr1: cutTick is the explicit new attack
      // requested by the user
      if (wasChord && cr1->isChord()) {
            Fraction preserveStartTick = cr1->tick();
            Fraction preserveEndTick = endTick;
            bool firstRange = true;

            for (auto it = preservedTieRanges.constBegin();
                 it != preservedTieRanges.constEnd();
                 ++it) {
                  if (preserveOriginalDuration
                      && (firstRange
                          || it.value().first < preserveStartTick)) {
                        preserveStartTick = it.value().first;
                        firstRange = false;
                        }

                  if (it.value().second > preserveEndTick)
                        preserveEndTick = it.value().second;
                  }

            ChordRest* currentCR =
                  preserveOriginalDuration
                        ? score->findCR(preserveStartTick, track)
                        : cr1;

            while (currentCR && currentCR->isChord()) {
                  Chord* currentChord = toChord(currentCR);

                  const Fraction nextTick =
                        currentChord->tick() + currentChord->actualTicks();

                  // Never tie beyond the end of the original ChordRest
                  if (nextTick >= preserveEndTick)
                        break;

                  ChordRest* nextCR =
                        chordRestAt(nextTick, track);

                  if (!nextCR || !nextCR->isChord())
                        break;

                  Chord* nextChord = toChord(nextCR);

                  for (Note* note : currentChord->notes()) {
                        if (!note)
                              continue;

                        Note* nextNote = nullptr;

                        for (Note* candidate : nextChord->notes()) {
                              if (candidate
                                  && candidate->pitch() == note->pitch()) {
                                    nextNote = candidate;
                                    break;
                                    }
                              }

                        if (!nextNote)
                              continue;

                        auto rangeIt =
                              preservedTieRanges.constFind(note->pitch());

                        if (rangeIt == preservedTieRanges.constEnd())
                              continue;

                        const Fraction logicalStart =
                              rangeIt.value().first;
                        const Fraction logicalEnd =
                              rangeIt.value().second;

                        // Different notes in the original chord may belong to
                        // tie chains with different logical extents
                        if (currentChord->tick() < logicalStart)
                              continue;

                        if (nextTick >= logicalEnd)
                              continue;

                        // Do not disturb an existing tie relationship
                        Tie* tie = note->tieFor();

                        if (tie) {
                              if (tie->endNote() == nextNote)
                                    continue;

                              continue;
                              }

                        if (nextNote->tieBack())
                              continue;

                        tie = new Tie(score);
                        tie->setStartNote(note);
                        tie->setEndNote(nextNote);
                        tie->setTrack(note->track());
                        tie->setTick(note->chord()->segment()->tick());
                        tie->setTicks(
                              nextNote->chord()->segment()->tick()
                              - note->chord()->segment()->tick());

                        score->undoAddElement(tie);
                        }

                  currentCR = nextCR;
                  }
            }

      return true;
      }


//---------------------------------------------------------
//   pickNote
//---------------------------------------------------------

PianoItem* PianoView::pickNote(int tick, int pitch)
      {
      const QVector<PianoItem*> candidates =
            noteCandidatesForTickRange(tick, tick);

      for (PianoItem* pi : candidates) {
            if (pi->intersects(tick, tick, pitch, pitch))
                  return pi;
            }

      return nullptr;
      }

//---------------------------------------------------------
//   pickNote
//---------------------------------------------------------

PianoItem* PianoView::pickNote(const QPointF& pos)
      {
      const int tick = scenePosToTick(pos);

      const QVector<PianoItem*> candidates =
            noteCandidatesForTickRange(tick, tick);

      for (PianoItem* item : candidates) {
            if (!item || !item->note())
                  continue;

            Note* note = item->note();

            if (_editNoteTool == PianoRollEditTool::EVENT_ADJUST) {
                  for (const NoteEvent& event : note->playEvents()) {
                        if (boundingRect(note, &event, true)
                            .contains(pos.toPoint()))
                              return item;
                        }
                  }
            else {
                  if (boundingRect(note, false)
                      .contains(pos.toPoint()))
                        return item;
                  }
            }

      return nullptr;
      }

//---------------------------------------------------------
//   clearNoteSelection
//---------------------------------------------------------

void PianoView::clearNoteSelection()
      {
      if (!_staff)
            return;

      Score* score = currentScore();

      score->startCmd();
      score->selection().deselectAll();

      scene()->update();
      score->endCmd();

      emit selectionChanged();
      }

//---------------------------------------------------------
//   selectItem
//---------------------------------------------------------

void PianoView::selectItem(PianoItem* item, NoteSelectType selType)
      {
      if (!_staff || !item || !item->note())
            return;

      Score* score = currentScore();

      QSet<Note*> oldSelection;
      for (PianoItem* pi : _noteList) {
            if (pi && pi->note() && pi->note()->selected())
                  oldSelection.insert(pi->note());
            }

      Note* clickedNote = item->note();

      score->startCmd();

      Selection& selection = score->selection();
      selection.deselectAll();

      for (PianoItem* pi : _noteList) {
            if (!pi || !pi->note())
                  continue;

            Note* note = pi->note();
            const bool wasSelected = oldSelection.contains(note);
            const bool clicked = note == clickedNote;

            bool selected = false;

            switch (selType) {
                  case NoteSelectType::REPLACE:
                  case NoteSelectType::FIRST:
                        selected = clicked;
                        break;

                  case NoteSelectType::XOR:
                        selected = clicked ? !wasSelected : wasSelected;
                        break;

                  case NoteSelectType::ADD:
                        selected = clicked || wasSelected;
                        break;

                  case NoteSelectType::SUBTRACT:
                        selected = wasSelected && !clicked;
                        break;
                  }

            if (selected)
                  selection.add(note);
            }

      scene()->update();
      score->endCmd();

      QList<PianoItem*> selectedItems = getSelectedItems();
      if (!selectedItems.isEmpty()) {
            ScoreView* scoreView = mscore->currentScoreView();
            if (scoreView)
                  scoreView->adjustCanvasPosition(
                        selectedItems.first()->note(), false);
            }

      emit selectionChanged();
      }

//---------------------------------------------------------
//   selectNotes
//---------------------------------------------------------

void PianoView::selectNotes(int startTick, int endTick, int lowPitch, int highPitch, NoteSelectType selType)
      {
      Score* score = currentScore();
      //score->masterScore()->cmdState().reset();      // DEBUG: should not be necessary
      score->startCmd();

      QList<PianoItem*> oldSel;
      for (int i = 0; i < _noteList.size(); ++i) {
            PianoItem* pi = _noteList[i];
            if (pi->note()->selected())
                  oldSel.append(pi);
            }

      Selection& selection = score->selection();
      selection.deselectAll();

      for (int i = 0; i < _noteList.size(); ++i) {
            PianoItem* pi = _noteList[i];
            bool inBounds = pi->intersects(startTick, endTick, highPitch, lowPitch);

            bool sel;
            switch (selType) {
                  default:
                  case NoteSelectType::REPLACE:
                        sel = inBounds;
                        break;
                  case NoteSelectType::XOR:
                        sel = inBounds != oldSel.contains(pi);
                        break;
                  case NoteSelectType::ADD:
                        sel = inBounds || oldSel.contains(pi);
                        break;
                  case NoteSelectType::SUBTRACT:
                        sel = !inBounds && oldSel.contains(pi);
                        break;
                  case NoteSelectType::FIRST:
                        sel = inBounds && selection.elements().empty();
                        break;
                  }

            if (sel)
                  selection.add(pi->note());
            }

      scene()->update();
      score->endCmd();

      QList<PianoItem*> selectedItems = getSelectedItems();

      if (!selectedItems.isEmpty()) {
            ScoreView* scoreView = mscore->currentScoreView();
            if (scoreView)
                  scoreView->adjustCanvasPosition(
                        selectedItems.first()->note(), false);
            }

      emit selectionChanged();
      }

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void PianoView::leaveEvent(QEvent* event)
      {
      emit pitchChanged(-1);
      _trackingPos.setInvalid();
      emit trackingPosChanged(_trackingPos);
      QGraphicsView::leaveEvent(event);
      }

//---------------------------------------------------------
//   playbackTickBeyondCenter
//---------------------------------------------------------

bool PianoView::playbackTickBeyondCenter(qreal tick) const
      {
      if (_orientation != PianoRollOrientation::HORIZONTAL)
            return true;

      const QRectF rect =
            mapToScene(viewport()->geometry()).boundingRect();

      return tickToPixelXF(tick) >= rect.center().x();
      }

//---------------------------------------------------------
//   ensureVisible
//---------------------------------------------------------

void PianoView::ensureVisible(qreal tick, qreal horizontalOffset)
      {
      QRectF rect = mapToScene(viewport()->geometry()).boundingRect();
      const bool vertical = _orientation == PianoRollOrientation::VERTICAL;
      const bool horizontal = _orientation == PianoRollOrientation::HORIZONTAL;
      const int activationMargin = 0;

      if (horizontal) {
            const qreal xpos = tickToPixelXF(tick);

            // horizontalOffset is zero during normal playback, which
            // centers the playhead. At playback startup, it initially
            // represents the playhead's existing screen position and
            // is gradually reduced to zero
            const qreal target =
                  qMax(xpos
                       - rect.width() / 2.0
                       - horizontalOffset,
                       0.0);

            horizontalScrollBar()->setValue(qRound(target));
            }
      else if (vertical) {
            qreal ypos = tickToPixelYF(tick);

            int viewportHeight = viewport()->height();
            int target = ypos - viewportHeight + activationMargin;

            verticalScrollBar()->setValue(target);
            }
      }

//---------------------------------------------------------
//   ensurePlaybackTickVisible
//---------------------------------------------------------

void PianoView::ensurePlaybackTickVisible(qreal tick)
      {
      if (_orientation != PianoRollOrientation::HORIZONTAL)
            return;

      const QRectF rect =
            mapToScene(viewport()->geometry()).boundingRect();

      const qreal xpos = tickToPixelXF(tick);

      if (xpos < rect.left()) {
            horizontalScrollBar()->setValue(
                  qMax(qRound(xpos), 0));
            }
      else if (xpos > rect.right()) {
            horizontalScrollBar()->setValue(
                  qMax(qRound(xpos - rect.width()), 0));
            }
      }

//---------------------------------------------------------
//   centerSelectionTimeInView
//---------------------------------------------------------

void PianoView::centerSelectionTimeInView()
      {
      QList<PianoItem*> selected = getSelectedItems();
      if (selected.isEmpty())
            return;

      QRectF selectionRect;
      bool first = true;

      for (PianoItem* item : selected) {
            QRectF noteRect =
                  boundingRect(item->note(), nullptr, false);

            if (first) {
                  selectionRect = noteRect;
                  first = false;
                  }
            else
                  selectionRect |= noteRect;
            }

      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            const qreal targetX =
                  selectionRect.center().x()
                  - viewport()->width() / 2.0;

            horizontalScrollBar()->setValue(
                  qMax(0, qRound(targetX)));
            }
      else {
            const qreal targetY =
                  selectionRect.center().y()
                  - viewport()->height() / 2.0;

            verticalScrollBar()->setValue(
                  qMax(0, qRound(targetY)));
            }
      }

//---------------------------------------------------------
//   ensureSelectionVisible
//---------------------------------------------------------

void PianoView::ensureSelectionVisible(bool force)
      {
      const int xMargin = 20;
      const int yMargin =
            _orientation == PianoRollOrientation::VERTICAL ? 0
                                                           : 20;

      QList<PianoItem*> selected = getSelectedItems();
      if (selected.isEmpty())
            return;

      QRectF visibleRect =
            mapToScene(viewport()->rect()).boundingRect();

      // With a single selected note, follow it only when it has
      // actually gone outside the current viewport
      if (selected.size() == 1) {
            QRectF noteRect =
                  boundingRect(selected.first()->note(), nullptr, false);

            if (force || !visibleRect.contains(noteRect))
                  QGraphicsView::ensureVisible(noteRect, xMargin, yMargin);

            return;
            }

      // For multi-selection, don't jump just because some selected
      // notes extend beyond the viewport. Only move if none of the
      // selected notes is currently visible
      if (!force) {
            for (PianoItem* item : selected) {
                  QRectF noteRect =
                        boundingRect(item->note(), nullptr, false);

                  if (visibleRect.intersects(noteRect))
                        return;
                  }
            }

      // Nothing selected is visible
      // Bring the first selected item in:
      QRectF noteRect =
            boundingRect(selected.first()->note(), nullptr, false);

      QGraphicsView::ensureVisible(noteRect, xMargin, yMargin);
      }


//---------------------------------------------------------
//   updateBoundingSize
//---------------------------------------------------------

void PianoView::updateBoundingSize()
      {
      Score* score = currentScore();
      if (!score)
            return;

      Measure* lm = score->lastMeasure();
      if (!lm)
            return;

      _ticks = (lm->tick() + lm->ticks()).ticks();

      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            scene()->setSceneRect(
                  0.0,
                  0.0,
                  double((_ticks + MAP_OFFSET * 2) * _xZoom),
                  _noteHeight * 128);
            }
      else {
            scene()->setSceneRect(
                  0.0,
                  0.0,
                  _noteHeight * 128,
                  double((_ticks + MAP_OFFSET * 2) * _xZoom));
            }
      }

//---------------------------------------------------------
//   setVerticalPitchLayout
//---------------------------------------------------------

void PianoView::setVerticalPitchLayout(VerticalPitchLayout layout)
      {
      if (_verticalPitchLayout == layout)
            return;

      _verticalPitchLayout = layout;

      scene()->update();
      }

//---------------------------------------------------------
//   setOrientation
//---------------------------------------------------------

void PianoView::setOrientation(PianoRollOrientation orientation)
      {
      if (_orientation == orientation)
            return;

      _orientation = orientation;

      if (_orientation == PianoRollOrientation::VERTICAL)
            setAlignment(Qt::Alignment(Qt::AlignLeft | Qt::AlignBottom));
      else
            setAlignment(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter));

      updateBoundingSize();
      updateNotes();
      }

//---------------------------------------------------------
//   setEditableStaff
//---------------------------------------------------------

void PianoView::setEditableStaff(Staff* st)
      {
      _staff = st;
      }

//---------------------------------------------------------
//   setStaff
//---------------------------------------------------------

void PianoView::setStaff(Staff* s, Pos* l)
      {
      _locator = l;

      if (_staff == s)
            return;

      Staff* const oldStaff = _staff;

      _staff = s;
      setEnabled(_staff != nullptr);
      if (!_staff) {
            scene()->blockSignals(true);  // block changeSelection()
            scene()->clear();
            clearNoteData();
            scene()->blockSignals(false);
            return;
            }

      bool repositionView = false;
      switch (_scope) {
            case PianoRollScope::STAFF:
                  repositionView = true;
                  break;

            case PianoRollScope::PART:
                  repositionView =
                        !oldStaff
                        || !s
                        || oldStaff->part() != s->part();
                  break;

            case PianoRollScope::SCORE:
                  repositionView = false;
                  break;
            }

      _trackingPos.setContext(currentScore()->tempomap(), currentScore()->sigmap());
      updateBoundingSize();

      updateNotes();

      if (repositionView) {
            QRectF allNotesRect;
            bool allNotesRectInit = false;
            QRectF selectedNotesRect;
            bool selectedNotesRectInit = false;

            for (PianoItem* item : qAsConst(_noteList)) {
                  const QRectF itemRect =
                        boundingRect(item->note(), nullptr, false);

                  if (!allNotesRectInit) {
                        allNotesRect = itemRect;
                        allNotesRectInit = true;
                        }
                  else
                        allNotesRect |= itemRect;

                  if (item->note()->selected()) {
                        if (!selectedNotesRectInit) {
                              selectedNotesRect = itemRect;
                              selectedNotesRectInit = true;
                              }
                        else
                              selectedNotesRect |= itemRect;
                        }
                  }

            QRectF viewRect = mapToScene(viewport()->geometry()).boundingRect();

            if (selectedNotesRectInit) {
                  horizontalScrollBar()->setValue(selectedNotesRect.x());
                  verticalScrollBar()->setValue(
                        qMax(selectedNotesRect.y() + (selectedNotesRect.height() - viewRect.height()) / 2,
                             0.0));
                  }
            else if (allNotesRectInit) {
                  horizontalScrollBar()->setValue(allNotesRect.x());
                  verticalScrollBar()->setValue(
                        qMax(allNotesRect.y() + (allNotesRect.height() - viewRect.height()) / 2,
                              0.0));
                  }
            else {
                  horizontalScrollBar()->setValue(0);
                  verticalScrollBar()->setValue(qMax(viewRect.y() - viewRect.height() / 2, 0.0));
                  }
            }
      }

//---------------------------------------------------------
//   indexNoteItem
//---------------------------------------------------------

void PianoView::indexNoteItem(PianoItem* item)
      {
      if (!item || !item->note())
            return;

      Note* note = item->note();
      Chord* chord = note->chord();

      if (!chord)
            return;

      // Establish a conservative time-range covering both the normal
      // note block and all possible playback-event positions
      Fraction noteTicks = chord->ticks();

      if (Tuplet* tuplet = chord->tuplet())
            noteTicks *= tuplet->ratio().inverse();

      const Fraction tieLen =
            note->playTicksFraction() - noteTicks;

      Fraction firstTick = chord->tick();
      Fraction lastTick =
            chord->tick() + noteTicks + tieLen;

      for (const NoteEvent& event : note->playEvents()) {
            Fraction eventStart =
                  chord->tick()
                  + noteTicks * event.ontime() / 1000;

            Fraction eventEnd =
                  eventStart
                  + noteTicks * event.len() / 1000
                  + tieLen;

            if (eventEnd < eventStart)
                  qSwap(eventStart, eventEnd);

            if (eventStart < firstTick)
                  firstTick = eventStart;

            if (eventEnd > lastTick)
                  lastTick = eventEnd;
            }

      // Keep one quarter-note of padding on either side.  This makes
      // the index deliberately conservative for diamonds, outlines,
      // event offsets, and notes close to a bucket boundary
      int first = firstTick.ticks() - DIVISION;
      int last  = lastTick.ticks() + DIVISION;

      pianoRollAddToTimeBuckets(
            _noteTimeBuckets,
            item,
            first,
            last,
            NOTE_TIME_BUCKET_TICKS);
      }

//---------------------------------------------------------
//   rebuildNoteTimeBuckets
//---------------------------------------------------------

void PianoView::rebuildNoteTimeBuckets()
      {
      _noteTimeBuckets.clear();

      for (PianoItem* item : qAsConst(_noteList))
            indexNoteItem(item);
      }

//---------------------------------------------------------
//   noteCandidatesForTickRange
//---------------------------------------------------------

QVector<PianoItem*> PianoView::noteCandidatesForTickRange(
      int startTick,
      int endTick) const
      {
      return pianoRollTimeBucketCandidates(
            _noteTimeBuckets,
            startTick,
            endTick,
            NOTE_TIME_BUCKET_TICKS);
      }

//---------------------------------------------------------
//   addChord
//---------------------------------------------------------

void PianoView::addChord(Chord* chord)
      {
      for (Chord*& c : chord->graceNotes())
            addChord(c);
      for (Note* note : chord->notes()) {
            if (note->tieBack())
                  continue;
            PianoItem* item = new PianoItem(note, this);
            _noteList.append(item);
            indexNoteItem(item);
            }
      }

//---------------------------------------------------------
//   updateNotes
//---------------------------------------------------------

void PianoView::updateNotes()
      {
      scene()->blockSignals(true);
      scene()->clearFocus();
      scene()->clear();
      clearNoteData();

      if (!_staff) {
            scene()->blockSignals(false);
            return;
            }

      const Score* const score = currentScore();
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

      scene()->blockSignals(false);
      scene()->update(sceneRect());
      }

//---------------------------------------------------------
//   clearNoteData
//---------------------------------------------------------

void PianoView::clearNoteData()
      {
      _noteTimeBuckets.clear();

      for (int i = 0; i < _noteList.size(); ++i)
            delete _noteList[i];

      _noteList.clear();
      }


//---------------------------------------------------------
//   getSelectedItems
//---------------------------------------------------------

QList<PianoItem*> PianoView::getSelectedItems()
      {
      QList<PianoItem*> items;

      for (PianoItem* item : qAsConst(_noteList)) {
            if (item->note()->selected())
                  items.append(item);
            }

      return items;
      }

//---------------------------------------------------------
//   getAction
//    returns action for shortcut
//---------------------------------------------------------

QAction* PianoView::getAction(const char* id)
      {
      Shortcut* s = Shortcut::getShortcut(id);
      return s ? s->action() : 0;
      }

//---------------------------------------------------------
//   showNoteTweaker
//---------------------------------------------------------

void PianoView::showNoteTweaker()
      {
      emit showNoteTweakerRequest();
      }


//---------------------------------------------------------
//   setVoices
//---------------------------------------------------------

void PianoView::setNotesToVoice(int voice)
      {
      if (!_staff || _noteList.isEmpty())
            return;

      bool hasSelection = false;
      for (PianoItem* item : _noteList) {
            if (item->note()->selected()) {
                  hasSelection = true;
                  break;
                  }
            }

      if (!hasSelection)
            return;

      currentScore()->changeVoice(voice);
      }

//---------------------------------------------------------
//   setSelectedNoteColor
//---------------------------------------------------------

void PianoView::setSelectedNoteColor()
      {
      if (!_staff || _noteList.isEmpty())
            return;

      QList<Note*> notes;

      for (PianoItem* item : _noteList) {
            if (!item)
                  continue;

            Note* note = item->note();
            if (note && note->selected() && !notes.contains(note))
                  notes.append(note);
            }

      if (notes.isEmpty())
            return;

      QColor initialColor = notes.front()->color();

      QColor color = QColorDialog::getColor(
            initialColor,
            this,
            tr("Select Note Color"),
            QColorDialog::ShowAlphaChannel);

      if (!color.isValid())
            return;

      Score* score = currentScore();
      score->startCmd();

      for (Note* note : notes)
            note->undoChangeProperty(Pid::COLOR, color);

      score->endCmd();

      scene()->update();
      }

//---------------------------------------------------------
//   resetSelectedNoteColor
//---------------------------------------------------------

void PianoView::resetSelectedNoteColor()
      {
      if (!_staff || _noteList.isEmpty())
            return;

      QList<Note*> notes;

      for (PianoItem* item : _noteList) {
            if (!item)
                  continue;

            Note* note = item->note();
            if (note && note->selected() && !notes.contains(note))
                  notes.append(note);
            }

      if (notes.isEmpty())
            return;

      Score* score = currentScore();
      score->startCmd();

      for (Note* note : notes)
            note->undoChangeProperty(Pid::COLOR, MScore::defaultColor);

      score->endCmd();

      scene()->update();
      }


//---------------------------------------------------------
//   setXZoom
//---------------------------------------------------------

void PianoView::setXZoom(int value)
      {
      if (_xZoom != value) {
            _xZoom = value;
            scene()->update();
            emit xZoomChanged(_xZoom);
            }
      }

//---------------------------------------------------------
//   setBarPattern
//---------------------------------------------------------

void PianoView::setBarPattern(int value)
      {
      if (_barPattern != value) {
            _barPattern = value;
            scene()->update();
            emit barPatternChanged(_barPattern);
            }
      }

//---------------------------------------------------------
//   setBarPattern
//---------------------------------------------------------

void PianoView::togglePitchHighlight(int pitch)
      {
      _pitchHighlight[pitch] = _pitchHighlight[pitch] ? 0 : 1;
      scene()->update();
      }

//---------------------------------------------------------
//   setSubBeats
//---------------------------------------------------------

void PianoView::setTuplet(int value)
      {
      if (_tuplet != value) {
            _tuplet = value;
            scene()->update();
            emit tupletChanged(_tuplet);
            }
      }

//---------------------------------------------------------
//   setSubdiv
//---------------------------------------------------------

void PianoView::setSubdiv(int value)
      {
      if (_subdiv != value) {
            _subdiv = value;
            scene()->update();
            emit subdivChanged(_subdiv);
            }
      }


//---------------------------------------------------------
//   serializeSelectedNotes
//---------------------------------------------------------

QString PianoView::serializeSelectedNotes()
      {
      Fraction firstTick;
      bool init = false;
      for (int i = 0; i < _noteList.size(); ++i) {
            if (_noteList[i]->note()->selected()) {
                  Note* note = _noteList.at(i)->note();
                  Fraction startTick = note->chord()->tick();

                  if (!init || firstTick > startTick) {
                        firstTick = startTick;
                        init = true;
                        }
                  }
            }

      //No valid notes
      if (!init)
            return QByteArray();

      QString xmlStrn;
      QXmlStreamWriter xml(&xmlStrn);
      xml.setAutoFormatting(true);
      xml.writeStartDocument();

      xml.writeStartElement("notes");
      xml.writeAttribute("firstN", QString::number(firstTick.numerator()));
      xml.writeAttribute("firstD", QString::number(firstTick.denominator()));

      //bundle notes into XML file & send to clipboard.
      //This is only affects pianoview and is not part of the regular copy/paste process
      for (int i = 0; i < _noteList.size(); ++i) {
            if (_noteList[i]->note()->selected()) {
                  Note* note = _noteList[i]->note();

                  Fraction flen = note->playTicksFraction();

                  Fraction startTick = note->chord()->tick();
                  int pitch = note->pitch();
                  int voice = note->voice();
                  int staffIdx = note->staffIdx();

                  int veloOff = note->veloOffset();
                  Note::ValueType veloType = note->veloType();

                  xml.writeStartElement("note");
                  xml.writeAttribute("startN", QString::number(startTick.numerator()));
                  xml.writeAttribute("startD", QString::number(startTick.denominator()));
                  xml.writeAttribute("lenN", QString::number(flen.numerator()));
                  xml.writeAttribute("lenD", QString::number(flen.denominator()));
                  xml.writeAttribute("pitch", QString::number(pitch));
                  xml.writeAttribute("voice", QString::number(voice));
                  xml.writeAttribute("staff", QString::number(staffIdx));
                  xml.writeAttribute("veloOff", QString::number(veloOff));
                  xml.writeAttribute("veloType", veloType == Note::ValueType::OFFSET_VAL ? "o" : "u");

                  for (NoteEvent& evt : note->playEvents()) {
                        int ontime = evt.ontime();
                        int len = evt.len();

                        xml.writeStartElement("evt");
                        xml.writeAttribute("ontime", QString::number(ontime));
                        xml.writeAttribute("len", QString::number(len));
                        xml.writeEndElement();
                        }

                  xml.writeEndElement();
                  }
            }

      xml.writeEndElement();
      xml.writeEndDocument();

      return xmlStrn;
      }


//---------------------------------------------------------
//   cutNotes
//---------------------------------------------------------

void PianoView::cutNotes()
      {
      copyNotes();

      Score* score = currentScore();
      score->startCmd();

      //score->cmdDeleteSelection();
      deleteSelectedNotes();

      score->endCmd();
      }

//---------------------------------------------------------
//   copyNotes
//---------------------------------------------------------

void PianoView::copyNotes()
      {
      QString copiedNotes = serializeSelectedNotes();
      if (copiedNotes.isEmpty())
            return;

      QMimeData* mimeData = new QMimeData;
      mimeData->setData(PIANO_NOTE_MIME_TYPE, copiedNotes.toUtf8());
      QApplication::clipboard()->setMimeData(mimeData);
      }


//---------------------------------------------------------
//   compactMeasures
//---------------------------------------------------------

void PianoView::compactMeasures(
      const QMap<Measure*, QSet<int>>& changedTracks)
      {
      Score* score = currentScore();

      for (auto it = changedTracks.constBegin();
           it != changedTracks.constEnd(); ++it) {
            Measure* m = it.key();

            if (!m)
                  continue;

            for (int track : it.value()) {
                  if (track < 0 || track >= score->ntracks())
                        continue;
                  ChordRest* cr = m->findChordRest(m->tick(), track);
                  if (!cr)
                        continue;

                  while (true) {
                        Fraction crTicks = cr->ticks();

                        Segment* segNext = cr->nextSegmentAfterCR(SegmentType::ChordRest);
                        ChordRest* crNext = segNext ? segNext->cr(track) : nullptr;

                        if (!crNext || crNext->measure() != m)
                              break;

                        // Do not compact across tuplets: setNoteRest() may delete/rebuild
                        // an entire tuplet while compactMeasures() is iterating ChordRests
                        if (cr->tuplet() || crNext->tuplet()) {
                              cr = crNext;
                              continue;
                              }

                        Fraction crNextTicks = crNext->ticks();

                        if (cr->isRest() && crNext->isRest()) {
                              Segment* newSeg = score->setNoteRest(cr->segment(), track, NoteVal(-1), crTicks + crNextTicks);
                              cr = newSeg->cr(track);
                              }
                        else if (cr->isChord() && crNext->isChord()) {
                              Chord* chord = toChord(cr);

                              bool allAreTied = true;
                              QList<NoteVal> pitchList;

                              for (Note* n : chord->notes()) {
                                    if (!n->tieFor()) {
                                          allAreTied = false;
                                          break;
                                          }
                                    pitchList.append(n->noteVal());
                                    }

                              if (allAreTied) {
                                    Segment* newSeg = score->setNoteRest(cr->segment(), track, NoteVal(pitchList.at(0)), crTicks + crNextTicks);
                                    cr = newSeg->cr(track);

                                    for (int i = 1; i < pitchList.size(); ++i)
                                          score->addNote(toChord(cr), pitchList.at(i));
                                    }
                              else
                                    cr = crNext;
                              }
                        else
                              cr = crNext;
                        }
                  }

            }
      }

//---------------------------------------------------------
//   rangeTouchesTuplet
//---------------------------------------------------------

bool PianoView::rangeTouchesTuplet(const Fraction& startTick,
                                   const Fraction& duration,
                                   int track) const
      {
      if (!_staff || duration <= Fraction(0, 1))
            return false;

      Score* score = currentScore();
      if (!score || track < 0 || track >= score->ntracks())
            return false;

      const Fraction endTick = startTick + duration;

      // Traverse ChordRest segments that can overlap the requested range
      Segment* seg = score->tick2segment(startTick);

      // tick2segment() can be null when there is no segment exactly
      // at startTick, so start from the containing measure instead
      if (!seg) {
            Measure* measure = score->tick2measure(startTick);
            if (!measure)
                  return false;

            seg = measure->first(SegmentType::ChordRest);
            }

      for (; seg && seg->tick() < endTick;
           seg = seg->next1(SegmentType::ChordRest)) {
            ChordRest* cr = seg->cr(track);
            if (!cr)
                  continue;

            const Fraction crStart = cr->tick();
            const Fraction crEnd = crStart + cr->actualTicks();

            if (crEnd <= startTick)
                  continue;

            if (cr->tuplet())
                  return true;
            }

      return false;
      }

//---------------------------------------------------------
//   pasteWouldTouchTuplet
//---------------------------------------------------------

bool PianoView::pasteWouldTouchTuplet(const QString& copiedNotes,
                                      Fraction pasteStartTick,
                                      Fraction lengthOffset,
                                      bool xIsOffset) const
      {
      if (!_staff)
            return false;

      QXmlStreamReader xml(copiedNotes);
      Fraction firstTick;

      while (!xml.atEnd()) {
            QXmlStreamReader::TokenType tt = xml.readNext();

            if (tt != QXmlStreamReader::StartElement)
                  continue;

            if (xml.name().toString() == "notes") {
                  const int n =
                        xml.attributes().value("firstN").toString().toInt();
                  const int d =
                        xml.attributes().value("firstD").toString().toInt();

                  firstTick = Fraction(n, d);
                  continue;
                  }

            if (xml.name().toString() != "note")
                  continue;

            const int sn =
                  xml.attributes().value("startN").toString().toInt();
            const int sd =
                  xml.attributes().value("startD").toString().toInt();

            const Fraction startTick(sn, sd);

            const int tn =
                  xml.attributes().value("lenN").toString().toInt();
            const int td =
                  xml.attributes().value("lenD").toString().toInt();

            Fraction tickLen(tn, td);
            tickLen += lengthOffset;

            if (tickLen <= Fraction(0, 1))
                  continue;

            const int voice =
                  xml.attributes().value("voice").toString().toInt();

            int staffIdx = _staff->idx();
            if (xml.attributes().hasAttribute("staff")) {
                  staffIdx =
                        xml.attributes().value("staff").toString().toInt();
                  }

            const int track = staff2track(staffIdx) + voice;

            const Fraction pos =
                  xIsOffset
                        ? startTick + pasteStartTick
                        : startTick - firstTick + pasteStartTick;

            if (rangeTouchesTuplet(pos, tickLen, track))
                  return true;
            }

      return false;
      }

//---------------------------------------------------------
//   deleteSelectedNotes
//---------------------------------------------------------

void PianoView::deleteSelectedNotes()
      {
      Score* score = currentScore();

      // deleteItem modifies selection().elements() list,
      // so we need a local copy:
      QList<Element*> el = score->selection().elements();

      QList<Note*> notesToDelete;
      QMap<Measure*, QSet<int>> changedTracks;

      for (Element* e : el) {
            if (!e->isNote())
                  continue;

            Measure* m = e->findMeasure();
            if (m)
                  changedTracks[m].insert(e->track());

            Note* noteStart = toNote(e)->firstTiedNote();

            notesToDelete.append(noteStart);
            for (Note* note = noteStart; note->tieFor() != nullptr; note = note->tieFor()->endNote()) {
                  notesToDelete.append(note->tieFor()->endNote());

                  m = note->findMeasure();
                  if (m)
                        changedTracks[m].insert(note->track());
                  }
            }

      for (Note* note : notesToDelete)
            score->deleteItem(note);

      compactMeasures(changedTracks);
      }

//---------------------------------------------------------
//   setEditNoteTool
//---------------------------------------------------------

void PianoView::setEditNoteTool(PianoRollEditTool tool)
      {
      _editNoteTool = tool;
      updateCursor();
      scene()->update();
      }

//---------------------------------------------------------
//   pasteNotesAtCursor
//---------------------------------------------------------

void PianoView::pasteNotesAtCursor()
      {
      //ScoreView::normalPaste();
      const QMimeData* ms = QApplication::clipboard()->mimeData();
      if (!ms)
            return;

      Score* score = currentScore();
      Fraction pasteStartTick = roundToNearestBeat(pixelXToTick(_popupMenuPos.x()));

      if (ms->hasFormat(PIANO_NOTE_MIME_TYPE)) {
            //Decode our XML format and recreate the notes
            QByteArray copiedNotes = ms->data(PIANO_NOTE_MIME_TYPE);

            score->startCmd();
            pasteNotes(copiedNotes, pasteStartTick, Fraction(0, 1), 0);
            score->endCmd();
            }

      }


//---------------------------------------------------------
//   finishNoteGroupDrag
//---------------------------------------------------------

void PianoView::finishNoteGroupDrag(QMouseEvent* event) {
      Fraction pasteTickOffset;
      Fraction pasteLengthOffset;
      int pitchOffset { 0 };
      if (!calculateNoteDragOffsets(pasteTickOffset, pasteLengthOffset, pitchOffset)) {
            return;
            }

      // Group drag is implemented as delete + recreate. Do not delete
      // anything unless every destination note can be recreated without
      // crossing tuplet material that setNoteRest()/makeGap() may destroy
      if (pasteWouldTouchTuplet(_dragNoteCache,
                                pasteTickOffset,
                                pasteLengthOffset,
                                true)) {
            _levelPreviewActive = false;
            _levelPreviewTickOffset = Fraction(0, 1);
            _levelPreviewLengthOffset = Fraction(0, 1);
            _levelPreviewEventTickDelta = Fraction(0, 1);

            update();
            return;
            }

      Score* score = currentScore();
      score->startCmd();

      if (!(event->modifiers() & Qt::ShiftModifier)) {
            deleteSelectedNotes();
            }
      QVector<Note*> notes = pasteNotes(_dragNoteCache, pasteTickOffset, pasteLengthOffset, pitchOffset, true);

      // Select the resulting pasted notes
      Selection& selection = score->selection();
      selection.deselectAll();
      for (Note*& note : notes) {
            selection.add(note);
            note->setSelected(true);
            }

      score->endCmd();

      _dragNoteCache = QByteArray();

      _levelPreviewActive = false;
      _levelPreviewTickOffset = Fraction(0, 1);
      _levelPreviewLengthOffset = Fraction(0, 1);
      _levelPreviewEventTickDelta = Fraction(0, 1);

      score->update();
      updateNotes();
      update();

      QList<PianoItem*> selectedItems = getSelectedItems();
      if (!selectedItems.isEmpty()) {
            ScoreView* scoreView = mscore->currentScoreView();
            if (scoreView)
                  scoreView->adjustCanvasPosition(
                        selectedItems.first()->note(), false);
            }

      emit selectionChanged();
      }

//---------------------------------------------------------
//   pasteNotes
//---------------------------------------------------------

QVector<Note*> PianoView::pasteNotes(const QString& copiedNotes, Fraction pasteStartTick, Fraction lengthOffset, int pitchOffset, bool xIsOffset)
      {
      QXmlStreamReader xml(copiedNotes);
      Fraction firstTick;
      QVector<Note*> addedNotes;
      QVector<Note*> currentNotes;

      while (!xml.atEnd()) {
            QXmlStreamReader::TokenType tt = xml.readNext();

            if (tt == QXmlStreamReader::StartElement) {
                  if (xml.name().toString() == "notes") {
                        int n = xml.attributes().value("firstN").toString().toInt();
                        int d = xml.attributes().value("firstD").toString().toInt();
                        firstTick = Fraction(n, d);
                        }

                  if (xml.name().toString() == "note") {
                        int sn = xml.attributes().value("startN").toString().toInt();
                        int sd = xml.attributes().value("startD").toString().toInt();
                        Fraction startTick = Fraction(sn, sd);

                        int tn = xml.attributes().value("lenN").toString().toInt();
                        int td = xml.attributes().value("lenD").toString().toInt();
                        Fraction tickLen = Fraction(tn, td);

                        tickLen += lengthOffset;
                        if (tickLen.numerator() <= 0) {
                              currentNotes.clear();
                              continue;
                              }

                        int pitch = xml.attributes().value("pitch").toString().toInt();
                        int voice = xml.attributes().value("voice").toString().toInt();
                        int veloOff = xml.attributes().value("veloOff").toString().toInt();

                        QString veloTypeStrn = xml.attributes().value("veloType").toString();

                        Note::ValueType veloType = veloTypeStrn == "o" ? Note::ValueType::OFFSET_VAL : Note::ValueType::USER_VAL;

                        int staffIdx = _staff->idx();
                        if (xml.attributes().hasAttribute("staff"))
                              staffIdx = xml.attributes().value("staff").toString().toInt();

                        int track = staff2track(staffIdx) + voice;

                        Fraction pos = xIsOffset ? startTick + pasteStartTick : startTick - firstTick + pasteStartTick;

                        currentNotes = addNote(pos, tickLen, pitch + pitchOffset,track);

                        for (Note* note : qAsConst(currentNotes)) {
                              note->setVeloOffset(veloOff);
                              note->setVeloType(veloType);
                              }

                        for (Note* note : qAsConst(currentNotes))
                              addedNotes.append(note);
                        }

                  if (xml.name().toString() == "evt") {
                        int ontime = xml.attributes().value("ontime").toString().toInt();

                        int len = xml.attributes().value("len").toString().toInt();

                        NoteEvent ne;
                        ne.setOntime(ontime);
                        ne.setLen(len);

                        // Event data belongs only to the most recently
                        // parsed <note>, not to every note pasted so far
                        for (Note* note : qAsConst(currentNotes)) {
                              NoteEventList& evtList = note->playEvents();

                              if (!evtList.isEmpty()) {
                                    NoteEvent* evt = note->noteEvent(evtList.length() - 1);
                                    currentScore()->undo(new ChangeNoteEvent(note, evt, ne));
                                    }
                              }
                        }
                  }
            }

      return addedNotes;
      }


//---------------------------------------------------------
//   drawDraggedNotes
//---------------------------------------------------------

void PianoView::drawDraggedNotes(QPainter* painter)
      {
      QColor noteColor;
      switch (preferences.effectiveGlobalStyle()) {
            case MuseScoreEffectiveStyleType::DARK_FUSION:
                  noteColor = QColor(preferences.getColor(PREF_UI_PIANOROLL_DARK_NOTE_DRAG_COLOR));
                  break;
            default:
                  noteColor = QColor(preferences.getColor(PREF_UI_PIANOROLL_LIGHT_NOTE_DRAG_COLOR));
                  break;
            }

      _levelPreviewActive = false;
      _levelEventPreviews.clear();
      _levelPreviewLengthOffset = Fraction(0, 1);

      if (_dragStyle == DragStyle::DRAW_NOTE) {
            const int pitch =
                  scenePosToPitch(_mouseDownPos);

            if (!pitchIsValid(pitch))
                  return;

            Fraction firstTick =
                  roundToNearestBeat(
                        scenePosToTick(_mouseDownPos),
                        true);

            firstTick =
                  clampTickToScore(firstTick);

            const bool onsetDiamond =
                  useOnsetDiamond(_staff, firstTick);

            if (onsetDiamond) {
                  const QVector<Fraction> ticks =
                        onsetPaintTicks(
                              _mouseDownPos,
                              _lastMousePos);

                  for (const Fraction& tick : ticks) {
                        if (tick < Fraction{} ||
                            tick > Fraction::fromTicks(_ticks)) {
                              continue;
                              }

                        const Fraction duration =
                              gridLengthAt(tick);

                        if (duration <= Fraction(0, 1))
                              continue;

                        const int voice =
                              insertionVoiceForNote(
                                    tick,
                                    duration,
                                    pitch,
                                    _staff->idx(),
                                    _editNoteVoice);

                        const int drumTrack =
                              staff2track(_staff->idx()) + voice;

                        drawDraggedNote(
                              painter,
                              tick,
                              duration,
                              pitch,
                              drumTrack,
                              noteColor,
                              pitchNameForMidi(pitch));
                        }

                  return;
                  }

            double startTick =
                  scenePosToTick(_mouseDownPos);

            double endTick =
                  scenePosToTick(_lastMousePos);

            if (startTick > endTick)
                  std::swap(startTick, endTick);

            Fraction startTickFrac =
                  roundToNearestBeat(startTick);

            Fraction endTickFrac =
                  roundToNearestBeat(endTick, false);

            startTickFrac =
                  clampTickToScore(startTickFrac);

            endTickFrac =
                  clampTickToScore(endTickFrac);

            if (endTickFrac != startTickFrac) {
                  const Fraction duration =
                        endTickFrac - startTickFrac;

                  const int voice =
                        insertionVoiceForNote(
                              startTickFrac,
                              duration,
                              pitch,
                              _staff->idx(),
                              _editNoteVoice);

                  const int track =
                        staff2track(_staff->idx()) + voice;

                  drawDraggedNote(
                        painter,
                        startTickFrac,
                        duration,
                        pitch,
                        track,
                        noteColor,
                        pitchNameForMidi(pitch));
                  }

            return;
            }
      if (_dragStyle == DragStyle::EVENT_LENGTH || _dragStyle == DragStyle::EVENT_MOVE
          || _dragStyle == DragStyle::EVENT_ONTIME) {

            Fraction tickDelta = Fraction::fromTicks(
                  scenePosToTick(_lastMousePos)
                  - scenePosToTick(_mouseDownPos));

            _levelPreviewActive = true;
            _levelPreviewTickOffset = Fraction(0, 1);
            _levelPreviewLengthOffset = Fraction(0, 1);
            _levelPreviewEventTickDelta = tickDelta;

            for (int i = 0; i < _noteList.size(); ++i) {
                  PianoItem* pi = _noteList[i];
                  if (pi->note()->selected()) {
                        for (NoteEvent& e : pi->note()->playEvents()) {
                              Chord* chord = pi->note()->chord();
                              Fraction ticks = chord->ticks();
                              Tuplet* tup = chord->tuplet();
                              if (tup) {
                                    Fraction frac = tup->ratio();
                                    ticks = ticks * frac.inverse();
                                    }

                              Fraction start = pi->note()->chord()->tick();
                              Fraction tieLen = pi->note()->playTicksFraction() - ticks;

                              Fraction startAdj =
                                    start + ticks * e.ontime() / 1000;

                              Fraction lenAdj =
                                    ticks * e.len() / 1000
                                    + tieLen;

                              //Calc start, duration of where we dragged to
                              Fraction startNew;
                              Fraction lenNew;
                              switch (_dragStyle) {
                                    case DragStyle::EVENT_ONTIME:
                                          startNew = startAdj + tickDelta;
                                          lenNew = lenAdj - tickDelta;
                                          break;
                                    case DragStyle::EVENT_MOVE:
                                          startNew = startAdj + tickDelta;
                                          lenNew = lenAdj;
                                          break;
                                    default:
                                    case DragStyle::EVENT_LENGTH:
                                          startNew = startAdj;
                                          lenNew = lenAdj + tickDelta;
                                          break;
                                    }

                              if (_dragStyle == DragStyle::EVENT_LENGTH) {
                                    const Fraction minLen =
                                          tieLen + ticks * Fraction(1, 1000);

                                    if (lenNew < minLen)
                                          lenNew = minLen;
                                    }

                              const int pitch = pi->note()->pitch();
                              const int voice = pi->note()->voice();
                              const int track = staff2track(_staff->idx()) + voice;

                              drawDraggedNote(painter,
                                              startNew,
                                              lenNew,
                                              pitch,
                                              track,
                                              noteColor,
                                              pi->note()->tpcUserName());

                              // Same as finishNoteEventAdjustDrag:
                              int evtOntimeNew = int(((startNew - start) / ticks).toDouble() * 1000);
                              int evtLenNew =
                                    int(((lenNew - tieLen) / ticks).toDouble() * 1000);
                              if (evtLenNew < 1) {
                                    evtLenNew = 1;
                                    }

                              LevelEventPreview preview;
                              preview.ontime = evtOntimeNew;
                              preview.len = evtLenNew;

                              _levelEventPreviews.insert(&e, preview);

                              emit onTimeDragged(evtOntimeNew);
                              emit tickLenDragged(evtLenNew);
                              }
                        }
                  }

            return;
            }

      Fraction pasteTickOffset;
      Fraction pasteLengthOffset;
      int pitchOffset { 0 };
      if (!calculateNoteDragOffsets(pasteTickOffset, pasteLengthOffset, pitchOffset)) {
            return;
            }

      _levelPreviewActive = true;
      _levelPreviewTickOffset = pasteTickOffset;
      _levelPreviewLengthOffset = pasteLengthOffset;
      _levelPreviewEventTickDelta = Fraction(0, 1);

      //Iterate thorugh note data
      QXmlStreamReader xml(_dragNoteCache);
      Fraction firstTick;

      while (!xml.atEnd()) {
            QXmlStreamReader::TokenType tt = xml.readNext();
            if (tt == QXmlStreamReader::StartElement){
                  if (xml.name().toString() == "notes") {
                        int n = xml.attributes().value("firstN").toString().toInt();
                        int d = xml.attributes().value("firstD").toString().toInt();
                        firstTick = Fraction(n, d);
                        }
                  if (xml.name().toString() == "note") {
                        int sn = xml.attributes().value("startN").toString().toInt();
                        int sd = xml.attributes().value("startD").toString().toInt();
                        Fraction fStartTick = Fraction(sn, sd);

                        int tn = xml.attributes().value("lenN").toString().toInt();
                        int td = xml.attributes().value("lenD").toString().toInt();
                        Fraction tickLen = Fraction(tn, td);
                        tickLen += pasteLengthOffset;
                        if (tickLen.numerator() <= 0) {
                              continue;
                              }

                        const int pitch = xml.attributes().value("pitch").toString().toInt();
                        const int voice = xml.attributes().value("voice").toString().toInt();
                        const int track = staff2track(_staff->idx()) + voice;

                        // NOTE_POSITION / NOTE_LENGTH_*
                        const int previewPitch = pitch + pitchOffset;

                        drawDraggedNote(painter,
                                        fStartTick + pasteTickOffset,
                                        tickLen,
                                        previewPitch,
                                        track,
                                        noteColor,
                                        pitchNameForMidi(previewPitch));
                        }
                  }
            }
      }

//---------------------------------------------------------
//   drawDraggedNote
//---------------------------------------------------------

void PianoView::drawDraggedNote(QPainter* painter,
                                Fraction startTick,
                                Fraction frac,
                                int pitch,
                                int track,
                                QColor color,
                                const QString& pitchName)
      {
      Staff* staff = nullptr;
      Score* score = currentScore();
      if (score) {
            const int staffIdx = track / VOICES;
            if (staffIdx >= 0 && staffIdx < score->nstaves())
                  staff = score->staff(staffIdx);
            }

      const bool onsetDiamond =
            staff && useOnsetDiamond(staff, startTick);

      painter->setBrush(color);

      const QColor borderColor =
            preferences.getBool(PREF_UI_PIANOROLL_NOTE_BORDER_COLOR_LIGHTER)
                  ? color.lighter(125)
                  : color.darker(175);

      painter->setPen(QPen(borderColor));

      if (onsetDiamond) {
            const int subbeats = _tuplet * (1 << _subdiv);
            const Fraction gridLength(1, 4 * subbeats);

            const qreal gridPixels = qAbs(
                  tickToPixelXF((startTick + gridLength).ticks())
                  - tickToPixelXF(startTick.ticks()));


            const qreal size =
                  qMax(6.0, qMin(static_cast<qreal>(_noteHeight), gridPixels));

            const qreal half = size / 2.0;

            qreal cx;
            qreal cy;

            if (_orientation == PianoRollOrientation::HORIZONTAL) {
                  cx = tickToPixelXF(startTick.ticks());
                  cy = (pitchToPixelY(pitch)
                        + pitchToPixelY(pitch + 1)) / 2.0;
                  }
            else {
                  if (_verticalPitchLayout == VerticalPitchLayout::KEYBOARD_ALIGNED)
                        cx = keyboardAlignedPitchLane(pitch).center().x();
                  else
                        cx = (pitch + 0.5) * _noteHeight;

                  cy = tickToPixelYF(startTick.ticks());
                  }

            QPolygonF diamond;
            diamond
                  << QPointF(cx,        cy - half)
                  << QPointF(cx + half, cy)
                  << QPointF(cx,        cy + half)
                  << QPointF(cx - half, cy);

            painter->drawPolygon(diamond);
            return;
            }

      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            int x0 = tickToPixelX(startTick.ticks());
            int x1 = tickToPixelX((startTick + frac).ticks());
            int y0 = pitchToPixelY(pitch);

            QRectF bounds(
                  x0,
                  y0 - _noteHeight,
                  x1 - x0,
                  _noteHeight
                  );

            painter->drawRoundedRect(
                  bounds,
                  PianoItem::NOTE_BLOCK_CORNER_RADIUS,
                  PianoItem::NOTE_BLOCK_CORNER_RADIUS
                  );

            if (!pitchName.isEmpty())
                  drawPitchText(painter, bounds, pitchName, color);
            }
      else {
            qreal center;

            if (_verticalPitchLayout == VerticalPitchLayout::KEYBOARD_ALIGNED) {
                  QRectF lane = keyboardAlignedPitchLane(pitch);
                  center = lane.center().x();
                  }
            else {
                  center = (pitch + 0.5) * _noteHeight;
                  }

            const qreal width = _noteHeight;
            const qreal x0 = center - width / 2.0;

            int y0 = tickToPixelY((startTick + frac).ticks());
            int y1 = tickToPixelY(startTick.ticks());

            QRectF bounds(
                  x0,
                  y0,
                  width,
                  y1 - y0
                  );

            painter->drawRoundedRect(
                  bounds,
                  PianoItem::NOTE_BLOCK_CORNER_RADIUS,
                  PianoItem::NOTE_BLOCK_CORNER_RADIUS
                  );

            if (!pitchName.isEmpty())
                  drawPitchText(painter, bounds, pitchName, color);
            }
      }

}
