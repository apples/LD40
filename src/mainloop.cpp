#include "mainloop.hpp"

#include "platform.hpp"

namespace mainloop {

std::list<_detail::state> states;
std::function<void()> swap_buffers;

void main_loop() {
   if (states.empty()) {
       platform::cancel_main_loop();
   } else {
       states.back()();
       swap_buffers();
   }
}

} //namespace mainloop
