#include <cstdlib>
#include <cstring>
#include <fstream>

namespace emberjs {

extern "C" {

char* ember_config_get() {
    const char* config = R"({
        "display": {
            "width": 800,
            "height": 600
        }
    })";
    auto str = (char*)malloc(strlen(config) + 1);
    strcpy(str, config);
    return str;
}

}

}
