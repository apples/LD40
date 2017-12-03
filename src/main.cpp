
#include "sdl.hpp"
#include "emberjs/config.hpp"

#include "utility.hpp"
#include "components.hpp"
#include "text.hpp"
#include "sprite.hpp"
#include "platform.hpp"
#include "resources.hpp"
#include "spritesheet.hpp"
#include "tilemap.hpp"
#include "animated_sprite.hpp"

#include <sushi/sushi.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <sol.hpp>

#include <soloud.h>
#include <soloud_wav.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <cstddef>
#include <cmath>
#include <functional>
#include <thread>
#include <chrono>
#include <random>

#ifdef __EMSCRIPTEN__
const auto vertexSource = R"(
    attribute vec3 position;
    attribute vec2 texcoord;
    attribute vec3 normal;
    varying vec2 v_texcoord;
    varying vec3 v_normal;
    uniform mat4 MVP;
    uniform mat4 normal_mat;
    void main()
    {
        v_texcoord = texcoord;
        v_normal = vec3(normal_mat * vec4(normal, 0.0));
        gl_Position = MVP * vec4(position, 1.0);
    }
)";

const auto fragmentSource = R"(
    precision mediump float;
    varying vec2 v_texcoord;
    varying vec3 v_normal;
    uniform sampler2D s_texture;
    uniform vec3 cam_forward;
    void main()
    {
        if (dot(v_normal, cam_forward) > 0.0) discard;
        gl_FragColor = texture2D(s_texture, v_texcoord);
    }
)";
#else
const auto vertexSource = R"(#version 400
    layout(location = 0) in vec3 position;
    layout(location = 1) in vec2 texcoord;
    layout(location = 2) in vec3 normal;
    out vec2 v_texcoord;
    out vec3 v_normal;
    uniform mat4 MVP;
    uniform mat4 normal_mat;
    void main()
    {
        v_texcoord = texcoord;
        v_normal = vec3(normal_mat * vec4(normal, 0.0));
        gl_Position = MVP * vec4(position, 1.0);
    }
)";

const auto fragmentSource = R"(#version 400
    in vec2 v_texcoord;
    in vec3 v_normal;
    uniform sampler2D s_texture;
    uniform vec3 cam_forward;
    layout(location = 0) out vec4 color;
    void main()
    {
        if (dot(v_normal, cam_forward) > 0.0) discard;
        color = texture(s_texture, v_texcoord);
    }
)";
#endif

std::function<void()> loop;
void main_loop() {
    loop();
}

int main(int argc, char* argv[]) try {
    std::clog << "Init..." << std::endl;

    std::clog << "Performing Lua test..." << std::endl;
    sol::state lua;
    lua.set_function("test", []{ std::clog << "Lua Test" << std::endl; });
    lua.script("test()");

    std::clog << "Initializing SDL..." << std::endl;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        throw std::runtime_error(SDL_GetError());
    }

    std::clog << "Initializing SoLoud..." << std::endl;
    SoLoud::Soloud soloud;
    soloud.init();

    std::clog << "Loading config..." << std::endl;
    auto config = emberjs::get_config();

    const auto display_width = int(config["display"]["width"]);
    const auto display_height = int(config["display"]["height"]);
    const auto aspect_ratio = float(display_width) / float(display_height);

    std::clog << "Creating window..." << std::endl;
    auto g_window = SDL_CreateWindow("LD40", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, display_width, display_height, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

    if (!g_window) {
        throw std::runtime_error("Failed to create window.");
    }

    std::clog << "Creating OpenGL context..." << std::endl;
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
#ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif
    auto glcontext = SDL_GL_CreateContext(g_window);

    if (!glcontext) {
        throw std::runtime_error("Failed to create OpenGL context.");
    }

    std::clog << "Loading OpenGL extenstions..." << std::endl;
    platform::load_gl_extensions();

    std::clog << "OpenGL info:" << std::endl;
    std::clog << "    Vendor: " << (char*)glGetString(GL_VENDOR) << std::endl;
    std::clog << "    Renderer: " << (char*)glGetString(GL_RENDERER) << std::endl;
    std::clog << "    GL Version: " << (char*)glGetString(GL_VERSION) << std::endl;
    std::clog << "    GLSL version: " << (char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    std::clog << "Loading basic shader..." << std::endl;
    auto program = sushi::link_program({
        sushi::compile_shader(sushi::shader_type::VERTEX, {vertexSource}),
        sushi::compile_shader(sushi::shader_type::FRAGMENT, {fragmentSource}),
    });

    std::clog << "Binding shader attributes..." << std::endl;
    sushi::set_program(program);
    sushi::set_uniform("s_texture", 0);
    glBindAttribLocation(program.get(), sushi::attrib_location::POSITION, "position");
    glBindAttribLocation(program.get(), sushi::attrib_location::TEXCOORD, "texcoord");
    glBindAttribLocation(program.get(), sushi::attrib_location::NORMAL, "normal");

    std::clog << "Loading fonts..." << std::endl;
    auto font = msdf_font("LiberationSans-Regular");

    std::clog << "Creating data structures..." << std::endl;

    auto rng = std::mt19937{std::random_device{}()};
    auto roll = [&](int low, int high) {
        auto dist = std::uniform_int_distribution<int>(low, high);
        return dist(rng);
    };
    auto rollf = [&](float low, float high) {
        auto dist = std::uniform_real_distribution<float>(low, high);
        return dist(rng);
    };

    database entities;
    auto deadentities = std::vector<database::ent_id>();

    std::clog << "Creating player..." << std::endl;
    auto player = entities.create_entity();
    entities.create_component(player, component::position{20,20});
    entities.create_component(player, component::velocity{0, 0});
    entities.create_component(player, component::aabb{-8,8,-8,8});
    entities.create_component(player, component::drunken{});
    // Default fist direction
    entities.create_component(player, component::fistdir::RIGHT);
    entities.create_component(player, component::health{3});
    entities.create_component(player, component::animated_sprite{"tipsy", "idle", 0, 0});

    // player handles most collisions
    auto player_collider = [&](database::ent_id self, database::ent_id other) {
        std::clog << "Player collided with something." << std::endl;
        if (entities.has_component<component::elf_tag>(other)) {
            std::clog << "    It was an elf!" << std::endl;
            auto& ppos = entities.get_component<component::position>(self);
            auto& epos = entities.get_component<component::position>(other);

            auto centerx = (ppos.x + epos.x) / 2;
            auto centery = (ppos.y + epos.y) / 2;

            auto dirx = ppos.x - epos.x;
            auto diry = ppos.y - epos.y;
            auto dirm = std::sqrt(dirx*dirx + diry*diry);
            dirx /= dirm;
            diry /= dirm;

            entities.create_component(self, component::timed_force{dirx*8, diry*8, 5});
            entities.create_component(other, component::timed_force{-dirx*8, -diry*8, 5});
        } else if (entities.has_component<component::booze>(other)) {
            auto& booze = entities.get_component<component::booze>(other);
            auto& drunk = entities.get_component<component::drunken>(self);

            drunk.bac += booze.value;
            deadentities.push_back(other);
        }
    };
    entities.create_component(player, component::collider{player_collider});

    // fist needs to punch elves
    auto fist_collider = [&](database::ent_id self, database::ent_id other) {
        if (entities.has_component<component::elf_tag>(other)) {
            auto dir = entities.get_component<component::fistdir>(self);
            auto force = component::timed_force{};
            switch (dir) {
            case component::fistdir::LEFT:
                force.x = -10;
                break;
            case component::fistdir::RIGHT:
                force.x = 10;
                break;
            case component::fistdir::DOWN:
                force.y = -10;
                break;
            case component::fistdir::UP:
                force.y = 10;
                break;
            }
            force.duration = 10;
            entities.create_component(other, force);
        }
    };

    auto enemythink = [&](database::ent_id self){
        auto& self_pos = entities.get_component<component::position>(self);
        auto& player_pos = entities.get_component<component::position>(player);
        // Vectors that point enemy towards player
        float dirx = player_pos.x - self_pos.x;
        float diry = player_pos.y - self_pos.y;
        // Normalize the vectors
        float hyp = sqrt((dirx * dirx) +(diry * diry));
        dirx /= hyp;
        diry /= hyp;
        // Translate enemy to player position via the vectors.
        self_pos.x += dirx;
        self_pos.y += diry;
    };

    std::clog << "Creating framebuffers..." << std::endl;

    auto framebuffer = sushi::create_framebuffer(utility::vectorify(sushi::create_uninitialized_texture_2d(320, 240)));
    auto framebuffer_mesh = sprite_mesh(framebuffer.color_texs[0]);

    std::clog << "Loading stage..." << std::endl;

    auto tiles_texture = resources::textures.get("tiles");
    auto tilesheet = spritesheet(tiles_texture, 16, 16);

    std::ifstream test_stage_file ("data/stages/test.json");
    nlohmann::json test_stage_json;
    test_stage_file >> test_stage_json;
    test_stage_file.close();
    auto test_stage = tilemap::tilemap(test_stage_json);

    for(auto& elf : test_stage_json["elves"])
    {
        auto enemy = entities.create_entity();
        entities.create_component(enemy, component::position{float(elf[1])*16+8, float(elf[0])*16+8});
        entities.create_component(enemy, component::animated_sprite{"elf", "idle", 0, 0});
        entities.create_component(enemy, component::brain{enemythink});
        entities.create_component(enemy, component::aabb{-8, 8, -8, 8});
        entities.create_component(enemy, component::elf_tag{});
    }

    for (auto& beerjson : test_stage_json["beers"])
    {
        auto ent = entities.create_entity();
        entities.create_component(ent, component::position{float(beerjson[1])*16+8, float(beerjson[0])*16+8});
        entities.create_component(ent, component::animated_sprite{"beer", "idle", 0, 0});
        entities.create_component(ent, component::aabb{-8, 8, -8, 8});
        entities.create_component(ent, component::booze{1});
    }

    auto last_update = std::chrono::steady_clock::now();
    auto frame_delay = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::seconds(1)) / 60;

    auto game_loop = [&]{
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
                    switch(event.key.keysym.scancode) {
                    //Fist animation spawn on spacebar
                    case SDL_SCANCODE_SPACE: {
                        auto& player_pos = entities.get_component<component::position>(player);
                        auto& player_vel = entities.get_component<component::velocity>(player);

                         auto fistfps = [&](database::ent_id self){
                             auto& timer = entities.get_component<component::fisttimer>(self);
                             deadentities.push_back(self);
                         };

                         auto fist = entities.create_entity();
                         entities.create_component(fist, component::fisttimer{fistfps, 20});
                         entities.create_component(fist, component::aabb{-8, 8, -8, 8});
                         entities.create_component(fist, component::collider{fist_collider});
                         entities.create_component(fist, player_vel);

                         auto dir = entities.get_component<component::fistdir>(player);

                         entities.create_component(fist, dir);

                         switch (dir) {
                            case component::fistdir::RIGHT:
                                entities.create_component(fist, component::position{player_pos.x+16, player_pos.y});
                                entities.create_component(fist, component::animated_sprite{"rightfist", "idle", 0, 0});
                                break;

                            case component::fistdir::LEFT:
                                entities.create_component(fist, component::position{player_pos.x-16, player_pos.y});
                                entities.create_component(fist, component::animated_sprite{"leftfist", "idle", 0, 0});
                                break;

                            case component::fistdir::UP:
                                entities.create_component(fist, component::position{player_pos.x, player_pos.y+16});
                                entities.create_component(fist, component::animated_sprite{"upfist", "idle", 0, 0});
                                break;

                            case component::fistdir::DOWN:
                                entities.create_component(fist, component::position{player_pos.x, player_pos.y-16});
                                entities.create_component(fist, component::animated_sprite{"downfist", "idle", 0, 0});
                                break;
                         }
                    } break;
                    }
                    }
                    break;
            }
        }

        const Uint8 *keys = SDL_GetKeyboardState(NULL);       

        //Player moment based on keys
        auto& player_vel = entities.get_component<component::velocity>(player);
        player_vel = {0,0};

        if (keys[SDL_SCANCODE_LEFT]) {
            player_vel.x -= 1;
            entities.create_component(player, component::fistdir::LEFT);
        }

        if (keys[SDL_SCANCODE_RIGHT]) {
            player_vel.x += 1;
            entities.create_component(player, component::fistdir::RIGHT);
        }

        if (keys[SDL_SCANCODE_DOWN]) {
            player_vel.y -= 1;
            entities.create_component(player, component::fistdir::DOWN);
        }

        if (keys[SDL_SCANCODE_UP]) {
            player_vel.y += 1;
            entities.create_component(player, component::fistdir::UP);
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

            // View matrix for camera
            // Camera should invert the player matrix to follow the player around
            // x+80 y+60 centers the camera
            auto& player_pos = entities.get_component<component::position>(player);
            auto cammat = glm::mat4(1.f);
            cammat = glm::translate(cammat, glm::vec3{-player_pos.x + 160, -player_pos.y + 120, 0});

            for (auto r = 0; r < test_stage.get_num_rows(); ++r) {
                for (auto c = 0; c < test_stage.get_num_cols(); ++c) {
                    auto& tile = test_stage.get(r, c);
                    if (tile.flags & tilemap::BACKGROUND || tile.flags & tilemap::FOREGROUND) {
                        auto modelmat = glm::mat4(1.f);
                        modelmat = glm::translate(modelmat, glm::vec3{8, 8, 0});
                        modelmat = glm::translate(modelmat, glm::vec3{c*16, r*16, 0});
                        sushi::set_program(program);
                        sushi::set_uniform("cam_forward", glm::vec3{0,0,-1});
                        sushi::set_uniform("s_texture", 0);
                        sushi::set_texture(0, tilesheet.get_texture());
                        if (tile.flags & tilemap::BACKGROUND) {
                            sushi::set_uniform("MVP", projmat * cammat * modelmat);
                            sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(modelmat)));
                            sushi::draw_mesh(tilesheet.get_mesh(tile.background/16, tile.background%16));
                        }
                        if (tile.flags & tilemap::FOREGROUND) {
                            auto raised_modelmat = glm::translate(modelmat, glm::vec3{0, 0, 1});
                            sushi::set_uniform("MVP", projmat * cammat * raised_modelmat);
                            sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(cammat * raised_modelmat)));
                            sushi::draw_mesh(tilesheet.get_mesh(tile.foreground/16, tile.foreground%16));
                        }
                    }
                }
            }

            entities.visit([&](component::brain& brain, database::ent_id self) {
                brain.think(self);
            });

            entities.visit([&](component::position& pos, component::timed_force& force, database::ent_id self) {
                if (--force.duration <= 0) {
                    entities.destroy_component<component::timed_force>(self);
                } else {
                    pos.x += force.x;
                    pos.y += force.y;
                }
            });

            entities.visit([&](component::position& pos, component::drunken& drunken) {
                constexpr auto SWAY_FACTOR = 1.f / 5.f;
                constexpr auto DRUNK_FACTOR = 1.f / 5.f;

                auto roll_x = (rollf(0,5) + rollf(0,5) - 5) / 5;
                auto roll_y = (rollf(0,5) + rollf(0,5) - 5) / 5;
                static_assert(std::is_same<decltype(roll_x), float>::value, "no float");

                if (roll_x < 0) {
                    drunken.wander_x += (drunken.wander_x + 1) * roll_x * SWAY_FACTOR;
                } else {
                    drunken.wander_x += (1 - drunken.wander_x) * roll_x * SWAY_FACTOR;
                }
                if (roll_y < 0) {
                    drunken.wander_y += (drunken.wander_y + 1) * roll_y * SWAY_FACTOR;
                } else {
                    drunken.wander_y += (1 - drunken.wander_y) * roll_y * SWAY_FACTOR;
                }

                pos.x += drunken.wander_x * drunken.bac * DRUNK_FACTOR;
                pos.y += drunken.wander_y * drunken.bac * DRUNK_FACTOR;
            });

            entities.visit([&](component::position& pos, const component::velocity& vel) {
                pos.x += vel.x;
                pos.y += vel.y;
            });

            entities.visit([&](component::fisttimer& timer, database::ent_id self) {
                --timer.duration;
                if(timer.duration == 0)
                {
                    timer.timer(self);
                }
            });

            entities.visit([&](database::ent_id eidA, component::collider& collider, component::position& posA, const component::aabb& aabbA) {
                auto a_left = posA.x + aabbA.left;
                auto a_right = posA.x + aabbA.right;
                auto a_bottom = posA.y + aabbA.bottom;
                auto a_top = posA.y + aabbA.top;
                entities.visit([&](database::ent_id eidB, component::position& posB, const component::aabb& aabbB) {
                    if (eidA == eidB) return;
                    auto b_left = posB.x + aabbB.left;
                    auto b_right = posB.x + aabbB.right;
                    auto b_bottom = posB.y + aabbB.bottom;
                    auto b_top = posB.y + aabbB.top;

                    if (a_left < b_right && a_right > b_left && a_bottom < b_top && a_top > b_bottom) {
                        collider.act(eidA, eidB);
                    }
                });
            });

            entities.visit([&](component::position& pos, component::aabb aabb){
                constexpr auto epsilon = 0.0001;
                aabb.left += epsilon;
                aabb.right -= epsilon;
                aabb.bottom += epsilon;
                aabb.top -= epsilon;
                {
                    auto r = int(pos.y + aabb.bottom) / 16;
                    auto c = int(pos.x + aabb.left) / 16;
                    auto rp = pos.y + aabb.bottom - r * 16;
                    auto cp = pos.x + aabb.left - c * 16;

                    if (r >= 0 && c >= 0 && r < test_stage.get_num_rows() && c < test_stage.get_num_cols()) {
                        if (test_stage.get(r, c).flags & tilemap::WALL) {
                            if (rp > cp) {
                                pos.y += 16 - rp;
                            } else {
                                pos.x += 16 - cp;
                            }
                        }
                    }
                }
                {
                    auto r = int(pos.y + aabb.bottom) / 16;
                    auto c = int(pos.x + aabb.right) / 16;
                    auto rp = pos.y + aabb.bottom - r * 16;
                    auto cp = pos.x + aabb.right - c * 16;

                    if (r >= 0 && c >= 0 && r < test_stage.get_num_rows() && c < test_stage.get_num_cols()) {
                        if (test_stage.get(r, c).flags & tilemap::WALL) {
                            if (rp > (16-cp)) {
                                pos.y += 16 - rp;
                            } else {
                                pos.x -= cp;
                            }
                        }
                    }
                }
                {
                    auto r = int(pos.y + aabb.top) / 16;
                    auto c = int(pos.x + aabb.left) / 16;
                    auto rp = pos.y + aabb.top - r * 16;
                    auto cp = pos.x + aabb.left - c * 16;

                    if (r >= 0 && c >= 0 && r < test_stage.get_num_rows() && c < test_stage.get_num_cols()) {
                        if (test_stage.get(r, c).flags & tilemap::WALL) {
                            if ((16-rp) > cp) {
                                pos.y -= rp;
                            } else {
                                pos.x += 16 - cp;
                            }
                        }
                    }
                }
                {
                    auto r = int(pos.y + aabb.top) / 16;
                    auto c = int(pos.x + aabb.right) / 16;
                    auto rp = pos.y + aabb.top - r * 16;
                    auto cp = pos.x + aabb.right - c * 16;

                    if (r >= 0 && c >= 0 && r < test_stage.get_num_rows() && c < test_stage.get_num_cols()) {
                        if (test_stage.get(r, c).flags & tilemap::WALL) {
                            if (rp < cp) {
                                pos.y -= rp;
                            } else {
                                pos.x -= cp;
                            }
                        }
                    }
                }
            });

            entities.visit([&](const component::position& pos, component::animated_sprite& sprite) {
                auto animation = resources::animated_sprites.get(sprite.name);
                auto& sheet = animation->get_spritesheet();
                auto& anim = animation->get_anim(sprite.anim);

                --sprite.frame_time;

                if (sprite.frame_time < 0) {
                    if (sprite.cur_frame == anim.frames.size()-1) {
                        if (anim.loops) {
                            sprite.cur_frame = 0;
                            sprite.frame_time = anim.frames[0].duration;
                        } else {
                            sprite.frame_time = 0;
                        }
                    } else {
                        ++sprite.cur_frame;
                        sprite.frame_time = anim.frames[sprite.cur_frame].duration;
                    }
                }

                auto& texture = sheet.get_texture();
                auto cell = anim.frames[sprite.cur_frame].cell;
                auto& mesh = sheet.get_mesh(cell/16, cell%16);

                auto modelmat = glm::mat4(1.f);
                modelmat = glm::translate(modelmat, glm::vec3{pos.x, pos.y, 0});
                sushi::set_program(program);
                sushi::set_uniform("cam_forward", glm::vec3{0,0,-1});
                sushi::set_uniform("s_texture", 0);
                sushi::set_uniform("MVP", projmat * cammat * modelmat);
                sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(modelmat)));
                sushi::set_texture(0, texture);
                sushi::draw_mesh(mesh);
            });

            {
                auto& phealth = entities.get_component<component::health>(player);
                auto wholeheart = resources::spritesheets.get("wholeheart", 9, 9);
                auto halfheart = resources::spritesheets.get("halfheart", 9, 9);
                auto emptyheart = resources::spritesheets.get("emptyheart", 9, 9);
                auto modelmat = glm::mat4(1.f);
                modelmat = glm::translate(modelmat, glm::vec3{5, 240-6, 0});
                for (int i = 0; i < phealth.value/2; ++i) {
                    sushi::set_program(program);
                    sushi::set_uniform("cam_forward", glm::vec3{0,0,-1});
                    sushi::set_uniform("s_texture", 0);
                    sushi::set_uniform("MVP", projmat * modelmat);
                    sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(modelmat)));
                    sushi::set_texture(0, wholeheart->get_texture());
                    sushi::draw_mesh(wholeheart->get_mesh(0,0));
                    modelmat = glm::translate(modelmat, glm::vec3{9, 0, 0});
                }
                for (int i = 0; i < phealth.value%2; ++i) {
                    sushi::set_program(program);
                    sushi::set_uniform("cam_forward", glm::vec3{0,0,-1});
                    sushi::set_uniform("s_texture", 0);
                    sushi::set_uniform("MVP", projmat * modelmat);
                    sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(modelmat)));
                    sushi::set_texture(0, halfheart->get_texture());
                    sushi::draw_mesh(halfheart->get_mesh(0,0));
                    modelmat = glm::translate(modelmat, glm::vec3{9, 0, 0});
                }
                for (int i = 0; i < (6-phealth.value)/2; ++i) {
                    sushi::set_program(program);
                    sushi::set_uniform("cam_forward", glm::vec3{0,0,-1});
                    sushi::set_uniform("s_texture", 0);
                    sushi::set_uniform("MVP", projmat * modelmat);
                    sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(modelmat)));
                    sushi::set_texture(0, emptyheart->get_texture());
                    sushi::draw_mesh(emptyheart->get_mesh(0,0));
                    modelmat = glm::translate(modelmat, glm::vec3{9, 0, 0});
                }
            }
        }

        sushi::set_framebuffer(nullptr);
        {
            glClearColor(0,0,0,1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
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
        }

        for(auto e : deadentities)
        {
            entities.destroy_entity(e);
        }
        deadentities.clear();

        SDL_GL_SwapWindow(g_window);

#ifdef __EMSCRIPTEN__
#else
        std::this_thread::sleep_until(last_update + frame_delay);
        last_update = std::chrono::steady_clock::now();
#endif
    };

    auto mainmenu_tex = resources::textures.get("mainmenu");
    auto mainmenu_mesh = sprite_mesh(*mainmenu_tex);

    auto mainmenu_loop = [&]{
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type) {
            case SDL_QUIT:
                std::clog << "Goodbye!" << std::endl;
                platform::cancel_main_loop();
                return;
            case SDL_KEYDOWN:
                loop = game_loop;
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

        SDL_GL_SwapWindow(g_window);

#ifdef __EMSCRIPTEN__
#else
        std::this_thread::sleep_until(last_update + frame_delay);
        last_update = std::chrono::steady_clock::now();
#endif
    };

    std::clog << "Init Success." << std::endl;

    std::clog << "Starting main loop..." << std::endl;
    loop = mainmenu_loop;
    platform::do_main_loop(main_loop, 60, 1);

    std::clog << "Cleaning up..." << std::endl;
    soloud.deinit();
    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(g_window);
    SDL_Quit();

    std::clog << "Graceful!" << std::endl;
    return EXIT_SUCCESS;
} catch (const std::exception& e) {
    std::clog << "Fatal exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
