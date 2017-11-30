#ifndef LD40_PLATFORM_HPP
#define LD40_PLATFORM_HPP

namespace platform {

using callback_func = void(*)();

void do_main_loop(callback_func func, int fps, int simulate_infinite_loop);

void cancel_main_loop();

void load_gl_extensions();

} //namespace platform

#endif //LD40_PLATFORM_HPP
