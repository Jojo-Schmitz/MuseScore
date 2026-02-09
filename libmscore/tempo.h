//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2002-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#ifndef __AL_TEMPO_H__
#define __AL_TEMPO_H__

namespace Ms {

class XmlWriter;

enum class TempoType : char { INVALID = 0x0, PAUSE = 0x1, FIX = 0x2, RAMP = 0x4};

typedef QFlags<TempoType> TempoTypes;
Q_DECLARE_OPERATORS_FOR_FLAGS(TempoTypes);

//---------------------------------------------------------
//   Tempo Event
//---------------------------------------------------------

struct TEvent {
      TempoTypes type = TempoType::INVALID;
      qreal tempo = 0.0;
      double pause = 0.0; // pause in seconds
      double time = 0.0;  // precomputed time for tick in sec

      TEvent() = default;
      TEvent(qreal bps, qreal pauseInSeconds, TempoType t);
      bool valid() const;
      };

class TempoMap : public std::map<int, TEvent> {
      qreal _tempo = 2.0;           // tempo if not using tempo list (beats per second)
      qreal _relTempo = 1.0;        // rel. tempo

      void normalize();

   public:
      TempoMap() = default;
      void clear();
      void clearRange(int tick1, int tick2);

      void dump() const;

      qreal tempo(int tick) const;

      qreal tick2time(int tick) const;
      int time2tick(qreal time) const;

      void setTempo(int t, qreal);
      void setPause(int t, qreal);
      void delTempo(int tick);

      void setRelTempo(qreal val);
      qreal relTempo() const { return _relTempo; }
      };

}     // namespace Ms
#endif
