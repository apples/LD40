#include "text.hpp"

void draw_string(msdf_font& font, const std::string& str, const glm::mat4& viewproj, const glm::vec2& pos, float scale, text_align align) {
    auto model_mat = glm::mat4(1.f);
    model_mat = glm::translate(model_mat, glm::vec3{pos, 0.f});
    model_mat = glm::scale(model_mat, glm::vec3{scale, scale, 1.f});

    font.bind_shader();

    auto draw_glyph = [&](auto& glyph) {
        sushi::set_uniform("MVP", viewproj*model_mat);
        sushi::set_uniform("texSize", glm::vec2{glyph.texture.width,glyph.texture.height});
        sushi::set_texture(0, glyph.texture);
        sushi::draw_mesh(glyph.mesh);
    };

    switch (align) {
        case text_align::LEFT:
            for (auto c : str) {
                auto& glyph = font.get_glyph(c);
                draw_glyph(glyph);
                model_mat = glm::translate(model_mat, glm::vec3{glyph.advance, 0.f, 0.f});
            }
            break;
        case text_align::RIGHT:
            for (auto c : utility::reversed(str)) {
                auto& glyph = font.get_glyph(c);
                model_mat = glm::translate(model_mat, glm::vec3{-glyph.advance, 0.f, 0.f});
                draw_glyph(glyph);
            }
            break;
    }
}

