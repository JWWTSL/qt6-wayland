# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from libhybris-egl-server.pro.

#####################################################################
## LibHybrisEglServerBufferPlugin Plugin:
#####################################################################

qt_internal_add_plugin(LibHybrisEglServerBufferPlugin
    OUTPUT_NAME libhybris-egl-server
    PLUGIN_TYPE wayland-graphics-integration-client
    SOURCES
        ../../../../hardwareintegration/client/libhybris-egl-server/libhybriseglserverbufferintegration.cpp ../../../../hardwareintegration/client/libhybris-egl-server/libhybriseglserverbufferintegration.h
        main.cpp
    INCLUDE_DIRECTORIES
        ../../../../hardwareintegration/client/libhybris-egl-server
    PUBLIC_LIBRARIES
        EGL::EGL
        Qt::Core
        Qt::Gui
        Qt::WaylandClientPrivate
        Wayland::Client
    QT_LICENSE_ID QT_COMMERCIAL_OR_LGPL3
)

qt6_generate_wayland_protocol_client_sources(LibHybrisEglServerBufferPlugin
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../hardwareintegration/client/libhybris-egl-server/../../../extensions/libhybris-egl-server-buffer.xml
)

#### Keys ignored in scope 1:.:.:libhybris-egl-server.pro:<TRUE>:
# OTHER_FILES = "libhybris-egl-server.json"
