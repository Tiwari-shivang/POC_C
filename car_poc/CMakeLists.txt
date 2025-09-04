cmake_minimum_required(VERSION 3.16)
project(car_poc LANGUAGES C)
 
# -------- C standard & warnings --------
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
 
option(HEADLESS "Build without SDL2 (CSV replayer)" ON)
 
if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_options(/W3)
else()
    add_compile_options(-std=c99 -O0 -g3 -Wall -Wextra -Werror
                        -Wconversion -Wsign-conversion -Wformat=2 -Wundef)
endif()
 
include_directories(inc cfg sim)
 
# -------- Sources --------
set(COMMON_SOURCES
    src/app_autobrake.c
    src/app_wipers.c
    src/app_speedgov.c
    src/app_autopark.c
    src/app_climate.c
    src/app_voice.c
    src/io_logger.c
    src/speed_sign_vision.c
    sim/scenario.c
)
 
# Decide platform sources (headless vs SDL)
set(PLATFORM_SOURCES)
set(USE_SDL_CONFIG FALSE)
set(USE_SDL_PKG    FALSE)
set(USE_SDL_MANUAL FALSE)
 
if(HEADLESS)
    list(APPEND PLATFORM_SOURCES
        src/main.c
        src/platform_pc.c
        src/hal_mock_pc.c
        ${COMMON_SOURCES}
    )
    add_compile_definitions(HEADLESS_BUILD=1 TEXT_UI_MODE=0)
else()
    # ---- Prefer SDL2 "config" mode (dev ZIP provides SDL2Config.cmake) ----
    # Pass -DSDL2_DIR=C:/SDKs/SDL2-2.30.12/cmake to CMake.
    find_package(SDL2 CONFIG QUIET)
    if(TARGET SDL2::SDL2 AND TARGET SDL2::SDL2main)
        set(USE_SDL_CONFIG TRUE)
    else()
        # ---- Fallback: pkg-config (Linux/macOS) ----
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(SDL2 QUIET sdl2)
            if(SDL2_FOUND OR SDL2_INCLUDE_DIRS)
                set(USE_SDL_PKG TRUE)
            endif()
        endif()
    endif()
 
    # ---- Last resort: manual path (set -DSDL2_ROOT=C:/Users/shivang.tiwari/SDL2-2.30.12) ----
    if(NOT USE_SDL_CONFIG AND NOT USE_SDL_PKG)
        if(NOT DEFINED SDL2_ROOT)
            set(SDL2_ROOT "C:/Users/shivang.tiwari/SDL2-2.30.12" CACHE PATH "Path to SDL2 dev package")
        endif()
        if(EXISTS "${SDL2_ROOT}/include/SDL.h")
            set(USE_SDL_MANUAL TRUE)
        endif()
    endif()
 
    # Choose SDL platform sources (if none of the above found, we still build text UI)
    if(USE_SDL_CONFIG OR USE_SDL_PKG OR USE_SDL_MANUAL)
        list(APPEND PLATFORM_SOURCES
            src/main.c
            src/platform_sdl.c
            src/hal_sdl.c
            ${COMMON_SOURCES}
        )
        add_compile_definitions(HEADLESS_BUILD=0 TEXT_UI_MODE=0)
    else()
        message(WARNING "SDL2 not found; falling back to text interactive UI (no window).")
        list(APPEND PLATFORM_SOURCES
            src/main.c
            src/platform_pc.c
            src/hal_interactive.c
            ${COMMON_SOURCES}
        )
        add_compile_definitions(HEADLESS_BUILD=0 TEXT_UI_MODE=1)
    endif()
endif()
 
# -------- OpenCV Support --------
option(USE_OPENCV "Enable OpenCV for speed sign recognition" OFF)
if(USE_OPENCV)
    find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
    if(OpenCV_FOUND)
        add_compile_definitions(USE_OPENCV=1)
        message(STATUS "OpenCV ${OpenCV_VERSION} found")
    else()
        message(WARNING "OpenCV not found; speed sign recognition will be disabled")
        set(USE_OPENCV OFF)
    endif()
endif()

# -------- Target --------
add_executable(car_poc ${PLATFORM_SOURCES})
 
# -------- Link SDL2 if used --------
if(NOT HEADLESS)
    if(USE_SDL_CONFIG)
        target_link_libraries(car_poc PRIVATE SDL2::SDL2 SDL2::SDL2main)
        # Try to copy the runtime DLL next to the exe (Windows)
        if(WIN32)
            # Common locations relative to SDL2_DIR=<...>/cmake
            if(DEFINED SDL2_DIR)
                get_filename_component(_SDL2_BASE "${SDL2_DIR}" DIRECTORY) # -> /SDL2-2.30.12
                if(EXISTS "${_SDL2_BASE}/lib/x64/SDL2.dll")
                    set(SDL2_DLL "${_SDL2_BASE}/lib/x64/SDL2.dll")
                elseif(EXISTS "${_SDL2_BASE}/bin/SDL2.dll")
                    set(SDL2_DLL "${_SDL2_BASE}/bin/SDL2.dll")
                endif()
                if(SDL2_DLL)
                    add_custom_command(TARGET car_poc POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                                "${SDL2_DLL}"
                                "$<TARGET_FILE_DIR:car_poc>/SDL2.dll"
                        COMMENT "Copying SDL2.dll to output dir")
                endif()
            endif()
        endif()
    elseif(USE_SDL_PKG)
        target_include_directories(car_poc PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_libraries(car_poc PRIVATE ${SDL2_LIBRARIES})
        if(SDL2_CFLAGS_OTHER)
            target_compile_options(car_poc PRIVATE ${SDL2_CFLAGS_OTHER})
        endif()
    elseif(USE_SDL_MANUAL)
        target_include_directories(car_poc PRIVATE "${SDL2_ROOT}/include")
        if(WIN32)
            target_link_directories(car_poc PRIVATE "${SDL2_ROOT}/lib/x64")
            target_link_libraries(car_poc PRIVATE SDL2 SDL2main)
            # Copy runtime DLL
            if(EXISTS "${SDL2_ROOT}/lib/x64/SDL2.dll")
                set(SDL2_DLL "${SDL2_ROOT}/lib/x64/SDL2.dll")
            elseif(EXISTS "${SDL2_ROOT}/bin/SDL2.dll")
                set(SDL2_DLL "${SDL2_ROOT}/bin/SDL2.dll")
            endif()
            if(SDL2_DLL)
                add_custom_command(TARGET car_poc POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            "${SDL2_DLL}"
                            "$<TARGET_FILE_DIR:car_poc>/SDL2.dll"
                    COMMENT "Copying SDL2.dll to output dir")
            endif()
        else()
            # Non-Windows manual link (rare)
            target_link_directories(car_poc PRIVATE "${SDL2_ROOT}/lib")
            target_link_libraries(car_poc PRIVATE SDL2)
        endif()
    endif()
endif()

# -------- OpenCV C++ source --------
if(USE_OPENCV)
    # Enable C++ for OpenCV integration
    enable_language(CXX)
    set_property(TARGET car_poc PROPERTY CXX_STANDARD 11)
    set_property(TARGET car_poc PROPERTY CXX_STANDARD_REQUIRED ON)
    
    # Add OpenCV C++ source
    target_sources(car_poc PRIVATE src/opencv_speed_detection.cpp)
    target_link_libraries(car_poc PRIVATE ${OpenCV_LIBS})
    target_include_directories(car_poc PRIVATE ${OpenCV_INCLUDE_DIRS})
endif()
 
# -------- Tests --------
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
        target_include_directories(${TEST_NAME} PRIVATE tests/unity inc cfg sim)
        if(NOT MSVC)
            target_link_libraries(${TEST_NAME} m)
        endif()
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endif()
endforeach()
 
# -------- Eval hooks (always built; default no-ops) --------
add_library(eval_hooks STATIC
    eval/hooks/eval_hooks.c
)
target_include_directories(eval_hooks PUBLIC eval/hooks)

# -------- Metrics impl (link only in eval builds/tests) --------
add_library(metrics_autobrake STATIC
    eval/metrics/metrics_autobrake.c
)
target_include_directories(metrics_autobrake PUBLIC eval/hooks)

# -------- Link eval_hooks to main target --------
target_link_libraries(car_poc PRIVATE eval_hooks)

# -------- Unity eval test (headless) --------
add_executable(test_autobrake_eval tests/test_autobrake_eval.c src/app_autobrake.c tests/unity/unity.c src/platform_pc.c)
target_link_libraries(test_autobrake_eval PRIVATE
    metrics_autobrake
)
target_include_directories(test_autobrake_eval PRIVATE tests/unity inc cfg sim eval/hooks)
if(NOT MSVC)
    target_link_libraries(test_autobrake_eval m)
endif()

# -------- CTest entry for eval --------
add_test(NAME eval_autobrake_ped_close COMMAND test_autobrake_eval)

# -------- Reports dir --------
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/eval/reports)

# -------- Static analysis convenience target --------
add_custom_target(static_analysis
    COMMAND ${CMAKE_SOURCE_DIR}/tools/run_static.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running static analysis"
)