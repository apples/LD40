#ifndef LD40_EDITLOAD_STATE_HPP
#define LD40_EDITLOAD_STATE_HPP

#include "tilemap.hpp"

#include <sushi/sushi.hpp>

#include <string>

class editload_state {
public:
    editload_state();
    void operator()();

private:
    sushi::framebuffer framebuffer;
    sushi::static_mesh framebuffer_mesh;
    sushi::unique_program program;

    std::string text;
};

#endif //LD40_EDITLOAD_STATE_HPP
