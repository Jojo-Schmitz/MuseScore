//=============================================================================
//  Awl
//  Audio Widget Library
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
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

#ifndef __AWLPITCHEDIT_H__
#define __AWLPITCHEDIT_H__

#include "libmscore/pitchspelling.h"

namespace Awl {

//---------------------------------------------------------
//   PitchEdit
//---------------------------------------------------------

class PitchEdit : public QSpinBox {
      Q_OBJECT

      bool deltaMode;
      int _typedTpc { Ms::TPC_INVALID };
      bool _mixedPitch { false };

   protected:
      virtual QString textFromValue(int v) const;
      void stepBy(int steps) override;
      virtual int valueFromText(const QString& text) const;
      bool parsePitchText(const QString&, int&, int&) const;
      QValidator::State validate(QString& input, int& pos) const override;
      virtual void keyPressEvent(QKeyEvent*);

   signals:
      void returnPressed();
      void escapePressed();

   public:
      PitchEdit(QWidget* parent = 0);
      void setDeltaMode(bool);
      void setPitch(int pitch, int tpc);
      void setMixedPitch();
      int typedTpc() const;
      };
}

#endif
