// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#include "inspectorChordBracket.h"
#include "resetButton.h"
#include "libmscore/property.h"

namespace Ms {
InspectorChordBracket::InspectorChordBracket(QWidget* parent)
   : InspectorElementBase(parent)
      {
      QWidget* container = addWidget();
      container->setObjectName("inspectorChordBracket");
      QVBoxLayout* box = new QVBoxLayout(container);
      box->setContentsMargins(0, 0, 0, 0);
      QToolButton* title = new QToolButton(container);
      title->setObjectName("chordBracketTitle");
      title->setText(tr("Chord bracket"));
      box->addWidget(title);
      QWidget* panel = new QWidget(container);
      box->addWidget(panel);
      QGridLayout* grid = new QGridLayout(panel);
      grid->setContentsMargins(3, 3, 3, 3);

      QComboBox* hooks = new QComboBox(panel);
      hooks->addItem(tr("Both"), int(Direction::AUTO));
      hooks->addItem(tr("Top (left hand)"), int(Direction::UP));
      hooks->addItem(tr("Bottom (right hand)"), int(Direction::DOWN));

      QDoubleSpinBox* length = new QDoubleSpinBox(panel);
      length->setRange(0.0, 20.0);
      length->setSingleStep(.1);
      length->setSuffix(tr(" sp"));

      QCheckBox* right = new QCheckBox(tr("Right side"), panel);
      QSpinBox* span = new QSpinBox(panel);
      span->setRange(1, 128);
      span->setToolTip(tr("Number of voices spanned, including the anchor voice.\n"
                          "For example, 5 reaches the same voice on the next staff."));

      std::vector<InspectorItem> items;
      auto addRow = [&](int row, const QString& label, QWidget* control, Pid property) {
            QLabel* caption = new QLabel(label, panel);
            caption->setBuddy(control);
            grid->addWidget(caption, row, 0);
            grid->addWidget(control, row, 1);
            ResetButton* reset = new ResetButton(panel);
            grid->addWidget(reset, row, 2);
            items.push_back({ property, 0, control, reset });
            };
      addRow(0, tr("Hooks:"), hooks, Pid::BRACKET_HOOK_POS);
      addRow(1, tr("Hook length:"), length, Pid::BRACKET_HOOK_LEN);
      addRow(2, tr("Placement:"), right, Pid::BRACKET_RIGHT_SIDE);
      // MuseScore Studio doesn't expose this to the user explicitly:
      // addRow(3, tr("Voice span:"), span, Pid::BRACKET_SPAN);
      mapSignals(items, { { title, panel } });
      }
}
