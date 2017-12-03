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

void tilemap::set_num_rows(int newr) {
    std::vector<tile> newtiles (newr*num_cols, tile{});
    for (int r=0; r<num_rows && r<newr; ++r) {
        for (int c=0; c<num_cols; ++c) {
            newtiles[r*num_cols+c] = get(r,c);
        }
    }
    tiles = std::move(newtiles);
    num_rows = newr;
}

void tilemap::set_num_cols(int newc) {
    std::vector<tile> newtiles (num_rows*newc);
    for (int r=0; r<num_rows; ++r) {
        for (int c=0; c<num_cols && c<newc; ++c) {
            newtiles[r*newc+c] = get(r,c);
        }
    }
    tiles = std::move(newtiles);
    num_cols = newc;
}

const tile& tilemap::get(int r, int c) const {
    return tiles[r * num_cols + c];
}

tile& tilemap::get(int r, int c) {
    const auto& self = *this;
    return const_cast<tile&>(self.get(r, c));
}

} //namespace tilemap
