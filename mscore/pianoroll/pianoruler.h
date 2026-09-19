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

#ifndef __PIANO_RULER_H__
#define __PIANO_RULER_H__

#include "libmscore/pos.h"
#include "pianoroll/pianorolledittool.h"
#include "pianoroll/pianoview.h"

namespace Ms {

class Score;
class PianoView;

static const int pianoRulerHeight = 28;
static const int MAP_OFFSET = 480;

//---------------------------------------------------------
//   PianoRuler
//---------------------------------------------------------

class PianoRuler : public QWidget {
      Q_OBJECT

      PianoView* _pianoView { nullptr };

      Score* _score { nullptr };
      Pos _cursor;
      Pos* _locator { nullptr };

      qreal _xZoom;
      int _xpos;
      TType _timeType;
      QFont _font1, _font2;

      PianoRollOrientation _orientation { PianoRollOrientation::HORIZONTAL };

      bool _loopEnabled { false };

      qreal _playbackLocatorTick { 0.0 };
      bool _playbackLocatorTickValid { false };

      static QPixmap* markIcon[3];

      virtual void paintEvent(QPaintEvent*);
      virtual void mousePressEvent(QMouseEvent*);
      virtual void mouseMoveEvent(QMouseEvent*);
      virtual void leaveEvent(QEvent*);

      void paintHorizontal(QPaintEvent*);
      void paintVertical(QPaintEvent*);

      Pos pix2pos(int) const;
      int pos2pix(const Pos& p) const;
      void moveLocator(QMouseEvent*);

   signals:
      void posChanged(const Pos&);
      void locatorMoved(int idx, const Pos&);

   public slots:
      void setXpos(int);
      void setXZoom(qreal);
      void setPos(const Pos&);

   public:
      PianoRuler(QWidget* parent = 0);
      void setScore(Score*, Pos* locator);
      int xpos() const { return _xpos; }
      qreal xZoom() const { return _xZoom; }

      void setPlaybackLocatorTick(qreal tick);
      void clearPlaybackLocatorTick();

      qreal tickToPixelF(qreal tick) const;

      void setOrientation(PianoRollOrientation);
      void setPianoView(PianoView*);

      bool playbackLocatorActive() const;

      void setLoopEnabled(bool);
      };


} // namespace Ms
#endif


