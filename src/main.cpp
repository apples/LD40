
#include "sdl.hpp"
#include "emberjs/config.hpp"

#include "utility.hpp"
#include "components.hpp"
#include "text.hpp"
#include "resource_cache.hpp"
#include "sprite.hpp"
#include "platform.hpp"

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
        //if (dot(v_normal, cam_forward) > 0.0) discard;
        color = vec4(texture(s_texture, v_texcoord).rgb, 1);
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

    database entities;

    resource_cache<sushi::texture_2d, std::string> texture_cache ([](const std::string& name) {
        std::clog << "Loading texture: " << name << std::endl;
        return sushi::load_texture_2d("data/textures/"+name+".png", true, true, true, true);
    });

    resource_cache<sushi::static_mesh, std::string> mesh_cache ([](const std::string& name) {
        std::clog << "Loading static mesh: " << name << std::endl;
        return sushi::load_static_mesh_file("data/models/"+name+".obj");
    });

    resource_cache<SoLoud::Wav, std::string> wav_cache ([](const std::string& name) {
        std::clog << "Loading WAV: " << name << std::endl;
        auto wav = std::make_shared<SoLoud::Wav>();
        wav->load(("data/sfx/"+name+".wav").c_str());
        return wav;
    });

    auto facetex = texture_cache.get("kawaii");
    auto facemesh = sprite_mesh(*facetex);
    auto wavtest = wav_cache.get("test");

    loop = [&]{
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type) {
                case SDL_QUIT:
                    std::clog << "Goodbye!" << std::endl;
                    platform::cancel_main_loop();
                    return;
                case SDL_KEYDOWN:
                    if (event.key.repeat == 0) {
                        switch (event.key.keysym.scancode) {
                            case SDL_SCANCODE_SPACE:
                                std::clog << "test" << std::endl;
                                soloud.play(*wavtest);
                                break;
                        }
                    }
                    break;
            }
        }

        const Uint8 *keys = SDL_GetKeyboardState(NULL);

        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        auto proj = glm::ortho(0.f, float(display_width), 0.f, float(display_height), -1.f, 1.f);

        {
            auto modelmat = glm::mat4(1.f);
            modelmat = glm::translate(modelmat, glm::vec3{display_width/2, display_height/2, 0.f});

            if (keys[SDL_SCANCODE_A]) modelmat = glm::translate(modelmat, glm::vec3{-10,0,0});
            if (keys[SDL_SCANCODE_D]) modelmat = glm::translate(modelmat, glm::vec3{10,0,0});
            if (keys[SDL_SCANCODE_S]) modelmat = glm::translate(modelmat, glm::vec3{0,-10,0});
            if (keys[SDL_SCANCODE_W]) modelmat = glm::translate(modelmat, glm::vec3{0,10,0});

            sushi::set_program(program);
            sushi::set_uniform("MVP", proj*modelmat);
            sushi::set_uniform("s_texture", 0);
            sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(modelmat)));
            sushi::set_uniform("cam_forward", glm::vec3{0,0,-1});
            sushi::set_texture(0, *facetex);
            sushi::draw_mesh(facemesh);
        }

        draw_string(font, "The quick brown fox jumped over the lazy dog. 1234567890", proj, {15, 15}, 50, text_align::LEFT);
        draw_string(font, "Right aligned test.", proj, {display_width, display_height-50}, 50, text_align::RIGHT);

        SDL_GL_SwapWindow(g_window);
    };

    std::clog << "Init Success." << std::endl;

    std::clog << "Starting main loop..." << std::endl;
    platform::do_main_loop(main_loop, 0, 1);

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
