#ifndef LD40_RESOURCES_HPP
#define LD40_RESOURCES_HPP

#include "resource_cache.hpp"
#include "animated_sprite.hpp"

#include <sushi/texture.hpp>
#include <sushi/mesh.hpp>
#include <soloud_wav.h>

namespace resources {

extern resource_cache<sushi::texture_2d, std::string> textures;

extern resource_cache<sushi::static_mesh, std::string> meshes;

extern resource_cache<SoLoud::Wav, std::string> wavs;

extern resource_cache<animated_sprite, std::string> animated_sprites;

} //namespace resources

#endif //LD40_RESOURCES_HPP
