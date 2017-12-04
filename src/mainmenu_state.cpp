#include "mainmenu_state.hpp"

#include "platform.hpp"
#include "sdl.hpp"
#include "mainloop.hpp"
#include "utility.hpp"
#include "sprite.hpp"
#include "basic_shader.hpp"
#include "resources.hpp"
#include "window.hpp"

#include "gameplay_state.hpp"
#include "editor_state.hpp"

#include <sushi/sushi.hpp>

#include <iostream>
#include <thread>

mainmenu_state::mainmenu_state() {
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

void mainmenu_state::operator()() {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
        case SDL_QUIT:
            std::clog << "Goodbye!" << std::endl;
            platform::cancel_main_loop();
            return;
        case SDL_KEYDOWN:
            if(event.key.repeat == 0)
            {
                switch (event.key.keysym.scancode) {
                case SDL_SCANCODE_F12:
                    mainloop::states.push_back(editload_state());
                    return;
                default:
                    mainloop::states.push_back(gameplay_state(0));
                    return;
            }
        }
    }

    sushi::set_framebuffer(framebuffer);
    {
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, 320, 240);

        auto mainmenu_tex = resources::textures.get("mainmenu");
        auto mainmenu_mesh = sprite_mesh(*mainmenu_tex);

        auto projmat = glm::ortho(0.f, 320.f, 0.f, 240.f, -10.f, 10.f);
        auto modelmat = glm::mat4(1.f);
        modelmat = glm::translate(modelmat, glm::vec3{160, 120, 0});
        sushi::set_program(program);
        sushi::set_uniform("MVP", projmat * modelmat);
        sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(modelmat)));
        sushi::set_uniform("cam_forward", glm::vec3{0,0,-1});
        sushi::set_uniform("s_texture", 0);
        sushi::set_texture(0, *mainmenu_tex);
        sushi::draw_mesh(mainmenu_mesh);
    }

    sushi::set_framebuffer(nullptr);
    {
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glViewport(0, 0, 640, 480);

        auto projmat = glm::ortho(-160.f, 160.f, 120.f, -120.f, -1.f, 1.f);
        auto modelmat = glm::mat4(1.f);
        sushi::set_program(program);
        sushi::set_uniform("MVP", projmat * modelmat);
        sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(modelmat)));
        sushi::set_uniform("cam_forward", glm::vec3{0,0,-1});
        sushi::set_uniform("s_texture", 0);
        sushi::set_texture(0, framebuffer.color_texs[0]);
        sushi::draw_mesh(framebuffer_mesh);
    }
}
