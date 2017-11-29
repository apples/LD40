#ifndef LD40_RESOURCE_CACHE_HPP
#define LD40_RESOURCE_CACHE_HPP

#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>

template <typename T, typename S>
class resource_cache {
public:
    resource_cache(std::function<T(const S&)> f) : factory(std::move(f)) {}

    std::shared_ptr<T> get(const S& s) {
        auto& ptr = cache[s];
        if (!ptr) {
            ptr = std::make_shared<T>(factory(s));
        }
        return ptr;
    }

    void clear() {
        cache.clear();
    }

private:
    std::function<T(const S&)> factory;
    std::map<S, std::shared_ptr<T>> cache;
};

#endif //LD40_RESOURCE_CACHE_HPP

