#include "platform.hpp"

#include <stdexcept>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#else
#include <glad/glad.h>
#include <SDL2/SDL.h>
#endif

namespace {

#ifdef __EMSCRIPTEN__
#else
bool stop_main_loop;
#endif

} //static

namespace platform {

void do_main_loop(callback_func func, int fps, int simulate_infinite_loop) {
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(func, fps, simulate_infinite_loop);
#else
    if (fps == 0 && simulate_infinite_loop == true) {
        stop_main_loop = false;
        while (!stop_main_loop) {
            func();
        }
    } else {
        throw std::runtime_error("Loop parameters not supported.");
    }
#endif
}

void cancel_main_loop() {
#ifdef __EMSCRIPTEN__
    emscripten_cancel_main_loop();
#else
    stop_main_loop = true;
#endif
}

void load_gl_extensions() {
#ifdef __EMSCRIPTEN__
    return;
#else
    if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
        throw std::runtime_error("Failed to load GLES2 extensions.");
    }
#endif
}

} //namespace platform
