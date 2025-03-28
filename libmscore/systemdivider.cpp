//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2015-2016 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "measure.h"
#include "score.h"
#include "systemdivider.h"
#include "xml.h"

namespace Ms {

//---------------------------------------------------------
//   SystemDivider
//---------------------------------------------------------

SystemDivider::SystemDivider(Score* s) : Symbol(s, ElementFlag::SYSTEM | ElementFlag::NOT_SELECTABLE)
      {
      // default value, but not valid until setDividerType()
      _dividerType = SystemDivider::Type::LEFT;
      _sym = SymId::systemDivider;
      }

//---------------------------------------------------------
//   SystemDivider
//---------------------------------------------------------

SystemDivider::SystemDivider(const SystemDivider& sd)
   : Symbol(sd)
      {
      _dividerType = sd._dividerType;
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void SystemDivider::layout()
      {
      SymId sid;
      ScoreFont* sf = score()->scoreFont();

      if (_dividerType == SystemDivider::LEFT)
            sid = Sym::name2id(score()->styleSt(Sid::dividerLeftSym));
      else
            sid = Sym::name2id(score()->styleSt(Sid::dividerRightSym));
      setSym(sid, sf);
      Symbol::layout();
      }

//---------------------------------------------------------
//   setDividerType
//---------------------------------------------------------

void SystemDivider::setDividerType(SystemDivider::Type v)
      {
      _dividerType = v;
      if (v == SystemDivider::LEFT)
            setOffset(QPointF(score()->styleD(Sid::dividerLeftX), score()->styleD(Sid::dividerLeftY)));
      else
            setOffset(QPointF(score()->styleD(Sid::dividerRightX), score()->styleD(Sid::dividerRightY)));
      }

//---------------------------------------------------------
//   drag
//---------------------------------------------------------

QRectF SystemDivider::drag(EditData& ed)
      {
      setGenerated(false);
      return Symbol::drag(ed);
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void SystemDivider::write(XmlWriter& xml) const
      {
      if (dividerType() == SystemDivider::Type::LEFT)
            xml.stag(this, "type=\"left\"");
      else
            xml.stag(this, "type=\"right\"");
      writeProperties(xml);
      xml.etag();
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void SystemDivider::read(XmlReader& e)
      {
      ScoreFont* sf = score()->scoreFont();
      if (e.attribute("type") == "left") {
            _dividerType = SystemDivider::Type::LEFT;
            SymId sym = Sym::name2id(score()->styleSt(Sid::dividerLeftSym));
            setSym(sym, sf);
            setOffset(QPointF(score()->styleD(Sid::dividerLeftX), score()->styleD(Sid::dividerLeftY)));
            }
      else {
            _dividerType = SystemDivider::Type::RIGHT;
            SymId sym = Sym::name2id(score()->styleSt(Sid::dividerRightSym));
            setSym(sym, sf);
            setOffset(QPointF(score()->styleD(Sid::dividerRightX), score()->styleD(Sid::dividerRightY)));
            }
      Symbol::read(e);
      }
}  // namespace Ms


