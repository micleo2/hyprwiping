cmake_minimum_required(VERSION 3.20)
project(wiping C CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-egl)
pkg_check_modules(EGL REQUIRED egl)
pkg_check_modules(GLES REQUIRED glesv2)
pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)

find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

set(XDG_SHELL_PROTO "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml")
set(LAYER_SHELL_PROTO "${CMAKE_SOURCE_DIR}/protocol/wlr-layer-shell-unstable-v1.xml")
set(PROTO_DIR "${CMAKE_BINARY_DIR}/protocol")
file(MAKE_DIRECTORY ${PROTO_DIR})

# generate protocol code
add_custom_command(
    OUTPUT ${PROTO_DIR}/xdg-shell-protocol.h
    COMMAND ${WAYLAND_SCANNER} client-header ${XDG_SHELL_PROTO} ${PROTO_DIR}/xdg-shell-protocol.h
    DEPENDS ${XDG_SHELL_PROTO})

add_custom_command(
    OUTPUT ${PROTO_DIR}/xdg-shell-protocol.c
    COMMAND ${WAYLAND_SCANNER} private-code ${XDG_SHELL_PROTO} ${PROTO_DIR}/xdg-shell-protocol.c
    DEPENDS ${XDG_SHELL_PROTO})

add_custom_command(
    OUTPUT ${PROTO_DIR}/wlr-layer-shell-protocol.h
    COMMAND ${WAYLAND_SCANNER} client-header ${LAYER_SHELL_PROTO} ${PROTO_DIR}/wlr-layer-shell-protocol.h
    DEPENDS ${LAYER_SHELL_PROTO})

add_custom_command(
    OUTPUT ${PROTO_DIR}/wlr-layer-shell-protocol.c
    COMMAND ${WAYLAND_SCANNER} private-code ${LAYER_SHELL_PROTO} ${PROTO_DIR}/wlr-layer-shell-protocol.c
    DEPENDS ${LAYER_SHELL_PROTO})

# protocol sources stay as C
add_library(wiping_proto OBJECT
    ${PROTO_DIR}/xdg-shell-protocol.h
    ${PROTO_DIR}/xdg-shell-protocol.c
    ${PROTO_DIR}/wlr-layer-shell-protocol.h
    ${PROTO_DIR}/wlr-layer-shell-protocol.c)
target_include_directories(wiping_proto PRIVATE ${PROTO_DIR})

add_executable(wiping
    src/main.cpp
    src/wayland.cpp
    src/render.cpp)

target_link_libraries(wiping PRIVATE
    wiping_proto
    ${WAYLAND_LIBRARIES}
    ${EGL_LIBRARIES}
    ${GLES_LIBRARIES}
    m)

target_include_directories(wiping PRIVATE
    src
    ${PROTO_DIR}
    ${WAYLAND_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${GLES_INCLUDE_DIRS})

target_compile_options(wiping PRIVATE -Wall -Wextra)
