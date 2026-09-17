//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2009-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#ifndef __PIANOROLLEDITTOOL_H__
#define __PIANOROLLEDITTOOL_H__

#include <QColor>
#include <QVector>
#include <QHash>
#include <QSet>

#include "preferences.h"

namespace Ms {

enum PianoRollEditTool
{
      SELECT,
      ADD,
      CUT,
      ERASE,
      EVENT_ADJUST,
      TIE,

      LAST  //Marker for end of list - not a tool
      };

enum class PianoRollScope {
      STAFF,
      PART,
      SCORE
      };

enum class Coloring {
      VOICING,
      STAFF,
      INSTRUMENT,
      };

enum class PianoRollOrientation {
      UNDEFINED = -1,
      HORIZONTAL,
      VERTICAL
      };

enum class VerticalPitchLayout {
      CHROMATIC,
      KEYBOARD_ALIGNED
      };

inline int pianoRollTimeBucket(int tick, int bucketTicks)
      {
      if (tick >= 0)
            return tick / bucketTicks;

      // Integer division truncates toward zero.  For negative ticks
      // we instead need the mathematical floor so that, e.g., tick -1
      // belongs to bucket -1 rather than bucket 0
      return -((-tick + bucketTicks - 1) / bucketTicks);
      }

template<typename T>
void pianoRollAddToTimeBuckets(QHash<int, QVector<T>>& buckets,
                               T value,
                               int firstTick,
                               int lastTick,
                               int bucketTicks)
      {
      if (lastTick < firstTick)
            qSwap(firstTick, lastTick);

      const int firstBucket =
            pianoRollTimeBucket(firstTick, bucketTicks);
      const int lastBucket =
            pianoRollTimeBucket(lastTick, bucketTicks);

      for (int bucket = firstBucket; bucket <= lastBucket; ++bucket)
            buckets[bucket].append(value);
      }

template<typename T>
QVector<T> pianoRollTimeBucketCandidates(const QHash<int, QVector<T>>& buckets,
                                         int startTick,
                                         int endTick,
                                         int bucketTicks)
      {
      if (endTick < startTick)
            qSwap(startTick, endTick);

      QVector<T> candidates;
      QSet<T> seen;

      const int firstBucket =
            pianoRollTimeBucket(startTick, bucketTicks);
      const int lastBucket =
            pianoRollTimeBucket(endTick, bucketTicks);

      for (int bucket = firstBucket; bucket <= lastBucket; ++bucket) {
            auto it = buckets.constFind(bucket);

            if (it == buckets.constEnd())
                  continue;

            for (T value : it.value()) {
                  if (seen.contains(value))
                        continue;

                  seen.insert(value);
                  candidates.append(value);
                  }
            }

      return candidates;
      }

class Note;
class Staff;

QVector<int> pianoRollScopeTracks(Staff* staff, PianoRollScope scope);

QColor pianoRollThemeColor(const QString& darkKey,
                           const QString& lightKey);

QColor pianoRollNoteColor(const Note* note,
                          Coloring coloring,
                          bool honorSelection,
                          bool honorCustomColor);

bool darkTheme();

bool darkTheme();

} // namespace Ms

#endif

