#=============================================================================
#  MuseScore
#  Music Composition & Notation
#
#  Copyright (C) 2020 MuseScore BVBA and others
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#=============================================================================

set(MODULE iex_musedata)

set(MODULE_SRC
    ${CMAKE_CURRENT_LIST_DIR}/musedatamodule.cpp
    ${CMAKE_CURRENT_LIST_DIR}/musedatamodule.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/musedatareader.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/musedatareader.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/musedata.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/musedata.h
    )

set(MODULE_LINK
    libmscore
    qzip
    notation
    )

include(${PROJECT_SOURCE_DIR}/build/module.cmake)

