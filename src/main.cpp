
#include "sdl.hpp"
#include "emberjs/config.hpp"

#include "utility.hpp"
#include "components.hpp"
#include "font.hpp"

#include <sushi/sushi.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <sol.hpp>

#include <emscripten.h>
#include <emscripten/html5.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <cstddef>
#include <cmath>
#include <functional>

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

const auto vertexSource_msdf = R"(
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

const auto fragmentSource_msdf = R"(
    #extension GL_OES_standard_derivatives : enable

    precision mediump float;
    varying vec2 v_texcoord;
    varying vec3 v_normal;
    uniform sampler2D msdf;
    uniform float pxRange;
    uniform vec2 texSize;
    uniform vec4 fgColor;

    float median(float r, float g, float b) {
        return max(min(r, g), min(max(r, g), b));
    }

    void main() {
        vec2 msdfUnit = pxRange/texSize;
        vec3 sample = texture2D(msdf, v_texcoord).rgb;
        float sigDist = median(sample.r, sample.g, sample.b) - 0.5;
        sigDist *= dot(msdfUnit, 0.5/fwidth(v_texcoord));
        float opacity = clamp(sigDist + 0.5, 0.0, 1.0);
        gl_FragColor = vec4(fgColor.rgb, fgColor.a*opacity);
    }
)";

std::function<void()> loop;
void main_loop() {
    loop();
}

int main() try {
    std::clog << "Init..." << std::endl;

    sol::state lua;
    lua.set_function("test", []{ std::clog << "Lua Test" << std::endl; });
    lua.script("test()");

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error(SDL_GetError());
    }

    auto error = [](const char *msg) {
        perror(msg);
        exit(0);
    };

    auto config = emberjs::get_config();

    const auto display_width = int(config["display"]["width"]);
    const auto display_height = int(config["display"]["height"]);
    const auto aspect_ratio = float(display_width) / float(display_height);

    auto g_window = SDL_CreateWindow("LD40", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, display_width, display_height, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    auto glcontext = SDL_GL_CreateContext(g_window);

    auto program = sushi::link_program({
        sushi::compile_shader(sushi::shader_type::VERTEX, {vertexSource}),
        sushi::compile_shader(sushi::shader_type::FRAGMENT, {fragmentSource}),
    });

    auto program_msdf = sushi::link_program({
        sushi::compile_shader(sushi::shader_type::VERTEX, {vertexSource_msdf}),
        sushi::compile_shader(sushi::shader_type::FRAGMENT, {fragmentSource_msdf}),
    });

    sushi::set_program(program);
    sushi::set_uniform("s_texture", 0);
    glBindAttribLocation(program.get(), sushi::attrib_location::POSITION, "position");
    glBindAttribLocation(program.get(), sushi::attrib_location::TEXCOORD, "texcoord");
    glBindAttribLocation(program.get(), sushi::attrib_location::NORMAL, "normal");

    sushi::set_program(program_msdf);
    glBindAttribLocation(program_msdf.get(), sushi::attrib_location::POSITION, "position");
    glBindAttribLocation(program_msdf.get(), sushi::attrib_location::TEXCOORD, "texcoord");
    glBindAttribLocation(program_msdf.get(), sushi::attrib_location::NORMAL, "normal");

    auto font = msdf_font("LiberationSans-Regular");

    auto draw_string = [&](const std::string& str, glm::mat4 viewproj, glm::vec2 pos, float scale) {
        auto model = glm::scale(glm::translate(glm::mat4(1.f), glm::vec3(pos, 0.f)), glm::vec3{scale, scale, 1.f});
        sushi::set_program(program_msdf);
        sushi::set_uniform("msdf", 0);
        sushi::set_uniform("pxRange", 4.f);
        sushi::set_uniform("fgColor", glm::vec4{1,1,1,1});

        for (auto c : str) {
            auto& glyph = font.get_glyph(c);
            sushi::set_uniform("MVP", (viewproj*model));
            sushi::set_uniform("texSize", glm::vec2{glyph.texture.width,glyph.texture.height});
            sushi::set_texture(0, glyph.texture);
            sushi::draw_mesh(glyph.mesh);
            model = glm::translate(model, glm::vec3{glyph.advance, 0.f, 0.f});
        }
    };

    auto draw_string_right = [&](const std::string& str, glm::mat4 viewproj, glm::vec2 pos, float scale) {
        auto model = glm::scale(glm::translate(glm::mat4(1.f), glm::vec3(pos, 0.f)), glm::vec3{scale, scale, 1.f});
        sushi::set_program(program_msdf);
        sushi::set_uniform("msdf", 0);
        sushi::set_uniform("pxRange", 4.f);
        sushi::set_uniform("fgColor", glm::vec4{1,1,1,1});

        for (auto c : utility::reversed(str)) {
            auto& glyph = font.get_glyph(c);
            model = glm::translate(model, glm::vec3{-glyph.advance, 0.f, 0.f});
            sushi::set_uniform("MVP", (viewproj*model));
            sushi::set_uniform("texSize", glm::vec2{glyph.texture.width,glyph.texture.height});
            sushi::set_texture(0, glyph.texture);
            sushi::draw_mesh(glyph.mesh);
        }
    };

    database entities;

    std::unordered_map<std::string, std::shared_ptr<sushi::static_mesh>> meshes;
    std::unordered_map<std::string, std::shared_ptr<sushi::texture_2d>> textures;

    auto get_mesh = [&](const std::string& name){
        auto& meshptr = meshes[name];
        if (!meshptr) {
            std::clog << "Loading mesh " << name << "..." << std::endl;
            meshptr = std::make_shared<sushi::static_mesh>(sushi::load_static_mesh_file("data/"+name+".obj"));
            std::clog << "Mesh " << name << " loaded." << std::endl;
        }
        return meshptr;
    };

    auto get_texture = [&](const std::string& name){
        auto& texptr = textures[name];
        if (!texptr) {
            std::clog << "Loading texture " << name << "..." << std::endl;
            texptr = std::make_shared<sushi::texture_2d>(sushi::load_texture_2d("data/"+name+".png", true, true, true, true));
            std::clog << "Texture " << name << " loaded." << std::endl;
        }
        return texptr;
    };

    loop = [&]{
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type) {
                case SDL_QUIT:
                    std::clog << "Goodbye!" << std::endl;
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
        draw_string("The quick brown fox jumped over the lazy dog. 1234567890", proj, {15, 15}, 50);
        draw_string_right("Right aligned test.", proj, {display_width, display_height-50}, 50);

        SDL_GL_SwapWindow(g_window);
    };

    std::clog << "Success." << std::endl;

    emscripten_set_main_loop(main_loop, 0, 1);

    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(g_window);
    SDL_Quit();

    return EXIT_SUCCESS;
} catch (const std::exception& e) {
    std::clog << "Fatal exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
