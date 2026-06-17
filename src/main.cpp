#ifndef __ANDROID__
    #define SDL_MAIN_HANDLED
#endif
#include <SDL2/SDL.h>
#include "game.hpp"
#include <iostream>

#ifdef __ANDROID__
    #include <GLES3/gl3.h>
#else
    #include <glad/glad.h>
#endif

// Shared game instance
Game game;

int main(int argc, char* argv[]) {
#ifndef __ANDROID__
    SDL_SetMainReady();
#endif
    // Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Set OpenGL attributes depending on platform
#ifdef __ANDROID__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    // Windows/Desktop - use standard core OpenGL 3.3 which matches GLES 3.0 features
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // Adapt screen settings based on Android vs Desktop
    bool isMobile = false;
#ifdef __ANDROID__
    isMobile = true;
    Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_SHOWN;
    int width = 1280; // Default resolution values (will be overwritten by window dimension)
    int height = 720;
#else
    Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN;
    int width = 1280;
    int height = 720;
#endif

    // Create SDL window
    SDL_Window* window = SDL_CreateWindow(
        "Anxiety Fog: Horror Loop",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        windowFlags
    );

    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    // Create OpenGL context
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "OpenGL context could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Retrieve active frame buffer dimensions
    SDL_GL_GetDrawableSize(window, &width, &height);

#ifndef __ANDROID__
    // Initialize GLAD on Desktop
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    // Enable relative mouse capture for modern FPS look-around on Desktop
    SDL_SetRelativeMouseMode(SDL_TRUE);
#endif

    // Setup viewport
    glViewport(0, 0, width, height);

    // Initialize Game engine
    game.init(width, height, isMobile);

    bool running = true;
    SDL_Event event;

    // Delta time counters
    Uint64 lastTime = SDL_GetPerformanceCounter();
    double freq = (double)SDL_GetPerformanceFrequency();

    // Game Main Loop
    while (running) {
        // Event processing
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            else if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED || 
                    event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    width = event.window.data1;
                    height = event.window.data2;
                    glViewport(0, 0, width, height);
                    game.ui.resize(width, height);
                    game.screenWidth = width;
                    game.screenHeight = height;
                }
            }
            else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
#ifdef __ANDROID__
                    // Pressing back/escape on Android can go back to diary or exit safely
                    running = false;
#else
                    // Toggle mouse trap on Windows to let player exit
                    if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                    } else {
                        SDL_SetRelativeMouseMode(SDL_TRUE);
                    }
#endif
                }
            }

            // Feed input handlers
            game.handleEvent(&event);
        }

        // Calculate precise delta time
        Uint64 currentTime = SDL_GetPerformanceCounter();
        float deltaTime = (float)((double)(currentTime - lastTime) / freq);
        lastTime = currentTime;

        // Keep a minimum FPS floor to prevent division-by-zero or extreme physics jumps
        if (deltaTime < 0.0001f) deltaTime = 0.0001f;

        // Update loop
        game.update(deltaTime);

        // Render pass
        game.render();

        // Swap buffers
        SDL_GL_SwapWindow(window);
    }

    // Free resources and exit
    game.cleanup();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
