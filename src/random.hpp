#ifndef LD40_RANDOM_HPP
#define LD40_RANDOM_HPP

#include <random>

namespace random_helpers {

std::mt19937& rng();

int roll(int low, int high);

float rollf(float low, float high);

} //namespace random

#endif //LD40_RANDOM_HPP
