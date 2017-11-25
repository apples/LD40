#include "entities.hpp"

namespace scripting {

template <>
void register_type<database>(sol::state& lua) {
    lua.new_usertype<database>("database",
        "create_entity", &database::create_entity,
        "destroy_entity", &database::destroy_entity,
        "create_component", [](database& db, database::ent_id eid, sol::userdata com){
            return com["_create_component"](db, eid, com);
        },
        "destroy_component", [](database& db, database::ent_id eid, sol::table com_type){
            return com_type["_destroy_component"](db, eid);
        },
        "get_component", [](database& db, database::ent_id eid, sol::table com_type){
            return com_type["_get_component"](db, eid);
        },
        "has_component", [](database& db, database::ent_id eid, sol::table com_type){
            return com_type["_has_component"](db, eid);
        });
}

} //namespace scripting

