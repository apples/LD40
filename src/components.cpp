#include "components.hpp"

namespace scripting {

template <>
void register_type<component::position>(sol::state& lua) {
    using position = component::position;
    new_component_usertype<position>(lua, "position",
        "x", &position::x,
        "y", &position::y);
}

template <>
void register_type<component::animated_sprite>(sol::state& lua) {
    using animated_sprite = component::animated_sprite;
    new_component_usertype<animated_sprite>(lua, "animated_sprite",
        "name", &animated_sprite::name,
        "anim", &animated_sprite::anim,
        "cur_frame", &animated_sprite::cur_frame,
        "frame_time", &animated_sprite::frame_time);
}

} //namespace scripting

