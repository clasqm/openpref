# OpenPref - cross-platform Preferans game
#
# Qt 5 build for Haiku (and any other platform with Qt 5.15).
#
#   mkdir build && cd build && cmake .. && make
#
# The Qt 5 installation lives under /boot/system on Haiku; if CMake cannot find
# it, pass -DCMAKE_PREFIX_PATH=/boot/system/develop

cmake_minimum_required(VERSION 3.16)

project(openpref VERSION 0.1.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# The forms include "ui_<form>.h"; the .ui files all live in src/src/ui.
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/src/src/ui)

find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets)

set(OPENPREF_SOURCES
    src/src/main.cpp
    src/src/kpref.cpp
    src/src/prfconst.cpp

    src/src/util/baser.cpp
    src/src/util/debug.cpp
    src/src/util/ncounter.cpp

    src/src/cardutil/card.cpp
    src/src/cardutil/cardlist.cpp

    src/src/forms/formbid.cpp
    src/src/forms/helpbrowser.cpp
    src/src/forms/newgameform.cpp
    src/src/forms/optform.cpp
    src/src/forms/qbidbutton.cpp

    src/src/logic/aialphabeta.cpp
    src/src/logic/aiplayer.cpp
    src/src/logic/human.cpp
    src/src/logic/player.cpp

    src/src/score/plscore.cpp

    src/src/desk/desktop.cpp
    src/src/desk/deskview.cpp
)

set(OPENPREF_HEADERS
    src/src/main.h
    src/src/kpref.h
    src/src/prfconst.h

    src/src/util/baser.h
    src/src/util/debug.h
    src/src/util/ncounter.h

    src/src/cardutil/card.h
    src/src/cardutil/cardlist.h

    src/src/forms/formbid.h
    src/src/forms/helpbrowser.h
    src/src/forms/newgameform.h
    src/src/forms/optform.h
    src/src/forms/qbidbutton.h

    src/src/logic/aialphabeta.h
    src/src/logic/aiplayer.h
    src/src/logic/human.h
    src/src/logic/player.h

    src/src/score/plscore.h

    src/src/desk/desktop.h
    src/src/desk/deskview.h
)

set(OPENPREF_FORMS
    src/src/ui/helpbrowser.ui
    src/src/ui/newgameform.ui
    src/src/ui/optform.ui
)

add_executable(openpref
    ${OPENPREF_SOURCES}
    ${OPENPREF_HEADERS}
    ${OPENPREF_FORMS}
    src/openpref.qrc
)

# Sources include each other by bare name ("card.h", "baser.h", ...).
target_include_directories(openpref PRIVATE
    src/src
    src/src/util
    src/src/cardutil
    src/src/forms
    src/src/logic
    src/src/score
    src/src/desk
)

target_link_libraries(openpref PRIVATE Qt5::Widgets)
