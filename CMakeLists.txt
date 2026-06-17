cmake_minimum_required(VERSION 3.14)
project(AnxietyHorror C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Shared Source Files
set(SOURCES
    src/main.cpp
    src/game.cpp
    src/renderer.cpp
    src/procedural.cpp
    src/camera.cpp
    src/ui.cpp
)

# Platform adaptations
if(ANDROID)
    message(STATUS "Building for Android NDK")

    # Add shared library
    add_library(main SHARED ${SOURCES})

    # Include directories
    target_include_directories(main PRIVATE src)

    # Link libraries for Android
    target_link_libraries(main
        GLESv3
        log
        android
        SDL2 # Compiled/linked from Android gradle build
    )

else()
    message(STATUS "Building for Desktop")

    # Include GLAD sources
    list(APPEND SOURCES third_party/glad/src/glad.c)

    # Define executable
    add_executable(AnxietyHorror ${SOURCES})

    # Include directories
    target_include_directories(AnxietyHorror PRIVATE
        src
        third_party/glad/include
    )

    # Try to find SDL2 on the system
    find_package(SDL2 CONFIG QUIET)
    
    if(SDL2_FOUND)
        message(STATUS "System SDL2 Found: linking against SDL2::SDL2")
        target_link_libraries(AnxietyHorror PRIVATE SDL2::SDL2)
    else()
        message(STATUS "System SDL2 not found. Fetching SDL2 from official GitHub release...")
        
        include(FetchContent)
        FetchContent_Declare(
            SDL2
            URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.28.5.zip
        )
        
        # Configure SDL2 build options to compile fast and embed statically
        set(SDL_SHARED OFF CACHE BOOL "Build shared SDL library" FORCE)
        set(SDL_STATIC ON CACHE BOOL "Build static SDL library" FORCE)
        set(SDL_TEST OFF CACHE BOOL "Build SDL test programs" FORCE)
        set(SDL_EVENTS ON CACHE BOOL "Enable events" FORCE)
        set(SDL_VIDEO ON CACHE BOOL "Enable video" FORCE)
        set(SDL_AUDIO OFF CACHE BOOL "Disable audio to speed up build" FORCE) # Disable sound build
        set(SDL_RENDER OFF CACHE BOOL "Disable 2D render API to speed up build" FORCE)

        # Make SDL2 subproject available
        FetchContent_MakeAvailable(SDL2)

        # Map targets
        if(TARGET SDL2-static)
            message(STATUS "Mapping SDL2-static to AnxietyHorror")
            target_link_libraries(AnxietyHorror PRIVATE SDL2-static)
            # SDL2 requires shell/setup libraries on Windows
            if(WIN32)
                target_link_libraries(AnxietyHorror PRIVATE user32 gdi32 winmm imm32 ole32 oleaut32 shell32 version uuid setupapi)
            endif()
        else()
            target_link_libraries(AnxietyHorror PRIVATE SDL2)
        endif()
    endif()

    # Link OpenGL on Desktop
    find_package(OpenGL REQUIRED)
    target_link_libraries(AnxietyHorror PRIVATE ${OPENGL_LIBRARIES})

    if(UNIX AND NOT APPLE)
        target_link_libraries(AnxietyHorror PRIVATE ${CMAKE_DL_LIBS} pthread)
    endif()

endif()
