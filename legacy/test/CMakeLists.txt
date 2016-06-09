CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
SET(fw_test "${fw_name}-test")

INCLUDE_DIRECTORIES(../include/)
link_directories(${CMAKE_SOURCE_DIR}/../)

SET(WIN_PKG "${WIN_PKG} ecore-wayland")

INCLUDE(FindPkgConfig)
pkg_check_modules(${fw_test} REQUIRED libtbm mm-player appcore-efl elementary ecore evas capi-media-sound-manager ${WIN_PKG})
FOREACH(flag ${${fw_test}_CFLAGS})
    SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -pie -Wno-deprecated-declarations")
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -Wl,--rpath=/usr/lib/gstreamer-1.0")

aux_source_directory(. sources)
FOREACH(src ${sources})
    GET_FILENAME_COMPONENT(src_name ${src} NAME_WE)
    MESSAGE("${src_name}")
    ADD_EXECUTABLE(${src_name} ${src})
    TARGET_LINK_LIBRARIES(${src_name} legacy-player ${${fw_test}_LDFLAGS})
    INSTALL(TARGETS ${src_name} DESTINATION bin)
ENDFOREACH()

