# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
##Client test shared components:
#####################################################################

qt_manual_moc(moc_files
    mockcompositor.h
    coreprotocol.h
    corecompositor.h
    datadevice.h
    fullscreenshellv1.h
    fractionalscalev1.h
    iviapplication.h
    textinput.h
    qttextinput.h
    viewport.h
    xdgdialog.h
    xdgoutputv1.h
    xdgshell.h
)

add_library(SharedClientTest
    OBJECT
        corecompositor.cpp corecompositor.h
        coreprotocol.cpp coreprotocol.h
        datadevice.cpp datadevice.h
        fullscreenshellv1.cpp fullscreenshellv1.h
        fractionalscalev1.cpp fractionalscalev1.h
        iviapplication.cpp iviapplication.h
        mockcompositor.cpp mockcompositor.h
        textinput.cpp textinput.h
        qttextinput.cpp qttextinput.h
        xdgoutputv1.cpp xdgoutputv1.h
        xdgshell.cpp xdgshell.h
        xdgdialog.cpp xdgdialog.h
        viewport.cpp viewport.h
        ${moc_files}
)

qt6_generate_wayland_protocol_server_sources(SharedClientTest
    FILES
        ${PROJECT_SOURCE_DIR}/src/3rdparty/protocol/cursor-shape/cursor-shape-v1.xml
        ${PROJECT_SOURCE_DIR}/src/3rdparty/protocol/fullscreen-shell/fullscreen-shell-unstable-v1.xml
        ${PROJECT_SOURCE_DIR}/src/3rdparty/protocol/ivi/ivi-application.xml
        ${PROJECT_SOURCE_DIR}/src/3rdparty/protocol/wp-primary-selection/wp-primary-selection-unstable-v1.xml
        ${PROJECT_SOURCE_DIR}/src/3rdparty/protocol/tablet/tablet-unstable-v2.xml
        ${PROJECT_SOURCE_DIR}/src/3rdparty/protocol/text-input/v2/text-input-unstable-v2.xml
        ${PROJECT_SOURCE_DIR}/src/extensions/qt-text-input-method-unstable-v1.xml
        ${PROJECT_SOURCE_DIR}/src/3rdparty/protocol/fractional-scale/fractional-scale-v1.xml
        ${PROJECT_SOURCE_DIR}/src/3rdparty/protocol/viewporter/viewporter.xml
        ${PROJECT_SOURCE_DIR}/src/3rdparty/protocol/wayland/wayland.xml
        ${PROJECT_SOURCE_DIR}/src/3rdparty/protocol/xdg-decoration/xdg-decoration-unstable-v1.xml
        ${PROJECT_SOURCE_DIR}/src/3rdparty/protocol/xdg-dialog/xdg-dialog-v1.xml
        ${PROJECT_SOURCE_DIR}/src/3rdparty/protocol/xdg-output/xdg-output-unstable-v1.xml
        ${PROJECT_SOURCE_DIR}/src/3rdparty/protocol/xdg-shell/xdg-shell.xml
)

if(QT_FEATURE_opengl)
    set(optional_libraries Qt::OpenGL)
endif()

target_link_libraries(SharedClientTest
    PUBLIC
        Qt::Gui
        ${optional_libraries}
        Qt::WaylandClientPrivate
        Wayland::Server
        Threads::Threads # special case
)

target_include_directories(SharedClientTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
