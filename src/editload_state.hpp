#ifndef LD40_EDITLOAD_STATE_HPP
#define LD40_EDITLOAD_STATE_HPP

#include "tilemap.hpp"

#include <sushi/sushi.hpp>

#include <string>

class editor_state;

class editload_state {
public:
    enum editload_type {
        LOAD,
        SAVE
    };

    editload_state(editload_type t, editor_state* e);
    void operator()();

private:
    sushi::framebuffer framebuffer;
    sushi::static_mesh framebuffer_mesh;
    sushi::unique_program program;

    std::string text;

    editload_type type;
    editor_state* editor;
};

#endif //LD40_EDITLOAD_STATE_HPP
