#include "editload_state.hpp"

#include "platform.hpp"
#include "sdl.hpp"
#include "mainloop.hpp"
#include "utility.hpp"
#include "sprite.hpp"
#include "basic_shader.hpp"
#include "resources.hpp"
#include "window.hpp"
#include "text.hpp"

#include "editor_state.hpp"

#include <sushi/sushi.hpp>

editload_state::editload_state(editload_type t, editor_state* e) {
    type = t;
    editor = e;

    framebuffer = sushi::create_framebuffer(utility::vectorify(sushi::create_uninitialized_texture_2d(320, 240)));
    framebuffer_mesh = sprite_mesh(framebuffer.color_texs[0]);

    std::clog << "Loading basic shader..." << std::endl;
    program = sushi::link_program({
        sushi::compile_shader(sushi::shader_type::VERTEX, {vertexSource}),
        sushi::compile_shader(sushi::shader_type::FRAGMENT, {fragmentSource}),
    });

    std::clog << "Binding shader attributes..." << std::endl;
    sushi::set_program(program);
    sushi::set_uniform("s_texture", 0);
    glBindAttribLocation(program.get(), sushi::attrib_location::POSITION, "position");
    glBindAttribLocation(program.get(), sushi::attrib_location::TEXCOORD, "texcoord");
    glBindAttribLocation(program.get(), sushi::attrib_location::NORMAL, "normal");
}

void editload_state::operator()() {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
        case SDL_QUIT:
            std::clog << "Goodbye!" << std::endl;
            platform::cancel_main_loop();
            return;
        case SDL_TEXTINPUT:
            text += event.text.text;
            break;
        case SDL_KEYDOWN:
            switch (event.key.keysym.scancode) {
            case SDL_SCANCODE_RETURN:
                switch (type) {
                case LOAD:
                    editor->load(text);
                    break;
                case SAVE:
                    editor->save(text);
                    break;
                }
                mainloop::states.pop_back();
                return;
            case SDL_SCANCODE_BACKSPACE:
                text.pop_back();
                return;
            case SDL_SCANCODE_ESCAPE:
                mainloop::states.pop_back();
                return;
            }
        }
    }

    sushi::set_framebuffer(nullptr);
    {
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, 640, 480);

        auto projmat = glm::ortho(0.f, 640.f, 0.f, 480.f, -1.f, 1.f);

        auto font = resources::fonts.get("LiberationSans-Regular");
        draw_string(*font, (type==LOAD?"Load: \"":"Save: \"")+text+"\"", projmat, {0,0}, 16, text_align::LEFT);
    }
}
