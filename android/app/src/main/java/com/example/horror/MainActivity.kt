package com.example.horror

import org.libsdl.app.SDLActivity

class MainActivity : SDLActivity() {
    /**
     * Specify the list of native libraries to load.
     * SDL2 handles the JNI interface and sets up the game's entry point main() automatically.
     */
    override fun getLibraries(): Array<String> {
        return arrayOf(
            "SDL2",
            "main"
        )
    }
}
