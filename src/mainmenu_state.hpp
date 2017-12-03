#ifndef LD40_MAINMENU_STATE_HPP
#define LD40_MAINMENU_STATE_HPP

#include <sushi/sushi.hpp>

class mainmenu_state {
public:
    mainmenu_state();
    void operator()();

private:
    sushi::framebuffer framebuffer;
    sushi::static_mesh framebuffer_mesh;
    sushi::unique_program program;
};

#endif //LD40_MAINMENU_STATE_HPP
