#ifndef LD40_SCRIPTING_HPP
#define LD40_SCRIPTING_HPP

#include <sol.hpp>

namespace scripting {

template <typename T>
void register_type(sol::state& lua);

} //namespace scripting

#endif //LD40_SCRIPTING_HPP
