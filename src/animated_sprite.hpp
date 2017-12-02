#ifndef LD40_ANIMATED_SPRITE_HPP
#define LD40_ANIMATED_SPRITE_HPP

#include "spritesheet.hpp"

#include "json.hpp"

#include <map>
#include <vector>
#include <string>

struct frame_info {
    int cell;
    int duration;
};

inline void from_json(const nlohmann::json& j, frame_info& f) {
    f.cell = j[0];
    f.duration = j[1];
}

inline void to_json(nlohmann::json& j, const frame_info& f) {
    j[0] = f.cell;
    j[1] = f.duration;
}

struct animation {
    bool loops;
    std::vector<frame_info> frames;
};

inline void from_json(const nlohmann::json& j, animation& a) {
    a.loops = j["loops"];
    a.frames = j["frames"].get<std::vector<frame_info>>();
}

inline void to_json(nlohmann::json& j, const animation& a) {
    j["loops"] = a.loops;
    j["frames"] = a.frames;
}

class animated_sprite {
public:
    animated_sprite() = default;

    animated_sprite(const nlohmann::json& json, std::shared_ptr<sushi::texture_2d> texture);

    const spritesheet& get_spritesheet() const;

    const animation& get_anim(const std::string& name) const;

private:
    spritesheet sprite;
    std::map<std::string, animation> anims;
};

#endif //LD40_ANIMATED_SPRITE_HPP
