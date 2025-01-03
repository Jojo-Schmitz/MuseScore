//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//
//  Copyright (C) 2002-2015 Werner Schweer and others
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

#ifndef __MUSICXML_H__
#define __MUSICXML_H__

/**
 \file
 Definition of class MusicXML
*/

#include "libmscore/line.h"
#include "libmscore/mscore.h"

namespace Ms {

//---------------------------------------------------------
//   MusicXmlPartGroup
//---------------------------------------------------------

struct MusicXmlPartGroup {
      int span;
      int start;
      BracketType type;
      bool barlineSpan;
      int column;
      };

const int MAX_LYRICS       = 16;
const int MAX_PART_GROUPS  = 8;
const int MAX_NUMBER_LEVEL = 16; // maximum number of overlapping MusicXML objects

//---------------------------------------------------------
//   CreditWords
//    a single parsed MusicXML credit-words element
//---------------------------------------------------------

struct CreditWords {
      int page;
      QString type;
      double defaultX;
      double defaultY;
      double fontSize;
      QString justify;
      QString hAlign;
      QString vAlign;
      QString words;
      CreditWords(int p, QString tp, double dx, double dy, double fs, QString j, QString ha, QString va, QString w)
            {
            page = p;
            type = tp;
            defaultX = dx;
            defaultY = dy;
            fontSize = fs;
            justify  = j;
            hAlign   = ha;
            vAlign   = va;
            words    = w;
            }
      };

typedef  QList<CreditWords*> CreditWordsList;
typedef  CreditWordsList::iterator iCreditWords;
typedef  CreditWordsList::const_iterator ciCreditWords;

//---------------------------------------------------------
//   JumpMarkerDesc
//---------------------------------------------------------

/**
 The description of Jumps and Markers to be added later
*/

class JumpMarkerDesc {
      Element* _el;
      Measure* _meas;

public:
      JumpMarkerDesc(Element* el, Measure* meas) : _el(el), _meas(meas) {}
      Element* el() const { return _el; }
      Measure* meas() const { return _meas; }
      };

typedef QList<JumpMarkerDesc> JumpMarkerDescList;

//---------------------------------------------------------
//   SlurDesc
//---------------------------------------------------------

/**
 The description of Slurs being handled
 */

class SlurDesc {
public:
      enum class State : char { NONE, START, STOP };
      SlurDesc() : _slur(0), _state(State::NONE) {}
      Slur* slur() const { return _slur; }
      void start(Slur* slur) { _slur = slur; _state = State::START; }
      void stop(Slur* slur) { _slur = slur; _state = State::STOP; }
      bool isStart() const { return _state == State::START; }
      bool isStop() const { return _state == State::STOP; }
private:
      Slur* _slur;
      State _state;
      };

// Ties are identified by the pitch and track of their first note
typedef std::pair<int, int> TieLocation;

//---------------------------------------------------------
//   MusicXml
//---------------------------------------------------------

typedef std::vector<MusicXmlPartGroup*> MusicXmlPartGroupList;
typedef QMap<SLine*, QPair<int, int> > MusicXmlSpannerMap;

} // namespace Ms
#endif
