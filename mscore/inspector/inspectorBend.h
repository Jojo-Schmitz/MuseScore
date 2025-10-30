//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2010-2019 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENSE.GPL
//=============================================================================

#ifndef __INSPECTOR_BEND_H__
#define __INSPECTOR_BEND_H__

#include "inspectorElementBase.h"
#include "ui_inspector_bend.h"

namespace Ms {

//---------------------------------------------------------
//   InspectorBend
//---------------------------------------------------------

class InspectorBend : public InspectorElementBase {
      Q_OBJECT

      Ui::InspectorBend g;

   private slots:
      void bendTypeChanged(int);
      void updateBend();

   public:
      InspectorBend(QWidget* parent);
      virtual void setElement() override;
      };

} // namespace Ms
#endif
