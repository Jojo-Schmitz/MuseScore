#=============================================================================
#  MuseScore
#  Music Composition & Notation
#
#  Copyright (C) 2011 Werner Schweer
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2
#  as published by the Free Software Foundation and appearing in
#  the file LICENSE.GPL
#=============================================================================

set(TARGET tst_sfzincludes)

include(${PROJECT_SOURCE_DIR}/mtest/CreateMtestTarget.cmake)

include_directories(
      ${SNDFILE_INCDIR}
      )

if (MSVC OR MINGW)
      target_link_libraries(tst_sfzincludes audio audiofile sndfiledll testutils)
else (MSVC OR MINGW)
      target_link_libraries(tst_sfzincludes audio audiofile ${SNDFILE_LIB} testutils)
endif (MSVC OR MINGW)
