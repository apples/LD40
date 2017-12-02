#ifndef LD40_SPRITESHEET_HPP
#define LD40_SPRITESHEET_HPP

#include <sushi/mesh.hpp>
#include <sushi/texture.hpp>

#include <memory>
#include <vector>

class spritesheet {
public:
    spritesheet() = default;

    spritesheet(std::shared_ptr<sushi::texture_2d> tex, int w, int h);

    const sushi::texture_2d& get_texture() const;

    const sushi::static_mesh& get_mesh(int r, int c) const;

    int get_sprite_width() const;

    int get_sprite_height() const;

    int get_num_rows() const;

    int get_num_cols() const;

private:
    std::shared_ptr<sushi::texture_2d> texture;
    std::vector<sushi::static_mesh> meshes;
    int sprite_width = 0;
    int sprite_height = 0;
    int num_rows = 0;
    int num_cols = 0;
};

#endif //LD40_SPRITESHEET_HPP
