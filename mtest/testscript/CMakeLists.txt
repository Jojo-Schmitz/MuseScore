#=============================================================================
#  MuseScore
#  Music Composition & Notation
#
#  Copyright (C) 2018 Werner Schweer
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2
#  as published by the Free Software Foundation and appearing in
#  the file LICENSE.GPL
#=============================================================================

set(TARGET tst_runscripts)

include(${PROJECT_SOURCE_DIR}/mtest/CreateMtestTarget.cmake)

add_dependencies(tst_runscripts mscore)
add_definitions(-DMSCORE_EXECUTABLE_PATH="$<TARGET_FILE:mscore>")

if (MSVC)
      install(DIRECTORY
            ${CMAKE_INSTALL_PREFIX}/workspaces
            USE_SOURCE_PERMISSIONS
            DESTINATION $<TARGET_FILE_DIR:mscore>/..
            )
endif (MSVC)
