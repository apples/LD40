#ifndef LD40_RESOURCES_HPP
#define LD40_RESOURCES_HPP

#include "resource_cache.hpp"

#include <sushi/texture.hpp>
#include <sushi/mesh.hpp>
#include <soloud_wav.h>

namespace resources {

extern resource_cache<sushi::texture_2d, std::string> texture;

extern resource_cache<sushi::static_mesh, std::string> mesh;

extern resource_cache<SoLoud::Wav, std::string> wav;

} //namespace resources

#endif //LD40_RESOURCES_HPP
