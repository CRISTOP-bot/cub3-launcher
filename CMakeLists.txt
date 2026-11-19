# CUB3 Launcher - Build System
# Lightweight by Design. Performance by Default.
cmake_minimum_required(VERSION 3.21)

# ---------------------------------------------------------------------------
# Project metadata
# ---------------------------------------------------------------------------
project(cub3-launcher
    VERSION 0.1.0
    DESCRIPTION "CUB3 Launcher - A lightweight Minecraft launcher"
    LANGUAGES CXX
)

# ---------------------------------------------------------------------------
# Global C++ settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Qt auto-processing (MOC, UIC, RCC)
# ---------------------------------------------------------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ---------------------------------------------------------------------------
# Output directories — keep build artefacts out of the source tree
# ---------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ---------------------------------------------------------------------------
# Find dependencies
# ---------------------------------------------------------------------------
find_package(Qt6 6.2 REQUIRED COMPONENTS
    Widgets
    Network
    Sql
    Concurrent
)
find_package(Qt6Keychain REQUIRED CONFIG)
# ---------------------------------------------------------------------------
# Compile options tuned for performance without breaking compatibility
# ---------------------------------------------------------------------------
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
    )
endif()

# Per-configuration optimisation flags
#   Release: optimise for size and speed, strip assertions cheaply
#   Debug:   keep assertions, enable sanitiser-friendly defaults
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi /DQT_DEBUG /DCUB3_DEBUG")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -DQT_DEBUG -DCUB3_DEBUG")
endif()

# ---------------------------------------------------------------------------
# Platform detection
# ---------------------------------------------------------------------------
if(WIN32)
    add_compile_definitions(CUB3_PLATFORM_WINDOWS)
elseif(UNIX AND NOT APPLE)
    add_compile_definitions(CUB3_PLATFORM_LINUX)
endif()

# ---------------------------------------------------------------------------
# Collect sources
# ---------------------------------------------------------------------------
file(GLOB_RECURSE CUB3_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

# Qt resource collection
set(CUB3_RESOURCES
    resources/cub3.qrc
)

# ---------------------------------------------------------------------------
# Target
# ---------------------------------------------------------------------------
qt_add_executable(cub3 ${CUB3_SOURCES} ${CUB3_RESOURCES})

target_include_directories(cub3 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)

target_link_libraries(cub3 PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::Concurrent
    Qt6Keychain::Qt6Keychain
)

# ---------------------------------------------------------------------------
# Link-time optimisation in Release
# ---------------------------------------------------------------------------
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(cub3 PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)
endif()

# ---------------------------------------------------------------------------
# Windows specifics: subsystem, icon, console suppression
# ---------------------------------------------------------------------------
if(WIN32)
    set_target_properties(cub3 PROPERTIES
        WIN32_EXECUTABLE ON
        OUTPUT_NAME cub3
    )
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/cub3.rc")
        target_sources(cub3 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/cub3.rc")
    endif()
endif()

# ---------------------------------------------------------------------------
# Install rules
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS cub3
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install desktop entry on Linux
if(UNIX AND NOT APPLE)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/cub3-launcher.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
    )
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/cub3.svg
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
    )

    # Build a native Debian/Ubuntu package that registers CUB3 in the
    # application menu and declares its shared-library dependencies.
    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGE_NAME "cub3-launcher")
    set(CPACK_PACKAGE_VENDOR "CUB3")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "CUB3 Minecraft Launcher")
    set(CPACK_PACKAGE_CONTACT "CUB3 Launcher maintainers")
    set(CPACK_DEBIAN_PACKAGE_SECTION "games")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "CUB3 Launcher maintainers")
    # Shared-library dependencies are derived from the linked executable by
    # dpkg-shlibdeps. These runtime components are loaded dynamically and are
    # therefore not discoverable from ELF linkage alone.
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6sql6-sqlite, qt6-qpa-plugins, unzip")
    set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "default-jre-headless")
    # Development packages use a monotonically increasing Debian revision so
    # APT can upgrade them instead of treating each build as a downgrade.
    if(DEFINED CUB3_PACKAGE_REVISION AND NOT CUB3_PACKAGE_REVISION STREQUAL "")
        set(CPACK_DEBIAN_PACKAGE_VERSION
            "${PROJECT_VERSION}+dev.${CUB3_PACKAGE_REVISION}")
    endif()
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_PACKAGE_FILE_NAME
        "cub3-launcher-${PROJECT_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")
    include(CPack)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# Configuration summary
# ---------------------------------------------------------------------------
message(STATUS "=== CUB3 Launcher ===")
message(STATUS "  Version:      ${PROJECT_VERSION}")
message(STATUS "  Build type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler:     ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Qt6 found:     ${Qt6_DIR}")
