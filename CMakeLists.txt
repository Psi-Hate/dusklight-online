cmake_minimum_required(VERSION 3.25)

project(dusklight_online LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(DUSK_DIR "../dusklight" CACHE PATH "Path to the Dusklight engine checkout")
option(DUSK_ONLINE_BUILD_SERVER "Build the dedicated online server" ON)

if(NOT TARGET dusklight_game_headers)
    get_filename_component(DUSK_DIR_ABS "${DUSK_DIR}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    if(NOT EXISTS "${DUSK_DIR_ABS}/CMakeLists.txt")
        message(FATAL_ERROR "DUSK_DIR must point at the Dusklight engine checkout")
    endif()

    add_subdirectory("${DUSK_DIR_ABS}" "${CMAKE_BINARY_DIR}/dusklight" EXCLUDE_FROM_ALL)
endif()

add_dusk_mod(dusklight-online
    SOURCES
        src/mod/main.cpp
        src/mod/online_puppet_actor.cpp
    MOD_JSON mod.json
    RES_DIR res
)
target_include_directories(dusklight-online PRIVATE src)

if(DUSK_ONLINE_BUILD_SERVER)
    add_executable(dusklight-online-server
        src/server/main.cpp
    )
    target_include_directories(dusklight-online-server PRIVATE src)
    target_compile_features(dusklight-online-server PRIVATE cxx_std_20)
endif()
