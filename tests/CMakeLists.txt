# CUB3 Launcher - Tests
# Collect test sources
file(GLOB CUB3_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

if(NOT CUB3_TEST_SOURCES)
    message(STATUS "No test sources found; skipping test targets")
    return()
endif()

find_package(Qt6 REQUIRED COMPONENTS Test)
enable_testing()

# Build a single test binary per test file
foreach(TEST_SRC ${CUB3_TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SRC})
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
    )
    target_link_libraries(${TEST_NAME} PRIVATE
        Qt6::Test
        Qt6::Sql
        Qt6::Network
        ZLIB::ZLIB
    )
    if(TEST_NAME STREQUAL "DiscordPresenceTests")
        target_sources(${TEST_NAME} PRIVATE ../src/discord/DiscordPresence.cpp)
    elseif(TEST_NAME STREQUAL "EngineWorldTests")
        target_sources(${TEST_NAME} PRIVATE ../src/engine/World.cpp)
    elseif(TEST_NAME STREQUAL "CppEditionPackTests")
        target_sources(${TEST_NAME} PRIVATE ../src/application/CppEditionPack.cpp)
    elseif(TEST_NAME STREQUAL "NativeNbtTests")
        target_sources(${TEST_NAME} PRIVATE ../src/engine/Nbt.cpp)
    elseif(TEST_NAME STREQUAL "AnvilRegionTests")
        target_sources(${TEST_NAME} PRIVATE ../src/engine/AnvilRegion.cpp)
    endif()
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()
