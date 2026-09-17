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

#include "pianokeyboard.h"

#include "libmscore/staff.h"
#include "libmscore/part.h"
#include "libmscore/drumset.h"
#include "preferences.h"

#include <QColor>

namespace Ms {

const char* PianoKeyboard::pitchNames[] =
            {"C", "C♯", "D", "D♯", "E", "F", "F♯", "G", "G♯", "A", "A♯", "B"}; // keep in sync with `valu` in limbscore/utils.cpp

//---------------------------------------------------------
//   PianoKeyboard
//---------------------------------------------------------

PianoKeyboard::PianoKeyboard(QWidget* parent)
   : QWidget(parent)
      {
      setMouseTracking(true);
      setAttribute(Qt::WA_NoSystemBackground);
      setAttribute(Qt::WA_StaticContents);
      yRange   = noteHeight * 128;
      curPitch = -1;
      _ypos    = 0;
      curKeyPressed = -1;
      noteHeight = DEFAULT_KEY_HEIGHT;
      _orientation = PianoOrientation::VERTICAL;
      _staff = 0;
      }

//---------------------------------------------------------
//   keyCurrentColor
//---------------------------------------------------------

QColor PianoKeyboard::keyCurrentColor(int pitch, const QColor& defaultColor) const
      {
      const Note* note = _playbackNotes.value(pitch, nullptr);

      if (!note)
            note = _selectionNotes.value(pitch, nullptr);

      return note ? pianoRollNoteColor(note, _coloring, false, _useNoteColors)
                  : defaultColor;
      }

//---------------------------------------------------------
//   paint
//---------------------------------------------------------

void PianoKeyboard::paintEvent(QPaintEvent* /*event*/)
      {
      QPainter p(this);
      p.setRenderHints(QPainter::Antialiasing
                       | QPainter::SmoothPixmapTransform
                       | QPainter::TextAntialiasing);

      const int fontSize = 8;
      QFont f("FreeSans", fontSize);
      p.setFont(f);

      // Check for drumset, if any
      Drumset* ds = nullptr;
      Interval transp;
      if (_staff) {
            Part* part = _staff->part();
            ds = part->instrument()->drumset();
            transp = part->instrument()->transpose();
            }

      p.setPen(QPen(Qt::black, 2));

      int keyboardLen = 128 * noteHeight;
      const int blackKeyLen = BLACK_KEY_WIDTH;

      // The original offsets are retained for the existing
      // VERTICAL keyboard implementation
      const qreal whiteKeyOffset[] = {
            0, 1.5, 3.5, 5, 6.5, 8.5, 10.5, 12
            };

      const qreal blackKeyOffset[] = {
            1.5, 3.5, 6.5, 8.5, 10.5
            };

      const qreal horizontalWhiteKeyWidth =
            pianoRollWhiteKeyWidth(noteHeight);

      //---------------------------------------------------
      // White keys
      //---------------------------------------------------

      p.setPen(QPen(Qt::black));

      for (int midiPitch = 0; midiPitch < 128; ++midiPitch) {
            int instrPitch = midiPitch - transp.chromatic;

            int octave = instrPitch / 12;
            int degree = (instrPitch + 60) % 12;

            const int key = pianoRollWhiteKeyIndex(degree);
            if (key == -1)
                  continue;

            QString noteName =
                  qApp->translate("utils", pitchNames[degree])
                  + QString::number(octave - 1);

            if (ds) {
                  noteName = qApp->translate(
                        "drumset",
                        ds->name(instrPitch).toUtf8().constData());
                  }

            p.setBrush(keyCurrentColor(midiPitch, Qt::white));

            if (_orientation == PianoOrientation::HORIZONTAL) {
                  // Seven equal-width white keys occupy the
                  // same [12 * noteHeight] octave width
                  qreal octaveOffset =
                        (octave * 12 + transp.chromatic)
                        * noteHeight;

                  qreal x =
                        octaveOffset
                        + key * horizontalWhiteKeyWidth;

                  QRectF rect(
                        -_ypos + x,
                        0,
                        horizontalWhiteKeyWidth,
                        height()
                        );

                  p.drawRect(rect);

                  if ((ds
                       || (preferences.getBool(PREF_UI_PIANO_SHOWPITCHHELP)
                           && degree == 0))
                      && noteHeight > fontSize + 2) {
                        p.save();

                        p.translate(rect.left() + 1,
                                    rect.bottom() - 1);
                        p.rotate(-90.0);

                        QRectF rectText(
                              0.0,
                              0.0,
                              rect.height() - 2,
                              rect.width() - 2);

                        p.drawText(
                              rectText,
                              Qt::AlignLeft | Qt::AlignVCenter,
                              noteName);

                        p.restore();
                        }
                  }
            else {
                  // Original vertical keyboard geometry:
                  qreal off1 =
                        whiteKeyOffset[key] * noteHeight
                        + (octave * 12 + transp.chromatic)
                              * noteHeight;

                  qreal off2 =
                        whiteKeyOffset[key + 1] * noteHeight
                        + (octave * 12 + transp.chromatic)
                              * noteHeight;

                  QRectF rect(
                        0,
                        -_ypos + keyboardLen - off2,
                        width(),
                        off2 - off1
                        );

                  p.drawRect(rect);

                  if (noteHeight > fontSize + 2) {
                        if (ds) {
                              QRectF rectText(
                                    rect.x() + 1,
                                    -_ypos
                                          + keyboardLen
                                          - (instrPitch + 1)
                                                * noteHeight,
                                    rect.width() - 1,
                                    noteHeight);

                              p.drawText(
                                    rectText,
                                    Qt::AlignBottom | Qt::AlignLeft,
                                    noteName);
                              }
                        else if (preferences.getBool(
                                       PREF_UI_PIANO_SHOWPITCHHELP)
                                 && degree == 0) {
                              QRectF rectText(
                                    rect.x(),
                                    rect.y(),
                                    rect.width() - 4,
                                    rect.height() - 1);

                              p.drawText(
                                    rectText,
                                    Qt::AlignRight | Qt::AlignBottom,
                                    noteName);
                              }
                        }
                  }
            }

      //---------------------------------------------------
      // Black keys
      //---------------------------------------------------

      for (int midiPitch = 0; midiPitch < 128; ++midiPitch) {
            int instrPitch = midiPitch - transp.chromatic;

            int octave = instrPitch / 12;
            int degree = instrPitch % 12;

            if (degree < 0)
                  degree += 12;

            const int key = pianoRollBlackKeyIndex(degree);
            if (key == -1)
                  continue;

            QString noteName =
                  qApp->translate("utils", pitchNames[degree])
                  + QString::number(octave);

            if (ds) {
                  noteName = qApp->translate(
                        "drumset",
                        ds->name(instrPitch).toUtf8().constData());
                  }

            p.setBrush(keyCurrentColor(midiPitch, Qt::black));

            if (_orientation == PianoOrientation::HORIZONTAL) {
                  // Center each black key on the appropriate
                  // boundary between equal-width white keys
                  qreal octaveOffset =
                        (octave * 12 + transp.chromatic)
                        * noteHeight;

                  qreal center =
                        octaveOffset
                        + pianoRollBlackKeyBoundary(key)
                              * horizontalWhiteKeyWidth;

                  qreal offset =
                        center - noteHeight / 2.0;

                  QRectF rect(
                        -_ypos + offset,
                        0,
                        noteHeight,
                        blackKeyLen
                        );

                  p.drawRect(rect);

                  if (ds && noteHeight > fontSize + 2) {
                        p.save();

                        p.setPen(QPen(Qt::white));

                        p.translate(rect.left() + 1,
                                    rect.bottom() - 1);
                        p.rotate(-90.0);

                        QRectF rectText(
                              0.0,
                              0.0,
                              rect.height() - 2,
                              rect.width() - 2);

                        p.drawText(
                              rectText,
                              Qt::AlignLeft | Qt::AlignBottom,
                              noteName);

                        p.restore();
                        }
                  }
            else {
                  // Original vertical keyboard geometry:
                  qreal center =
                        blackKeyOffset[key] * noteHeight;

                  qreal offset =
                        center
                        - noteHeight / 2.0
                        + (octave * 12 + transp.chromatic)
                              * noteHeight;

                  QRectF rect(
                        0,
                        -_ypos
                              + keyboardLen
                              - offset
                              - noteHeight,
                        blackKeyLen,
                        noteHeight
                        );

                  p.drawRect(rect);

                  if (noteHeight > fontSize + 2) {
                        if (ds) {
                              p.setPen(QPen(Qt::white));
                              p.drawText(
                                    rect,
                                    Qt::AlignLeft | Qt::AlignBottom,
                                    noteName);
                              }
                        }
                  }
            }
      }

//---------------------------------------------------------
//   setYpos
//---------------------------------------------------------

void PianoKeyboard::setYpos(int val)
      {
      if (_ypos != val) {
            _ypos = val;
            update();
            }
      }

//---------------------------------------------------------
//   setNoteHeight
//---------------------------------------------------------

void PianoKeyboard::setNoteHeight(int nh)
      {
      if (noteHeight != nh) {
            noteHeight = nh;
            update();
            }
      }

//---------------------------------------------------------
//   setPitch
//---------------------------------------------------------

void PianoKeyboard::setPitch(int val)
      {
      if (curPitch != val) {
            curPitch = val;
            update();
            }
      }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void PianoKeyboard::mousePressEvent(QMouseEvent* event)
      {
      int offset = _orientation == PianoOrientation::HORIZONTAL
            ? event->pos().x()
            : 128 * noteHeight - (event->y() + _ypos);

      int pitch = curKeyPressed = offset / noteHeight;
      if (!pitchIsValid(pitch))
            pitch = -1;

      uint modifiers = QGuiApplication::keyboardModifiers();
      bool bnCtrl = modifiers & Qt::ControlModifier;
      if (bnCtrl)
            emit pitchHighlightToggled(curKeyPressed);
      else {
            curKeyPressed = pitch;
            emit keyPressed(curKeyPressed);
            }
      }

//---------------------------------------------------------
//   mouseReleaseEvent
//---------------------------------------------------------

void PianoKeyboard::mouseReleaseEvent(QMouseEvent*)
      {
      if (curKeyPressed != -1) {
            emit keyReleased(curKeyPressed);
            curKeyPressed = -1;
            }
      }

//---------------------------------------------------------
//   mouseMoveEvent
//---------------------------------------------------------

void PianoKeyboard::mouseMoveEvent(QMouseEvent* event)
      {
      int offset = _orientation == PianoOrientation::HORIZONTAL
            ? event->pos().x()
            : 128 * noteHeight - (event->y() + _ypos);

      int pitch = offset / noteHeight;

      if (!pitchIsValid(pitch)) {
            if (curKeyPressed != -1) {
                  emit keyReleased(curKeyPressed);
                  curKeyPressed = -1;
                  }

            if (curPitch != -1) {
                  curPitch = -1;
                  update();
                  }

            return;
            }

      if (pitch != curPitch) {
            curPitch = pitch;

            //Set tooltip
            int degree = curPitch % 12;
            int octave = curPitch / 12;
            QString text = qApp->translate("utils", pitchNames[degree]) + QString::number(octave - 1);
            Part* part = _staff->part();
            Drumset* ds = part->instrument()->drumset();
            if (ds)
                  text += " - " + qApp->translate("drumset", ds->name(curPitch).toUtf8().constData());

            setToolTip(text);

            //Send event
            emit pitchChanged(curPitch);
            if ((curKeyPressed != -1) && (curKeyPressed != pitch)) {
                  emit keyReleased(curKeyPressed);
                  curKeyPressed = pitch;
                  emit keyPressed(curKeyPressed);
                  }
            update();
            }
      }

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void PianoKeyboard::leaveEvent(QEvent*)
      {
      if (curPitch != -1) {
            curPitch = -1;
            emit pitchChanged(-1);
            update();
            }
      }


//---------------------------------------------------------
//   setStaff
//---------------------------------------------------------

void PianoKeyboard::setStaff(Staff* staff)
      {
      _staff = staff;
      update();
      }

//---------------------------------------------------------
//   setOrientation
//---------------------------------------------------------

void PianoKeyboard::setOrientation(PianoOrientation o)
      {
      _orientation = o;
      update();
      }

//---------------------------------------------------------
//   setColoring
//---------------------------------------------------------

void PianoKeyboard::setColoring(Coloring c)
      {
      if (_coloring == c)
            return;

      _coloring = c;
      update();
      }

//---------------------------------------------------------
//   setUseNoteColors
//---------------------------------------------------------

void PianoKeyboard::setUseNoteColors(bool value)
      {
      _useNoteColors = value;
      }

//---------------------------------------------------------
//   setSelectionNotes
//---------------------------------------------------------

void PianoKeyboard::setSelectionNotes(
      const QHash<int, const Note*>& notes)
      {
      if (_selectionNotes == notes)
            return;

      _selectionNotes = notes;
      update();
      }

//---------------------------------------------------------
//   setPlaybackNotes
//---------------------------------------------------------

void PianoKeyboard::setPlaybackNotes(const QHash<int, const Note*>& notes)
      {
      if (_playbackNotes == notes)
            return;

      _playbackNotes = notes;
      update();
      }
}
