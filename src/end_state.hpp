#ifndef LD40_END_STATE
#define LD40_END_STATE

#include <sushi/sushi.hpp>

#include <string>

class end_state {
public:
    end_state(std::string name);
    void operator()();

private:
    sushi::framebuffer framebuffer;
    sushi::static_mesh framebuffer_mesh;
    sushi::unique_program program;

    std::string end_name;
};

#endif //LD40_END_STATE
