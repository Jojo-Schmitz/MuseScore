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

#ifndef __PIANOLEVELS_H__
#define __PIANOLEVELS_H__

#include <QWidget>
#include <QHash>
#include <QVector>

#include "pianorolledittool.h"
#include "pianoroll/pianoview.h"

#include "libmscore/pos.h"

namespace Ms {

class Score;
class Staff;
class Chord;
class Note;
class NoteEvent;
class PianoItem;
class PianoLevelsFilter;

enum class PianoRollOrientation;


//---------------------------------------------------------
//   PianoLevels
//---------------------------------------------------------

class PianoLevels : public QWidget
{
      Q_OBJECT

      enum DragStyle {
            LERP, OFFSET
            };

      struct LevelDragTarget {
            Note* note { nullptr };
            NoteEvent* event { nullptr };
            int startValue { 0 };
            };

      struct LevelEditState {
            bool mouseDown { false };
            QPointF mouseDownPos;
            QPointF lastMousePos;
            bool dragging { false };
            DragStyle dragStyle { DragStyle::OFFSET };

            QSet<const Note*> interactionNotes;
            QVector<LevelDragTarget> dragTargets;
            int dragAnchorValue { 0 };

            bool commandActive { false };
            };

      LevelEditState _edit;

      PianoView* _pianoView { nullptr };
      PianoRollOrientation _orientation { PianoRollOrientation::HORIZONTAL };
      Coloring _coloring;
      bool _useNoteColors { false };
      Score* _score { nullptr };
      int _xpos;
      qreal _xZoom;
      Pos _cursor;
      Pos* _locator { nullptr };
      Staff* _staff { nullptr };
      int _tuplet;
      int _subdiv;
      int _levelsIndex;
      int vMargin;
      int levelLen;
      int pickRadius = 4;

      qreal _playbackLocatorTick { 0.0 };
      bool _playbackLocatorValid { false };

      PianoRollScope _scope;

      int minBeatGap;

      QList<Note*> noteList;
      QHash<int, QVector<Note*>> _noteTimeBuckets;

      virtual void paintEvent(QPaintEvent*);
      virtual void mousePressEvent(QMouseEvent*);
      virtual void mouseReleaseEvent(QMouseEvent* event);
      virtual void mouseMoveEvent(QMouseEvent* event);
      virtual void leaveEvent(QEvent*);

      int noteStartTick(Note* note, NoteEvent* evt);
      void moveLocator(QMouseEvent*);
      void addChord(Chord* chord);
      void clearNoteData();

      void indexNote(Note* note);
      QVector<Note*> noteCandidatesForTickRange(int startTick, int endTick) const;

      bool pickNoteEvent(int x, int y, bool selectedOnly,
                         Note*& pickedNote, NoteEvent*& pickedNoteEvent);

      bool pickNearestLevelInTimeBand(int timePixel,
                                      int valuePixel,
                                      int timeRadius,
                                      bool selectedOnly,
                                      Note*& pickedNote,
                                      NoteEvent*& pickedEvent);

      void captureLevelDragTargets(Note* anchorNote,
                                   NoteEvent* anchorEvent,
                                   bool selectedOnly);

      void adjustCapturedLevels(int value);
      void clearLevelInteraction();
      void adjustLevelLerp(int tick0, int value0, int tick1, int value1, bool selectedOnly = true);
      void adjustLevel(Note* note, NoteEvent* noteEvt, int value);

      void drawTimeAxisLine(QPainter& painter, int pos) const;
      void drawValueAxisLine(QPainter& painter, int pos) const;

      void drawTimeGrid(QPainter& painter,
                        int timeStart,
                        int timeEnd,
                        const QPen& penLineMajor,
                        const QPen& penLineMinor,
                        const QPen& penLineSub) const;

      void drawValueGrid(QPainter& painter,
                         PianoLevelsFilter* filter,
                         const QPen& penLineMajor,
                         const QPen& penLineMinor,
                         const QColor& textColor) const;

      void drawNoteLevels(QPainter& painter,
                          PianoLevelsFilter* filter,
                          int timeStart,
                          int timeEnd);

      void drawLevelBar(QPainter& painter,
                        int timePixel,
                        int valuePixel,
                        int zeroPixel,
                        const QColor& color);

      QRect levelHitRect(int timePixel,
                         int valuePixel,
                         int zeroPixel) const;

      int pixelToTick(int pixel) const;
      int tickToPixel(int tick) const;

      int valToPixel(int value) const;
      int pixelToVal(int pixel) const;

      bool hasSelectedNotes() const;

signals:
      void posChanged(const Pos&);
      void tupletChanged(int);
      void subdivChanged(int);
      void levelsIndexChanged(int);
      void locatorMoved(int idx, const Pos&);
      void noteLevelsChanged();

public slots:
      void setXpos(int);
      void setTuplet(int);
      void setSubdiv(int);
      void setXZoom(qreal);
      void setPos(const Pos&);
      void setLevelsIndex(int index);

public:
      PianoLevels(QWidget *parent = 0);
      ~PianoLevels();

      void setUseNoteColors(bool value);
      void setColoring(Coloring);
      void setPianoView(PianoView*);
      void setOrientation(PianoRollOrientation);
      void setScore(Score*, Pos* locator);
      Staff* staff() { return _staff; }
      void setEditableStaff(Staff* st);
      void setStaff(Staff*, Pos* locator);
      void updateNotes();
      int tuplet() const { return _tuplet; }
      int subdiv() const { return _subdiv; }

      void setPlaybackLocatorTick(qreal tick);
      void clearPlaybackLocatorTick();

      void setScope(PianoRollScope scope);

      int mouseTimePixel(const QPointF& pos) const;
      int mouseValuePixel(const QPointF& pos) const;

      int xpos() const { return _xpos; }
      qreal xZoom() const { return _xZoom; }
};

}
#endif // __PIANOLEVELS_H__
