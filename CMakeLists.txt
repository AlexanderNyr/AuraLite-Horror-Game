cmake_minimum_required(VERSION 3.16)
project(AnxietyHorror LANGUAGES C CXX)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ANXIETYHORROR_WARNINGS "Enable useful compiler warnings" ON)

set(GAME_SOURCES
    src/main.cpp
    src/game.cpp
    src/renderer.cpp
    src/procedural.cpp
    src/camera.cpp
    src/ui.cpp
    src/localization.cpp
    src/audio.cpp
    src/save_system.cpp
    src/settings_system.cpp
)

function(configure_common_target target_name)
    target_include_directories(${target_name} PRIVATE src)
    target_compile_features(${target_name} PRIVATE cxx_std_17)

    if(ANXIETYHORROR_WARNINGS)
        if(MSVC)
            target_compile_options(${target_name} PRIVATE /W4 /permissive-)
            target_compile_definitions(${target_name} PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
        else()
            target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
        endif()
    endif()
endfunction()

if(ANDROID)
    message(STATUS "Building AnxietyHorror for Android / OpenGL ES 3.2")

    # SDL2: prefer a local checkout, otherwise fetch the official release.
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/SDL2/CMakeLists.txt")
        message(STATUS "Using local SDL2 from third_party/SDL2")
        set(SDL2_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/SDL2")
        set(SDL_SHARED ON CACHE BOOL "Build shared SDL2 library" FORCE)
        set(SDL_STATIC OFF CACHE BOOL "Build static SDL2 library" FORCE)
        set(SDL_TEST OFF CACHE BOOL "Build SDL tests" FORCE)
        add_subdirectory("${SDL2_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/SDL2")
    else()
        message(STATUS "SDL2 not found locally. Fetching SDL2 2.28.5...")
        include(FetchContent)
        FetchContent_Declare(
            SDL2
            URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.28.5.zip
            URL_HASH SHA256=9f7f2f3024df8fb6f54ca5906ef55fe8724443ebc07f4192123c4d2b43e95caf
        )
        set(SDL_SHARED ON CACHE BOOL "Build shared SDL2 library" FORCE)
        set(SDL_STATIC OFF CACHE BOOL "Build static SDL2 library" FORCE)
        set(SDL_TEST OFF CACHE BOOL "Build SDL tests" FORCE)
        FetchContent_MakeAvailable(SDL2)
        set(SDL2_SOURCE_DIR "${sdl2_SOURCE_DIR}")
    endif()

    add_library(main SHARED ${GAME_SOURCES})
    configure_common_target(main)

    target_include_directories(main PRIVATE "${SDL2_SOURCE_DIR}/include")

    target_link_libraries(main PRIVATE
        GLESv3
        log
        android
        SDL2
    )
else()
    message(STATUS "Building AnxietyHorror for desktop / OpenGL 4.5")

    add_executable(AnxietyHorror
        ${GAME_SOURCES}
        third_party/glad/src/glad.c
    )
    configure_common_target(AnxietyHorror)
    target_include_directories(AnxietyHorror PRIVATE third_party/glad/include)

    # SDL2: prefer a system/package-manager install; fall back to FetchContent.
    find_package(SDL2 CONFIG QUIET)
    if(TARGET SDL2::SDL2)
        message(STATUS "SDL2 found via config package")
        target_link_libraries(AnxietyHorror PRIVATE SDL2::SDL2)
    else()
        find_package(SDL2 QUIET)
        if(SDL2_FOUND)
            message(STATUS "SDL2 found via CMake module")
            target_include_directories(AnxietyHorror PRIVATE ${SDL2_INCLUDE_DIRS})
            target_link_libraries(AnxietyHorror PRIVATE ${SDL2_LIBRARIES})
        else()
            message(STATUS "SDL2 not found. Fetching SDL2 2.28.5...")
            include(FetchContent)
            FetchContent_Declare(
                SDL2
                URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.28.5.zip
                URL_HASH SHA256=9f7f2f3024df8fb6f54ca5906ef55fe8724443ebc07f4192123c4d2b43e95caf
            )

            set(SDL_SHARED OFF CACHE BOOL "Build shared SDL library" FORCE)
            set(SDL_STATIC ON CACHE BOOL "Build static SDL library" FORCE)
            set(SDL_TEST OFF CACHE BOOL "Build SDL test programs" FORCE)
            set(SDL_EVENTS ON CACHE BOOL "Enable events" FORCE)
            set(SDL_VIDEO ON CACHE BOOL "Enable video" FORCE)
            set(SDL_AUDIO ON CACHE BOOL "Enable audio for procedural horror synth" FORCE)
            set(SDL_RENDER OFF CACHE BOOL "Disable SDL 2D renderer API" FORCE)

            FetchContent_MakeAvailable(SDL2)
            if(TARGET SDL2-static)
                target_link_libraries(AnxietyHorror PRIVATE SDL2-static)
            else()
                target_link_libraries(AnxietyHorror PRIVATE SDL2)
            endif()
        endif()
    endif()

    find_package(OpenGL REQUIRED)
    target_link_libraries(AnxietyHorror PRIVATE OpenGL::GL)

    if(UNIX AND NOT APPLE)
        target_link_libraries(AnxietyHorror PRIVATE ${CMAKE_DL_LIBS} pthread)
    endif()

    if(WIN32)
        target_link_libraries(AnxietyHorror PRIVATE user32 gdi32 winmm imm32 ole32 oleaut32 shell32 version uuid setupapi)
    endif()

    add_custom_command(TARGET AnxietyHorror POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/lang
                $<TARGET_FILE_DIR:AnxietyHorror>/lang
        COMMENT "Copying .lang localization files next to the executable")
endif()
