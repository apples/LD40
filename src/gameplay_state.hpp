#ifndef LD40_GAMEPLAY_STATE_HPP
#define LD40_GAMEPLAY_STATE_HPP

#include "tilemap.hpp"

#include "entities.hpp"

#include <sushi/framebuffer.hpp>
#include <sushi/mesh.hpp>
#include <sushi/shader.hpp>

class gameplay_state {
public:
    gameplay_state(std::string);
    void init();
    void operator()();
private:
    tilemap::tilemap test_stage;

    sushi::framebuffer framebuffer;
    sushi::static_mesh framebuffer_mesh;
    sushi::unique_program program;

    database entities;
    std::vector<database::ent_id> deadentities;
    database::ent_id player;

    bool initted = false;

    std::string levelname;
};

#endif //LD40_GAMEPLAY_STATE_HPP
