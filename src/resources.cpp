#include "resources.hpp"

#include <iostream>

namespace resources {

resource_cache<sushi::texture_2d, std::string> textures ([](const std::string& name) {
    std::clog << "Loading texture: " << name << std::endl;
    return sushi::load_texture_2d("data/textures/"+name+".png", false, false, false, false);
});

resource_cache<sushi::static_mesh, std::string> meshes ([](const std::string& name) {
    std::clog << "Loading static mesh: " << name << std::endl;
    return sushi::load_static_mesh_file("data/models/"+name+".obj");
});

resource_cache<SoLoud::Wav, std::string> wavs ([](const std::string& name) {
    std::clog << "Loading WAV: " << name << std::endl;
    auto wav = std::make_shared<SoLoud::Wav>();
    wav->load(("data/sfx/"+name+".wav").c_str());
    return wav;
});

resource_cache<animated_sprite, std::string> animated_sprites ([](const std::string& name) {
    std::clog << "Loading anim: " << name << std::endl;
    auto texture = textures.get(name);
    std::ifstream jsonfile ("data/anims/"+name+".json");
    nlohmann::json json;
    jsonfile >> json;
    return animated_sprite(json, texture);
});

resource_cache<spritesheet, std::string, int, int> spritesheets ([](const std::string& name, int w, int h) {
    std::clog << "Loading sheet: (" << name << ", " << w << ", " << h << ")" << std::endl;
    auto texture = textures.get(name);
    return spritesheet(texture, w, h);
});

} //namespace resources
