//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2019 MuseScore BVBA
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

#include "icons.h"
#include "musescore.h"
#include "musescoredialogs.h"
#include "preferences.h"
#include "scoreview.h"

namespace Ms {

//---------------------------------------------------------
// InsertMeasuresDialog
//---------------------------------------------------------

InsertMeasuresDialog::InsertMeasuresDialog(QWidget* parent)
   : QDialog(parent)
      {
      setObjectName("InsertMeasuresDialog");
      setupUi(this);
      setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
      setModal(true);
      insmeasures->setFocus();
      insmeasures->selectAll();
      connect(buttonBox, SIGNAL(clicked(QAbstractButton*)), SLOT(buttonBoxClicked(QAbstractButton*)));
      }

//---------------------------------------------------------
//   buttonBoxClicked
//---------------------------------------------------------

void InsertMeasuresDialog::buttonBoxClicked(QAbstractButton* button)
      {
      switch (buttonBox->buttonRole(button)) {
            case QDialogButtonBox::AcceptRole:
                  accept();
                  // fall through
            case QDialogButtonBox::RejectRole:
                  close();
            default:
                  break;
            }
      }

//---------------------------------------------------------
// Insert Measure -->   accept
//---------------------------------------------------------

void InsertMeasuresDialog::accept()
      {
      int n = insmeasures->value();
      if (mscore->currentScore())
            mscore->currentScoreView()->cmdInsertMeasures(n, ElementType::MEASURE);
      done(1);
      }

//---------------------------------------------------------
// InsertMeasuresDialog hideEvent
//---------------------------------------------------------

void InsertMeasuresDialog::hideEvent(QHideEvent* event)
      {
      MuseScore::saveGeometry(this);
      QDialog::hideEvent(event);
      }

//---------------------------------------------------------
//   MeasuresDialog
//---------------------------------------------------------

MeasuresDialog::MeasuresDialog(QWidget* parent)
   : QDialog(parent)
      {
      setObjectName("MeasuresDialog");
      setupUi(this);
      setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
      setModal(true);
      measures->setFocus();
      measures->selectAll();
      connect(buttonBox, SIGNAL(clicked(QAbstractButton*)), SLOT(buttonBoxClicked(QAbstractButton*)));
      }

//---------------------------------------------------------
//   buttonBoxClicked
//---------------------------------------------------------

void MeasuresDialog::buttonBoxClicked(QAbstractButton* button)
      {
      switch (buttonBox->buttonRole(button)) {
            case QDialogButtonBox::AcceptRole:
                  accept();
                  // fall through
            case QDialogButtonBox::RejectRole:
                  close();
            default:
                  break;
            }
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void MeasuresDialog::accept()
      {
      int n = measures->value();
      if (mscore->currentScore())
            mscore->currentScoreView()->cmdAppendMeasures(n, ElementType::MEASURE);
      done(1);
      }

//---------------------------------------------------------
// MeasuresDialog hideEvent
//---------------------------------------------------------

void MeasuresDialog::hideEvent(QHideEvent* event)
      {
      MuseScore::saveGeometry(this);
      QDialog::hideEvent(event);
      }


//---------------------------------------------------------
//   AboutBoxDialog
//---------------------------------------------------------

AboutBoxDialog::AboutBoxDialog()
      {
      setupUi(this);
      museLogo->setPixmap(QPixmap(preferences.isThemeDark() ?
            ":/data/musescore-logo-transbg-m.png" : ":/data/musescore_logo_full.png"));

      if (MuseScore::unstable())
            versionLabel->setText(tr("Unstable Prerelease for Version: %1").arg(VERSION) + tr(" Evolution"));
      else {
            auto msVersion = QString(VERSION);
            if (strlen(BUILD_NUMBER))
                  msVersion += QString("-") + QString(BUILD_NUMBER); // + QString(" Beta");
            versionLabel->setText(tr("Version: %1").arg(msVersion) + tr(" Evolution"));
      }

      if (!revision.isEmpty())
            revisionLabel->setText(tr("Revision: %1").arg(QString("<a href=\"https://github.com/Jojo-Schmitz/musescore/commit/%1\">%1</a>").arg(revision)));
      else {
            revisionLabel->setText("");
            copyRevisionButton->setVisible(false);
            }
      setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

      auto compilerDateISO = []() -> QString {
            // Attempt to convert __DATE__ into ISO 8601 format (YYYY-MM-DD)
            QDate date = QDate::fromString(__DATE__, "MMM dd yyyy");
            if (!date.isValid())
                  date = QDate::fromString(__DATE__, "MMM  d yyyy"); // for single digit days __DATE__ may use a leading space rather than a leading 0
            return date.isValid() ? date.toString(Qt::ISODate) : __DATE__;
            };

      QString dateTime;
      dateTime += preferences.getBool(PREF_UI_APP_BUILD_DATE_ISO) ? compilerDateISO() : __DATE__;
      dateTime += " ";
      dateTime += __TIME__;
      if (!revision.isEmpty())
            dateTime += " UTC"; // local builds (most likely not having revision set) use local time, GitHub CI uses UTC

      buildDateLabel->setText(tr("Build date: %1").arg(dateTime));

      QString visitAndDonateString;
#if !defined(FOR_WINSTORE)
      visitAndDonateString = tr("Visit %1 for new versions and more information.\nGet %2help%3 with the program or %4contribute%5 to its development.")
                  .arg("<a href=\"https://github.com/Jojo-Schmitz/MuseScore/wiki\">github.com/Jojo-Schmitz/MuseScore/wiki</a>",
                       "<a href=\"https://www.musescore.org/forum\">", "</a>",
                       "<a href=\"https://github.com/Jojo-Schmitz/MuseScore/wiki/Contribute\">", "</a>");
      visitAndDonateString += "\n\n";
#endif
      QString finalString = visitAndDonateString + tr("Copyright &copy; 1999-2026 MuseScore Limited and others.\nPublished under the %1GNU General Public License version 2%2.")
                  .arg("<a href=\"https://www.gnu.org/licenses/old-licenses/gpl-2.0.html\">", "</a>");
      finalString.replace("\n", "<br/>");
      copyrightLabel->setText(QString("<span style=\"font-size:10pt;\">%1</span>").arg(finalString));
      connect(copyRevisionButton, SIGNAL(clicked()), this, SLOT(copyRevisionToClipboard()));
      copyRevisionButton->setIcon(*icons[int(Icons::copy_ICON)]);
      }

//---------------------------------------------------------
//   copyRevisionToClipboard
//---------------------------------------------------------

void AboutBoxDialog::copyRevisionToClipboard()
      {
      QApplication::clipboard()->setText(
            QString("OS: %1, Arch.: %2, MuseScore Studio version (%3-bit): %4-%5")
                  .arg(QSysInfo::prettyProductName()
                       + ((QSysInfo::productType() == "windows" && (QSysInfo::productVersion() == "10" || QSysInfo::productVersion() == "11"))
                          ? " or later" : ""), QSysInfo::currentCpuArchitecture())
                  .arg(QSysInfo::WordSize)
                  .arg(VERSION, BUILD_NUMBER)
            + QString(revision.isEmpty() ? "" : ", revision: [%1](https://github.com/Jojo-Schmitz/MuseScore/commit/%1)")
                  .arg(revision));
      }

//---------------------------------------------------------
//   AboutBoxDialog
//---------------------------------------------------------

AboutMusicXMLBoxDialog::AboutMusicXMLBoxDialog()
      {
      setupUi(this);
      setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
      label->setText(QString("<span style=\"font-size:10pt;\">%1<br/></span>")
                     .arg(tr(   "MusicXML is an open file format for exchanging digital sheet music,\n"
                                "supported by many applications.\n"
                                "Copyright © 2004-2017 the Contributors to the MusicXML\n"
                                "Specification, published by the W3C Music Notation Community\n"
                                "Group under the W3C Community Final Specification Agreement:\n%1\n"
                                "A human-readable summary is available:\n%2")
                          .arg( "\n&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"https://www.w3.org/community/about/process/final/\">https://www.w3.org/community/about/process/final/</a>\n",
                                "\n&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"https://www.w3.org/community/about/process/fsa-deed/\">https://www.w3.org/community/about/process/fsa-deed/</a>\n")
                          .replace("\n","<br/>")));
      }

} // namespace Ms
