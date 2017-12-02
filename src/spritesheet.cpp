#include "spritesheet.hpp"

spritesheet::spritesheet(std::shared_ptr<sushi::texture_2d> tex, int w, int h) :
    texture(std::move(tex)),
    meshes(),
    sprite_width(w),
    sprite_height(h),
    num_rows(),
    num_cols()
{
    num_rows = texture->height / sprite_height;
    num_cols = texture->width / sprite_width;

    auto rowstride = float(sprite_height) / float(texture->height);
    auto colstride = float(sprite_width) / float(texture->width);

    auto left = -sprite_width/2;
    auto right = left + sprite_width;
    auto bottom = -sprite_height/2;
    auto top = bottom + sprite_height;

    for (auto r = 0; r < num_rows; ++r) {
        auto uvbottom = r * rowstride;
        auto uvtop = (r + 1) * rowstride;

        for (auto c = 0; c < num_cols; ++c) {
            auto uvleft = c * colstride;
            auto uvright = (c + 1) * colstride;

            meshes.emplace_back(sushi::load_static_mesh_data(
                {{left, bottom, 0.f},{left, top, 0.f},{right, top, 0.f},{right, bottom, 0.f}},
                {{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f}},
                {{uvleft, uvtop},{uvleft, uvbottom},{uvright, uvbottom},{uvright, uvtop}},
                {{{{0,0,0},{1,1,1},{2,2,2}}},{{{2,2,2},{3,3,3},{0,0,0}}}}
            ));
        }
    }
}

const sushi::texture_2d& spritesheet::get_texture() const {
    return *texture;
}

const sushi::static_mesh& spritesheet::get_mesh(int r, int c) const {
    return meshes[r * get_num_cols() + c];
}

int spritesheet::get_sprite_width() const {
    return sprite_width;
}

int spritesheet::get_sprite_height() const {
    return sprite_height;
}

int spritesheet::get_num_rows() const {
    return num_rows;
}

int spritesheet::get_num_cols() const {
    return num_cols;
}
