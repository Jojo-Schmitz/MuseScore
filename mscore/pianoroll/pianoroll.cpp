//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2009-2013 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include <QAbstractItemView>

#include <QHelpEvent>
#include <QToolTip>

#include "pianoroll.h"
#include "shortcut.h"
#include "config.h"
#include "pianokeyboard.h"
#include "pianoruler.h"
#include "pianolevels.h"
#include "pianolevelschooser.h"
#include "pianoview.h"
#include "musescore.h"
#include "seq.h"
#include "scoreview.h"
#include "preferences.h"
#include "notetweakerdialog.h"
#include "libmscore/accidental.h"
#include "libmscore/staff.h"
#include "libmscore/measure.h"
#include "libmscore/note.h"
#include "libmscore/repeatlist.h"
#include "libmscore/score.h"
#include "libmscore/tempo.h"
#include "libmscore/undo.h"
#include "libmscore/part.h"
#include "libmscore/instrument.h"
#include "awl/pitchlabel.h"
#include "awl/pitchedit.h"
#include "awl/poslabel.h"


namespace Ms {

//---------------------------------------------------------
//   PianoRollComboBox
//
//    Provide Qt 5.9.9 fixes:
//    - doItemsLayout() before popup
//          → fixes clipped/scrolled popup
//    - custom ToolTip event
//          → fixes stale tooltip sizing
//
//    Seems like these issues don't exist in Qt 5.15+
//
//    QTBUG-64550: A font inherited by the style sheet
//                 might change the size
//
//    See also QTBUG-113765 for a related QComboBox
//    popup-geometry issue caused by stale view state.
//---------------------------------------------------------

class PianoRollComboBox : public QComboBox
      {
public:
      using QComboBox::QComboBox;

      void showPopup() override
            {
            if (view())
                  view()->doItemsLayout();

            QComboBox::showPopup();
            }

      bool event(QEvent* event) override
            {
            if (event->type() == QEvent::ToolTip && !toolTip().isEmpty()) {
                  QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);

                  QToolTip::showText(helpEvent->globalPos(), toolTip());

                  event->accept();
                  return true;
                  }

            return QComboBox::event(event);
            }
      };

//---------------------------------------------------------
//   PianorollEditor
//---------------------------------------------------------

PianorollEditor::PianorollEditor(QWidget* parent)
   : QWidget(parent)
      {
      setObjectName("Pianoroll");
      setWindowTitle(QString("MuseScore"));

      _score   = 0;
      staff    = 0;

      _scope = PianoRollScope::SCORE;
      _orientation = PianoRollOrientation::HORIZONTAL;

      QToolTip::setFont(QApplication::font());

      const QSize toolbarIconSize(
            preferences.getInt(PREF_UI_THEME_ICONWIDTH),
            preferences.getInt(PREF_UI_THEME_ICONHEIGHT));

      QActionGroup* ag = Shortcut::getActionGroupForWidget(MsWidget::PIANO_ROLL_EDITOR);
      ag->setParent(this);
      addActions(ag->actions());
      connect(ag, SIGNAL(triggered(QAction*)), this, SLOT(handleAction(QAction*)));

      noteTweakerDlg = new NoteTweakerDialog(this);


      QWidget* mainWidget = new QWidget;
      tbMain = new QToolBar("Toolbar Main", this);
      tbMain->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
      tbMain->setIconSize(toolbarIconSize);

      if (qApp->layoutDirection() == Qt::LayoutDirection::LeftToRight) {
            tbMain->addAction(getAction("undo"));
            tbMain->addAction(getAction("redo"));
            }
      else {
            tbMain->addAction(getAction("redo"));
            tbMain->addAction(getAction("undo"));
            }
      tbMain->addSeparator();
#ifdef HAS_MIDI
      tbMain->addAction(getAction("midi-on"));
#endif
      tbMain->addSeparator();

      tbMain->addAction(getAction("rewind"));
      tbMain->addAction(getAction("play"));
      tbMain->addSeparator();

      QAction* loopAction = getAction("loop");
      tbMain->addAction(loopAction);
      tbMain->addSeparator();
      tbMain->addAction(getAction("repeat"));
      QAction* followAction = getAction("follow");
      followAction->setChecked(preferences.getBool(PREF_APP_PLAYBACK_FOLLOWSONG));
      tbMain->addAction(followAction);
      tbMain->addSeparator();
      tbMain->addAction(getAction("metronome"));

      tbMain->addSeparator();

      staffBox = new PianoRollComboBox;
      staffBox->setToolTip(tr("Editable staff"));
      tbMain->addWidget(staffBox);

      connect(staffBox,
              QOverload<int>::of(&QComboBox::activated),
              this,
              [this](int index) {
                    if (!_score)
                          return;

                    const int staffIdx = staffBox->itemData(index).toInt();

                    if (staffIdx >= 0 && staffIdx < _score->nstaves())
                          setStaff(_score->staff(staffIdx));

                    restoreScoreViewFocus();
                    });

      // Option: Orientation Horizontal/Vertical
      tbMain->addSeparator();
      QComboBox* orientationBox = new PianoRollComboBox;
      orientationBox->setToolTip(tr("View orientation"));
      orientationBox->addItem(tr("Horizontal"), int(PianoRollOrientation::HORIZONTAL));
      orientationBox->addItem(tr("Vertical"),   int(PianoRollOrientation::VERTICAL));

      int orientationIndex = orientationBox->findData(int(_orientation));
      if (orientationIndex != -1)
            orientationBox->setCurrentIndex(orientationIndex);

      tbMain->addWidget(orientationBox);

      connect(orientationBox,
              QOverload<int>::of(&QComboBox::activated),
              this,
              [this, orientationBox](int index) {
                    setOrientation(
                          PianoRollOrientation(
                                orientationBox->itemData(index).toInt()));
                    });

      tbMain->addSeparator();

      // Option: Scope
      scopeBox = new PianoRollComboBox;
      scopeBox->addItem(tr("Staff"), int(PianoRollScope::STAFF));
      scopeBox->setToolTip(tr("Displayed scope"));
      scopeBox->addItem(tr("Part"),  int(PianoRollScope::PART));
      scopeBox->addItem(tr("Score"),  int(PianoRollScope::SCORE));

      int scopeIndex = scopeBox->findData(int(_scope));
      if (scopeIndex != -1)
            scopeBox->setCurrentIndex(scopeIndex);

      tbMain->addWidget(scopeBox);

      connect(scopeBox,
              QOverload<int>::of(&QComboBox::activated),
              this,
              [this](int index) {
                    setScope(PianoRollScope(scopeBox->itemData(index).toInt()));
                    });

      tbMain->addSeparator();

      // Option: Show levels editor
      _showPianoLevels = preferences.getBool(PREF_UI_PIANOROLL_SHOW_LEVELS_EDITOR);
      QAction* showLevelsAction = new QAction(tr("Levels Editor"), this);
      showLevelsAction->setCheckable(true);
      showLevelsAction->setChecked(_showPianoLevels);

      connect(showLevelsAction, &QAction::toggled,
              this, &PianorollEditor::setPianoLevelsVisible);

      tbMain->addAction(showLevelsAction);

      tbMain->addSeparator();

      // Option: Voice coloring / Unselect preference coloring:
      QComboBox* coloringBox = new PianoRollComboBox;
      coloringBox->setToolTip(tr("Coloring scheme"));
      coloringBox->addItem(tr("Voice"),   int(Coloring::VOICING));
      coloringBox->addItem(tr("Staff"),  int(Coloring::STAFF));
      coloringBox->addItem(tr("Instrument"), int(Coloring::INSTRUMENT));

      tbMain->addWidget(coloringBox);

      auto applyColoring = [this, coloringBox](int index) {
            Coloring c = static_cast<Coloring>(coloringBox->itemData(index).toInt());
            setColoring(c);
            };

      connect(coloringBox,
              QOverload<int>::of(&QComboBox::activated),
              this,
              applyColoring);
      // Call applyColoring after other constructions later:


      // Option: Duration / onset note representation
      noteShapeBox = new PianoRollComboBox;
      noteShapeBox->setToolTip(
            tr("Piano roll note representation for the editable instrument"));

      noteShapeBox->addItem(
            tr("Auto"),
            int(PianoRollNoteShape::AUTO));

      noteShapeBox->addItem(
            tr("Duration"),
            int(PianoRollNoteShape::RECTANGLE));

      noteShapeBox->addItem(
            tr("Onset"),
            int(PianoRollNoteShape::DIAMOND));

      noteShapeBox->setItemData(
            0,
            tr("Automatic: rectangles for pitched instruments, diamonds for drums"),
            Qt::ToolTipRole);

      noteShapeBox->setItemData(
            1,
            tr("Rectangle"),
            Qt::ToolTipRole);

      noteShapeBox->setItemData(
            2,
            tr("Diamond"),
            Qt::ToolTipRole);

      noteShapeBox->setEnabled(false);

      tbMain->addWidget(noteShapeBox);

      connect(noteShapeBox,
              QOverload<int>::of(&QComboBox::activated),
              this,
              [this](int index) {
                    const PianoRollNoteShape shape =
                          PianoRollNoteShape(
                                noteShapeBox->itemData(index).toInt());

                    setPianoRollNoteShape(shape);
                    updateNoteShapeToolState();

                    restoreScoreViewFocus();
                    });


      // Option: custon note color to be honored or not
      QAction* useNoteColorsAction = new QAction(
            *icons[int(Icons::noteheadColor_ICON)],
            QString(),
            this);

      _useNoteColors = preferences.getBool(PREF_UI_PIANOROLL_USE_NOTE_COLORS);
      useNoteColorsAction->setCheckable(true);
      useNoteColorsAction->setChecked(_useNoteColors);
      useNoteColorsAction->setToolTip(
            tr("Honor user-defined note colors"));

      connect(useNoteColorsAction,
              &QAction::toggled,
              this,
              [this](bool checked) {
                    setUseNoteColors(checked);
                    restoreScoreViewFocus();
                    });

      tbMain->addAction(useNoteColorsAction);

      tbMain->addSeparator();

      // Option: Show pitch names
      QAction* showPitchNamesAction = new QAction(
            QIcon(":/data/icons/note-show-pitch.svg"),
            tr("Show pitch names"),
            this);

      showPitchNamesAction->setCheckable(true);
      showPitchNamesAction->setChecked(
            preferences.getBool(PREF_UI_PIANOROLL_SHOW_PITCH_TEXT));

      showPitchNamesAction->setToolTip(tr("Show pitch names"));
      showPitchNamesAction->setStatusTip(tr("Show pitch names"));

      connect(showPitchNamesAction,
              &QAction::toggled,
              this,
              [this](bool checked) {
                    preferences.setPreference(
                          PREF_UI_PIANOROLL_SHOW_PITCH_TEXT,
                          checked);

                    if (pianoView)
                          pianoView->viewport()->update();

                    restoreScoreViewFocus();
                    });

      tbMain->addAction(showPitchNamesAction);

      QAction* regroupVoicingAction = tbMain->addAction(tr("Regroup Voicing"));

      regroupVoicingAction->setToolTip(
            tr("Regroup existing voices by pitch order"));

      connect(regroupVoicingAction, &QAction::triggered,
              this, &PianorollEditor::regroupSelectedVoicing);

      createEditToolbars(toolbarIconSize);

      Awl::PitchLabel* pitchLabel = createTweakToolbar(toolbarIconSize);

      topLeftSpacer = new QWidget;
      topLeftSpacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      topLeftSpacer->setFixedWidth(PIANO_KEYBOARD_WIDTH);
      topLeftSpacer->setFixedHeight(pianoRulerHeight);

      verticalCornerSpacer = new QWidget;
      verticalCornerSpacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      verticalCornerSpacer->setFixedWidth(pianoRulerHeight);
      verticalCornerSpacer->setFixedHeight(PIANO_KEYBOARD_WIDTH);
      verticalCornerSpacer->hide();

      controlsChevronButton = new QToolButton;
      controlsChevronButton->setText(QStringLiteral("⌃")); // "roll-up" the toolbar
      controlsChevronButton->setToolTip(tr("Toggle piano roll controls"));
      controlsChevronButton->setFixedSize(22, 22);
      controlsChevronButton->setAutoRaise(false);
      controlsChevronButton->setStyleSheet(
            "QToolButton {"
            "  border: 0px solid palette(dark);"
            "  border-radius: 12px;"
            "  background: transparent;"
            "  padding: 0px;"
            "}"
            "QToolButton:hover {"
            "  background: palette(midlight);"
            "}"
            "QToolButton:pressed {"
            "  background: palette(dark);"
            "}"
            );

      ruler = new PianoRuler;
      pianoKbd = new PianoKeyboard;
      pianoView = new PianoView;
      pianoView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      pianoView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

      connect(automaticVoiceAction,
              &QAction::toggled,
              this,
              [this](bool checked) {
                    preferences.setPreference(
                          PREF_UI_PIANOROLL_USE_AUTO_VOICE,
                          checked);

                    pianoView->setAutomaticVoiceAssignment(checked);
                    restoreScoreViewFocus();
                    });

      ruler->setPianoView(pianoView);

      ruler->setLoopEnabled(loopAction->isChecked());

      connect(loopAction,
              &QAction::toggled,
              ruler,
              &PianoRuler::setLoopEnabled);

      hsb = new QScrollBar(Qt::Horizontal);
      connect(pianoView->horizontalScrollBar(), SIGNAL(rangeChanged(int,int)),
            SLOT(rangeChanged(int,int)));

      QWidget* noteAreaWidget = new QWidget;

      noteAreaLayout = new QGridLayout;
      noteAreaLayout->setContentsMargins(0, 0, 0, 0);
      noteAreaLayout->setSpacing(0);

      noteAreaWidget->setLayout(noteAreaLayout);
      updateOrientationLayout();

      // levels area
      pianoLevelsChooser = new PianoLevelsChooser;
      pianoLevelsChooser->setPianoView(pianoView);
      pianoLevelsChooser->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
      pianoLevelsChooser->setFixedWidth(PIANO_KEYBOARD_WIDTH);

      pianoLevels = new PianoLevels;
      pianoLevels->setPianoView(pianoView);
      pianoLevels->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      pianoLevels->setOrientation(_orientation);

      levelsAreaWidget = new QWidget;
      QHBoxLayout* levelsAreaLayout = new QHBoxLayout;
      levelsAreaLayout->setContentsMargins(0, 0, 0, 0);
      levelsAreaLayout->setSpacing(0);
      levelsAreaLayout->addWidget(pianoLevelsChooser);
      levelsAreaLayout->addWidget(pianoLevels);
      levelsAreaWidget->setLayout(levelsAreaLayout);
      levelsAreaWidget->setVisible(_showPianoLevels);

      // layout
      QSplitter* editAreaSplitter = new QSplitter(Qt::Vertical);
      editAreaSplitter->addWidget(noteAreaWidget);
      editAreaSplitter->addWidget(levelsAreaWidget);
      editAreaSplitter->setFrameShape(QFrame::NoFrame);

      editAreaSplitter->setSizes(QList<int>({300, 100}));

      split = new QSplitter(Qt::Vertical);
      split->setFrameShape(QFrame::NoFrame);

      QGridLayout* layout = new QGridLayout;
      layout->setContentsMargins(0, 0, 0, 0);
      layout->setSpacing(0);
      layout->setColumnMinimumWidth(0, PIANO_KEYBOARD_WIDTH);
      layout->addWidget(editAreaSplitter, 1, 0, 1, 1);

      mainWidget->setLayout(layout);

      QVBoxLayout* mainLayout = new QVBoxLayout(this);
      mainLayout->setContentsMargins(0, 0, 0, 0);
      mainLayout->setSpacing(0);

      toolbarArea = new QWidget(this);
      toolbarArea->setSizePolicy(
            QSizePolicy::Preferred,
            QSizePolicy::Fixed);

      QVBoxLayout* toolbarLayout = new QVBoxLayout(toolbarArea);
      toolbarLayout->setContentsMargins(0, 0, 0, 0);
      toolbarLayout->setSpacing(0);

      QWidget* toolbarRow1 = new QWidget(toolbarArea);
      QHBoxLayout* toolbarRow1Layout = new QHBoxLayout(toolbarRow1);
      toolbarRow1Layout->setContentsMargins(0, 0, 0, 0);
      toolbarRow1Layout->setSpacing(0);

      toolbarRow1Layout->addWidget(tbMain);
      toolbarRow1Layout->addStretch(1);

      QWidget* toolbarRow2 = new QWidget(toolbarArea);
      QHBoxLayout* toolbarRow2Layout = new QHBoxLayout(toolbarRow2);
      toolbarRow2Layout->setContentsMargins(0, 0, 0, 0);
      toolbarRow2Layout->setSpacing(0);

      toolbarRow2Layout->addWidget(tbTool);
      toolbarRow2Layout->addWidget(tbNoteLen);
      toolbarRow2Layout->addWidget(tbDots);
      toolbarRow2Layout->addWidget(tbVoices);
      toolbarRow2Layout->addStretch(1);

      toolbarLayout->addWidget(toolbarRow1);
      toolbarLayout->addWidget(toolbarRow2);

      mainLayout->addWidget(toolbarArea);
      mainLayout->addWidget(tbTweak);

      mainLayout->addWidget(mainWidget);

      connect(controlsChevronButton,
              &QToolButton::clicked,
              this,
              [this]() {
                    _controlsVisible = !_controlsVisible;

                    toolbarArea->setVisible(_controlsVisible);
                    tbTweak->setVisible(_controlsVisible);

                    controlsChevronButton->setText(
                          _controlsVisible
                                ? QStringLiteral("⌃")
                                : QStringLiteral("⌄"));

                    controlsChevronButton->setToolTip(
                          _controlsVisible
                                ? tr("Hide piano roll controls")
                                : tr("Show piano roll controls"));

                    restoreScoreViewFocus();
                    });

      // Re-enable right-click menu for enable/disable toolbars now with dockable widget
      const QList<QToolBar*> toolbars {
            tbMain,
            tbNoteLen,
            tbDots,
            tbTool,
            tbVoices,
            tbTweak
            };


      for (QToolBar* toolbar : toolbars) {
            toolbar->setContextMenuPolicy(Qt::CustomContextMenu);

            connect(toolbar,
                    &QToolBar::customContextMenuRequested,
                    this,
                    [toolbar, toolbars](const QPoint& pos) {
                          QMenu menu;

                          for (QToolBar* tb : toolbars)
                                menu.addAction(tb->toggleViewAction());

                          menu.exec(toolbar->mapToGlobal(pos));
                          });
            }

      // And right-sided area
      toolbarArea->setContextMenuPolicy(Qt::CustomContextMenu);

      connect(toolbarArea,
              &QWidget::customContextMenuRequested,
              this,
              [this, toolbars](const QPoint& pos) {
                    QMenu menu;

                    for (QToolBar* tb : toolbars)
                          menu.addAction(tb->toggleViewAction());

                    menu.exec(toolbarArea->mapToGlobal(pos));
                    });

      _playbackFollowTimer = new QTimer(this);
      _playbackFollowTimer->setTimerType(Qt::PreciseTimer);
      _playbackFollowTimer->setInterval(8);

      connect(_playbackFollowTimer, &QTimer::timeout,
              this, &PianorollEditor::updatePlaybackFollow);

      connectEditorSignals(pitchLabel, coloringBox);

      // readSettings();

      const QList<QAction*> editorActions {
            getAction("tie"),
            getAction("play"),
            getAction("delete"),
            getAction("pitch-up"),
            getAction("pitch-down"),
            getAction("pitch-up-octave"),
            getAction("pitch-down-octave")
            };

      addActions(editorActions);

      setXpos(0);
      }

//---------------------------------------------------------
//   ~PianorollEditor
//---------------------------------------------------------

PianorollEditor::~PianorollEditor()
      {
      if (_score)
            _score->removeViewer(this);
      }

//---------------------------------------------------------
//   regroupSelectedVoicing
//---------------------------------------------------------

void PianorollEditor::regroupSelectedVoicing()
      {
      if (!_score)
            return;

      const Selection& selection = _score->selection();
      Fraction startTick;
      Fraction endTick;
      int startStaff = -1;
      int endStaff   = -1;

      if (selection.isRange()) {
            startTick  = selection.tickStart();
            endTick    = selection.tickEnd();
            startStaff = selection.staffStart();
            endStaff   = selection.staffEnd();
            }
      else {
            bool found = false;

            for (Element* e : selection.elements()) {
                  Chord* chord = nullptr;

                  if (e->isNote())
                        chord = toNote(e)->chord();
                  else if (e->isChord())
                        chord = toChord(e);

                  if (!chord || chord->isGrace())
                        continue;

                  const Fraction chordStart = chord->tick();
                  const Fraction chordEnd =
                        chordStart + chord->actualTicks();

                  const int staffIdx = chord->staffIdx();

                  if (!found) {
                        startTick  = chordStart;
                        endTick    = chordEnd;
                        startStaff = staffIdx;
                        endStaff   = staffIdx + 1;
                        found = true;
                        }
                  else {
                        if (chordStart < startTick)
                              startTick = chordStart;

                        if (chordEnd > endTick)
                              endTick = chordEnd;

                        if (staffIdx < startStaff)
                              startStaff = staffIdx;

                        if (staffIdx >= endStaff)
                              endStaff = staffIdx + 1;
                        }
                  }

            if (!found)
                  return;
            }

      _score->startCmd();

      for (int staffIdx = startStaff; staffIdx < endStaff; ++staffIdx) {
            _score->regroupVoicing(startTick, endTick, staffIdx);
            }

      _score->endCmd();
      }

//---------------------------------------------------------
//   createEditToolbars
//---------------------------------------------------------

void PianorollEditor::createEditToolbars(const QSize& iconSize)
      {
      tbTool = new QToolBar("Action Buttons", this);
      bngrpActionBns = new QButtonGroup(this);
      tbTool->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
      tbTool->setIconSize(iconSize);

      struct ToolIconData {
            QString icon;
            QString tooltip;
            PianoRollEditTool tool;
            bool selected;
            };

      const ToolIconData toolData[] = {
            { QStringLiteral(":/data/icons/preEdit-select.svg"),
              tr("Select Notes"),
              PianoRollEditTool::SELECT,
              false },
            { QStringLiteral(":/data/icons/preEdit-insertNote.svg"),
              tr("Add Note"),
              PianoRollEditTool::ADD,
              false },
            { QStringLiteral(":/data/icons/preEdit-cutNote.svg"),
              tr("Cut Chord"),
              PianoRollEditTool::CUT,
              false },
            { QStringLiteral(":/data/icons/preEdit-eraseNote.svg"),
              tr("Erase Note"),
              PianoRollEditTool::ERASE,
              false },
            { QStringLiteral(":/data/icons/preEdit-changeLength.svg"),
              tr("Change Playback Length"),
              PianoRollEditTool::EVENT_ADJUST,
              true },
            { QStringLiteral(":/data/icons/preEdit-tie.svg"),
              tr("Toggle Tie"),
              PianoRollEditTool::TIE,
              false },
            { QString(), QString(), PianoRollEditTool::LAST, false }
            };

      for (const ToolIconData* p = toolData;
           p->tool != PianoRollEditTool::LAST;
           ++p) {
            QToolButton* button = new QToolButton;

            QIcon icon;
            icon.addFile(p->icon, QSize(), QIcon::Normal, QIcon::Off);

            button->setIcon(icon);
            button->setIconSize(iconSize);
            button->setCheckable(true);
            button->setToolTip(p->tooltip);

            const PianoRollEditTool tool = p->tool;

            if (tool == PianoRollEditTool::CUT)
                  cutToolButton = button;
            else if (tool == PianoRollEditTool::TIE)
                  tieToolButton = button;

            connect(button, &QToolButton::clicked,
                  this, [this, tool]() {
                        setEditNoteTool(tool);

                        const bool showAutoVoice =
                              tool == PianoRollEditTool::ADD;

                        if (automaticVoiceAction)
                              automaticVoiceAction->setVisible(showAutoVoice);

                        if (automaticVoiceSeparator)
                              automaticVoiceSeparator->setVisible(showAutoVoice);
                        });

            if (p->selected)
                  button->setChecked(true);

            bngrpActionBns->addButton(button, int(tool));
            tbTool->addWidget(button);
            }

      struct LenIconData {
            QString icon;
            int measureFrac;
            bool selected;
            };

      const LenIconData lenData[] = {
            { QStringLiteral(":/data/icons/note-longa.svg"), 2, false },
            { QStringLiteral(":/data/icons/note-breve.svg"), 1, false },
            { QStringLiteral(":/data/icons/note-1.svg"), 0, false },
            { QStringLiteral(":/data/icons/note-2.svg"), -1, false },
            { QStringLiteral(":/data/icons/note-4.svg"), -2, true },
            { QStringLiteral(":/data/icons/note-8.svg"), -3, false },
            { QStringLiteral(":/data/icons/note-16.svg"), -4, false },
            { QStringLiteral(":/data/icons/note-32.svg"), -5, false },
            { QStringLiteral(":/data/icons/note-64.svg"), -6, false },
            { QStringLiteral(":/data/icons/note-128.svg"), -7, false },
            { QStringLiteral(":/data/icons/note-256.svg"), -8, false },
            { QStringLiteral(":/data/icons/note-512.svg"), -9, false },
            { QStringLiteral(":/data/icons/note-1024.svg"), -10, false },
            { QString(), 0, false }
            };

      tbNoteLen = new QToolBar("Toolbar Note Length", this);
      bngrpNoteLen = new QButtonGroup(this);
      tbNoteLen->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
      tbNoteLen->setIconSize(iconSize);

      for (const LenIconData* p = lenData; !p->icon.isEmpty(); ++p) {
            QToolButton* button = new QToolButton;

            QIcon icon;
            icon.addFile(p->icon, QSize(), QIcon::Normal, QIcon::Off);

            button->setIcon(icon);
            button->setIconSize(iconSize);
            button->setCheckable(true);

            const int length = p->measureFrac;
            button->setProperty("measureFrac", length);

            connect(button, &QToolButton::clicked,
                  this, [this, length]() {
                        setEditNoteLength(length);
                        });

            if (p->selected)
                  button->setChecked(true);

            bngrpNoteLen->addButton(button);
            tbNoteLen->addWidget(button);
            }

      tbDots = new QToolBar("Toolbar Dots", this);
      bngrpNoteDot = new QButtonGroup(this);
      tbDots->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
      tbDots->setIconSize(iconSize);

      struct DotIconData {
            QString icon;
            int dots;
            };

      const DotIconData dotData[] = {
            { QStringLiteral(":/data/icons/note-dot.svg"), 1 },
            { QStringLiteral(":/data/icons/note-double-dot.svg"), 2 },
            { QStringLiteral(":/data/icons/note-dot3.svg"), 3 },
            { QStringLiteral(":/data/icons/note-dot4.svg"), 4 },
            { QString(), -1 }
            };

      for (const DotIconData* p = dotData; p->dots != -1; ++p) {
            QToolButton* button = new QToolButton;

            QIcon icon;
            icon.addFile(p->icon, QSize(), QIcon::Normal, QIcon::Off);

            button->setIcon(icon);
            button->setIconSize(iconSize);
            button->setCheckable(true);

            const int dots = p->dots;
            button->setProperty("dots", dots);

            connect(button, &QToolButton::clicked,
                  this, [this, dots, button]() {
                        setEditNoteDots(dots, button);
                        });

            bngrpNoteDot->addButton(button);
            tbDots->addWidget(button);
            }

      tbVoices = new QToolBar("Toolbar Voices", this);

      QButtonGroup* voiceGroup = new QButtonGroup(this);

      tbVoices->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
      tbVoices->setIconSize(iconSize);

      struct VoiceIconData {
            QString icon;
            QString tooltip;
            int voice;
            bool selected;
            };

      const VoiceIconData voiceData[] = {
            { QStringLiteral(":/data/icons/voice-1.svg"), tr("Voice 1"), 0, true },
            { QStringLiteral(":/data/icons/voice-2.svg"), tr("Voice 2"), 1, false },
            { QStringLiteral(":/data/icons/voice-3.svg"), tr("Voice 3"), 2, false },
            { QStringLiteral(":/data/icons/voice-4.svg"), tr("Voice 4"), 3, false },
            { QString(), QString(), -1, false }
            };

      for (const VoiceIconData* p = voiceData; p->voice != -1; ++p) {
            QToolButton* button = new QToolButton;

            QIcon icon;
            icon.addFile(p->icon, QSize(), QIcon::Normal, QIcon::Off);

            button->setIcon(icon);
            button->setIconSize(iconSize);
            button->setCheckable(true);
            button->setToolTip(p->tooltip);

            const int voice = p->voice;

            connect(button, &QToolButton::clicked,
                  this, [this, voice]() {
                        setEditNoteVoice(voice);
                        });

            if (p->selected)
                  button->setChecked(true);

            voiceGroup->addButton(button);
            tbVoices->addWidget(button);
            }

      const bool automaticVoice =
            preferences.getBool(PREF_UI_PIANOROLL_USE_AUTO_VOICE);

      automaticVoiceSeparator = tbVoices->addSeparator();

      automaticVoiceAction = new QAction(tr("Auto Voice"), this);
      automaticVoiceAction->setCheckable(true);
      automaticVoiceAction->setChecked(automaticVoice);
      automaticVoiceAction->setToolTip(
            tr("Automatically choose a compatible voice when inserting notes"));

      tbVoices->addAction(automaticVoiceAction);

      const bool showAutoVoice =
            bngrpActionBns->checkedId()
            == int(PianoRollEditTool::ADD);

      automaticVoiceSeparator->setVisible(showAutoVoice);
      automaticVoiceAction->setVisible(showAutoVoice);
      }

//---------------------------------------------------------
//   createTweakToolbar
//---------------------------------------------------------

Awl::PitchLabel* PianorollEditor::createTweakToolbar(const QSize& iconSize)
      {
      tbTweak = new QToolBar("Toolbar Tweak", this);
      tbTweak->setIconSize(iconSize);

      tbTweak->addWidget(new QLabel(tr("Cursor:")));

      pos = new Awl::PosLabel;
      pos->setFrameStyle(static_cast<int>(QFrame::NoFrame)
                         | static_cast<int>(QFrame::Plain));
      tbTweak->addWidget(pos);

      Awl::PitchLabel* pitchLabel = new Awl::PitchLabel;
      pitchLabel->setFrameStyle(static_cast<int>(QFrame::NoFrame)
                                | static_cast<int>(QFrame::Plain));
      tbTweak->addWidget(pitchLabel);

      tbTweak->addSeparator();

      tbTweak->addWidget(new QLabel(tr("Subdiv.:")));

      subdiv = new QSpinBox;
      subdiv->setToolTip(tr("Subdivide the beat this many times"));
      subdiv->setMinimum(0);
      subdiv->setValue(0);
      tbTweak->addWidget(subdiv);

      tbTweak->addWidget(new QLabel(tr("Tuplet:")));

      tuplet = new QSpinBox;
      tuplet->setToolTip(tr("Edit notes aligned to tuplets of this many beats"));
      tuplet->setMinimum(1);
      tuplet->setValue(1);
      tbTweak->addWidget(tuplet);

      tbTweak->addWidget(new QLabel(tr("Stripe Pattern:")));

      barPattern = new PianoRollComboBox;
      barPattern->setToolTip(tr("White stripes show the tones of this chord."));

      for (int i = 0; !PianoView::barPatterns[i].name.isEmpty(); ++i) {
            barPattern->addItem(
                  qApp->translate("BarPattern",
                                  PianoView::barPatterns[i].name.toUtf8().data()),
                  i);
            }

      tbTweak->addWidget(barPattern);

      // Option: Keyboard aligned grid
      keyboardAlignedGridSeparator = tbTweak->addSeparator();

      keyboardAlignedGrid = new QCheckBox(tr("Keyboard-aligned grid"));

      keyboardAlignedGrid->setToolTip(
            tr("Align the vertical piano-roll pitch lanes with the keyboard"));

      keyboardAlignedGrid->setChecked(
            preferences.getBool(PREF_UI_PIANOROLL_VERTICAL_KEYBOARD_ALIGNED_GRID));

      keyboardAlignedGridAction =
            tbTweak->addWidget(keyboardAlignedGrid);

      connect(keyboardAlignedGrid, &QCheckBox::toggled, this, [this](bool checked) {
            preferences.setPreference(
                  PREF_UI_PIANOROLL_VERTICAL_KEYBOARD_ALIGNED_GRID,
                  checked);

            pianoView->setVerticalPitchLayout(
                  checked ? VerticalPitchLayout::KEYBOARD_ALIGNED
                          : VerticalPitchLayout::CHROMATIC);

            restoreScoreViewFocus();
            });

      tbTweak->addSeparator();

      // Option: Velocity type
      tbTweak->addWidget(new QLabel(tr("Velocity:")));

      veloType = new PianoRollComboBox;
      veloType->addItem(tr("Offset"), int(Note::ValueType::OFFSET_VAL));
      veloType->addItem(tr("User"), int(Note::ValueType::USER_VAL));
      tbTweak->addWidget(veloType);

      velocity = new QSpinBox;
      velocity->setRange(-127, 127);
      velocity->setReadOnly(true);

      velocity->setPrefix("+");
      const int velocityWidth = velocity->sizeHint().width();
      velocity->setPrefix("");
      velocity->setMinimumWidth(velocityWidth);

      tbTweak->addWidget(velocity);

      tbTweak->addWidget(new QLabel(tr("Pitch:")));

      pitch = new Awl::PitchEdit;
      pitch->setReadOnly(false);
      tbTweak->addWidget(pitch);

      tbTweak->addWidget(new QLabel(tr("OnTime:")));

      onTime = new QSpinBox;
      onTime->setRange(-60000, +60000);
      onTime->setSingleStep(50);
      tbTweak->addWidget(onTime);

      tbTweak->addWidget(new QLabel(tr("Len:")));

      tickLen = new QSpinBox;
      tickLen->setRange(-2000, 60000);
      tickLen->setSingleStep(50);
      tbTweak->addWidget(tickLen);

      return pitchLabel;
      }

//---------------------------------------------------------
//   connectEditorSignals
//---------------------------------------------------------

void PianorollEditor::connectEditorSignals(
      Awl::PitchLabel* pitchLabel,
      QComboBox* coloringBox)
      {
      connect(pianoView, &PianoView::onTimeDragged, this, [this](int value) {
            _previewOnTime = value;

            setOnTime(value);

            pianoLevelsChooser->setEventPreviewValues(
                  _previewOnTime,
                  _previewLen);

            if (_showPianoLevels && pianoLevels)
                  pianoLevels->update();
            });

      connect(pianoView, &PianoView::tickLenDragged, this, [this](int value) {
            _previewLen = value;

            setTickLen(value);

            pianoLevelsChooser->setEventPreviewValues(
                  _previewOnTime,
                  _previewLen);

            if (_showPianoLevels && pianoLevels)
                  pianoLevels->update();
            });

      connect(pianoView, &PianoView::noteEventsChanged, this, [this]() {
            if (_showPianoLevels && pianoLevels)
                  pianoLevels->update();

            updateSelection();
            });

      connect(pianoView, &PianoView::editNoteLengthChanged,
              this, &PianorollEditor::updateNoteLengthControls);

      velocity->installEventFilter(this);
      pitch->installEventFilter(this);
      onTime->installEventFilter(this);
      tickLen->installEventFilter(this);
      subdiv->installEventFilter(this);
      tuplet->installEventFilter(this);

      const int coloringIndex = coloringBox->currentIndex();
      setColoring(static_cast<Coloring>(
            coloringBox->itemData(coloringIndex).toInt()));

      pianoView->setUseNoteColors(_useNoteColors);
      pianoKbd->setUseNoteColors(_useNoteColors);
      pianoLevels->setUseNoteColors(_useNoteColors);

      connect(pianoView->horizontalScrollBar(), SIGNAL(valueChanged(int)),
              hsb, SLOT(setValue(int)));

      connect(pianoView, SIGNAL(xZoomChanged(qreal)),
              ruler, SLOT(setXZoom(qreal)));

      connect(pianoView, SIGNAL(xZoomChanged(qreal)),
              pianoLevels, SLOT(setXZoom(qreal)));

      connect(pianoView, SIGNAL(noteHeightChanged(int)),
              pianoKbd, SLOT(setNoteHeight(int)));

      connect(pianoView, SIGNAL(pitchChanged(int)),
              pitchLabel, SLOT(setPitch(int)));

      connect(pianoView, SIGNAL(pitchChanged(int)),
              pianoKbd, SLOT(setPitch(int)));

      connect(pianoKbd, SIGNAL(pitchChanged(int)),
              pitchLabel, SLOT(setPitch(int)));

      connect(pianoView, &PianoView::trackingPosChanged,
              pos, &Awl::PosLabel::setValue);

      connect(pianoView, &PianoView::trackingPosChanged,
              ruler, &PianoRuler::setPos);

      connect(pianoView, &PianoView::trackingPosChanged,
              pianoLevels, &PianoLevels::setPos);

      connect(ruler, &PianoRuler::posChanged,
              pos, &Awl::PosLabel::setValue);

      connect(pianoLevels, &PianoLevels::posChanged,
              pos, &Awl::PosLabel::setValue);

      connect(tuplet, SIGNAL(valueChanged(int)),
              pianoView, SLOT(setTuplet(int)));

      connect(tuplet, SIGNAL(valueChanged(int)),
              pianoLevels, SLOT(setTuplet(int)));

      connect(tuplet, QOverload<int>::of(&QSpinBox::valueChanged),
              this, [this](int value) {
                  QAbstractButton* cutButton =
                        bngrpActionBns->button(
                              static_cast<int>(PianoRollEditTool::CUT));

                  if (!cutButton)
                        return;

                  const bool enabled = value == 1;
                  cutButton->setEnabled(enabled);

                  if (!enabled && cutButton->isChecked()) {
                        QAbstractButton* adjustButton =
                              bngrpActionBns->button(
                                    static_cast<int>(PianoRollEditTool::EVENT_ADJUST));

                        if (adjustButton)
                              adjustButton->setChecked(true);

                        setEditNoteTool(PianoRollEditTool::EVENT_ADJUST);
                        }
                  });

      connect(barPattern, QOverload<int>::of(&QComboBox::activated),
              this, [this](int index) {
                  pianoView->setBarPattern(index);
                  restoreScoreViewFocus();
                  });

      connect(pitch, &Awl::PitchEdit::returnPressed,
              this, [this]() {
                  applyPitchEdit();
                  restoreScoreViewFocus();
                  });

      connect(pianoView->horizontalScrollBar(), &QScrollBar::valueChanged,
              this, [this](int value) {
                  if (_orientation == PianoRollOrientation::VERTICAL) {
                        pianoKbd->setYpos(value);
                        }
                  });

      connect(pianoView->verticalScrollBar(), &QScrollBar::valueChanged,
              this, [this](int value) {
                  if (_orientation == PianoRollOrientation::HORIZONTAL) {
                        pianoKbd->setYpos(value);
                        }
                  else if (_orientation == PianoRollOrientation::VERTICAL) {
                        if (_showPianoLevels && pianoLevels)
                              pianoLevels->update();
                        }
                  });

      connect(subdiv, SIGNAL(valueChanged(int)),
              pianoView, SLOT(setSubdiv(int)));

      connect(subdiv, SIGNAL(valueChanged(int)),
              pianoLevels, SLOT(setSubdiv(int)));

      connect(pianoLevelsChooser, SIGNAL(levelsIndexChanged(int)),
              pianoLevels, SLOT(setLevelsIndex(int)));

      connect(pianoKbd, SIGNAL(pitchHighlightToggled(int)),
              pianoView, SLOT(togglePitchHighlight(int)));

      connect(hsb,
              SIGNAL(valueChanged(int)),
              SLOT(setXpos(int)));

      connect(ruler, &PianoRuler::locatorMoved,
              this, &PianorollEditor::moveLocator);

      connect(pianoLevels, &PianoLevels::locatorMoved,
              this, &PianorollEditor::moveLocator);

      connect(veloType,
            SIGNAL(activated(int)),
            SLOT(veloTypeChanged(int)));

      connect(velocity,
            SIGNAL(valueChanged(int)),
            SLOT(velocityChanged(int)));

      connect(onTime,
            SIGNAL(valueChanged(int)),
            SLOT(onTimeChanged(int)));

      connect(tickLen,
            SIGNAL(valueChanged(int)),
            SLOT(tickLenChanged(int)));

      connect(pianoView,
            SIGNAL(selectionChanged()),
            SLOT(selectionChanged()));

      connect(pianoView,
            SIGNAL(showNoteTweakerRequest()),
            SLOT(showNoteTweaker()));

      connect(pianoKbd,
            SIGNAL(keyPressed(int)),
            SLOT(keyPressed(int)));

      connect(pianoKbd,
            SIGNAL(keyReleased(int)),
            SLOT(keyReleased(int)));

      connect(pianoLevels,
            SIGNAL(noteLevelsChanged()),
            SLOT(selectionChanged()));

      connect(noteTweakerDlg,
            SIGNAL(notesChanged()),
            SLOT(selectionChanged()));

      connect(pianoLevelsChooser,
            SIGNAL(notesChanged()),
            SLOT(selectionChanged()));
      }

//---------------------------------------------------------
//   setEditNoteLength
//---------------------------------------------------------

void PianorollEditor::setEditNoteLength(int len)
      {
      pianoView->setEditNoteLength(
            Fraction::fromTicks(pow(2, len + 2) * DIVISION));

      if (tbDots)
            tbDots->setEnabled(true);
      }

//---------------------------------------------------------
//   setEditNoteVoice
//---------------------------------------------------------

void PianorollEditor:: setEditNoteVoice(int voice)
      {
            pianoView->setEditNoteVoice(voice);
      }

//---------------------------------------------------------
//   setEditNoteDots
//---------------------------------------------------------

void PianorollEditor::setEditNoteDots(int value, QToolButton* bn)
      {
      if (pianoView->editNoteDots() == value) {
            bn->group()->setExclusive(false);
            bn->setChecked(false);
            bn->group()->setExclusive(true);
            pianoView->setEditNoteDots(0);
            }
      else
            pianoView->setEditNoteDots(value);
      }

//---------------------------------------------------------
//   staffDisplayName
//---------------------------------------------------------

QString PianorollEditor::staffDisplayName(Staff* st) const
      {
      if (!st)
            return QString();

      QString label = st->partName();

      Part* part = st->part();
      if (part && part->nstaves() > 1) {
            const int staffIndex = part->staves()->indexOf(st);

            if (staffIndex >= 0)
                  label += tr(": Staff %1").arg(staffIndex + 1);
            }

      return label;
      }

//---------------------------------------------------------
//   updateNoteLengthControls
//---------------------------------------------------------

void PianorollEditor::updateNoteLengthControls(const Fraction& duration)
      {
      if (!bngrpNoteLen || !bngrpNoteDot)
            return;

      QToolButton* lengthButton = nullptr;
      QToolButton* dotButton = nullptr;
      int matchedMeasureFrac = 0;
      int matchedDots = 0;

      for (QAbstractButton* button : bngrpNoteLen->buttons()) {
            QToolButton* toolButton = qobject_cast<QToolButton*>(button);
            if (!toolButton)
                  continue;

            const int measureFrac =
                  toolButton->property("measureFrac").toInt();

            const Fraction base =
                  Fraction::fromTicks(
                        pow(2, measureFrac + 2) * DIVISION);

            for (int dots = 0; dots <= 4; ++dots) {
                  Fraction value = base;

                  if (dots > 0) {
                        const int denominator = 1 << dots;
                        const int numerator =
                              (1 << (dots + 1)) - 1;

                        value *= Fraction(numerator, denominator);
                        }

                  if (value == duration) {
                        lengthButton = toolButton;
                        matchedMeasureFrac = measureFrac;
                        matchedDots = dots;
                        break;
                        }
                  }

            if (lengthButton)
                  break;
            }

      // Temporarily disable exclusivity so that a custom duration
      // can leave all duration and dot buttons unchecked
      bngrpNoteLen->setExclusive(false);
      for (QAbstractButton* button : bngrpNoteLen->buttons())
            button->setChecked(false);
      bngrpNoteLen->setExclusive(true);

      bngrpNoteDot->setExclusive(false);
      for (QAbstractButton* button : bngrpNoteDot->buttons())
            button->setChecked(false);
      bngrpNoteDot->setExclusive(true);

      if (!lengthButton) {
            // Not representable by the duration/dot controls.
            // Keep the exact dragged value as a custom duration:
            pianoView->setEditNoteLength(duration);
            pianoView->setEditNoteDots(0);
            tbDots->setEnabled(false);
            return;
            }

      lengthButton->setChecked(true);

      if (matchedDots > 0) {
            for (QAbstractButton* button : bngrpNoteDot->buttons()) {
                  if (button->property("dots").toInt() == matchedDots) {
                        dotButton = qobject_cast<QToolButton*>(button);
                        break;
                        }
                  }

            if (dotButton)
                  dotButton->setChecked(true);
            }

      // Store the decoded base duration and dots in PianoView
      setEditNoteLength(matchedMeasureFrac);
      pianoView->setEditNoteDots(matchedDots);
      }

//---------------------------------------------------------
//   updateNoteShapeToolState
//---------------------------------------------------------

void PianorollEditor::updateNoteShapeToolState()
      {
      if (!noteShapeBox)
            return;

      const PianoRollNoteShape shape =
            PianoRollNoteShape(
                  noteShapeBox->currentData().toInt());

      const Fraction referenceTick = pianoView
            ? Fraction::fromTicks(pianoView->viewportReferenceTick())
            : Fraction{};

      const bool onsetMode =
            shape == PianoRollNoteShape::DIAMOND
            || (shape == PianoRollNoteShape::AUTO
                && staff
                && staff->isDrumStaff(referenceTick));

      if (cutToolButton)
            cutToolButton->setEnabled(!onsetMode);

      if (tieToolButton)
            tieToolButton->setEnabled(!onsetMode);

      if (!onsetMode || !bngrpActionBns)
            return;

      QAbstractButton* checked =
            bngrpActionBns->checkedButton();

      if (!checked)
            return;

      const int toolId =
            bngrpActionBns->id(checked);

      if (toolId != int(PianoRollEditTool::CUT)
          && toolId != int(PianoRollEditTool::TIE)) {
            return;
            }

      setEditNoteTool(PianoRollEditTool::SELECT);

      if (QAbstractButton* selectButton =
          bngrpActionBns->button(int(PianoRollEditTool::SELECT))) {
            selectButton->setChecked(true);
            }
      }

//---------------------------------------------------------
//   setEditNoteTool
//---------------------------------------------------------

void PianorollEditor::setEditNoteTool(PianoRollEditTool value)
      {
      pianoView->setEditNoteTool(value);

      pianoLevelsChooser->setPlaybackEditingEnabled(
            value == PianoRollEditTool::EVENT_ADJUST);
      }

//---------------------------------------------------------
//   handleAction
//---------------------------------------------------------

void PianorollEditor::handleAction(QAction* a)
      {
      QString cmd(a->data().toString());

      if (cmd == "zoom-in-horiz-pre")
            zoom(1, true);
      else if (cmd == "zoom-out-horiz-pre")
            zoom(-1, true);
      else if (cmd == "zoom-in-vert-pre")
            zoom(1, false);
      else if (cmd == "zoom-out-vert-pre")
            zoom(-1, false);
      }


//---------------------------------------------------------
//   showNoteTweaker
//---------------------------------------------------------

void PianorollEditor::showNoteTweaker()
      {
      noteTweakerDlg->show();
      }

//---------------------------------------------------------
//   setOnTime
//---------------------------------------------------------

void PianorollEditor::setOnTime(int v)
      {
      QSignalBlocker blocker(onTime);
      onTime->setValue(v);;
      }

//---------------------------------------------------------
//   setTickLen
//---------------------------------------------------------

void PianorollEditor::setTickLen(int v)
      {
      QSignalBlocker blocker(tickLen);
      tickLen->setValue(v);
      }

//---------------------------------------------------------
//   setPianoLevelsVisible
//---------------------------------------------------------

void PianorollEditor::setPianoLevelsVisible(bool visible)
      {
      if (_showPianoLevels == visible)
            return;

      _showPianoLevels = visible;

      preferences.setPreference(PREF_UI_PIANOROLL_SHOW_LEVELS_EDITOR, visible);

      if (!levelsAreaWidget)
            return;

      levelsAreaWidget->setVisible(visible);

      if (pianoLevels) {
            if (visible) {
                  pianoLevels->updateNotes();

                  pianoLevels->setXpos(
                        pianoView->horizontalScrollBar()->value());

                  pianoLevels->update();
                  }
            else {
                  pianoLevels->clearPlaybackLocatorTick();
                  }
            }
      }

//---------------------------------------------------------
//   focusOnPosition
//---------------------------------------------------------

void PianorollEditor::focusOnPosition(Position* p)
      {
      if (!p || !p->segment)
            return;

      // move view so that view is centered on this element
      pianoView->ensureVisible(p->segment->tick().ticks());
      }

//---------------------------------------------------------
//   eventFilter
//---------------------------------------------------------

bool PianorollEditor::eventFilter(QObject* obj, QEvent* event)
      {
      if (event->type() == QEvent::ShortcutOverride) {
            QKeyEvent* const ke = static_cast<QKeyEvent*>(event);

            const bool spinBoxHasFocus =
                  qobject_cast<QAbstractSpinBox*>(obj);

            const bool verticalArrowPress =
                  ke->key() == Qt::Key_Up
                  || ke->key() == Qt::Key_Down;

            if (spinBoxHasFocus && verticalArrowPress) {
                  event->accept();
                  return true;
                  }

            const bool pitchEnterPress =
                  obj == pitch
                  && (ke->key() == Qt::Key_Return
                      || ke->key() == Qt::Key_Enter);

            if (pitchEnterPress) {
                  event->accept();
                  return true;
                  }
            }

      return QWidget::eventFilter(obj, event);
      }

//---------------------------------------------------------
//   restoreScoreViewFocus
//---------------------------------------------------------

void PianorollEditor::restoreScoreViewFocus()
      {
      ScoreView* scoreView = mscore->currentScoreView();
      if (scoreView)
            scoreView->setFocus();
      }

//---------------------------------------------------------
//   updateEditableStaffUi
//---------------------------------------------------------

void PianorollEditor::updateEditableStaffUi()
      {
      updateNoteShapeBox();

      if (staffBox) {
            QSignalBlocker blocker(staffBox);

            const int index =
                  staff ? staffBox->findData(staff->idx()) : -1;

            staffBox->setCurrentIndex(index);
            }

      pianoKbd->setStaff(staff);
      noteTweakerDlg->setStaff(staff);

      updateNoteShapeToolState();
      }

//---------------------------------------------------------
//   setEditableStaff
//---------------------------------------------------------

void PianorollEditor::setEditableStaff(Staff* st)
      {
      if (!st || st == staff)
            return;

      staff = st;

      pianoView->setEditableStaff(st);
      pianoLevels->setEditableStaff(st);

      updateEditableStaffUi();

      restoreScoreViewFocus();
      }

//---------------------------------------------------------
//   setStaff
//---------------------------------------------------------

void PianorollEditor::setStaff(Staff* st)
      {
      if (staff == st)
            return;

      if ((st && st->score() != _score) || (!st && _score)) {
            if (_score) {
                  _score->removeViewer(this);
                  disconnect(_score, SIGNAL(posChanged(POS,unsigned)),
                             this, SLOT(posChanged(POS,unsigned)));
                  disconnect(_score->masterScore(), &Score::partColorChanged,
                             this, &PianorollEditor::redraw);
                  }
            _score = st ? st->score() : nullptr;
            if (_score) {
                  _score->addViewer(this);
                  setLocator(POS::CURRENT, _score->pos(POS::CURRENT).ticks());
                  setLocator(POS::LEFT,    _score->pos(POS::LEFT).ticks());
                  setLocator(POS::RIGHT,   _score->pos(POS::RIGHT).ticks());
                  connect(_score, &Score::posChanged,
                          this, &PianorollEditor::posChanged);
                  connect(_score->masterScore(), &Score::partColorChanged,
                          this, &PianorollEditor::redraw);
                  }
            updateStaffBox();
            }
      staff = st;

      if (staff) {
            setWindowTitle(tr("<%1> Staff: %2").arg(_score->masterScore()->fileInfo()->completeBaseName()).arg(st->idx()));
            TempoMap* tl = _score->tempomap();
            TimeSigMap*  sl = _score->sigmap();
            for (int i = 0; i < 3; ++i)
                  locator[i].setContext(tl, sl);
            pos->setContext(tl, sl);
            }
      else
            setWindowTitle(tr("Piano roll editor"));

      ruler->setScore(_score, locator);
      pianoView->setStaff(staff, locator);
      pianoLevels->setScore(_score, locator);
      pianoLevels->setStaff(staff, locator);

      pianoView->setScope(_scope);
      pianoLevels->setScope(_scope);

      updateEditableStaffUi();

      updateSelection();
      setEnabled(st);
      }

//---------------------------------------------------------
//   updateToolbarIconSize
//---------------------------------------------------------

void PianorollEditor::updateToolbarIconSize()
      {
      const QSize iconSize(
            preferences.getInt(PREF_UI_THEME_ICONWIDTH),
            preferences.getInt(PREF_UI_THEME_ICONHEIGHT));

      const QList<QToolBar*> toolbars {
            tbMain,
            tbTool,
            tbNoteLen,
            tbDots,
            tbVoices,
            tbTweak
            };

      for (QToolBar* toolbar : toolbars) {
            if (!toolbar)
                  continue;

            toolbar->setIconSize(iconSize);

            const QList<QToolButton*> buttons =
                  toolbar->findChildren<QToolButton*>();

            for (QToolButton* button : buttons)
                  button->setIconSize(iconSize);
            }
      }

//---------------------------------------------------------
//   updateOrientationLayout
//---------------------------------------------------------

void PianorollEditor::updateOrientationLayout()
      {
      while (QLayoutItem* item = noteAreaLayout->takeAt(0)) {
            // Removes the layout item only. The widgets themselves
            // remain alive and owned by their existing parents.
            delete item;
            }

      if (pianoLevels)
            pianoLevels->setOrientation(_orientation);

      const bool vertical =
            _orientation == PianoRollOrientation::VERTICAL;

      if (keyboardAlignedGridSeparator)
            keyboardAlignedGridSeparator->setVisible(vertical);

      if (keyboardAlignedGridAction)
            keyboardAlignedGridAction->setVisible(vertical);

      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            // Widgets exclusive to horizontal orientation:
            topLeftSpacer->show();

            controlsChevronButton->setParent(topLeftSpacer);
            controlsChevronButton->move(
                  (topLeftSpacer->width() - controlsChevronButton->width()) / 2,
                  (topLeftSpacer->height() - controlsChevronButton->height()) / 2);
            controlsChevronButton->show();
            controlsChevronButton->raise();

            verticalCornerSpacer->hide();

            ruler->show();
            hsb->show();

            ruler->setOrientation(PianoRollOrientation::HORIZONTAL);

            ruler->setSizePolicy(
                  QSizePolicy::Expanding,
                  QSizePolicy::Fixed);

            ruler->setMinimumWidth(0);
            ruler->setMaximumWidth(QWIDGETSIZE_MAX);
            ruler->setFixedHeight(pianoRulerHeight);

            // Release constraints left by vertical mode:
            pianoKbd->setMinimumHeight(0);
            pianoKbd->setMaximumHeight(QWIDGETSIZE_MAX);

            pianoView->setOrientation(PianoRollOrientation::HORIZONTAL);
            pianoKbd->setOrientation(PianoOrientation::VERTICAL);
            pianoKbd->setSizePolicy(
                  QSizePolicy::Fixed,
                  QSizePolicy::Expanding);
            pianoKbd->setFixedWidth(PIANO_KEYBOARD_WIDTH);

            pianoKbd->setYpos(pianoView->verticalScrollBar()->value());

            noteAreaLayout->addWidget(topLeftSpacer, 0, 0, 1, 1);
            noteAreaLayout->addWidget(ruler,         0, 1, 1, 1);
            noteAreaLayout->addWidget(pianoKbd,      1, 0, 1, 1);
            noteAreaLayout->addWidget(pianoView,     1, 1, 1, 1);
            noteAreaLayout->addWidget(hsb,           2, 1, 1, 1);
            }
      else {
            // VERTICAL

            // Horizontal-only widgets are still children of
            // noteAreaWidget even after being removed from the layout
            topLeftSpacer->hide();

            verticalCornerSpacer->show();

            controlsChevronButton->setParent(verticalCornerSpacer);
            controlsChevronButton->move(
                  (verticalCornerSpacer->width() - controlsChevronButton->width()) / 2,
                  (verticalCornerSpacer->height() - controlsChevronButton->height()) / 2);
            controlsChevronButton->show();
            controlsChevronButton->raise();

            noteAreaLayout->addWidget(ruler,                0, 0);
            noteAreaLayout->addWidget(pianoView,            0, 1);
            noteAreaLayout->addWidget(verticalCornerSpacer, 1, 0);
            noteAreaLayout->addWidget(pianoKbd,             1, 1);

            ruler->show();
            hsb->hide();

            ruler->setOrientation(PianoRollOrientation::VERTICAL);

            ruler->setSizePolicy(
                  QSizePolicy::Fixed,
                  QSizePolicy::Expanding);

            ruler->setMinimumHeight(0);
            ruler->setMaximumHeight(QWIDGETSIZE_MAX);
            ruler->setFixedWidth(pianoRulerHeight);

            // Release constraints left by horizontal mode:
            pianoKbd->setMinimumWidth(0);
            pianoKbd->setMaximumWidth(QWIDGETSIZE_MAX);

            pianoView->setOrientation(PianoRollOrientation::VERTICAL);
            pianoKbd->setOrientation(PianoOrientation::HORIZONTAL);
            pianoKbd->setSizePolicy(
                  QSizePolicy::Expanding,
                  QSizePolicy::Fixed);
            pianoKbd->setFixedHeight(PIANO_KEYBOARD_WIDTH);

            pianoKbd->setYpos(pianoView->horizontalScrollBar()->value());
            }
      }

//---------------------------------------------------------
//   setOrientation
//---------------------------------------------------------

void PianorollEditor::setOrientation(PianoRollOrientation orientation)
      {
      if (_orientation == orientation)
            return;

      const int referenceTick = pianoView->viewportReferenceTick();

      // Remember the pitch viewport belonging to the orientation
      // we are leaving:
      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            _horizontalPitchScrollPos =
                  pianoView->verticalScrollBar()->value();

            _horizontalPitchScrollValid = true;
            }
      else {
            _verticalPitchScrollPos =
                  pianoView->horizontalScrollBar()->value();

            _verticalPitchScrollValid = true;
            }

      _orientation = orientation;
      updateOrientationLayout();

      // Pitch viewport belongs independently to each orientation
      if (_orientation == PianoRollOrientation::HORIZONTAL) {
            if (_horizontalPitchScrollValid) {
                  pianoView->verticalScrollBar()->setValue(
                        _horizontalPitchScrollPos);
                  }
            }
      else {
            if (_verticalPitchScrollValid) {
                  pianoView->horizontalScrollBar()->setValue(
                        _verticalPitchScrollPos);
                  }
            }

      const bool hasSelection =
            !pianoView->getSelectedItems().isEmpty();

      if (hasSelection)
            pianoView->centerSelectionTimeInView();
      else
            pianoView->positionViewportAtTick(referenceTick);

      restoreScoreViewFocus();
      }

//---------------------------------------------------------
//   setScope
//---------------------------------------------------------

void PianorollEditor::setScope(PianoRollScope scope)
      {
      if (_scope == scope)
            return;

      _scope = scope;

      if (scopeBox) {
            const int index = scopeBox->findData(int(scope));
            if (index != -1)
                  scopeBox->setCurrentIndex(index);
            }

      pianoView->setScope(scope);
      pianoLevels->setScope(scope);
      restoreScoreViewFocus();
      }

//---------------------------------------------------------
//   setColoring
//---------------------------------------------------------

void PianorollEditor::setColoring(Coloring c)
      {
      _coloring = c;

      if (pianoView)
            pianoView->setColoring(c);

      if (pianoKbd)
            pianoKbd->setColoring(c);

      if (pianoLevels)
            pianoLevels->setColoring(c);

      update();
      restoreScoreViewFocus();
      }

//---------------------------------------------------------
//   setUseNoteColors
//---------------------------------------------------------

void PianorollEditor::setUseNoteColors(bool value)
      {
      if (_useNoteColors == value)
            return;

      _useNoteColors = value;

      preferences.setPreference(
            PREF_UI_PIANOROLL_USE_NOTE_COLORS,
            value);

      pianoView->setUseNoteColors(value);
      pianoKbd->setUseNoteColors(value);
      pianoLevels->setUseNoteColors(value);

      redraw();
      }

//---------------------------------------------------------
//   clearPlaybackPitches
//---------------------------------------------------------

void PianorollEditor::clearPlaybackPitches()
      {
      if (pianoKbd)
            pianoKbd->setPlaybackNotes(QHash<int, const Note*>());

      if (pianoView) {
            pianoView->setPlaybackActive(false);
            pianoView->clearPlaybackNoteEvents();
            }
      }

//---------------------------------------------------------
//   writeSettings
//---------------------------------------------------------

void PianorollEditor::writeSettings()
      {
      MuseScore::saveGeometry(this);
      }

//---------------------------------------------------------
//   readSettings
//---------------------------------------------------------

void PianorollEditor::readSettings()
      {
      resize(QSize(800, 600)); // ensure default size if no geometry in settings
      MuseScore::restoreGeometry(this);
      }

//---------------------------------------------------------
//   setXpos
//---------------------------------------------------------

void PianorollEditor::setXpos(int x)
      {
      pianoView->horizontalScrollBar()->setValue(x);
      ruler->setXpos(x);

      if (_showPianoLevels)
            pianoLevels->setXpos(x);
      }

//---------------------------------------------------------
//   rangeChanged
//---------------------------------------------------------

void PianorollEditor::rangeChanged(int min, int max)
      {
      hsb->setRange(min, max);
      }

//---------------------------------------------------------
//   updateSelection
//---------------------------------------------------------

void PianorollEditor::updateSelection()
      {
      QList<PianoItem*> items = pianoView->getSelectedItems();

      if (items.size() == 1) {
            PianoItem* item = items.front();

            if (item && item->note()) {
                  Staff* selectedStaff = item->note()->staff();

                  if (selectedStaff && selectedStaff != staff)
                        setEditableStaff(selectedStaff);
                  }
            }

      QHash<int, const Note*> keyboardSelection;

      if (preferences.getBool(PREF_UI_PIANOROLL_SELECTION_HIGHLIGHT_KEYBOARD)) {
            for (PianoItem* item : items) {
                  if (!item || !item->note())
                        continue;

                  Note* note = item->note();

                  // One physical keyboard key can represent only one
                  // selected note at a given pitch
                  if (!keyboardSelection.contains(note->pitch()))
                        keyboardSelection.insert(note->pitch(), note);
                  }
            }

      pianoKbd->setSelectionNotes(keyboardSelection);
      bool enabled = false;

      // Pitch can represent either a common selected pitch
      // or a mixed selection
      if (!items.empty()) {
            Note* first = items[0]->note();

            const int firstPitch = first->pitch();
            const int firstTpc = first->concertPitch()
                  ? first->tpc1()
                  : first->tpc2();

            bool mixed = false;

            for (int i = 1; i < items.size(); ++i) {
                  Note* note = items[i]->note();

                  const int noteTpc = note->concertPitch()
                        ? note->tpc1()
                        : note->tpc2();

                  if (note->pitch() != firstPitch || noteTpc != firstTpc) {
                        mixed = true;
                        break;
                        }
                  }

            if (mixed)
                  pitch->setMixedPitch();
            else
                  pitch->setPitch(firstPitch, firstTpc);
            }

      // These fields only have an unambiguous value
      // when exactly one PianoItem is selected
      if (items.size() == 1) {
            PianoItem* item = items[0];
            Note* note = item->note();

            NoteEvent* event = item->getTweakNoteEvent();
            if (event) {
                  QSignalBlocker onTimeBlocker(onTime);
                  QSignalBlocker tickLenBlocker(tickLen);

                  onTime->setValue(event->ontime());
                  tickLen->setValue(event->len());

                  _previewOnTime = event->ontime();
                  _previewLen = event->len();
                  }

            updateVelocity(note);
            }

      // If all selected notes don't have the same veloType,
      // velocity field should be disabled
      bool sameVeloType = true;
      if (!items.empty()) {
            enabled = true;

            Note::ValueType vt = items[0]->note()->veloType();
            for (int i = 1; i < items.size(); i++) {
                  if (items[i]->note()->veloType() != vt) {
                        sameVeloType = false;
                        break;
                        }
                  }
            }

      velocity->setEnabled(enabled && sameVeloType);
      pitch->setEnabled(enabled);
      veloType->setEnabled(enabled);
      onTime->setEnabled(enabled);
      tickLen->setEnabled(enabled);
      pianoLevelsChooser->updateSetboxValue();
      }

//---------------------------------------------------------
//   selectionChanged
//    called if selection in PianoView changed
//---------------------------------------------------------

void PianorollEditor::selectionChanged()
      {
      QList<PianoItem*> items = pianoView->getSelectedItems();
      if (items.size() == 1) {
            Note* note = items[0]->note();
            _score->select(note, SelectType::SINGLE, 0);
            }
      else if (items.size() == 0)
            _score->select(0, SelectType::SINGLE, 0);
      else {
            _score->deselectAll();
            for (PianoItem*& item : items) {
                  Note* note = item->note();
                  if (!note->selected())
                        _score->select(note, SelectType::ADD, 0);
                  }
            }

      if (!_score->selection().isNone())
            _score->selection().setSource(SelectionSource::PIANO_ROLL);


      pianoView->scene()->update();

      if (_showPianoLevels && pianoLevels)
            pianoLevels->update();

      updateSelection();
      }

//---------------------------------------------------------
//   changeSelection
//---------------------------------------------------------

void PianorollEditor::changeSelection(SelState)
      {
      if (!_score || !pianoView)
            return;

      // PianoItem uses the actual score Note::selected() state,
      // so no separate selection transfer is required
      pianoView->scene()->update();

      if (_showPianoLevels && pianoLevels)
            pianoLevels->update();

      updateSelection();

      // Selection by mouse should not disturb an already useful view.
      // Keyboard/navigation selection that goes off-screen should follow
      pianoView->ensureSelectionVisible();
      }

//---------------------------------------------------------
//   veloTypeChanged
//---------------------------------------------------------

void PianorollEditor::veloTypeChanged(int val)
      {
      QList<PianoItem*> items = pianoView->getSelectedItems();
      if (!items.size())
            return;

      const Note::ValueType vt = Note::ValueType(val);
      switch (vt) {
            case Note::ValueType::USER_VAL:
                  velocity->setRange(0, 127);
                  velocity->setPrefix("");
                  break;

            case Note::ValueType::OFFSET_VAL:
                  velocity->setRange(-127, 127);
                  velocity->setPrefix(
                        velocity->value() > 0 ? "+" : "");
                  break;
            }

      _score->startCmd();
      for (int i = 0; i < items.size(); i++) {
            PianoItem* item = items[i];
            Note* note = item->note();
            if (Note::ValueType(val) == note->veloType())
                  continue;

            int newVelocity = note->veloOffset();
            int dynamicsVel = staff->velocities().val(note->tick());

            switch (Note::ValueType(val)) {
                  case Note::ValueType::USER_VAL:
                        // relative offset -> absolute velocity
                        newVelocity = qBound(0, dynamicsVel + newVelocity, 127);
                        break;

                  case Note::ValueType::OFFSET_VAL:
                        // absolute velocity -> relative offset
                        newVelocity = qBound(-127, newVelocity - dynamicsVel, 127);
                        break;
                  }

            _score->undo(new ChangeVelocity(note, Note::ValueType(val), newVelocity));
            updateVelocity(note);
            }
      _score->endCmd();

      restoreScoreViewFocus();
      }

//---------------------------------------------------------
//   updateNoteShapeBox
//---------------------------------------------------------

void PianorollEditor::updateNoteShapeBox()
      {
      if (!noteShapeBox)
            return;

      QSignalBlocker blocker(noteShapeBox);

      if (!staff || !staff->part()) {
            noteShapeBox->setCurrentIndex(-1);
            noteShapeBox->setEnabled(false);
            return;
            }

      const Instrument* instrument =
            staff->part()->instrument();

      if (!instrument) {
            noteShapeBox->setCurrentIndex(-1);
            noteShapeBox->setEnabled(false);
            return;
            }

      const int index =
            noteShapeBox->findData(
                  int(instrument->pianoRollNoteShape()));

      noteShapeBox->setCurrentIndex(index);
      noteShapeBox->setEnabled(index != -1);
      }

//---------------------------------------------------------
//   setPianoRollNoteShape
//---------------------------------------------------------

void PianorollEditor::setPianoRollNoteShape(
      PianoRollNoteShape shape)
      {
      if (!_score || !staff || !staff->part())
            return;

      Part* part = staff->part();
      Instrument* current = part->instrument();

      if (!current || current->pianoRollNoteShape() == shape) {
            updateNoteShapeBox();
            return;
            }

      _score->startCmd();

      _score->undo(new ChangePianoRollNoteShape(current, shape));

      _score->endCmd();

      updateNoteShapeBox();
      updateNoteData();
      redraw();
      }

//---------------------------------------------------------
//   updateStaffBox
//---------------------------------------------------------

void PianorollEditor::updateStaffBox()
      {
      if (!staffBox)
            return;

      QSignalBlocker blocker(staffBox);

      staffBox->clear();

      if (!_score) {
            staffBox->setEnabled(false);
            return;
            }

      for (Staff* st : _score->staves()) {
            if (!st)
                  continue;

            staffBox->addItem(staffDisplayName(st), st->idx());
            }

      staffBox->setEnabled(staffBox->count() > 0);
      }

//---------------------------------------------------------
//   updateVelocity
//---------------------------------------------------------

void PianorollEditor::updateVelocity(Note* note)
      {
      const Note::ValueType vt = note->veloType();
      const int value = note->veloOffset();

      QSignalBlocker typeBlocker(veloType);
      QSignalBlocker velocityBlocker(velocity);

      veloType->setCurrentIndex(int(vt));

      velocity->setReadOnly(false);
      velocity->setSuffix("");

      switch (vt) {
            case Note::ValueType::USER_VAL:
                  velocity->setRange(0, 127);
                  velocity->setPrefix("");
                  break;

            case Note::ValueType::OFFSET_VAL:
                  velocity->setRange(-127, 127);
                  velocity->setPrefix(value > 0 ? "+" : "");
                  break;
            }

      velocity->setValue(value);

      if (_showPianoLevels && pianoLevels)
            pianoLevels->update();
      }

//---------------------------------------------------------
//   velocityChanged
//---------------------------------------------------------

void PianorollEditor::velocityChanged(int val)
      {
      QList<PianoItem*> items = pianoView->getSelectedItems();
      if (!items.size())
            return;

      const Note::ValueType currentType =
            Note::ValueType(veloType->currentIndex());

      if (currentType == Note::ValueType::OFFSET_VAL && val > 0)
            velocity->setPrefix("+");
      else
            velocity->setPrefix("");

      _score->startCmd();
      for (int i = 0; i < items.size(); i++) {
            PianoItem* item = items[i];
            Note* note = item->note();
            Note::ValueType vt = note->veloType();

            if (val == note->veloOffset())
                  continue;

            _score->undo(new ChangeVelocity(note, vt, val));
            }
      _score->endCmd();

      if (_showPianoLevels && pianoLevels)
            pianoLevels->update();
      }

//---------------------------------------------------------
//   keyPressed
//---------------------------------------------------------

void PianorollEditor::keyPressed(int p)
      {
      seq->startNote(staff->part()->instrument()->channel(0)->channel(), p, 80, 0, 0.0);
      }

//---------------------------------------------------------
//   keyReleased
//---------------------------------------------------------

void PianorollEditor::keyReleased(int /*p*/)
      {
      seq->stopNotes();
      }

//---------------------------------------------------------
//   updatePlaybackFollow
//---------------------------------------------------------

void PianorollEditor::updatePlaybackFollow()
      {
      if (!_playbackFollowActive)
            return;

      // Stop visual following when playback or Follow Song is no longer active
      if (!seq || !seq->isPlaying() || !preferences.getBool(PREF_APP_PLAYBACK_FOLLOWSONG)) {
            stopPlaybackFollow();
            return;
            }

      // One real sequencer interval is needed before knowing playback velocity
      if (!_playbackFollowVelocityValid)
            return;

      const qreal elapsed =
            _playbackFollowElapsed.nsecsElapsed()
            / 1000000000.0;

      const qreal predictedTick =
            _playbackFollowPaused
                  ? _playbackFollowBaseTick
                  : _playbackFollowBaseTick
                        + elapsed * _playbackFollowTicksPerSecond;

      const bool showPlaybackCursor =
            preferences.getBool(PREF_UI_PIANOROLL_PLAYBACK_SHOW_CURSOR);

      if (showPlaybackCursor) {
            pianoView->setPlaybackLocatorTick(predictedTick);

            if (_showPianoLevels && pianoLevels)
                  pianoLevels->setPlaybackLocatorTick(predictedTick);
            }
      else {
            pianoView->clearPlaybackLocatorTick();
            ruler->clearPlaybackLocatorTick();

            if (pianoLevels)
                  pianoLevels->clearPlaybackLocatorTick();
            }

      // Leave the viewport completely untouched until playback reaches
      // the center of the visible time range, at which point normal
      // centered following occurs
      if (!_playbackFollowScrolling) {
            if (!pianoView->playbackTickBeyondCenter(predictedTick)) {
                  pianoView->ensurePlaybackTickVisible(predictedTick);

                  if (showPlaybackCursor
                      && _orientation == PianoRollOrientation::HORIZONTAL) {
                        ruler->setPlaybackLocatorTick(predictedTick);
                        }

                  return;
                  }

            _playbackFollowScrolling = true;
            }

      pianoView->ensureVisible(predictedTick, 0.0);
      ruler->setPlaybackLocatorTick(predictedTick);
      }

//---------------------------------------------------------
//   stopPlaybackFollow
//---------------------------------------------------------

void PianorollEditor::stopPlaybackFollow()
      {
      _playbackFollowScrolling = false;
      _playbackFollowActive = false;
      _playbackFollowVelocityValid = false;
      _playbackFollowPaused = false;
      _playbackFollowTicksPerSecond = 0.0;

      pianoView->clearPlaybackLocatorTick();
      ruler->clearPlaybackLocatorTick();
      if (pianoLevels)
            pianoLevels->clearPlaybackLocatorTick();

      if (_playbackFollowTimer->isActive())
            _playbackFollowTimer->stop();
      }

//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void PianorollEditor::heartBeat(Seq* s)
      {
      unsigned tick = s->getCurTick();

      if (score()->masterScore())
            tick = score()->masterScore()->repeatList().utick2tick(tick);

      pianoView->setPlaybackActive(s->isPlaying());

      // Keep the authoritative PRE playback position synchronized
      // with the sequencer:
      if (locator[0].tick() != tick)
            posChanged(POS::CURRENT, tick);


      QHash<int, const Note*> playbackNotes;

      if (preferences.getBool(PREF_UI_PIANOROLL_PLAYBACK_HIGHLIGHT_KEYBOARD)) {
            const auto& active = s->activePitches();
            for (auto it = active.constBegin(); it != active.constEnd(); ++it) {
                  if (it.value().count > 0 && it.value().note)
                        playbackNotes.insert(it.key(), it.value().note);
                  }
            }

      pianoKbd->setPlaybackNotes(playbackNotes);


      if (preferences.getBool(PREF_UI_PIANOROLL_PLAYBACK_HIGHLIGHT_NOTES)) {
            QHash<const Note*, QSet<int>> playbackNoteEvents;
            const auto& activeNoteEvents = s->activeNoteEvents();
            for (const ActiveNoteEventInfo& info : activeNoteEvents) {
                  if (info.owner && info.noteEventIndex >= 0)
                        playbackNoteEvents[info.owner].insert(
                              info.noteEventIndex);
                  }

            pianoView->setPlaybackNoteEvents(playbackNoteEvents);
            }
      else {
            pianoView->clearPlaybackNoteEvents();
            }

      // Smooth viewport following is purely visual. It does not
      // replace the authoritative locator/playback position above
      if (!preferences.getBool(PREF_APP_PLAYBACK_FOLLOWSONG)
          || !s->isPlaying()) {
            stopPlaybackFollow();
            return;
            }

      const TempoMap* tempoMap = _score->tempomap();

      const qreal newTicksPerSecond =
            DIVISION
            * tempoMap->tempo(tick)
            * tempoMap->relTempo();

      const auto pauseEvent = tempoMap->find(tick);
      const bool atPlaybackPause =
            pauseEvent != tempoMap->end()
            && pauseEvent->second.pause > 0.0;

      // First playback sample establishes both the visual time
      // origin and the playback velocity
      if (!_playbackFollowActive) {
            _playbackFollowActive = true;
            _playbackFollowVelocityValid = true;
            _playbackFollowScrolling = false;
            _playbackFollowPaused = false;

            _playbackFollowBaseTick = qreal(tick);
            _playbackFollowLastSampleTick = tick;
            _playbackFollowTicksPerSecond = newTicksPerSecond;

            _playbackFollowElapsed.restart();
            _playbackFollowTimer->start();

            return;
            }

      const qreal visualElapsed =
            _playbackFollowElapsed.nsecsElapsed()
            / 1000000000.0;

      const qreal predictedTick =
            _playbackFollowPaused
                  ? _playbackFollowBaseTick
                  : _playbackFollowBaseTick
                        + visualElapsed * _playbackFollowTicksPerSecond;

      // Detect playback discontinuities
      //
      // Backward movement is always a discontinuity.  For forward
      // movement, compare the authoritative sequencer position with
      // the position predicted by the smooth visual clock.  Ordinary
      // playback stays close to that prediction - a seek does not
      const qreal seekThreshold =
            qMax<qreal>(DIVISION / 8.0,
                        _playbackFollowTicksPerSecond * 0.10);

      const bool backwardJump =
            tick < _playbackFollowLastSampleTick;

      const bool forwardJump =
            qreal(tick) - predictedTick > seekThreshold;

      if (backwardJump || forwardJump) {
            _playbackFollowBaseTick = qreal(tick);
            _playbackFollowLastSampleTick = tick;
            _playbackFollowTicksPerSecond = newTicksPerSecond;
            _playbackFollowVelocityValid = true;
            _playbackFollowPaused = false;

            _playbackFollowElapsed.restart();

            pianoView->ensureVisible(tick);
            return;
            }

      // If playback was paused and the authoritative sequencer tick
      // has begun moving again, resume smooth visual interpolation
      // from the real position
      if (_playbackFollowPaused
          && tick != _playbackFollowLastSampleTick) {
            _playbackFollowPaused = false;
            _playbackFollowBaseTick = qreal(tick);
            _playbackFollowLastSampleTick = tick;
            _playbackFollowTicksPerSecond = newTicksPerSecond;
            _playbackFollowElapsed.restart();
            return;
            }

      // During a real TempoMap pause, the sequencer deliberately holds
      // the same tick while clock time continues. Freeze the smooth
      // visual clock at that authoritative position rather than
      // extrapolating through the pause
      if (tick == _playbackFollowLastSampleTick) {
            if (atPlaybackPause) {
                  _playbackFollowPaused = true;
                  _playbackFollowBaseTick = qreal(tick);
                  _playbackFollowElapsed.restart();
                  }

            return;
            }

      // If the tempo has changed, preserve the exact current visual
      // position while changing the slope from this point
      if (!qFuzzyCompare(newTicksPerSecond,
                         _playbackFollowTicksPerSecond)) {
            _playbackFollowBaseTick = predictedTick;
            _playbackFollowElapsed.restart();

            _playbackFollowTicksPerSecond =
                  newTicksPerSecond;
            }

      _playbackFollowLastSampleTick = tick;
      }

//---------------------------------------------------------
//   moveLocator
//---------------------------------------------------------

void PianorollEditor::moveLocator(int i, const Pos& p)
      {
      if (!locator[i].valid())
            return;

      const int tick = p.tick();

      // Locator 0 is the current playback position
      if (i == 0 && seq && seq->isPlaying()) {
            const int uTick =
                  score()->repeatList().tick2utick(tick);

            seq->seek(uTick);
            return;
            }

      if (i == 1 || i == 2) {
            preferences.setPreference(
                  PREF_APP_PLAYBACK_LOOPTOSELECTIONONPLAY,
                  false);
            }

      score()->setPos(POS(i), Fraction::fromTicks(tick));
      }

//---------------------------------------------------------
//   dataChanged
//---------------------------------------------------------

void PianorollEditor::dataChanged(const QRectF&)
      {
      }

//---------------------------------------------------------
//   removeScore
//---------------------------------------------------------

void PianorollEditor::removeScore()
      {
      _score = nullptr;
      setStaff(nullptr);
      }

//---------------------------------------------------------
//   changeEditElement
//---------------------------------------------------------

void PianorollEditor::changeEditElement(Element*)
      {
      }

//---------------------------------------------------------
//   cursor
//---------------------------------------------------------

QCursor PianorollEditor::cursor() const
      {
      return QCursor();
      }

//---------------------------------------------------------
//   setCursor
//---------------------------------------------------------

void PianorollEditor::setCursor(const QCursor&)
      {
      }

//---------------------------------------------------------
//   matrix
//---------------------------------------------------------

const QTransform& PianorollEditor::matrix() const
      {
      static QTransform t;
      return t;
      }

//---------------------------------------------------------
//   elementNear
//---------------------------------------------------------

Element* PianorollEditor::elementNear(QPointF)
      {
      return 0;
      }

//---------------------------------------------------------
//   updateNoteData
//---------------------------------------------------------

void PianorollEditor::updateNoteData()
      {
      pianoView->updateNotes();

      if (_showPianoLevels && pianoLevels)
            pianoLevels->updateNotes();
      }

//---------------------------------------------------------
//   updateAll
//---------------------------------------------------------

void PianorollEditor::updateAll()
      {
      if (updateScheduled)
            return;

      QTimer::singleShot(0, this, &PianorollEditor::doUpdate);
      updateScheduled = true;
      }

//---------------------------------------------------------
//   doUpdate
//---------------------------------------------------------

void PianorollEditor::doUpdate()
      {
      updateScheduled = false;

      const bool staffRemoved = staff && (staff->idx() == -1);
      if (staffRemoved) {
            removeScore();
            return;
            }

      updateNoteData();
      updateNoteShapeBox();
      }

//---------------------------------------------------------
//   applyPitchEdit
//---------------------------------------------------------

void PianorollEditor::applyPitchEdit()
      {
      if (!_score)
            return;

      const int newPitch = pitch->value();
      const int tpc = pitch->typedTpc();

      if (!pitchIsValid(newPitch) || !tpcIsValid(tpc))
            return;

      std::list<Note*> notes = _score->selection().uniqueNotes();
      if (notes.empty())
            return;

      _score->startCmd();

      for (Note* note : notes) {
            int newTpc1;
            int newTpc2;

            // The typed TPC describes the pitch spelling currently
            // presented to the user. Derive the corresponding other
            // TPC for concert/transposed representation
            if (note->concertPitch()) {
                  newTpc1 = tpc;
                  newTpc2 = note->transposeTpc(tpc);
                  }
            else {
                  newTpc2 = tpc;
                  newTpc1 = note->transposeTpc(tpc);
                  }

            if (note->pitch() == newPitch
                && note->tpc1() == newTpc1
                && note->tpc2() == newTpc2)
                  continue;

            if (note->accidental())
                  _score->undoRemoveElement(note->accidental());

            _score->undoChangePitch(
                  note,
                  newPitch,
                  newTpc1,
                  newTpc2);
            }

      _score->endCmd();

      // Immediately refresh PRE note-geometry and the tweak controls
      updateNoteData();
      updateSelection();
      pianoView->ensureSelectionVisible();
      }

//---------------------------------------------------------
//   redraw
//---------------------------------------------------------

void PianorollEditor::redraw() const
      {
      if (pianoView)
            pianoView->viewport()->update();
      if (pianoKbd)
            pianoKbd->update();
      if (pianoLevels && _showPianoLevels)
            pianoLevels->update();
      }

//---------------------------------------------------------
//   posChanged
//    position in score has changed
//---------------------------------------------------------

void PianorollEditor::posChanged(POS p, unsigned tick)
      {
      if (locator[int(p)].tick() == unsigned(tick))
            return;

      setLocator(p, tick);

      if (p != POS::CURRENT) {
            pianoView->moveLocator(int(p));
            ruler->update();

            if (_showPianoLevels && pianoLevels)
                  pianoLevels->update();

            return;
            }

      // The PianoView playback locator itself is still
      // horizontal-only. We'll avoid the full scene update
      // in vertical mode
      if (_orientation == PianoRollOrientation::HORIZONTAL
          && !_playbackFollowActive) {
            pianoView->moveLocator(int(p));
            }

      if (!ruler->playbackLocatorActive())
            ruler->update();

      // No continuous pianoLevels->update() for POS::CURRENT
      }

//---------------------------------------------------------
//   changeSelectedNoteEventValue
//---------------------------------------------------------

void PianorollEditor::changeSelectedNoteEventValue(NoteEventField field, int value)
      {
      QList<PianoItem*> items = pianoView->getSelectedItems();
      if (items.isEmpty())
            return;

      _score->startCmd();

      for (PianoItem* item : qAsConst(items)) {
            Note* note = item->note();
            NoteEvent* event = item->getTweakNoteEvent();

            if (!event)
                  continue;

            NoteEvent ne = *event;

            switch (field) {
                  case NoteEventField::ON_TIME:
                        if (event->ontime() == value)
                              continue;

                        ne.setOntime(value);
                        break;

                  case NoteEventField::LENGTH:
                        if (event->len() == value)
                              continue;

                        ne.setLen(value);
                        break;
                  }

            _score->undo(new ChangeNoteEvent(note, event, ne));
            }

      _score->endCmd();

      updateNoteData();
      }

//---------------------------------------------------------
//   onTimeChanged
//---------------------------------------------------------

void PianorollEditor::onTimeChanged(int val)
      {
      changeSelectedNoteEventValue(NoteEventField::ON_TIME, val);
      }

//---------------------------------------------------------
//   tickLenChanged
//---------------------------------------------------------

void PianorollEditor::tickLenChanged(int val)
      {
      changeSelectedNoteEventValue(NoteEventField::LENGTH, val);
      }

//---------------------------------------------------------
//   zoom
//---------------------------------------------------------

void PianorollEditor::zoom(int amount, bool horiz)
      {
      int cx = pianoView->width() / 2;
      int cy = pianoView->height() / 2;

      pianoView->zoomView(amount, horiz, cx, cy);
      }

}
