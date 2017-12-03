#include "random.hpp"

namespace random_helpers {

std::mt19937& rng() {
    static std::mt19937 device {std::random_device{}()};
    return device;
}

int roll(int low, int high) {
    auto dist = std::uniform_int_distribution<int>(low, high);
    return dist(rng());
}

float rollf(float low, float high) {
    auto dist = std::uniform_real_distribution<float>(low, high);
    return dist(rng());
}

}
