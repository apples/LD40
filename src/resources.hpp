#ifndef LD40_RESOURCES_HPP
#define LD40_RESOURCES_HPP

#include "resource_cache.hpp"
#include "animated_sprite.hpp"
#include "spritesheet.hpp"
#include "font.hpp"

#include <sushi/texture.hpp>
#include <sushi/mesh.hpp>
#include <soloud_wav.h>

#include <string>
#include <tuple>

namespace resources {

extern resource_cache<sushi::texture_2d, std::string> textures;

extern resource_cache<sushi::static_mesh, std::string> meshes;

extern resource_cache<SoLoud::Wav, std::string> wavs;

extern resource_cache<animated_sprite, std::string> animated_sprites;

extern resource_cache<spritesheet, std::string, int, int> spritesheets;

extern resource_cache<msdf_font, std::string> fonts;

} //namespace resources

#endif //LD40_RESOURCES_HPP
