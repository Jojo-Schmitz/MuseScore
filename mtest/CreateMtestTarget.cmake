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

add_executable(
      ${TARGET}
      ${ui_headers}
      ${mocs}
      ${TARGET}.cpp
      )

if (MTEST_LINK_MSCOREAPP)
      target_link_libraries(${TARGET} testutils_mscoreapp mscoreapp)
else (MTEST_LINK_MSCOREAPP)
      target_link_libraries(${TARGET} testutils)
endif (MTEST_LINK_MSCOREAPP)

if (MSVC)
      include(FindStaticLibrary)
endif (MSVC)

target_link_libraries(
      ${TARGET}
      ${QT_LIBRARIES}
      ${QT_QTTEST_LIBRARY}
      testResources
      libmscore
      audio
      qzip
      )

if (OMR)
      target_link_libraries(${TARGET} omr poppler-qt5)
      if (OCR)
            target_link_libraries(${TARGET} tesseract_api)
      endif (OCR)
endif (OMR)

# ZIP library
if (MSVC)
      target_link_libraries(
            ${TARGET}
            zlibstat
            )
else (MSVC)
      target_link_libraries(
            ${TARGET}
            z
            )
endif (MSVC)

if (USE_SYSTEM_FREETYPE)
      target_link_libraries(${TARGET} freetype)
else (USE_SYSTEM_FREETYPE)
      target_link_libraries(${TARGET} mscore_freetype)
endif (USE_SYSTEM_FREETYPE)

if (NOT MINGW AND NOT APPLE AND NOT MSVC)
   target_link_libraries(${TARGET}
      dl
      pthread)
endif (NOT MINGW AND NOT APPLE AND NOT MSVC)

if (APPLE)
target_link_libraries(${TARGET} ${OsxFrameworks})
target_link_libraries(${TARGET}
      dl
      pthread
      )
set_target_properties (
      ${TARGET}
      PROPERTIES
      AUTOMOC true
      COMPILE_FLAGS "-D QT_GUI_LIB -D TESTROOT=\\\"${PROJECT_SOURCE_DIR}\\\" -g -Wall -Wextra"
      LINK_FLAGS    "-g -stdlib=libc++"
      )
else(APPLE)
      if (MSVC)
            set_target_properties (
                  ${TARGET}
                  PROPERTIES
                  AUTOMOC true
                  COMPILE_FLAGS "/Zi /D QT_GUI_LIB  /D TESTROOT=\\\"${PROJECT_SOURCE_DIR}\\\""
                  )
      else (MSVC)
            set_target_properties (
                  ${TARGET}
                  PROPERTIES
                  AUTOMOC true
                  COMPILE_FLAGS "-D QT_GUI_LIB -D TESTROOT=\\\"${PROJECT_SOURCE_DIR}\\\" -g -Wall -Wextra"
                  LINK_FLAGS    "-g"
                  )
      endif (MSVC)
endif(APPLE)

if (BUILD_PCH)
      target_use_pch(${TARGET})
endif (BUILD_PCH)

add_test(${TARGET} ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}  -xunitxml -o result.xml)

# On Windows some tests need access to supporting files
# MSVC has a different definition for CMAKE_CURRENT_BINARY_DIR
# so we use this install destination variable instead
if (MSVC)
      set_target_properties(
            ${TARGET}
            PROPERTIES
            MTEST_INSTALL_DESTINATION_DIR "${CMAKE_CURRENT_BINARY_DIR}"
            )
else (MSVC)
      set_target_properties(
            ${TARGET}
            PROPERTIES
            MTEST_INSTALL_DESTINATION_DIR "${CMAKE_CURRENT_BINARY_DIR}/.."
            )
endif (MSVC)
