#ifndef LD40_TEXT_HPP
#define LD40_TEXT_HPP

#include "font.hpp"
#include "utility.hpp"

enum class text_align {
    LEFT,
    RIGHT
};

void draw_string(msdf_font& font, const std::string& str, const glm::mat4& viewproj, const glm::vec2& pos, float scale, text_align align);

#endif //LD40_TEXT_HPP

