#include "font.hpp"

#include "json.hpp"

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

msdfgen::FreetypeHandle* font_init() {
    static const auto ft = msdfgen::initializeFreetype();
    return ft;
}

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

} //static

msdf_font::msdf_font(const std::string& fontname) {
    auto ft = font_init();

    if (!ft) {
        throw std::runtime_error("Failed to initialize FreeType.");
    }

    font = decltype(font)(msdfgen::loadFont(ft, ("data/fonts/"+fontname+".ttf").c_str()));

    if (!font) {
        throw std::runtime_error("Failed to load font "+fontname+".");
    }

    program = sushi::link_program({
        sushi::compile_shader(sushi::shader_type::VERTEX, {vertexSource_msdf}),
        sushi::compile_shader(sushi::shader_type::FRAGMENT, {fragmentSource_msdf}),
    });

    sushi::set_program(program);
    glBindAttribLocation(program.get(), sushi::attrib_location::POSITION, "position");
    glBindAttribLocation(program.get(), sushi::attrib_location::TEXCOORD, "texcoord");
    glBindAttribLocation(program.get(), sushi::attrib_location::NORMAL, "normal");
}

void msdf_font::bind_shader() const {
    sushi::set_program(program);
    sushi::set_uniform("msdf", 0);
    sushi::set_uniform("pxRange", 4.f);
    sushi::set_uniform("fgColor", glm::vec4{1,1,1,1});
}

const msdf_font::glyph& msdf_font::get_glyph(int unicode) {
    auto iter = glyphs.find(unicode);

    if (iter == end(glyphs)) {
        msdfgen::Shape shape;
        double advance;

        if (!msdfgen::loadGlyph(shape, font.get(), unicode, &advance)) {
            return glyphs.at(0);
        }

        shape.normalize();
        msdfgen::edgeColoringSimple(shape, 3.0);

        double left=0, bottom=0, right=0, top=0;
        shape.bounds(left, bottom, right, top);

        left -= 1;
        bottom -= 1;
        right += 1;
        top += 1;

        auto width = int(right - left + 1);
        auto height = int(top - bottom + 1);

        msdfgen::Bitmap<msdfgen::FloatRGB> msdf(width, height);
        msdfgen::generateMSDF(msdf, shape, 4.0, 1.0, msdfgen::Vector2(-left, -bottom));

        std::vector<unsigned char> pixels;
        pixels.reserve(4*msdf.width()*msdf.height());
        for (int y = 0; y < msdf.height(); ++y) {
            for (int x = 0; x < msdf.width(); ++x) {
                pixels.push_back(msdfgen::clamp(int(msdf(x, y).r*0x100), 0xff));
                pixels.push_back(msdfgen::clamp(int(msdf(x, y).g*0x100), 0xff));
                pixels.push_back(msdfgen::clamp(int(msdf(x, y).b*0x100), 0xff));
                pixels.push_back(255);
            }
        }

        double em;
        msdfgen::getFontScale(em, font.get());

        left /= em;
        right /= em;
        bottom /= em;
        top /= em;
        advance /= em;

        auto g = glyph{};

        g.mesh = sushi::load_static_mesh_data(
            {{left, bottom, 0.f},{left, top, 0.f},{right, top, 0.f},{right, bottom, 0.f}},
            {{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f}},
            {{0.f, 0.f},{0.f, 1.f},{1.f, 1.f},{1.f, 0.f}},
            {{{{0,0,0},{1,1,1},{2,2,2}}},{{{2,2,2},{3,3,3},{0,0,0}}}});

        g.texture.handle = sushi::make_unique_texture();
        g.texture.width = width;
        g.texture.height = height;

        glBindTexture(GL_TEXTURE_2D, g.texture.handle.get());
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, msdf.width(), msdf.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);

        g.advance = advance;

        iter = glyphs.insert({unicode, std::move(g)}).first;
    }

    return iter->second;
}

