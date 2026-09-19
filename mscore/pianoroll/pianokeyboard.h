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

#ifndef __PIANO_KEYBOARD_H__
#define __PIANO_KEYBOARD_H__

#include "piano.h"
#include "pianorolledittool.h"

#include "libmscore/note.h"

namespace Ms {

class Staff;

static const int PIANO_KEYBOARD_WIDTH = 100;
static const int BLACK_KEY_WIDTH = PIANO_KEYBOARD_WIDTH * 9 / 14;
const int MAX_KEY_HEIGHT = 48;
const int MIN_KEY_HEIGHT = 8;
const int DEFAULT_KEY_HEIGHT = 18;
const int BEAT_WIDTH_IN_PIXELS = 50;
const double X_ZOOM_RATIO = 1.1;
const double X_ZOOM_INITIAL = 0.1;

inline int pianoRollWhiteKeyIndex(int degree)
      {
      static const int degrees[] = {
            0, 2, 4, 5, 7, 9, 11
            };

      for (int i = 0; i < 7; ++i) {
            if (degrees[i] == degree)
                  return i;
            }

      return -1;
      }

inline int pianoRollBlackKeyIndex(int degree)
      {
      static const int degrees[] = {
            1, 3, 6, 8, 10
            };

      for (int i = 0; i < 5; ++i) {
            if (degrees[i] == degree)
                  return i;
            }

      return -1;
      }

inline int pianoRollBlackKeyBoundary(int key)
      {
      // With the horizontal keyboard, all seven white keys
      // have equal width. These identify the white-key
      // boundaries occupied by the five black keys:
      static const int boundaries[] = {
            1, 2, 4, 5, 6
            };

      return key >= 0 && key < 5
            ? boundaries[key]
            : -1;
      }

inline bool pianoRollHasBlackKeyBoundary(int boundary)
      {
      for (int i = 0; i < 5; ++i) {
            if (pianoRollBlackKeyBoundary(i) == boundary)
                  return true;
            }

      return false;
      }

inline qreal pianoRollWhiteKeyWidth(qreal noteHeight)
      {
      return 12.0 * noteHeight / 7.0;
      }
      
//Alternative implementation with evenly spaced notes
class PianoKeyboard : public QWidget {
      Q_OBJECT

      static const char* pitchNames[];

      PianoOrientation _orientation;
      int _ypos;

      int noteHeight;
      int yRange;
      int curPitch;
      int curKeyPressed;
      QHash<int, const Note*> _playbackNotes;
      QHash<int, const Note*> _selectionNotes;
      Coloring _coloring;
      bool _useNoteColors { false };
      Staff* _staff { nullptr };

      virtual void paintEvent(QPaintEvent*);
      virtual void mousePressEvent(QMouseEvent*);
      virtual void mouseReleaseEvent(QMouseEvent*);
      virtual void mouseMoveEvent(QMouseEvent* event);
      virtual void leaveEvent(QEvent*);

      QColor keyCurrentColor(int pitch, const QColor& defaultColor) const;

   signals:
      void pitchChanged(int);
      void keyPressed(int pitch);
      void keyReleased(int pitch);
      void pitchHighlightToggled(int pitch);

   public slots:
      void setYpos(int val);
      void setNoteHeight(int);
      void setPitch(int);

   public:
      PianoKeyboard(QWidget* parent = 0);
      Staff* staff() { return _staff; }
      void setStaff(Staff* staff);
      void setOrientation(PianoOrientation);

      void setSelectionNotes(const QHash<int, const Note*>& notes);
      void setPlaybackNotes(const QHash<int, const Note*>& notes);
      void setColoring(Coloring);
      void setUseNoteColors(bool value);
      };


} // namespace Ms
#endif

