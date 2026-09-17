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

#ifndef __PIANOROLL_H__
#define __PIANOROLL_H__

namespace Awl {
      class PitchEdit;
      class PitchLabel;
      class PosLabel;
      };

#include "libmscore/mscoreview.h"
#include "libmscore/pos.h"
#include "libmscore/score.h"
#include "libmscore/select.h"
#include "pianorolledittool.h"

#include <QElapsedTimer>
#include <QTimer>

namespace Ms {

class Score;
class Staff;
class PianoView;
class PianoKeyboard;
class PianoLevels;
class PianoLevelsChooser;
class NoteTweakerDialog;
class Note;
class PianoRuler;
class Seq;

enum class PianoRollNoteShape : char;

//---------------------------------------------------------
//   PianorollEditor
//---------------------------------------------------------

class PianorollEditor : public QWidget, public MuseScoreView {
      Q_OBJECT

      enum class NoteEventField {
            ON_TIME,
            LENGTH
            };

      QToolBar* tbMain { nullptr };
      QToolBar* tbTool { nullptr };
      QToolBar* tbNoteLen { nullptr };
      QToolBar* tbDots { nullptr };
      QToolBar* tbVoices { nullptr };
      QToolBar* tbTweak { nullptr };
      QToolButton* tieToolButton { nullptr };
      QToolButton* cutToolButton { nullptr };

      QButtonGroup* bngrpNoteLen { nullptr };
      QButtonGroup* bngrpNoteDot { nullptr };
      QButtonGroup* bngrpActionBns { nullptr };

      PianoView* pianoView { nullptr };
      PianoKeyboard* pianoKbd { nullptr };
      PianoLevels* pianoLevels { nullptr };
      PianoLevelsChooser* pianoLevelsChooser { nullptr };
      QWidget* levelsAreaWidget { nullptr };
      bool _showPianoLevels { true };
      QScrollBar* hsb { nullptr };        // horizontal scroll bar for pianoView
      QGridLayout* noteAreaLayout { nullptr };
      QWidget* topLeftSpacer { nullptr };

      QToolButton* controlsChevronButton { nullptr };
      QWidget* verticalCornerSpacer { nullptr };
      QWidget* toolbarArea { nullptr };
      bool _controlsVisible { true };

      Score* _score { nullptr };
      Staff* staff { nullptr };
      QComboBox* staffBox { nullptr };
      QComboBox* scopeBox { nullptr };
      QComboBox* noteShapeBox { nullptr };
      Awl::PitchEdit* pitch { nullptr };
      QSpinBox* velocity { nullptr };
      QSpinBox* onTime { nullptr };
      QSpinBox* tickLen { nullptr };
      Pos locator[3];
      QComboBox* barPattern { nullptr };
      QComboBox* veloType { nullptr };
      QSpinBox* subdiv { nullptr };
      QSpinBox* tuplet { nullptr };
      Awl::PosLabel* pos { nullptr };
      PianoRuler* ruler { nullptr };
      QCheckBox* keyboardAlignedGrid { nullptr };
      QAction* keyboardAlignedGridAction { nullptr };
      QAction* keyboardAlignedGridSeparator { nullptr };
      QAction* automaticVoiceAction { nullptr };
      QAction* automaticVoiceSeparator { nullptr };
      QSplitter* split { nullptr };
      PianoRollScope _scope;
      PianoRollOrientation _orientation;
      Coloring _coloring;
      bool _useNoteColors;

      int _horizontalPitchScrollPos { 0 };
      int _verticalPitchScrollPos { 0 };
      bool _horizontalPitchScrollValid { false };
      bool _verticalPitchScrollValid { false };

      int _previewOnTime { 0 };
      int _previewLen { 1000 };

      bool _playbackFollowScrolling { false };
      QTimer* _playbackFollowTimer { nullptr };
      QElapsedTimer _playbackFollowElapsed;
      qreal _playbackFollowBaseTick { 0.0 };
      unsigned _playbackFollowLastSampleTick { 0 };
      bool _playbackFollowActive { false };
      qreal _playbackFollowTicksPerSecond { 0.0 };
      bool _playbackFollowVelocityValid { false };
      bool _playbackFollowPaused { false };

      bool updateScheduled = false;
      NoteTweakerDialog* noteTweakerDlg { nullptr };

      QString staffDisplayName(Staff*) const;
      void updateNoteLengthControls(const Fraction& duration);
      void updateEditableStaffUi();
      void updateNoteShapeBox();
      void setPianoRollNoteShape(PianoRollNoteShape shape);
      void updateStaffBox();
      void updateVelocity(Note* note);
      void updateSelection();
      void updateNoteData();
      void updateNoteShapeToolState();
      void changeSelectedNoteEventValue(NoteEventField field, int value);
      void readSettings();
      void doUpdate();
      void applyPitchEdit();
      void redraw() const;
      void regroupSelectedVoicing();
      void createEditToolbars(const QSize& iconSize);
      Awl::PitchLabel* createTweakToolbar(const QSize& iconSize);
      void connectEditorSignals(Awl::PitchLabel* pitchLabel, QComboBox* coloringBox);

      void updatePlaybackFollow();
      void stopPlaybackFollow();

   private slots:
      void selectionChanged();
      void veloTypeChanged(int);
      void velocityChanged(int);
      void keyPressed(int);
      void keyReleased(int);
      void moveLocator(int, const Pos&);
      void rangeChanged(int min, int max);
      void setXpos(int x);
      void posChanged(POS pos, unsigned tick);
      void tickLenChanged(int);
      void onTimeChanged(int val);

   public slots:
      void changeSelection(SelState);
      void handleAction(QAction*);
      void showNoteTweaker();
      void setOnTime(int);
      void setTickLen(int);
      void setPianoLevelsVisible(bool visible);

   public:
      PianorollEditor(QWidget* parent = 0);
      virtual ~PianorollEditor();

      bool eventFilter(QObject* obj, QEvent* event) override;

      Score* score() const { return _score; }

      void restoreScoreViewFocus();

      void setEditableStaff(Staff*);
      void setStaff(Staff* staff);
      void focusOnPosition(Position* p);
      void heartBeat(Seq*);

      void setEditNoteLength(int);
      void setEditNoteVoice(int);
      void setEditNoteTool(PianoRollEditTool);
      void setEditNoteDots(int, QToolButton*);

      virtual void dataChanged(const QRectF&) override;
      virtual void updateAll() override;
      virtual void removeScore() override;
      virtual void changeEditElement(Element*) override;
      virtual QCursor cursor() const override;
      virtual void setCursor(const QCursor&) override;
      const QTransform& matrix() const;
      virtual Element* elementNear(QPointF) override;
      virtual void drawBackground(QPainter* /*p*/, const QRectF& /*r*/) const override {}

      void clearPlaybackPitches();

      void setLocator(POS posi, int tick) { locator[int(posi)].setTick(tick); }

      void updateToolbarIconSize();
      void updateOrientationLayout();
      void setOrientation(PianoRollOrientation);
      void setScope(PianoRollScope scope);
      void setColoring(Coloring);
      void setUseNoteColors(bool);


      void writeSettings();
      virtual const QRect geometry() const override { return QWidget::geometry(); }

      void zoom(int amount = 1, bool horiz = true);
      };


} // namespace Ms
#endif


