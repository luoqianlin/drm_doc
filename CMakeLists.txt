cmake_minimum_required(VERSION 3.21)
project(drm_test)

set(CMAKE_C_STANDARD 11)
include_directories(
        #        /usr/include/drm
        /usr/local/include/libdrm
)
link_directories(
        /usr/local/lib/x86_64-linux-gnu
)
add_compile_definitions(_POSIX_C_SOURCE=200809L
        )

link_libraries(
        drm
#        kms
)

add_executable(01_intro
        01_intro/src/main.c)


add_executable(02_modesetting
        02_modesetting/src/main.c
        02_modesetting/src/util.c
        )

add_executable(03_vsync
        03_vsync/src/main.c
        03_vsync/src/util.c

        )

add_executable(modeset-plane-test
        modeset-plane-test.cpp
        )

add_executable(modeset-atomic-crtc
        modeset-atomic-crtc.c
        )
add_executable(
        modeteset
        modetest.c
)

add_executable(
        modeset-atomic-plane
        modeset-atomic-plane.c
)

add_library(
        util
        util/format.c
        util/kms.c
        util/pattern.c
)

add_executable(
        modetest
        modetest/buffers.c
        modetest/cursor.c
        modetest/modetest.c
)
target_include_directories(
        modetest
        PUBLIC .
)

target_link_libraries(
        modetest
        util
        pthread
        drm

)

add_executable(
        bind_test
        bind_test.cpp
)