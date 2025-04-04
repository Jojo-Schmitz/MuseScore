//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2018 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#ifndef __IMPORTMXMLNOTEPITCH_H__
#define __IMPORTMXMLNOTEPITCH_H__

#include "libmscore/accidental.h"

namespace Ms {

class MxmlLogger;
class Score;

//---------------------------------------------------------
//   mxmlNotePitch
//---------------------------------------------------------

/**
 Parse the note pitch related part of the /score-partwise/part/measure/note node.
 */

class mxmlNotePitch
      {
public:
      mxmlNotePitch(MxmlLogger* logger) : _logger(logger) { /* nothing so far */ }
      void pitch(QXmlStreamReader& e);
      bool readProperties(QXmlStreamReader& e, Score* score);
      Accidental* acc() const { return _acc; }
      AccidentalType accType() const { return _accType; }
      int alter() const { return _alter; }
      qreal tuning() const { return _tuning; }
      int displayOctave() const { return _displayOctave; }
      int displayStep() const { return _displayStep; }
      void displayStepOctave(QXmlStreamReader& e);
      int octave() const { return _octave; }
      int step() const { return _step; }
      bool unpitched() const { return _unpitched; }

private:
      Accidental* _acc = 0;                                 // created based on accidental element
      AccidentalType _accType = AccidentalType::NONE;       // set by pitch() based on alter value (can be microtonal)
      int _alter = 0;
      qreal _tuning = 0.0;
      int _displayStep = -1;                                // invalid
      int _displayOctave = -1;                              // invalid
      int _octave = -1;
      int _step = 0;
      bool _unpitched = false;
      MxmlLogger* _logger;                                  ///< Error logger
      };


} // namespace Ms

#endif
