//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2020 MuseScore BVBA and others
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

#include "albummanagerdialog.h"
#include "albummanager.h"
#include "musescore.h"
#include "scoreview.h"
#include "libmscore/album.h"
#include "libmscore/score.h"
#include "ui_albummanagerdialog.h"

namespace Ms {
//---------------------------------------------------------
//   AlbumManagerDialog
//---------------------------------------------------------

AlbumManagerDialog::AlbumManagerDialog(QWidget* parent)
    : QDialog(parent)
{
    setObjectName("AlbumManagerDialog");
    setupUi(this);
    connect(buttonBox, &QDialogButtonBox::clicked, this, &AlbumManagerDialog::buttonBoxClicked);
    albumViewModeCombo->setAccessibleName(tr("View Mode"));
    albumViewModeCombo->addItem(tr("Page View"), int(LayoutMode::PAGE));
    albumViewModeCombo->addItem(tr("Continuous View"), int(LayoutMode::LINE));
    albumViewModeCombo->addItem(tr("Single Page"), int(LayoutMode::SYSTEM));
    connect(albumViewModeCombo, SIGNAL(activated(int)), SLOT(setAlbumLayoutMode(int)));
    connect(buttonPause, &QPushButton::clicked, this, &AlbumManagerDialog::applyPauseClicked);
}

AlbumManagerDialog::~AlbumManagerDialog()
{
    delete ui;
}

//---------------------------------------------------------
//   start
//---------------------------------------------------------

void AlbumManagerDialog::start()
{
    update();
    show();
}

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void AlbumManagerDialog::apply()
{
    AlbumManager* albumManager = static_cast<AlbumManager*>(parent());
    if (! albumManager->isAlbum()) {
        return;
    }
    Album& a = albumManager->album();

    a.setDefaultPause(playbackDelayBox->value());

    if (a.drawFrontCover() != checkCreateFrontCover->isChecked()) {
        a.setDrawFrontCover(checkCreateFrontCover->isChecked());
        a.updateFrontCover();
    }

    if (a.generateContents() != checkContentsGeneration->isChecked()) {
        a.setGenerateContents(checkContentsGeneration->isChecked());
        if (checkContentsGeneration->isChecked() && a.getCombinedScore()) {
            a.getCombinedScore()->setfirstRealMovement(2);
        } else if (a.getCombinedScore()) {
            a.getCombinedScore()->setfirstRealMovement(1);
        }
        a.updateContents();
    }

    a.setAddPageBreaksEnabled(checkAddPageBreak->isChecked());
    if (checkAddPageBreak->isChecked()) {
        a.addAlbumPageBreaks();
    } else {
        a.removeAlbumPageBreaks();
    }

    a.setTitleAtTheBottom(checkTitleLayout->isChecked());
    a.setIncludeAbsolutePaths(checkAbsolutePathsEnabled->isChecked());
}

//---------------------------------------------------------
//   update
//---------------------------------------------------------

void AlbumManagerDialog::update()
{
    AlbumManager* albumManager = static_cast<AlbumManager*>(parent());
    if (! albumManager->isAlbum()) {
        return;
    }
    Album& a = albumManager->album();

    playbackDelayBox->setValue(a.defaultPause());
    checkCreateFrontCover->setChecked(a.drawFrontCover());
    checkContentsGeneration->setChecked(a.generateContents());
    checkAddPageBreak->setChecked(a.addPageBreaksEnabled());
    checkTitleLayout->setChecked(a.titleAtTheBottom());
    checkAbsolutePathsEnabled->setChecked(a.includeAbsolutePaths());
}

//---------------------------------------------------------
//   buttonBoxClicked
//---------------------------------------------------------

void AlbumManagerDialog::buttonBoxClicked(QAbstractButton* button)
{
    switch (buttonBox->standardButton(button)) {
    case QDialogButtonBox::Apply:
        apply();
        break;
    case QDialogButtonBox::Ok:
        apply();
    // fall through
    case QDialogButtonBox::Cancel:
    default:
        hide();
        break;
    }
}

//---------------------------------------------------------
//   setAlbumLayoutMode
//---------------------------------------------------------

void AlbumManagerDialog::setAlbumLayoutMode(int i)
{
    AlbumManager* albumManager = static_cast<AlbumManager*>(parent());
    if (! albumManager->isAlbum()) {
        return;
    }
    Album& a = albumManager->album();

    a.setAlbumLayoutMode(static_cast<LayoutMode>(albumViewModeCombo->itemData(i).toInt()));
}

//---------------------------------------------------------
//   applyPauseClicked
//---------------------------------------------------------

void AlbumManagerDialog::applyPauseClicked(bool b)
{
    Q_UNUSED(b);

    AlbumManager* albumManager = static_cast<AlbumManager*>(parent());
    if (! albumManager->isAlbum()) {
        return;
    }
    Album& a = albumManager->album();

    a.setDefaultPause(playbackDelayBox->value());
    a.applyDefaultPauseToSectionBreaks();
}

}
