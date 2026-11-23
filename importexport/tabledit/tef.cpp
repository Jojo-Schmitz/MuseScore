//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//
//  Copyright (C) 2010-2026 Werner Schweer and others
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

#include "libmscore/score.h"

#include "importtef.h"

namespace Ms {

//---------------------------------------------------------
//   importTef
//---------------------------------------------------------

Score::FileError importTef(MasterScore* score, const QString& path)
      {
      qDebug("Score::importTef(%s)", qPrintable(path));

      QFile file(path);
      if(!file.exists())
            return Score::FileError::FILE_NOT_FOUND;
      if (!file.open(QIODevice::ReadOnly))
            return Score::FileError::FILE_OPEN_ERROR;
      tabledit::TablEdit tablEdit{ &file, score };
      Score::FileError err = tablEdit.import();
      qDebug("Score::importTef() done result %d", static_cast<int>(err));
      return Score::FileError::FILE_NO_ERROR;      // OK
      }

} // namespace Ms
