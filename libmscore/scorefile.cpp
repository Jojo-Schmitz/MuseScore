//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2011-2012 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENSE.GPL
//=============================================================================

#include "audio.h"
#include "barline.h"
#include "beam.h"
#include "chordrest.h"
#include "clef.h"
#include "config.h"
#include "element.h"
#include "excerpt.h"
#include "imageStore.h"
#include "keysig.h"
#include "mscore.h"
#include "measure.h"
#include "revisions.h"
#include "page.h"
#include "part.h"
#include "rest.h"
#include "score.h"
#include "scoreOrder.h"
#include "segment.h"
#include "sig.h"
#include "staff.h"
#include "stafftype.h"
#include "system.h"
#include "tuplet.h"
#include "undo.h"
#include "volta.h"
#include "xml.h"

#include "mscore/preferences.h"

#ifdef OMR
#include "omr/omr.h"
#include "omr/omrpage.h"
#endif

#ifdef AVSOMR
#include "avsomr/msmrwriter.h"
#endif

#include "thirdparty/qzip/qzipreader_p.h"
#include "thirdparty/qzip/qzipwriter_p.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace Ms {

//---------------------------------------------------------
//   writeMeasure
//---------------------------------------------------------

static void writeMeasure(XmlWriter& xml, MeasureBase* m, int staffIdx, bool writeSystemElements, bool forceTimeSig)
      {
      //
      // special case multi measure rest
      //
      if (m->isMeasure() || staffIdx == 0)
            m->write(xml, staffIdx, writeSystemElements, forceTimeSig);

      if (m->score()->styleB(Sid::createMultiMeasureRests) && m->isMeasure() && toMeasure(m)->mmRest())
            toMeasure(m)->mmRest()->write(xml, staffIdx, writeSystemElements, forceTimeSig);

      xml.setCurTick(m->endTick());
      }

//---------------------------------------------------------
//   writeMovement
//---------------------------------------------------------

void Score::writeMovement(XmlWriter& xml, bool selectionOnly)
      {
      // if we have multi measure rests and some parts are hidden,
      // then some layout information is missing:
      // relayout with all parts set visible

      QList<Part*> hiddenParts;
      bool unhide = false;
      if (styleB(Sid::createMultiMeasureRests)) {
            for (Part* part : qAsConst(_parts)) {
                  if (!part->show()) {
                        if (!unhide) {
                              startCmd();
                              unhide = true;
                              }
                        part->undoChangeProperty(Pid::VISIBLE, true);
                        hiddenParts.append(part);
                        }
                  }
            }
      if (unhide) {
            doLayout();
            for (Part* p : hiddenParts)
                  p->setShow(false);
            }

      xml.stag(this);
      if (excerpt()) {
            Excerpt* e = excerpt();
            QMultiMap<int, int> trackList = e->tracks();
            QMapIterator<int, int> i(trackList);
            if (!(trackList.size() == e->nstaves() * VOICES) && !trackList.isEmpty()) {
                  while (i.hasNext()) {
                      i.next();
                      xml.tagE(QString("Tracklist sTrack=\"%1\" dstTrack=\"%2\"").arg(i.key()).arg(i.value()));
                      }
                  }
            }

      if (lineMode())
            xml.tag("layoutMode", "line");
      if (systemMode())
            xml.tag("layoutMode", "system");

#ifdef OMR
      if (masterScore()->omr() && xml.writeOmr())
            masterScore()->omr()->write(xml);
#endif
      if (isMaster() && masterScore()->showOmr() && xml.writeOmr())
            xml.tag("showOmr", masterScore()->showOmr());
      if (_audio && xml.writeOmr()) {
            xml.tag("playMode", int(_playMode));
            _audio->write(xml);
            }

      for (int i = 0; i < 32; ++i) {
            if (!_layerTags[i].isEmpty()) {
                  xml.tag(QString("LayerTag id=\"%1\" tag=\"%2\"").arg(i).arg(_layerTags[i]),
                     _layerTagComments[i]);
                  }
            }
      int n = _layer.size();
      for (int i = 1; i < n; ++i) {       // don’t save default variant
            const Layer& l = _layer[i];
            xml.tagE(QString("Layer name=\"%1\" mask=\"%2\"").arg(l.name).arg(l.tags));
            }
      xml.tag("currentLayer", _currentLayer);

      if (isTopScore() && !MScore::testMode)
            _synthesizerState.write(xml);

      if (pageNumberOffset())
            xml.tag("page-offset", pageNumberOffset());
      xml.tag("Division", MScore::division);
      xml.setCurTrack(-1);

      if (isTopScore())                    // only top score
            style().save(xml, true);       // save only differences to buildin style

      xml.tag("showInvisible",   _showInvisible);
      xml.tag("showUnprintable", _showUnprintable);
      xml.tag("showFrames",      _showFrames);
      xml.tag("showMargins",     _showPageborders);
      xml.tag("markIrregularMeasures", _markIrregularMeasures, true);

      QMapIterator<QString, QString> i(_metaTags);
      while (i.hasNext()) {
            i.next();
            // do not output "platform" and "creationDate" in test and save template mode
            if ((!MScore::testMode && !MScore::saveTemplateMode) || (i.key() != "platform" && i.key() != "creationDate"))
                  xml.tag(QString("metaTag name=\"%1\"").arg(i.key().toHtmlEscaped()), i.value());
            }

      if (_scoreOrder && !_scoreOrder->isCustom()) {
            ScoreOrder* order = _scoreOrder->clone();
            order->updateInstruments(this);
            order->write(xml);
            delete order;
            }

      xml.setCurTrack(0);
      int staffStart;
      int staffEnd;
      MeasureBase* measureStart;
      MeasureBase* measureEnd;

      if (selectionOnly) {
            staffStart   = _selection.staffStart();
            staffEnd     = _selection.staffEnd();
            // make sure we select full parts
            Staff* sStaff = staff(staffStart);
            Part* sPart = sStaff->part();
            Staff* eStaff = staff(staffEnd - 1);
            Part* ePart = eStaff->part();
            staffStart = staffIdx(sPart);
            staffEnd = staffIdx(ePart) + ePart->nstaves();
            measureStart = _selection.startSegment()->measure();
            if (measureStart->isMeasure() && toMeasure(measureStart)->isMMRest())
                  measureStart = toMeasure(measureStart)->mmRestFirst();
            if (_selection.endSegment())
                  measureEnd   = _selection.endSegment()->measure()->next();
            else
                  measureEnd   = 0;
            }
      else {
            staffStart   = 0;
            staffEnd     = nstaves();
            measureStart = first();
            measureEnd   = 0;
            }

      // Let's decide: write midi mapping to a file or not
      masterScore()->checkMidiMapping();
      for (const Part* part : qAsConst(_parts)) {
            if (!selectionOnly || ((staffIdx(part) >= staffStart) && (staffEnd >= staffIdx(part) + part->nstaves())))
                  part->write(xml);
            }

      xml.setCurTrack(0);
      xml.setTrackDiff(-staffStart * VOICES);
      if (measureStart) {
            for (int staffIdx = staffStart; staffIdx < staffEnd; ++staffIdx) {
                  xml.stag(staff(staffIdx), QString("id=\"%1\"").arg(staffIdx + 1 - staffStart));
                  xml.setCurTick(measureStart->tick());
                  xml.setTickDiff(xml.curTick());
                  xml.setCurTrack(staffIdx * VOICES);
                  bool writeSystemElements = (staffIdx == staffStart);
                  bool firstMeasureWritten = false;
                  bool forceTimeSig = false;
                  for (MeasureBase* m = measureStart; m != measureEnd; m = m->next()) {
                        // force timesig if first measure and selectionOnly
                        if (selectionOnly && m->isMeasure()) {
                              if (!firstMeasureWritten) {
                                    forceTimeSig = true;
                                    firstMeasureWritten = true;
                                    }
                              else
                                    forceTimeSig = false;
                              }
                        writeMeasure(xml, m, staffIdx, writeSystemElements, forceTimeSig);
                        }
                  xml.etag();
                  }
            }
      xml.setCurTrack(-1);
      if (isMaster()) {
            if (!selectionOnly) {
                  for (const Excerpt* excerpt : excerpts()) {
                        if (excerpt->partScore() != this)
                              excerpt->partScore()->write(xml, false);       // recursion
                        }
                  }
            }
      else
            xml.tag("name", excerpt()->title());
      xml.etag();

      if (unhide)
            endCmd(true);
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Score::write(XmlWriter& xml, bool selectionOnly)
      {
      if (isMaster()) {
            MasterScore* score = static_cast<MasterScore*>(this);

            while (score->prev())
                  score = score->prev();
            while (score) {
                  score->writeMovement(xml, selectionOnly);
                  score = score->next();
                  }
            }
      else
            writeMovement(xml, selectionOnly);
      }

//---------------------------------------------------------
//   Staff
//---------------------------------------------------------

void Score::readStaff(XmlReader& e)
      {
      int staff = e.intAttribute("id", 1) - 1;
      int measureIdx = 0;
      e.setCurrentMeasureIndex(0);
      e.setTick(Fraction(0,1));
      e.setTrack(staff * VOICES);

      if (staff == 0) {
            while (e.readNextStartElement()) {
                  const QStringRef& tag(e.name());

                  if (tag == "Measure") {
                        Measure* measure = 0;
                        measure = new Measure(this);
                        measure->setTick(e.tick());
                        e.setCurrentMeasureIndex(measureIdx++);
                        //
                        // inherit timesig from previous measure
                        //
                        Measure* m = e.lastMeasure(); // measure->prevMeasure();
                        Fraction f(m ? m->timesig() : Fraction(4,4));
                        measure->setTicks(f);
                        measure->setTimesig(f);

                        measure->read(e, staff);
                        measure->checkMeasure(staff);
                        if (!measure->isMMRest()) {
                              measures()->add(measure);
                              e.setLastMeasure(measure);
                              e.setTick(measure->tick() + measure->ticks());
                              }
                        else {
                              // this is a multi measure rest
                              // always preceded by the first measure it replaces
                              Measure* m1 = e.lastMeasure();

                              if (m1) {
                                    m1->setMMRest(measure);
                                    measure->setTick(m1->tick());
                                    }
                              }
                        }
                  else if (tag == "HBox" || tag == "VBox" || tag == "TBox" || tag == "FBox") {
                        MeasureBase* mb = toMeasureBase(Element::name2Element(tag, this));
                        mb->read(e);
                        mb->setTick(e.tick());
                        measures()->add(mb);
                        }
                  else if (tag == "tick")
                        e.setTick(Fraction::fromTicks(fileDivision(e.readInt())));
                  else
                        e.unknown();
                  }
            }
      else {
            Measure* measure = firstMeasure();
            while (e.readNextStartElement()) {
                  const QStringRef& tag(e.name());

                  if (tag == "Measure") {
                        if (measure == 0) {
                              qDebug("Score::readStaff(): missing measure!");
                              measure = new Measure(this);
                              measure->setTick(e.tick());
                              measures()->add(measure);
                              }
                        e.setTick(measure->tick());
                        e.setCurrentMeasureIndex(measureIdx++);
                        measure->read(e, staff);
                        measure->checkMeasure(staff);
                        if (measure->isMMRest())
                              measure = e.lastMeasure()->nextMeasure();
                        else {
                              e.setLastMeasure(measure);
                              if (measure->mmRest())
                                    measure = measure->mmRest();
                              else
                                    measure = measure->nextMeasure();
                              }
                        }
                  else if (tag == "tick")
                        e.setTick(Fraction::fromTicks(fileDivision(e.readInt())));
                  else
                        e.unknown();
                  }
            }
      }

//---------------------------------------------------------
//   saveFile
///   If file has generated name, create a modal file save dialog
///   and ask filename.
///   Rename old file to backup file (.xxxx.msc?,).
///   Default is to save score in .mscz format,
///   Return true if OK and false on error.
//---------------------------------------------------------

bool MasterScore::saveFile(bool generateBackup)
      {
      if (readOnly())
            return false;
      QString suffix = info.suffix();
      if (info.exists() && !info.isWritable()) {
            MScore::lastError = tr("The following file is locked: \n%1 \n\nTry saving to a different location.").arg(info.filePath());
            return false;
            }
      //
      // step 1
      // save into temporary file to prevent partially overwriting
      // the original file in case of "disc full"
      //

      QString tempName = info.filePath() + QString(".temp");
      QFile temp(tempName);
      if (!temp.open(QIODevice::WriteOnly)) {
            MScore::lastError = tr("Open Temp File\n%1\nfailed: %2").arg(tempName, strerror(errno));
            return false;
            }

      bool rv = false;
      if ("mscx" == suffix) {
           rv = Score::saveFile(&temp, false);
            }
#ifdef AVSOMR
      else if ("msmr" == suffix) {
            Avs::MsmrWriter msmrWriter;
            rv = msmrWriter.saveMsmrFile(this, &temp, info);
            }
#endif
      else {
           QString fileName = info.completeBaseName() + ".mscx";
           rv = Score::saveCompressedFile(&temp, fileName, false);
            }

      if (!rv) {
            return false;
            }

      if (temp.error() != QFile::NoError) {
            MScore::lastError = tr("Save File failed: %1").arg(temp.errorString());
            return false;
            }
      temp.close();

      const QString name(info.filePath());
      const QString basename(info.fileName());
      QDir dir(info.path());
      if (!saved() && generateBackup) {
            // if file was already saved in this session
            // save but don't overwrite backup again

            const QString backupSubdirString = preferences.getString(PREF_APP_BACKUP_SUBFOLDER);
            const QString backupDirString = info.path() + QString(QDir::separator()) + backupSubdirString;
            QDir backupDir(backupDirString);
            if (!backupDir.exists()) {
                  dir.mkdir(backupSubdirString);
#ifdef Q_OS_WIN
                  const QString backupDirNativePath = QDir::toNativeSeparators(backupDirString);
                  SetFileAttributesW(reinterpret_cast<LPCWSTR>(backupDirNativePath.utf16()), FILE_ATTRIBUTE_HIDDEN);
#endif
                  }
            const QString backupName = QString(".") + info.fileName() + QString(",");
            if (backupDir.exists(backupName)) {
                  if (!backupDir.remove(backupName)) {
//                      if (!MScore::noGui)
//                            QMessageBox::critical(0, QObject::tr("Save File"),
//                               tr("Removing old backup file %1 failed").arg(backupName));
                        }
                  }
            //
            // step 2
            // backup files prior to 3.5 were saved in the same directory as the file itself.
            // remove these old backup files if needed
            //
            if (dir != backupDir && dir.exists(backupName)) {
                  if (!dir.remove(backupName)) {
//                      if (!MScore::noGui)
//                            QMessageBox::critical(0, QObject::tr("Save File"),
//                               tr("Removing old backup file %1 failed").arg(backupName));
                        }
                  }

            //
            // step 3
            // rename old file into backup
            //
            if (dir.exists(basename)) {
                  if (!QFile::rename(name, backupDirString + (backupDirString.endsWith("/") ? "" : "/") + backupName)) {
//                      if (!MScore::noGui)
//                            QMessageBox::critical(0, tr("Save File"),
//                               tr("Renaming old file <%1> to backup <%2> failed").arg(name, backupDirString + "/" + backupName);
                        }
                  }

            QFileInfo fileBackup(backupDir, backupName);
            _sessionStartBackupInfo = fileBackup;
            }
      else {
            // file has previously been saved - remove the old file
            if (dir.exists(basename)) {
                  if (!dir.remove(basename)) {
//                      if (!MScore::noGui)
//                            QMessageBox::critical(0, tr("Save File"),
//                               tr("Removing old file %1 failed").arg(name));
                        }
                  }
            }

      //
      // step 4
      // rename temp name into file name
      //
      if (!QFile::rename(tempName, name)) {
            MScore::lastError = tr("Renaming temp. file <%1> to <%2> failed:\n%3").arg(tempName, name, strerror(errno));
            return false;
            }
      // make file readable by all
      QFile::setPermissions(name, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser
         | QFile::ReadGroup | QFile::ReadOther);

      undoStack()->setClean();
      setSaved(true);
      info.refresh();
      update();
      return true;
      }

//---------------------------------------------------------
//   saveCompressedFile
//---------------------------------------------------------

bool Score::saveCompressedFile(QFileInfo& info, bool onlySelection, bool createThumbnail)
      {
      if (readOnly() && info == *masterScore()->fileInfo())
            return false;
      QFile fp(info.filePath());
      if (!fp.open(QIODevice::WriteOnly)) {
            MScore::lastError = tr("Open File\n%1\nfailed: %2").arg(info.filePath(), strerror(errno));
            return false;
            }

      QString fileName = info.completeBaseName() + ".mscx";
      return saveCompressedFile(&fp, fileName, onlySelection, createThumbnail);
      }

//---------------------------------------------------------
//   createThumbnail
//---------------------------------------------------------

QImage Score::createThumbnail()
      {
      LayoutMode mode = layoutMode();
      setLayoutMode(LayoutMode::PAGE);
      doLayout();

      Page* page = pages().at(0);
      QRectF fr  = page->abbox();
      qreal mag  = 256.0 / qMax(fr.width(), fr.height());
      int w      = int(fr.width() * mag);
      int h      = int(fr.height() * mag);

      QImage pm(w, h, QImage::Format_ARGB32_Premultiplied);

      int dpm = (int)lrint(DPMM * 1000.0);
      pm.setDotsPerMeterX(dpm);
      pm.setDotsPerMeterY(dpm);
      pm.fill(0xffffffff);

      double pr = MScore::pixelRatio;
      MScore::pixelRatio = 1.0;

      QPainter p(&pm);
      p.setRenderHint(QPainter::Antialiasing, true);
      p.setRenderHint(QPainter::TextAntialiasing, true);
      p.scale(mag, mag);
      print(&p, 0);
      p.end();

      MScore::pixelRatio = pr;

      if (layoutMode() != mode) {
            setLayoutMode(mode);
            doLayout();
            }
      return pm;
      }

//---------------------------------------------------------
//   saveCompressedFile
//    file is already opened
//---------------------------------------------------------

bool Score::saveCompressedFile(QIODevice* f, const QString& fn, bool onlySelection, bool doCreateThumbnail)
      {
      MQZipWriter uz(f);

      QBuffer cbuf;
      cbuf.open(QIODevice::ReadWrite);
      XmlWriter xml(this, &cbuf);
      xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
      xml.stag("container");
      xml.stag("rootfiles");
      xml.stag(QString("rootfile full-path=\"%1\"").arg(XmlWriter::xmlString(fn)));
      xml.etag();
      for (ImageStoreItem* ip : imageStore) {
            if (!ip->isUsed(this))
                  continue;
            QString path = QString("Pictures/") + ip->hashName();
            xml.tag("file", path);
            }

      xml.etag();
      xml.etag();
      cbuf.seek(0);
      //uz.addDirectory("META-INF");
      uz.addFile("META-INF/container.xml", cbuf.data());

      QBuffer dbuf;
      dbuf.open(QIODevice::ReadWrite);
      saveFile(&dbuf, true, onlySelection);
      dbuf.seek(0);
      uz.addFile(fn, dbuf.data());

      QFileDevice* fd = dynamic_cast<QFileDevice*>(f);
      if (fd) // if is file (may be buffer)
            fd->flush(); // flush to preserve score data in case of
                         // any failures on the further operations.

      // save images
      //uz.addDirectory("Pictures");
      for (ImageStoreItem* ip : imageStore) {
            if (!ip->isUsed(this))
                  continue;
            QString path = QString("Pictures/") + ip->hashName();
            uz.addFile(path, ip->buffer());
            }

      // create thumbnail
      if (doCreateThumbnail && !pages().isEmpty()) {
            QImage pm = createThumbnail();

            QByteArray ba;
            QBuffer b(&ba);
            if (!b.open(QIODevice::WriteOnly))
                  qDebug("open buffer failed");
            if (!pm.save(&b, "PNG"))
                  qDebug("save failed");
            uz.addFile("Thumbnails/thumbnail.png", ba);
            }

#ifdef OMR
      //
      // save OMR page images
      //
      if (masterScore()->omr()) {
            int n = masterScore()->omr()->numPages();
            for (int i = 0; i < n; ++i) {
                  QString path = QString("OmrPages/page%1.png").arg(i+1);
                  QBuffer cbuf1;
                  OmrPage* page = masterScore()->omr()->page(i);
                  const QImage& image = page->image();
                  if (!image.save(&cbuf1, "PNG")) {
                        MScore::lastError = tr("Save file: cannot save image (%1x%2)").arg(image.width(), image.height());
                        return false;
                        }
                  uz.addFile(path, cbuf1.data());
                  cbuf1.close();
                  }
            }
#endif
      //
      // save audio
      //
      if (_audio)
            uz.addFile("audio.ogg", _audio->data());

      uz.close();
      return true;
      }

//---------------------------------------------------------
//   saveFile
//    return true on success
//---------------------------------------------------------

bool Score::saveFile(QFileInfo& info)
      {
      if (readOnly() && info == *masterScore()->fileInfo())
            return false;
      if (info.suffix().isEmpty())
            info.setFile(info.filePath() + ".mscx");
      QFile fp(info.filePath());
      if (!fp.open(QIODevice::WriteOnly)) {
            MScore::lastError = tr("Open File\n%1\nfailed: %2").arg(info.filePath(), strerror(errno));
            return false;
            }
      saveFile(&fp, false, false);
      fp.close();
      return true;
      }

//---------------------------------------------------------
//   loadStyle
//---------------------------------------------------------

bool Score::loadStyle(const QString& fn, bool ign, const bool overlap)
      {
      QFile f(fn);
      if (f.open(QIODevice::ReadOnly)) {
            MStyle st = style();
            if (st.load(&f, ign)) {
                  undo(new ChangeStyle(this, st, overlap));
                  return true;
                  }
             else {
                  MScore::lastError = QObject::tr("The style file is not compatible with this version of MuseScore.");
                  return false;
                  }
            }
      MScore::lastError = strerror(errno);
      return false;
      }

//---------------------------------------------------------
//   saveStyle
//---------------------------------------------------------

bool Score::saveStyle(const QString& name)
      {
      QString ext(".mss");
      QFileInfo info(name);

      if (info.suffix().isEmpty())
            info.setFile(info.filePath() + ext);
      QFile f(info.filePath());
      if (!f.open(QIODevice::WriteOnly)) {
            MScore::lastError = tr("Open Style File\n%1\nfailed: %2").arg(info.filePath(), strerror(errno));
            return false;
            }

      XmlWriter xml(this, &f);
      xml.header();
      xml.stag("museScore version=\"" MSC_VERSION "\"");
      style().save(xml, false);     // save complete style
      xml.etag();
      if (f.error() != QFile::NoError) {
            MScore::lastError = QObject::tr("Write Style failed: %1").arg(f.errorString());
            return false;
            }
      return true;
      }

//---------------------------------------------------------
//   saveFile
//    return true on success
//---------------------------------------------------------

extern QString revision;

bool Score::saveFile(QIODevice* f, bool msczFormat, bool onlySelection)
      {
      XmlWriter xml(this, f);
      xml.setWriteOmr(msczFormat);
      xml.header();

      xml.stag("museScore version=\"" MSC_VERSION "\"");

      if (!MScore::testMode) {
            xml.tag("programVersion", "3.6.3");
            xml.tag("programRevision", revision);
            }
      write(xml, onlySelection);
      xml.etag();
      if (isMaster())
            masterScore()->revisions()->write(xml);
      if (!onlySelection) {
            //update version values for i.e. plugin access
            _mscoreVersion = "3.6.3";
            _mscoreRevision = revision.toInt(0, 16);
            _mscVersion = MSCVERSION;
            }
      return true;
      }

//---------------------------------------------------------
//   readRootFile
//---------------------------------------------------------

QString readRootFile(MQZipReader* uz, QList<QString>& images)
      {
      QString rootfile;

      QByteArray cbuf = uz->fileData("META-INF/container.xml");
      if (cbuf.isEmpty()) {
            qDebug("can't find container.xml");
            return rootfile;
            }

      XmlReader e(cbuf);

      while (e.readNextStartElement()) {
            if (e.name() != "container") {
                  e.unknown();
                  continue;
                  }
            while (e.readNextStartElement()) {
                  if (e.name() != "rootfiles") {
                        e.unknown();
                        continue;
                        }
                  while (e.readNextStartElement()) {
                        const QStringRef& tag(e.name());

                        if (tag == "rootfile") {
                              QString file = e.attribute("full-path");

                              // pick first .mscx (or .xml, for workspaces) from a .mscz (resp. .workspace), works for Mu4 too.
                              if (rootfile.isEmpty() && (file.endsWith(".mscx") || file.endsWith(".xml"))) {
                                    rootfile = file;
                                    }

                              // 4.x images are recorded as rootfile items, not as file items
                              if (file.startsWith("Pictures/")) {
                                    images.append(file);
                                    }
                              e.skipCurrentElement();
                              }
                        else if (tag == "file")
                              images.append(e.readElementText());
                        else
                              e.unknown();
                        }
                  }
            }
      return rootfile;
      }

//---------------------------------------------------------
//   loadCompressedMsc
//    return false on error
//---------------------------------------------------------

Score::FileError MasterScore::loadCompressedMsc(QIODevice* io, bool ignoreVersionError)
      {
      MQZipReader uz(io);

      QList<QString> images;
      QString rootfile = readRootFile(&uz, images);
      if (rootfile.isEmpty())
            return FileError::FILE_NO_ROOTFILE;

      //
      // load images
      //
      if (!MScore::noImages) {
            for (const QString& s : images) {
                  QByteArray dbuf = uz.fileData(s);
                  imageStore.add(s, dbuf);
                  }
            }

      QByteArray dbuf = uz.fileData(rootfile);
      if (dbuf.isEmpty()) {
            QVector<MQZipReader::FileInfo> fil = uz.fileInfoList();
            for (const MQZipReader::FileInfo& fi : fil) {
                  if (fi.filePath.endsWith(".mscx")) {
                        dbuf = uz.fileData(fi.filePath);
                        break;
                        }
                  }
            }

      QByteArray sbuf = uz.fileData("score_style.mss"); // exists in Mu4 scores only
      if (!sbuf.isEmpty() && ignoreVersionError) { // needs to be read before the actual score
            XmlReader el(sbuf);
            while (el.readNextStartElement()) {
                  if (el.name() == "museScore") {
                        QString version = el.attribute("version");
                        QStringList sl  = version.split('.');
                        int mscVersion  = sl[0].toInt() * 100 + sl[1].toInt();

                        while (el.readNextStartElement()) {
                              if (el.name() == "Style") { // should be the next element after "museScore"
                                    masterScore()->style().load(el, mscVersion);
                                    break; // no need to read any further
                                    }
                              el.unknown();
                              }
                        }
                  }
            }

      XmlReader e(dbuf);
      e.setDocName(masterScore()->fileInfo()->completeBaseName());

      FileError retval = read1(e, ignoreVersionError);

      QByteArray vbuf = uz.fileData("viewsettings.json"); // exists in Mu4 scores only
      if (!vbuf.isEmpty() && ignoreVersionError) {
            QJsonDocument doc = QJsonDocument::fromJson(vbuf);
            QJsonObject obj = doc.object();
            QString viewMode = obj["notation"].toObject()["viewMode"].toString();

            if (viewMode == "continuous_v")
                  masterScore()->setLayoutMode(LayoutMode::LINE);
            else if ( viewMode == "continuous_h")
                  masterScore()->setLayoutMode(LayoutMode::SYSTEM);
            else if (viewMode == "page") // not really neeed, is the default anyhow
                  masterScore()->setLayoutMode(LayoutMode::PAGE);
            else // may never happen
                  masterScore()->setLayoutMode(LayoutMode::FLOAT);
            }

#ifdef OMR
      //
      // load OMR page images
      //
      if (masterScore()->omr()) {
            int n = masterScore()->omr()->numPages();
            for (int i = 0; i < n; ++i) {
                  QString path = QString("OmrPages/page%1.png").arg(i+1);
                  QByteArray dbuf1 = uz.fileData(path);
                  OmrPage* page = masterScore()->omr()->page(i);
                  QImage image;
                  if (image.loadFromData(dbuf1, "PNG")) {
                        page->setImage(image);
                        }
                  else
                        qDebug("load image failed");
                  }
            }
#endif
      //
      //  read audio
      //
      if (audio()) {
            QByteArray dbuf1 = uz.fileData("audio.ogg");
            audio()->setData(dbuf1);
            }
      return retval;
      }

int MasterScore::styleDefaultByMscVersion(const int mscVer) const
      {
      constexpr int LEGACY_MSC_VERSION_V3 = 301;
      constexpr int LEGACY_MSC_VERSION_V2 = 206;
      constexpr int LEGACY_MSC_VERSION_V1 = 114;

      if (mscVer > LEGACY_MSC_VERSION_V2 && mscVer < MSCVERSION)
            return LEGACY_MSC_VERSION_V3;

      if (mscVer > LEGACY_MSC_VERSION_V1 && mscVer <= LEGACY_MSC_VERSION_V2)
            return LEGACY_MSC_VERSION_V2;

      if (mscVer <= LEGACY_MSC_VERSION_V1)
            return LEGACY_MSC_VERSION_V1;

      return MSCVERSION;
      }

int MasterScore::readStyleDefaultsVersion()
      {
      if (styleB(Sid::usePre_3_6_defaults))
            return style().defaultStyleVersion();

      XmlReader e(readToBuffer());
      e.setDocName(masterScore()->fileInfo()->completeBaseName());

      while (!e.atEnd()) {
            e.readNext();
            if (e.name() == "defaultsVersion")
                  return e.readInt();
            }

      return styleDefaultByMscVersion(mscVersion());
      }

//---------------------------------------------------------
//   loadMsc
//    return true on success
//---------------------------------------------------------

Score::FileError MasterScore::loadMsc(QString name, bool ignoreVersionError)
      {
      QFile f(name);
      if (!f.open(QIODevice::ReadOnly)) {
            MScore::lastError = f.errorString();
            return FileError::FILE_OPEN_ERROR;
            }
      return loadMsc(name, &f, ignoreVersionError);
      }

Score::FileError MasterScore::loadMsc(QString name, QIODevice* io, bool ignoreVersionError)
      {
      ScoreLoad sl;
      fileInfo()->setFile(name);

      if (name.endsWith(".mscz", Qt::CaseInsensitive) || name.endsWith(".mscz,", Qt::CaseInsensitive))
            return loadCompressedMsc(io, ignoreVersionError);
      else {
            XmlReader r(io);
            return read1(r, ignoreVersionError);
            }
      }

//---------------------------------------------------------
//   parseVersion
//---------------------------------------------------------

void MasterScore::parseVersion(const QString& val)
      {
      QRegExp re("(\\d+)\\.(\\d+)\\.(\\d+)");
      int v1, v2, v3, rv1, rv2, rv3;
      if (re.indexIn(VERSION) != -1) {
            QStringList sl = re.capturedTexts();
            if (sl.size() == 4) {
                  v1 = sl[1].toInt();
                  v2 = sl[2].toInt();
                  v3 = sl[3].toInt();
                  if (re.indexIn(val) != -1) {
                        sl = re.capturedTexts();
                        if (sl.size() == 4) {
                              rv1 = sl[1].toInt();
                              rv2 = sl[2].toInt();
                              rv3 = sl[3].toInt();

                              int currentVersion = v1 * 10000 + v2 * 100 + v3;
                              int readVersion = rv1 * 10000 + rv2 * 100 + rv3;
                              if (readVersion > currentVersion) {
                                    qDebug("read future version");
                                    }
                              }
                        }
                  else {
                        QRegExp re1("(\\d+)\\.(\\d+)");
                        if (re1.indexIn(val) != -1) {
                              sl = re.capturedTexts();
                              if (sl.size() == 3) {
                                    rv1 = sl[1].toInt();
                                    rv2 = sl[2].toInt();

                                    int currentVersion = v1 * 10000 + v2 * 100 + v3;
                                    int readVersion = rv1 * 10000 + rv2 * 100;
                                    if (readVersion > currentVersion) {
                                          qDebug("read future version");
                                          }
                                    }
                              }
                        else
                              qDebug("1cannot parse <%s>", qPrintable(val));
                        }
                  }
            }
      else
            qDebug("2cannot parse <%s>", VERSION);
      }

//---------------------------------------------------------
//   read1
//    return true on success
//---------------------------------------------------------

Score::FileError MasterScore::read1(XmlReader& e, bool ignoreVersionError)
      {
      while (e.readNextStartElement()) {
            if (e.name() == "museScore") {
                  const QString& version = e.attribute("version");
                  QStringList sl = version.split('.');
                  setMscVersion(sl[0].toInt() * 100 + (sl.size() > 1 ? sl[1].toInt() : 0));

                  if (!ignoreVersionError) {
                        if (mscVersion() > MSCVERSION)
                              return FileError::FILE_TOO_NEW;
                        if (mscVersion() < 114)
                              return FileError::FILE_TOO_OLD;
                        if (mscVersion() == 300)
                              return FileError::FILE_OLD_300_FORMAT;
                        }

                  if (created() && !preferences.getString(PREF_SCORE_STYLE_DEFAULTSTYLEFILE).isEmpty()) {
                        setStyle(MScore::defaultStyle());
                        }
                  else if (mscVersion() < 400) { // 4.x compat, style read already
                        int defaultsVersion = readStyleDefaultsVersion();

                        setStyle(*MStyle::resolveStyleDefaults(defaultsVersion));
                        style().setDefaultStyleVersion(defaultsVersion);
                        }

                  Score::FileError error;
                  if (mscVersion() <= 114)
                        error = read114(e);
                  else if (mscVersion() <= 207)
                        error = read206(e);
                  else
                        error = read302(e);
                  setExcerptsChanged(false);
                  return error;
                  }
            else
                  e.unknown();
            }
      return FileError::FILE_CORRUPTED;
      }

//---------------------------------------------------------
//   print
//---------------------------------------------------------

void Score::print(QPainter* painter, int pageNo)
      {
      _printing  = true;
      MScore::pdfPrinting = true;
      Page* page = pages().at(pageNo);
      QRectF fr  = page->abbox();

      QList<Element*> ell = page->items(fr);
      std::stable_sort(ell.begin(), ell.end(), elementLessThan);
      for (const Element* e : qAsConst(ell)) {
            if (!e->visible())
                  continue;
            painter->save();
            painter->translate(e->pagePos());
            e->draw(painter);
            painter->restore();
            }
      MScore::pdfPrinting = false;
      _printing = false;
      }

//---------------------------------------------------------
//   readCompressedToBuffer
//---------------------------------------------------------

QByteArray MasterScore::readCompressedToBuffer()
      {
      MQZipReader uz(info.filePath());
      if (!uz.exists()) {
            qDebug("Score::readCompressedToBuffer: cannot read zip file");
            return QByteArray();
            }
      QList<QString> images;
      QString rootfile = readRootFile(&uz, images);

      //
      // load images
      //
      foreach(const QString& s, images) {
            QByteArray dbuf = uz.fileData(s);
            imageStore.add(s, dbuf);
            }

      if (rootfile.isEmpty()) {
            qDebug("=can't find rootfile in: %s", qPrintable(info.filePath()));
            return QByteArray();
            }
      return uz.fileData(rootfile);
      }

//---------------------------------------------------------
//   readToBuffer
//---------------------------------------------------------

QByteArray MasterScore::readToBuffer()
      {
      QByteArray ba;
      QString cs  = info.suffix();

      if (cs == "mscz") {
            ba = readCompressedToBuffer();
            }
      if (cs.toLower() == "msc" || cs.toLower() == "mscx") {
            QFile f(info.filePath());
            if (f.open(QIODevice::ReadOnly)) {
                  ba = f.readAll();
                  f.close();
                  }
            }
      return ba;
      }

//---------------------------------------------------------
//   createRevision
//---------------------------------------------------------

void Score::createRevision()
      {
#if 0
qDebug("createRevision");
      QBuffer dbuf;
      dbuf.open(QIODevice::ReadWrite);
      saveFile(&dbuf, false, false);
      dbuf.close();

      QByteArray ba1 = readToBuffer();

      QString os = QString::fromUtf8(ba1.data(), ba1.size());
      QString ns = QString::fromUtf8(dbuf.buffer().data(), dbuf.buffer().size());

      diff_match_patch dmp;
      Revision* r = new Revision();
      r->setDiff(dmp.patch_toText(dmp.patch_make(ns, os)));
      r->setId("1");
      _revisions->add(r);

//      qDebug("patch:\n%s\n==========", qPrintable(patch));
      //
#endif
      }

//---------------------------------------------------------
//   writeVoiceMove
//    write <move> and starting <voice> tags to denote
//    change in position.
//    Returns true if <voice> tag was written.
//---------------------------------------------------------

static bool writeVoiceMove(XmlWriter& xml, Segment* seg, const Fraction& startTick, int track, int* lastTrackWrittenPtr)
      {
      bool voiceTagWritten = false;
      int& lastTrackWritten = *lastTrackWrittenPtr;
      if ((lastTrackWritten < track) && !xml.clipboardmode()) {
            while (lastTrackWritten < (track - 1)) {
                  xml.tagE("voice");
                  ++lastTrackWritten;
                  }
            xml.stag("voice");
            xml.setCurTick(startTick);
            xml.setCurTrack(track);
            ++lastTrackWritten;
            voiceTagWritten = true;
            }

      if ((xml.curTick() != seg->tick()) || (track != xml.curTrack())) {
            Location curr = Location::absolute();
            Location dest = Location::absolute();
            curr.setFrac(xml.curTick());
            dest.setFrac(seg->tick());
            curr.setTrack(xml.curTrack());
            dest.setTrack(track);

            dest.toRelative(curr);
            dest.write(xml);

            xml.setCurTick(seg->tick());
            xml.setCurTrack(track);
            }

      return voiceTagWritten;
      }

//---------------------------------------------------------
//   writeSegments
//    ls  - write upto this segment (excluding)
//          can be zero
//---------------------------------------------------------

void Score::writeSegments(XmlWriter& xml, int strack, int etrack,
   Segment* sseg, Segment* eseg, bool writeSystemElements, bool forceTimeSig)
      {
      Fraction startTick = xml.curTick();
      Fraction endTick   = eseg ? eseg->tick() : lastMeasure()->endTick();
      bool clip          = xml.clipboardmode();

      // in clipboard mode, ls might be in an mmrest
      // since we are traversing regular measures,
      // force them out of mmRest
      if (clip) {
            Measure* lm = eseg ? eseg->measure() : 0;
            Measure* fm = sseg ? sseg->measure() : 0;
            if (lm && lm->isMMRest()) {
                  lm = lm->mmRestLast();
                  if (lm)
                        eseg = lm->nextMeasure() ? lm->nextMeasure()->first() : nullptr;
                  else
                        qDebug("writeSegments: no measure for end segment in mmrest");
                  }
            if (fm && fm->isMMRest()) {
                  fm = fm->mmRestFirst();
                  if (fm)
                        sseg = fm->first();
                  }
            }

      QList<Spanner*> spanners;
#if 0
      auto endIt   = spanner().upper_bound(endTick);
      for (auto i = spanner().begin(); i != endIt; ++i) {
            Spanner* s = i->second;
#else
      auto sl = spannerMap().findOverlapping(sseg->tick().ticks(), endTick.ticks());
      for (auto i : sl) {
            Spanner* s = i.value;
#endif
            if (s->generated() || !xml.canWrite(s))
                  continue;
            // don't write voltas to clipboard
            if (clip && s->isVolta())
                  continue;
            spanners.push_back(s);
            }

      int lastTrackWritten = strack - 1; // for counting necessary <voice> tags
      for (int track = strack; track < etrack; ++track) {
            if (!xml.canWriteVoice(track))
                  continue;

            bool voiceTagWritten = false;

            bool timeSigWritten = false; // for forceTimeSig
            bool crWritten = false;      // for forceTimeSig
            bool keySigWritten = false;  // for forceTimeSig

            for (Segment* segment = sseg; segment && segment != eseg; segment = segment->next1()) {
                  if (!segment->enabled())
                        continue;
                  if (track == 0)
                        segment->setWritten(false);
                  Element* e = segment->element(track);

                  //
                  // special case: - barline span > 1
                  //               - part (excerpt) staff starts after
                  //                 barline element
                  bool needMove = (segment->tick() != xml.curTick() || (track > lastTrackWritten));
                  if ((segment->isEndBarLineType()) && !e && writeSystemElements && ((track % VOICES) == 0)) {
                        // search barline:
                        for (int idx = track - VOICES; idx >= 0; idx -= VOICES) {
                              if (segment->element(idx)) {
                                    int oDiff = xml.trackDiff();
                                    xml.setTrackDiff(idx);          // staffIdx should be zero
                                    segment->element(idx)->write(xml);
                                    xml.setTrackDiff(oDiff);
                                    break;
                                    }
                              }
                        }
                  for (Element* e1 : segment->annotations()) {
                        if (e1->track() != track || e1->generated() || (e1->systemFlag() && !writeSystemElements))
                              continue;
                        if (needMove) {
                              voiceTagWritten |= writeVoiceMove(xml, segment, startTick, track, &lastTrackWritten);
                              needMove = false;
                              }
                        e1->write(xml);
                        }
                  Measure* m = segment->measure();
                  // don't write spanners for multi measure rests

                  if ((!(m && m->isMMRest())) && segment->isChordRestType()) {
                        for (Spanner* s : spanners) {
                              if (s->track() == track) {
                                    bool end = false;
                                    if (s->anchor() == Spanner::Anchor::CHORD || s->anchor() == Spanner::Anchor::NOTE)
                                          end = s->tick2() < endTick;
                                    else
                                          end = s->tick2() <= endTick;
                                    if (s->tick() == segment->tick() && (!clip || end) && !s->isSlur()) {
                                          if (needMove) {
                                                voiceTagWritten |= writeVoiceMove(xml, segment, startTick, track, &lastTrackWritten);
                                                needMove = false;
                                                }
                                          s->writeSpannerStart(xml, segment, track);
                                          }
                                    }
                              if ((s->tick2() == segment->tick())
                                 && !s->isSlur()
                                 && (s->effectiveTrack2() == track)
                                 && (!clip || s->tick() >= sseg->tick())
                                 ) {
                                    if (needMove) {
                                          voiceTagWritten |= writeVoiceMove(xml, segment, startTick, track, &lastTrackWritten);
                                          needMove = false;
                                          }
                                    s->writeSpannerEnd(xml, segment, track);
                                    }
                              }
                        }

                  if (!e || !xml.canWrite(e))
                        continue;
                  if (e->generated())
                        continue;
                  if (forceTimeSig && track2voice(track) == 0 && segment->segmentType() == SegmentType::ChordRest && !timeSigWritten && !crWritten) {
                        // Ensure that <voice> tag is open
                        voiceTagWritten |= writeVoiceMove(xml, segment, startTick, track, &lastTrackWritten);
                        // we will miss a key sig!
                        if (!keySigWritten) {
                              Key k = score()->staff(track2staff(track))->key(segment->tick());
                              KeySig* ks = new KeySig(this);
                              ks->setKey(k);
                              ks->write(xml);
                              delete ks;
                              keySigWritten = true;
                              }
                        // we will miss a time sig!
                        Fraction tsf = sigmap()->timesig(segment->tick()).timesig();
                        TimeSig* ts = new TimeSig(this);
                        ts->setSig(tsf);
                        ts->write(xml);
                        delete ts;
                        timeSigWritten = true;
                        }
                  if (needMove) {
                        voiceTagWritten |= writeVoiceMove(xml, segment, startTick, track, &lastTrackWritten);
                        needMove = false;
                        }
                  if (e->isChordRest()) {
                        ChordRest* cr = toChordRest(e);
                        cr->writeTupletStart(xml);
                        }
//                  if (segment->isEndBarLine() && (m->mmRestCount() < 0 || m->mmRest())) {
//                        BarLine* bl = toBarLine(e);
//TODO                        bl->setBarLineType(m->endBarLineType());
//                        bl->setVisible(m->endBarLineVisible());
//                        }
                  e->write(xml);

                  if (e->isChordRest()) {
                        ChordRest* cr = toChordRest(e);
                        cr->writeTupletEnd(xml);
                        }

                  if (!(e->isRest() && toRest(e)->isGap()))
                        segment->write(xml);    // write only once
                  if (forceTimeSig) {
                        if (segment->segmentType() == SegmentType::KeySig)
                              keySigWritten = true;
                        if (segment->segmentType() == SegmentType::TimeSig)
                              timeSigWritten = true;
                        if (segment->segmentType() == SegmentType::ChordRest)
                              crWritten = true;
                        }
                  }

            //write spanner ending after the last segment, on the last tick
            if (clip || eseg == 0) {
                  for (Spanner* s : spanners) {
                        if ((s->tick2() == endTick)
                          && !s->isSlur()
                          && (s->track2() == track || (s->track2() == -1 && s->track() == track))
                          && (!clip || s->tick() >= sseg->tick())
                          ) {
                              s->writeSpannerEnd(xml, lastMeasure(), track, endTick);
                              }
                        }
                  }

            if (voiceTagWritten)
                  xml.etag(); // </voice>
            }
      }

//---------------------------------------------------------
//   searchTuplet
//    search complete Dom for tuplet id
//    last resort in case of error
//---------------------------------------------------------

Tuplet* Score::searchTuplet(XmlReader& /*e*/, int /*id*/)
      {
      return 0;
      }

}

