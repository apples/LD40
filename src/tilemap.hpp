#ifndef LD40_TILEMAP_HPP
#define LD40_TILEMAP_HPP

#include "json.hpp"

#include <cstdint>
#include <vector>

namespace tilemap {

enum tile_flags : std::uint8_t {
    NONE = 0,
    BACKGROUND = 1<<0,
    FOREGROUND = 1<<1,
    WALL = 1<<2
};

struct tile {
    tile_flags flags;
    std::uint8_t background;
    std::uint8_t foreground;
};

class tilemap {
public:
    tilemap() = default;

    tilemap(int rows, int cols);

    tilemap(const nlohmann::json& json);

    int get_num_rows() const;

    int get_num_cols() const;

    const tile& get(int r, int c) const;

    tile& get(int r, int c);

private:
    std::vector<tile> tiles;
    int num_rows = 0;
    int num_cols = 0;
};

} //namespace tilemap

#endif //LD40_TILEMAP_HPP
