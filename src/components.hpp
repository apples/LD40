#ifndef LD40_COMPONENTS_HPP
#define LD40_COMPONENTS_HPP

#include "json.hpp"
#include "scripting.hpp"
#include "entities.hpp"

#include <functional>
#include <string>
#include <type_traits>

namespace component {

struct position {
    float x;
    float y;
};

} //namespace component

namespace scripting {

namespace _detail {

template <typename T>
struct is_tag : std::false_type {};

template <typename T>
struct is_tag<ginseng::tag<T>> : std::true_type {};

} //namespace _detail

template <typename T, typename... Args, std::enable_if_t<!_detail::is_tag<T>::value, int> = 0>
void new_component_usertype(sol::state& lua, const std::string& name, Args&&... args) {
    lua.new_usertype<T>(name, std::forward<Args>(args)...,
        "_create_component", [](database& db, database::ent_id eid, T com) -> database::com_id {
            return db.create_component(eid, std::move(com));
        },
        "_destroy_component", [](database& db, database::ent_id eid) {
            db.destroy_component<T>(eid);
        },
        "_get_component", [](database& db, database::ent_id eid) -> std::reference_wrapper<T> {
            return std::ref(db.get_component<T>(eid));
        },
        "_has_component", [](database& db, database::ent_id eid) {
            return db.has_component<T>(eid);
        });
}

template <typename T, typename... Args, std::enable_if_t<_detail::is_tag<T>::value, int> = 0>
void new_component_usertype(sol::state& lua, const std::string& name, Args&&... args) {
    lua.new_usertype<T>(name, std::forward<Args>(args)...,
        "_create_component", [](database& db, database::ent_id eid, T com) -> database::com_id {
            db.create_component(eid, std::move(com));
        },
        "_destroy_component", [](database& db, database::ent_id eid) {
            db.destroy_component<T>(eid);
        },
        "_has_component", [](database& db, database::ent_id eid) {
            return db.has_component<T>(eid);
        });
}

template <>
void register_type<component::position>(sol::state& lua);

} //namespace scripting

#endif //LD40_COMPONENTS_HPP
