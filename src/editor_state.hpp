#ifndef LD40_EDITOR_STATE_HPP
#define LD40_EDITOR_STATE_HPP

#include "tilemap.hpp"

#include <sushi/sushi.hpp>

#include <set>
#include <tuple>

class editor_state {
public:
    editor_state(std::string fname);
    void operator()();
    void save();

private:
    sushi::framebuffer framebuffer;
    sushi::static_mesh framebuffer_mesh;
    sushi::unique_program program;

    glm::vec2 position = {};
    glm::vec2 spawn = {};
    tilemap::tilemap map = {1,1};

    std::set<std::tuple<int,int>> elves;
    std::set<std::tuple<int,int>> beers;

    int time_limit = 60;

    int cursor_tile = 0;

    std::string filename;
};

#endif //LD40_EDITOR_STATE_HPP
