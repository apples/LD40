#include "animated_sprite.hpp"

animated_sprite::animated_sprite(const nlohmann::json& json, std::shared_ptr<sushi::texture_2d> texture) {
    int sprite_width = json["sprite_width"];
    int sprite_height = json["sprite_height"];

    sprite = spritesheet(texture, sprite_width, sprite_height);

    anims = json["anims"].get<std::map<std::string, animation>>();
}

const spritesheet& animated_sprite::get_spritesheet() const {
    return sprite;
}

const animation& animated_sprite::get_anim(const std::string& name) const {
    return anims.at(name);
}
