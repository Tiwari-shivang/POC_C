cmake_minimum_required(VERSION 3.16)
project(car_poc LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(HEADLESS "Build without SDL2 (CSV replayer)" ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -O0 -g3 -Wall -Wextra -Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wconversion -Wsign-conversion -Wformat=2 -Wundef")

include_directories(inc cfg)

set(COMMON_SOURCES
    src/app_autobrake.c
    src/app_wipers.c
    src/app_speedgov.c
    src/app_autopark.c
    src/app_climate.c
    src/app_voice.c
    src/io_logger.c
    sim/scenario.c
)

if(HEADLESS)
    set(PLATFORM_SOURCES
        src/main.c
        src/platform_pc.c
        src/hal_mock_pc.c
        ${COMMON_SOURCES}
    )
    add_definitions(-DHEADLESS_BUILD=1)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    
    set(PLATFORM_SOURCES
        src/main.c
        src/platform_sdl.c
        src/hal_sdl.c
        ${COMMON_SOURCES}
    )
    add_definitions(-DHEADLESS_BUILD=0)
endif()

add_executable(car_poc ${PLATFORM_SOURCES})

if(NOT HEADLESS)
    target_link_libraries(car_poc ${SDL2_LIBRARIES})
    target_include_directories(car_poc PRIVATE ${SDL2_INCLUDE_DIRS})
    target_compile_options(car_poc PRIVATE ${SDL2_CFLAGS_OTHER})
endif()

enable_testing()

set(TEST_SOURCES
    tests/test_autobrake.c
    tests/test_wipers.c
    tests/test_speedgov.c
    tests/test_autopark.c
    tests/test_climate.c
    tests/unity/unity.c
)

foreach(TEST_SOURCE ${TEST_SOURCES})
    if(NOT ${TEST_SOURCE} MATCHES "unity.c")
        get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
        add_executable(${TEST_NAME} ${TEST_SOURCE} tests/unity/unity.c)
        target_include_directories(${TEST_NAME} PRIVATE tests/unity inc cfg)
        target_link_libraries(${TEST_NAME} m)
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endif()
endforeach()

add_custom_target(static_analysis
    COMMAND ${CMAKE_SOURCE_DIR}/tools/run_static.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running static analysis"
)