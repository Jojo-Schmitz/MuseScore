//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2011 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENSE.GPL
//=============================================================================

#include "libmscore/measure.h"
#include "libmscore/repeatlist.h"
#include "libmscore/score.h"
#include "libmscore/segment.h"
#include "libmscore/system.h"
#include "libmscore/xml.h"

#include "mscore/musescore.h"
#include "mscore/preferences.h"

namespace Ms {

static QHash<void*, int> segs;

//---------------------------------------------------------
//   saveMeasureEvents
//---------------------------------------------------------

static void saveMeasureEvents(XmlWriter& xml, Measure* m, int offset)
      {
      for (Segment* s = m->first(SegmentType::ChordRest); s; s = s->next(SegmentType::ChordRest)) {
            int tick = s->tick().ticks() + offset;
            int id = segs[(void*)s];
            int time = (int)lrint(m->score()->repeatList().utick2utime(tick) * 1000);
            xml.tagE(QString("event elid=\"%1\" position=\"%2\"")
               .arg(id)
               .arg(time)
               );
            }
      }

//---------------------------------------------------------
//   savePositions
//    output in 100 dpi
//---------------------------------------------------------

bool MuseScore::savePositions(Score* score, QIODevice* device, bool segments)
      {
      segs.clear();
      XmlWriter xml(score, device);
      xml.header();
      xml.stag("score");
      xml.stag("elements");
      int id = 0;

      qreal ndpi = ((qreal) preferences.getDouble(PREF_EXPORT_PNG_RESOLUTION) / DPI) * 12.0;
      if (segments) {
            Measure* m = score->firstMeasureMM();
            for (Segment* s = (m ? m->first(SegmentType::ChordRest) : nullptr);
               s; s = s->next1MM(SegmentType::ChordRest)) {
                  qreal sx   = 0;
                  int tracks = score->nstaves() * VOICES;
                  for (int track = 0; track < tracks; track++) {
                        Element* e = s->element(track);
                        if (e)
                              sx = qMax(sx, e->width());
                        }

                  sx      *= ndpi;
                  int sy   = s->measure()->system()->height() * ndpi;
                  int x    = s->pagePos().x() * ndpi;
                  int y    = s->pagePos().y() * ndpi;

                  Page* p  = s->measure()->system()->page();
                  int page = score->pageIdx(p);

                  xml.tagE(QString("element id=\"%1\" x=\"%2\" y=\"%3\" sx=\"%4\""
                  " sy=\"%5\" page=\"%6\"")
                     .arg(id)
                     .arg(x)
                     .arg(y)
                     .arg(sx)
                     .arg(sy)
                     .arg(page));

                  segs[(void*)s] = id++;
                  }
            xml.etag();
            }
      else {
            for (Measure* m = score->firstMeasureMM(); m; m = m->nextMeasureMM()) {
                  qreal sx   = m->bbox().width() * ndpi;
                  qreal sy   = m->system()->height() * ndpi;
                  qreal x    = m->pagePos().x() * ndpi;
                  qreal y    = m->system()->pagePos().y() * ndpi;

                  Page* p  = m->system()->page();
                  int page = score->pageIdx(p);

                  xml.tagE(QString("element id=\"%1\" x=\"%2\" y=\"%3\" sx=\"%4\""
                  " sy=\"%5\" page=\"%6\"")
                     .arg(id)
                     .arg(x)
                     .arg(y)
                     .arg(sx)
                     .arg(sy)
                     .arg(page));

                  segs[(void*)m] = id++;
                  }
            xml.etag();
            }

      xml.stag("events");
      score->masterScore()->setExpandRepeats(true);
      for (const RepeatSegment* rs : score->repeatList()) {
            int startTick  = rs->tick;
            int endTick    = startTick + rs->len();
            int tickOffset = rs->utick - rs->tick;
            for (Measure* m = score->tick2measureMM(Fraction::fromTicks(startTick)); m; m = m->nextMeasureMM()) {
                        if (segments)
                              saveMeasureEvents(xml, m, tickOffset);
                        else {
                              int tick = m->tick().ticks() + tickOffset;
                              int i = segs[(void*)m];
                              int time = (int)lrint(m->score()->repeatList().utick2utime(tick) * 1000);
                              xml.tagE(QString("event elid=\"%1\" position=\"%2\"")
                                 .arg(i)
                                 .arg(time)
                                 );
                              }
                  if (m->endTick().ticks() >= endTick)
                        break;
                  }
            }
      xml.etag();

      xml.etag(); // score
      return true;
      }

bool MuseScore::savePositions(Score* score, const QString& name, bool segments)
      {
      QFile fp(name);
      if (!fp.open(QIODevice::WriteOnly)) {
            qDebug("Open <%s> failed", qPrintable(name));
            return false;
            }
      return savePositions(score, &fp, segments);
      }
}

