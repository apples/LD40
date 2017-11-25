#ifndef LD40_EMBERJS_CONFIG_HPP
#define LD40_EMBERJS_CONFIG_HPP

#include <json.hpp>

namespace emberjs {

    extern "C" {
        extern char* ember_config_get();
    }

    nlohmann::json get_config();

} //namespace emberjs

#endif //LD40_EMBERJS_CONFIG_HPP
