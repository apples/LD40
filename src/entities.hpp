#ifndef LD40_ENTITIES_HPP
#define LD40_ENTITIES_HPP

#include "scripting.hpp"

#include <ginseng/ginseng.hpp>

using database = ginseng::database;

namespace scripting {

template <>
void register_type<database>(sol::state& lua);

} //namespace scripting

#endif //LD40_ENTITIES_HPP
