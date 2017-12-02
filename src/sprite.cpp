#include "sprite.hpp"

sushi::static_mesh sprite_mesh(const sushi::texture_2d& texture) {
    auto left = -texture.width / 2.f;
    auto right = texture.width / 2.f;
    auto bottom = -texture.height / 2.f;
    auto top = texture.height / 2.f;

    return sushi::load_static_mesh_data(
        {{left, bottom, 0.f},{left, top, 0.f},{right, top, 0.f},{right, bottom, 0.f}},
        {{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f}},
        {{0.f, 0.f},{0.f, 1.f},{1.f, 1.f},{1.f, 0.f}},
        {{{{0,0,0},{1,1,1},{2,2,2}}},{{{2,2,2},{3,3,3},{0,0,0}}}});
}

