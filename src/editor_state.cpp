#include "editor_state.hpp"

#include "platform.hpp"
#include "sdl.hpp"
#include "mainloop.hpp"
#include "utility.hpp"
#include "sprite.hpp"
#include "basic_shader.hpp"
#include "resources.hpp"
#include "window.hpp"
#include "text.hpp"

#include "gameplay_state.hpp"

#include <sushi/sushi.hpp>

#include <iostream>
#include <fstream>
#include <thread>

editor_state::editor_state() {
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

void editor_state::save() {
    nlohmann::json json;
    json["num_rows"] = map.get_num_rows();
    json["num_cols"] = map.get_num_cols();

    for (int r = 0; r < map.get_num_rows(); ++r) {
        for (int c = 0; c < map.get_num_cols(); ++c) {
            json["tiles"].push_back({map.get(r,c).flags, map.get(r,c).background, map.get(r,c).foreground});
        }
    }

    for (auto& elf : elves) {
        json["elves"].push_back({std::get<0>(elf), std::get<1>(elf)});
    }

    for (auto& beer : beers) {
        json["beers"].push_back({std::get<0>(beer), std::get<1>(beer)});
    }

    json["spawn"]["r"] = int(spawn.y);
    json["spawn"]["c"] = int(spawn.x);

    json["time_limit"] = time_limit;

    std::ofstream file ("data/stages/test_editor.json");
    file << json;
}

void editor_state::operator()() {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
        case SDL_QUIT:
            std::clog << "Goodbye!" << std::endl;
            platform::cancel_main_loop();
            return;
        case SDL_KEYDOWN:
            switch (event.key.keysym.scancode) {
            case SDL_SCANCODE_LEFT:
                position.x -= 1;
                if (position.x < 0) position.x = 0;
                break;
            case SDL_SCANCODE_RIGHT:
                position.x += 1;
                if (position.x >= map.get_num_cols()) position.x = map.get_num_cols()-1;
                break;
            case SDL_SCANCODE_DOWN:
                position.y -= 1;
                if (position.y < 0) position.y = 0;
                break;
            case SDL_SCANCODE_UP:
                position.y += 1;
                if (position.y >= map.get_num_rows()) position.y = map.get_num_rows()-1;
                break;
            case SDL_SCANCODE_KP_4:
                map.set_num_cols(map.get_num_cols()-1);
                break;
            case SDL_SCANCODE_KP_6:
                map.set_num_cols(map.get_num_cols()+1);
                break;
            case SDL_SCANCODE_KP_2:
                map.set_num_rows(map.get_num_rows()-1);
                break;
            case SDL_SCANCODE_KP_8:
                map.set_num_rows(map.get_num_rows()+1);
                break;
            case SDL_SCANCODE_Q:
                cursor_tile -= 1;
                if (cursor_tile < 0) cursor_tile = 0;
                break;
            case SDL_SCANCODE_W:
                cursor_tile += 1;
                if (cursor_tile >= 256) cursor_tile = 255;
                break;
            case SDL_SCANCODE_R:
                map.get(position.y, position.x).background = cursor_tile;
                map.get(position.y, position.x).flags |= tilemap::BACKGROUND;
                break;
            case SDL_SCANCODE_F:
                map.get(position.y, position.x).foreground = cursor_tile;
                map.get(position.y, position.x).flags |= tilemap::FOREGROUND;
                break;
            case SDL_SCANCODE_T:
                map.get(position.y, position.x).flags &= ~tilemap::BACKGROUND;
                break;
            case SDL_SCANCODE_G:
                map.get(position.y, position.x).flags &= ~tilemap::FOREGROUND;
                break;
            case SDL_SCANCODE_C:
                map.get(position.y, position.x).flags |= tilemap::WALL;
                break;
            case SDL_SCANCODE_V:
                map.get(position.y, position.x).flags &= ~tilemap::WALL;
                break;
            case SDL_SCANCODE_F1:
                save();
                break;
            case SDL_SCANCODE_ESCAPE:
                mainloop::states.pop_back();
                return;
            case SDL_SCANCODE_O: {
                auto iter = elves.find({position.y, position.x});
                if (iter == elves.end()) {
                    elves.insert({position.y, position.x});
                } else {
                    elves.erase(iter);
                }
            } break;
            case SDL_SCANCODE_P: {
                auto iter = beers.find({position.y, position.x});
                if (iter == beers.end()) {
                    beers.insert({position.y, position.x});
                } else {
                    beers.erase(iter);
                }
            } break;
            case SDL_SCANCODE_RIGHTBRACKET:
                time_limit += 5;
                break;
            case SDL_SCANCODE_LEFTBRACKET:
                time_limit -= 5;
                break;
            case SDL_SCANCODE_S:
                spawn = position;
                break;
            }
            break;
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

        auto projmat = glm::ortho(0.f, 320.f, 0.f, 240.f, -10.f, 10.f);

        auto cammat = glm::mat4(1.f);
        cammat = glm::translate(cammat, glm::vec3{-(position.x*16+8) + 160, -(position.y*16+8)+ 120, 0});

        auto tilesheet = resources::spritesheets.get("tiles", 16, 16);

        auto editor_sprites = resources::spritesheets.get("editor", 16, 16);

        for (auto r = 0; r < map.get_num_rows(); ++r) {
            for (auto c = 0; c < map.get_num_cols(); ++c) {
                auto& tile = map.get(r, c);
                if (tile.flags & tilemap::BACKGROUND || tile.flags & tilemap::FOREGROUND) {
                    auto modelmat = glm::mat4(1.f);
                    modelmat = glm::translate(modelmat, glm::vec3{8, 8, 0});
                    modelmat = glm::translate(modelmat, glm::vec3{c*16, r*16, 0});
                    sushi::set_program(program);
                    sushi::set_uniform("cam_forward", glm::vec3{0,0,-1});
                    sushi::set_uniform("s_texture", 0);
                    sushi::set_texture(0, tilesheet->get_texture());
                    if (tile.flags & tilemap::BACKGROUND) {
                        sushi::set_uniform("MVP", projmat * cammat * modelmat);
                        sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(modelmat)));
                        sushi::draw_mesh(tilesheet->get_mesh(tile.background/16, tile.background%16));
                    }
                    if (tile.flags & tilemap::FOREGROUND) {
                        auto raised_modelmat = glm::translate(modelmat, glm::vec3{0, 0, 1});
                        sushi::set_uniform("MVP", projmat * cammat * raised_modelmat);
                        sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(cammat * raised_modelmat)));
                        sushi::draw_mesh(tilesheet->get_mesh(tile.foreground/16, tile.foreground%16));
                    }
                    if (tile.flags & tilemap::WALL) {
                        auto raised_modelmat = glm::translate(modelmat, glm::vec3{0, 0, 2});
                        sushi::set_uniform("MVP", projmat * cammat * raised_modelmat);
                        sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(cammat * raised_modelmat)));
                        sushi::set_texture(0, editor_sprites->get_texture());
                        sushi::draw_mesh(editor_sprites->get_mesh(0,0));
                    }
                }
            }
        }

        auto elf_sheet = resources::spritesheets.get("elf", 16, 16);

        for (auto& elf : elves) {
            auto r = std::get<0>(elf);
            auto c = std::get<1>(elf);
            auto modelmat = glm::mat4(1.f);
            modelmat = glm::translate(modelmat, glm::vec3{8, 8, 0});
            modelmat = glm::translate(modelmat, glm::vec3{c*16, r*16, 0});
            sushi::set_program(program);
            sushi::set_uniform("cam_forward", glm::vec3{0,0,-1});
            sushi::set_uniform("s_texture", 0);
            sushi::set_uniform("MVP", projmat * cammat * modelmat);
            sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(modelmat)));
            sushi::set_texture(0, elf_sheet->get_texture());
            sushi::draw_mesh(elf_sheet->get_mesh(0,0));
        }

        auto beer_sheet = resources::spritesheets.get("beer", 16, 16);

        for (auto& beer : beers) {
            auto r = std::get<0>(beer);
            auto c = std::get<1>(beer);
            auto modelmat = glm::mat4(1.f);
            modelmat = glm::translate(modelmat, glm::vec3{8, 8, 0});
            modelmat = glm::translate(modelmat, glm::vec3{c*16, r*16, 0});
            sushi::set_program(program);
            sushi::set_uniform("cam_forward", glm::vec3{0,0,-1});
            sushi::set_uniform("s_texture", 0);
            sushi::set_uniform("MVP", projmat * cammat * modelmat);
            sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(modelmat)));
            sushi::set_texture(0, beer_sheet->get_texture());
            sushi::draw_mesh(beer_sheet->get_mesh(0,0));
        }

        {
            auto modelmat = glm::mat4(1.f);
            modelmat = glm::translate(modelmat, glm::vec3{spawn.x*16+8, spawn.y*16+8, 5});
            sushi::set_program(program);
            sushi::set_uniform("cam_forward", glm::vec3{0,0,-1});
            sushi::set_uniform("s_texture", 0);
            sushi::set_uniform("MVP", projmat * cammat * modelmat);
            sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(cammat * modelmat)));
            sushi::set_texture(0, editor_sprites->get_texture());
            sushi::draw_mesh(editor_sprites->get_mesh(1,0));
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

        auto projmat = glm::ortho(-160.f, 160.f, -120.f, 120.f, -1.f, 1.f);
        auto modelmat = glm::mat4(1.f);
        sushi::set_program(program);
        sushi::set_uniform("MVP", projmat * modelmat);
        sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(modelmat)));
        sushi::set_uniform("cam_forward", glm::vec3{0,0,-1});
        sushi::set_uniform("s_texture", 0);
        sushi::set_texture(0, framebuffer.color_texs[0]);
        sushi::draw_mesh(framebuffer_mesh);

        projmat = glm::ortho(0.f, 640.f, 0.f, 480.f, -1.f, 1.f);

        auto tilesheet = resources::spritesheets.get("tiles", 16, 16);

        auto editor_sprites = resources::spritesheets.get("editor", 16, 16);

        auto font = resources::fonts.get("LiberationSans-Regular");
        draw_string(*font, "Brush:", projmat, {0,0}, 16, text_align::LEFT);

        {
            auto modelmat = glm::mat4(1.f);
            modelmat = glm::translate(modelmat, glm::vec3{64, 8, 0});
            sushi::set_program(program);
            sushi::set_uniform("cam_forward", glm::vec3{0,0,-1});
            sushi::set_uniform("s_texture", 0);
            sushi::set_uniform("MVP", projmat * modelmat);
            sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(modelmat)));
            sushi::set_texture(0, tilesheet->get_texture());
            sushi::draw_mesh(tilesheet->get_mesh(cursor_tile/16,cursor_tile%16));
        }

        {
            auto modelmat = glm::mat4(1.f);
            modelmat = glm::translate(modelmat, glm::vec3{320, 240, 0});
            sushi::set_program(program);
            sushi::set_uniform("cam_forward", glm::vec3{0,0,-1});
            sushi::set_uniform("s_texture", 0);
            sushi::set_uniform("MVP", projmat * modelmat);
            sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(modelmat)));
            sushi::set_texture(0, editor_sprites->get_texture());
            sushi::draw_mesh(editor_sprites->get_mesh(0,1));
        }

        int tr = 1;
        draw_string(*font, "Q: Brush+", projmat, {0,480-16*tr++}, 16, text_align::LEFT);
        draw_string(*font, "W: Brush-", projmat, {0,480-16*tr++}, 16, text_align::LEFT);
        draw_string(*font, "R: Set BG", projmat, {0,480-16*tr++}, 16, text_align::LEFT);
        draw_string(*font, "F: Set FG", projmat, {0,480-16*tr++}, 16, text_align::LEFT);
        draw_string(*font, "T: Del BG", projmat, {0,480-16*tr++}, 16, text_align::LEFT);
        draw_string(*font, "G: Del FG", projmat, {0,480-16*tr++}, 16, text_align::LEFT);
        draw_string(*font, "C: Wall+", projmat, {0,480-16*tr++}, 16, text_align::LEFT);
        draw_string(*font, "V: Wall-", projmat, {0,480-16*tr++}, 16, text_align::LEFT);
        draw_string(*font, "O: Elf", projmat, {0,480-16*tr++}, 16, text_align::LEFT);
        draw_string(*font, "P: Beer", projmat, {0,480-16*tr++}, 16, text_align::LEFT);
        draw_string(*font, "S: Tipsy", projmat, {0,480-16*tr++}, 16, text_align::LEFT);
        draw_string(*font, "]: Time+", projmat, {0,480-16*tr++}, 16, text_align::LEFT);
        draw_string(*font, "[: Time-", projmat, {0,480-16*tr++}, 16, text_align::LEFT);
        draw_string(*font, "F1: Save", projmat, {0,480-16*tr++}, 16, text_align::LEFT);

        auto size_str = "R,C = "+std::to_string(map.get_num_rows())+","+std::to_string(map.get_num_cols());
        draw_string(*font, size_str, projmat, {640, 0}, 16, text_align::RIGHT);

        auto time_str = "Time = " + std::to_string(time_limit);
        draw_string(*font, time_str, projmat, {640, 480-16}, 16, text_align::RIGHT);
    }
}
