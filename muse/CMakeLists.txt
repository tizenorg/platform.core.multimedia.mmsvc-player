CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
SET(service "muse")
SET(submodule "player")

SET(dependents "dlog mused mm-common libtbm capi-system-info")
SET(fw_name "${service}-${submodule}")

PROJECT(${fw_name})

SET(CMAKE_INSTALL_PREFIX /usr)
SET(PREFIX ${CMAKE_INSTALL_PREFIX})

SET(INC_DIR include)
INCLUDE_DIRECTORIES(${INC_DIR})

INCLUDE(FindPkgConfig)
pkg_check_modules(${fw_name} REQUIRED ${dependents})
FOREACH(flag ${${fw_name}_CFLAGS})
    SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "-I./include -I../legacy/include ${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -fPIC -Wall -Werror -Wno-deprecated -Wno-deprecated-declarations")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g")

IF("${ARCH}" STREQUAL "arm")
    ADD_DEFINITIONS("-DTARGET")
ENDIF("${ARCH}" STREQUAL "arm")

IF(WAYLAND_SUPPORT)
    ADD_DEFINITIONS("-DHAVE_WAYLAND")
ENDIF(WAYLAND_SUPPORT)
IF(X11_SUPPORT)
    ADD_DEFINITIONS("-DHAVE_X11")
ENDIF(X11_SUPPORT)

ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
ADD_DEFINITIONS("-DTIZEN_DEBUG")

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -Wl,--rpath=${LIB_INSTALL_DIR}")

aux_source_directory(src MUSED_SOURCES)
ADD_LIBRARY(${fw_name} SHARED ${MUSED_SOURCES})

TARGET_LINK_LIBRARIES(${fw_name} legacy-player ${${fw_name}_LDFLAGS})

SET_TARGET_PROPERTIES(${fw_name}
     PROPERTIES
     VERSION ${FULLVER}
     SOVERSION ${MAJORVER}
     CLEAN_DIRECT_OUTPUT 1
)

INSTALL(TARGETS ${fw_name} DESTINATION ${LIB_INSTALL_DIR})
INSTALL(
        DIRECTORY ${INC_DIR}/ DESTINATION include/media
        FILES_MATCHING
        PATTERN "*_private.h" EXCLUDE
        PATTERN "${INC_DIR}/*.h"
        )

