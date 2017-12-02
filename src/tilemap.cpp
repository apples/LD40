#include "tilemap.hpp"

namespace tilemap {

tilemap::tilemap(int rows, int cols) :
    tiles(rows*cols),
    num_rows(rows),
    num_cols(cols)
{}

tilemap::tilemap(const nlohmann::json& json) :
    tiles(),
    num_rows(),
    num_cols()
{
    num_rows = json["num_rows"];
    num_cols = json["num_cols"];

    tiles.reserve(num_rows * num_cols);

    for (auto&& tilejson : json["tiles"]) {
        tile t;
        t.flags = tilejson[0];
        t.background = tilejson[1];
        t.foreground = tilejson[2];
        tiles.emplace_back(t);
    }
}

int tilemap::get_num_rows() const {
    return num_rows;
}

int tilemap::get_num_cols() const {
    return num_cols;
}

const tile& tilemap::get(int r, int c) const {
    return tiles[r * num_cols + c];
}

tile& tilemap::get(int r, int c) {
    const auto& self = *this;
    return const_cast<tile&>(self.get(r, c));
}

} //namespace tilemap
