/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2025 MuseScore Limited
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef __TUPLETHANDLER_H__
#define __TUPLETHANDLER_H__

#include "importtef.h"

#include "libmscore/chordrest.h"

using namespace Ms;
using namespace std;

namespace tabledit {
class TupletHandler
{
public:
    Ms::Fraction doTuplet(const TefNote* const tefNote); // todo rename
    void addCr(Ms::Measure* measure, Ms::ChordRest* cr);
private:
    int count { 0 };  // support overly simple algorithm: simply count notes
    bool inTuplet { false };
    int totalLength { 0 }; // sum of note duration in TablEdit units
    Ms::Tuplet* tuplet { nullptr };
};
} // namespace tabledit
#endif
