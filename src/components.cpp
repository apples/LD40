#include "components.hpp"

namespace scripting {

template <>
void register_type<component::position>(sol::state& lua) {
    using position = component::position;
    new_component_usertype<position>(lua, "position",
        "x", &position::x,
        "y", &position::y);
}

} //namespace scripting

