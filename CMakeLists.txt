cmake_minimum_required(VERSION 3.28)

# Top-level only: MSYS2 moc DLL search (0xC0000135 without ucrt64/bin).
# As a subdirectory the parent owns the Qt toolchain / PATH.
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    include("${CMAKE_CURRENT_LIST_DIR}/cmake/msys-qt-env.cmake")
endif ()

project(irbis VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(STATIC_BUILD OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/ui
        ${CMAKE_CURRENT_SOURCE_DIR}/ui/tabs
        ${CMAKE_CURRENT_SOURCE_DIR}/ui/dialogs
)

if (NOT Qt6_DIR)
    if (EXISTS "C:/msys64/ucrt64/lib/cmake/Qt6/Qt6Config.cmake")
        set(Qt6_DIR "C:/msys64/ucrt64/lib/cmake/Qt6" CACHE PATH "Qt6 CMake package dir")
        list(PREPEND CMAKE_PREFIX_PATH "C:/msys64/ucrt64")
    elseif (EXISTS "C:/Qt/Qt6.8.0/mingw_64/lib/cmake/Qt6/Qt6Config.cmake")
        set(Qt6_DIR "C:/Qt/Qt6.8.0/mingw_64/lib/cmake/Qt6" CACHE PATH "Qt6 CMake package dir")
        list(PREPEND CMAKE_PREFIX_PATH "C:/Qt/Qt6.8.0/mingw_64")
    endif ()
endif ()
find_package(Qt6 COMPONENTS Widgets Charts Svg REQUIRED)

if (NOT TARGET numina::numina)
    set(NUMINA_ROOT "C:/cpp/projects/static-libs/numina" CACHE PATH "Path to numina sources")
    if (NOT EXISTS "${NUMINA_ROOT}/CMakeLists.txt")
        message(FATAL_ERROR "numina not found at NUMINA_ROOT=${NUMINA_ROOT}")
    endif ()
    add_subdirectory("${NUMINA_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/numina_build")
endif ()

include(cmake/sources.cmake)
file(GLOB_RECURSE irbis_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/irbis/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/irbis/*.hpp"
)

# Library target is irbis-ui so the desktop executable can keep the historical
# name `irbis` (CLion run config). Consumers link irbis::irbis.
set(IRBIS_LIB irbis-ui)
qt_add_library(${IRBIS_LIB} STATIC ${irbis_SOURCES} ${irbis_HEADERS})
add_library(irbis::irbis ALIAS ${IRBIS_LIB})
if (NOT PROJECT_IS_TOP_LEVEL)
    add_library(irbis ALIAS ${IRBIS_LIB})
endif ()

target_include_directories(${IRBIS_LIB}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_features(${IRBIS_LIB} PUBLIC cxx_std_23)

target_link_libraries(${IRBIS_LIB} PUBLIC
        Qt6::Widgets
        Qt6::Charts
        Qt6::Svg
        numina::numina
)

include(cmake/compiler-flags.cmake)
irbis_apply_flags(${IRBIS_LIB})

if (CCACHE_FOUND)
    set_property(TARGET ${IRBIS_LIB} PROPERTY CXX_COMPILER_LAUNCHER ccache)
endif ()

if (PROJECT_IS_TOP_LEVEL)
    include(cmake/top-level.cmake)
endif ()
